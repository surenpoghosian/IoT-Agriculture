#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

//#include <async.h>


#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "GyverButton.h"

#define BTN_PIN 12

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define BAND 915E6

#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

//byte LoRaData;
byte LoRaData;
String info = "";
String all_data = "";
String rx_data = "";
String temp = "";
String humd = "";
String val1 = "";
String val2 = "";

int Count = 0;
int msgCount = 0;

uint8_t destination_array[2] = {0x01, 0x02};

byte localAddress = 0x69;     // address of this device
byte destination = 0x01;
int lot = 0;
long lastSendTime = 0;        // last send time
int interval = 2000;

const char* ssid     = "Cryptex";
const char* password = "1234567890";


const char* serverName = "http://94.228.123.230:8070/";
String request_data = "";

GButton butt1(BTN_PIN);

void setup() {
  Serial.begin(115200);

  pinMode(2, OUTPUT);
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("RECEIVER ");
  display.display();

  //  LoRa.setSyncWord(0x34);
  Serial.println("Receiver Test");

  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting failed!");
    while (1);
  }
//  WiFi.begin(ssid, password);
//  Serial.println("Connecting");
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(500);
//    Serial.print(".");
//  }
//  Serial.println("");
//  Serial.print("Connected to WiFi network with IP Address: ");
//  Serial.println(WiFi.localIP());
//
//  Serial.println("WiFi Disconnected");

  Serial.println("Initializing OK!");
  display.setCursor(0, 10);
  display.println("Initializing OK!");
  display.display();

}


void sendMessage(byte outgoing, byte destination, byte localAddress) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(outgoing);                 // add payload
  //  LoRa.write(outgoing.length());    // add payload length
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  //  LoRa.write(msgCount);             // add message ID
  LoRa.endPacket();                     // finish packet and send it
}


void blink(int len) {
  for (int i = 0; i < len; i++) {
    digitalWrite(2, HIGH);
    delay(150);
    digitalWrite(2, LOW);
    delay(150);
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


void loop() {

  butt1.tick();
  if (butt1.isSingle()) {
    Serial.println("Single");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Reciever Mode 1");
    display.display();



    blink(1);

    while (true) {
      if (millis() - lastSendTime > interval) {

        byte flag = 0xFF;
        byte device = 0x09;
        sendMessage(flag, device, localAddress);
        Serial.println("Sending " + String(flag));
        lastSendTime = millis();          // timestamp the message
        interval = random(2000) + 500;    // delay 2-5 sec.
        //
        //        if (lot >= (sizeof(destination_array) - 1))
        //        {
        //          lot = 0;
        //        }
        //        else {
        //          lot++;
        //        }
      }


      int packetSize = LoRa.parsePacket();

      if (packetSize) {


        while (LoRa.available()) {
          LoRaData = LoRa.read();
          if (LoRaData == 105) {
            byte addr = LoRa.read();
            String rx_data = LoRa.readString();
            Serial.println(LoRaData);
            Serial.println(addr);
            Serial.println(rx_data);

            String txed_address = String(addr);
            info = txed_address + "|" + rx_data;
            val1 = getValue(rx_data, '&', 0);
            val2 = getValue(rx_data, '&', 1);
          }


        }

        // вывести RSSI пакета
        int rssi = LoRa.packetRssi();
        Serial.print(" with RSSI ");
        Serial.println(rssi);

        // Показать информацию на OLED дисплее
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("RECEIVER Mode 1");
        display.setCursor(0, 20);
        display.print("Received packet:");
        display.setCursor(0, 30);
        display.print("--> " + val1);
        display.setCursor(0, 55);
        display.print("RSSI:");
        display.setCursor(30, 55);
        display.print(rssi);
        
        display.display();
        if (WiFi.status() == WL_CONNECTED) {
          WiFiClient client;
          HTTPClient http;
          http.begin(client, serverName);
          http.addHeader("Content-Type", "text/plain");

          int httpResponseCode = http.POST(val1);


          if (httpResponseCode > 0) {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);

          }
          else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
          }

          http.end();
        }
        else {
          Serial.println("Error");
        }


      }



    }
  }




  if (butt1.isDouble()) {
    Serial.println("Double");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Reciever Mode 2");
    display.display();


    blink(2);

    while (true) {
      if (millis() - lastSendTime > interval) {

        byte flag = 0xFF;

        sendMessage(flag, destination_array[lot], localAddress);
        Serial.println("Sending " + String(flag));
        lastSendTime = millis();            // timestamp the message
        interval = random(2000) + 500;    // delay 2-5 sec.

        if (lot >= (sizeof(destination_array) - 1))
        {
          lot = 0;
        }
        else {
          lot++;
        }
      }


      int packetSize = LoRa.parsePacket();

      if (packetSize) {


        while (LoRa.available()) {
          LoRaData = LoRa.read();
          if (LoRaData == 105) {
            byte addr = LoRa.read();
            String rx_data = LoRa.readString();
            Serial.println(LoRaData);
            Serial.println(addr);
            Serial.println(rx_data);

            String txed_address = String(addr);
            info = txed_address + "|" + rx_data;
            temp = getValue(rx_data, '&', 0);
            humd = getValue(rx_data, '&', 1);

          }


        }

      }
      // вывести RSSI пакета
      //      int rssi = LoRa.packetRssi();

      // Показать информацию на OLED дисплее
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Mode 2");
      display.setCursor(0, 20);
      display.setCursor(0, 30);
      display.print("Hmd - " + humd);
      display.setCursor(30, 55);
      //      display.print(rssi);
      display.display();

    }
  }

  if (butt1.isTriple()) {
    Serial.println("Triple");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Reciever Mode 3");
    display.display();


    blink(3);

    while (true) {


      int packetSize = LoRa.parsePacket();

      if (packetSize) {


        while (LoRa.available()) {
          LoRaData = LoRa.read();
          if (LoRaData == 105) {
            byte addr = LoRa.read();
            String rx_data = LoRa.readString();
            Serial.println(LoRaData);
            Serial.println(addr);
            Serial.println(rx_data);

            String txed_address = String(addr);
            info = txed_address + "|" + rx_data;
            val1 = getValue(rx_data, '&', 0);
            val2 = getValue(rx_data, '&', 1);

          }


        }

      }
      // вывести RSSI пакета
      //      int rssi = LoRa.packetRssi();

      // Показать информацию на OLED дисплее
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Mode 3");
      display.setCursor(0, 20);
      display.print("Distance");
      display.setCursor(0, 30);
      display.print(val1);
      //      display.setCursor(30, 55);
      //      display.print(rssi);
      display.display();

    }
  }
}
