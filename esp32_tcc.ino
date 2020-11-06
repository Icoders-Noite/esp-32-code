#include <ArduinoJson.h>

#include <WiFi.h>
#include "dht.h"
#include <stdlib.h>
#include <SPI.h>
#include <mySD.h>
#include <HTTPClient.h>


//sensor dht11
#define pinoDHT11 25//PINO ANALÓGICO UTILIZADO PELO DHT11
//buzzer
const byte buzzerPin = 26;
int8_t freq = 2000;
int8_t channel = 0;
int8_t resolution = 10;




// Pino ligado ao CS do modulo
#define chipSelect 5
//valvula
#define pin_valvula  14
//luz
#define pin_luz 32
// sensor solo
#define sensor_solo  33


dht DHT; //VARIÁVEL DO TIPO DHT

//Variaveis para cartao SD
String parameter;
byte line;
String configs[2];

//parametros
String ssid = "", password = "";
int status = WL_IDLE_STATUS;

int8_t luzStatus = 0;

int8_t  analogSoloSeco = 4095; //VALOR MEDIDO COM O SOLO SECO (VOCÊ PODE FAZER TESTES E AJUSTAR ESTE VALOR)
int8_t  analogSoloMolhado = 1200; //VALOR MEDIDO COM O SOLO MOLHADO (VOCÊ PODE FAZER TESTES E AJUSTAR ESTE VALOR)
int8_t percSoloSeco = 0; //MENOR PERCENTUAL DO SOLO SECO (0% - NÃO ALTERAR)
int8_t percSoloMolhado = 100;

unsigned long millisTarefa = millis();

//wifi


int solo =  -1;
int humidity = -1;
int temperature = -1;

void setup() {
  pinMode(pin_valvula, OUTPUT);
  pinMode(pin_luz, OUTPUT);
digitalWrite(pin_valvula,HIGH);
digitalWrite(pin_luz,HIGH);
  Serial.begin(115200); //INICIALIZA A SERIAL
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(buzzerPin, channel);
  pinMode(sensor_solo, INPUT);

  delay(2000); //INTERVALO DE 2 SEGUNDO ANTES DE INICIAR
  start();
  while (!Serial);
  Serial.println(F("Initializing SD card..."));
  if (!SD.begin(chipSelect, 23, 19, 18))
  {
    Serial.println(F("Card failed, or not present"));

  }
  Serial.println(F("card initialized."));
  File myFile = SD.open("CONFIG.txt");
  if (myFile)
  {
    while (myFile.available())
    {
      char c = myFile.read();
      if (isPrintable(c))
      {
        parameter.concat(c);
      }
      else if (c == '\n')
      {
        Serial.println(parameter);
        configs[line] = parameter;
        parameter = "";
        line++;
      }
    }
  }

  myFile.close();
  configParametros();

  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");

  //quando está tudo certo
  musica();


}

void loop() {

  if ((millis() - millisTarefa) > 3000) {
    millisTarefa = millis();
    solo =  sensorSolo();
    DHT.read11(pinoDHT11);
    delay(2000);
    humidity = DHT.humidity;
    temperature = DHT.temperature;
    delay(100);
  }

  char url[300] = "";
  String payload = "";
  sprintf(url, "https://us-central1-meu-tcc-1995.cloudfunctions.net/function-1/set-status-sensores?l=%i&s=%i&i=%i&u=%i/%i", luzStatus, solo, 15, humidity, temperature);

  Serial.println(url);
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status

    HTTPClient http;
    http.setTimeout(100000);
    http.begin(url); //Specify the URL
    int httpCode = http.GET();                                        //Make the request

    if (httpCode > 0) { //Check for the returning code

      payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
    }

    else {
      Serial.println("Error on HTTP request");
    }

    http.end(); //Free the resources
  }

  const size_t capacity = JSON_OBJECT_SIZE(5) + 140;
  DynamicJsonDocument doc(capacity);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }


 String valvula = doc["valvula"];
   String luz = doc["luz"];
Serial.println(valvula);
Serial.println(luz);

  if (valvula=="1") {
    acionaValvula();
  }

  if (luz== "1") {

    ligaLuz();
    luzStatus = 1;
  } else {
    if (luz== "0") {

      desligaLuz();
      luzStatus = 0;
    }
  }
  solo = -1;
  humidity = -1;
  temperature = -1;


}




void musica() {
  ledcWriteTone(channel, 200);
  delay(500);
  ledcWriteTone(channel, 0);
  ledcWriteTone(channel, 300);
  delay(500);
  ledcWriteTone(channel, 0);
  ledcWriteTone(channel, 400);
  delay(500);
  ledcWriteTone(channel, 0);

}
void start() {
  ledcWriteTone(channel, 500);
  delay(100);
  ledcWriteTone(channel, 0);
}

void configParametros()
{
  ssid = configs[0];
  password = configs[1];

}

void ligaLuz() {
  digitalWrite(pin_luz, LOW);
}
void desligaLuz() {
  digitalWrite(pin_luz, HIGH);
}

void acionaValvula() {
  digitalWrite(pin_valvula, LOW);
  delay(1000);
  digitalWrite(pin_valvula, HIGH);
}

int sensorSolo() {
  int ValorAnalog = analogRead(sensor_solo);
  int UmidPercent = map(ValorAnalog, 1145, 4095, 0, 100); //Mapeando o valor entre 0 e 100
  int  umidade = 100 - UmidPercent;
  return  umidade ;
}
