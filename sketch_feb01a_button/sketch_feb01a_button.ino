#include <SPI.h>
#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "Adafruit_SHT31.h"

#include "GyverButton.h"

#define BTN_PIN 12
#define relay 17
#define white 33
#define green 32


//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 915E6

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//uint8_t packetBuffer[55];

int db_rate = 19;
//packet packet_counter
int packet_counter = 0;

String info = "";
String LoRadata = "";
String all_data = "";

byte localAddress = 0x01;     // address of this device
byte destination = 0x69;

int flag;

byte LoRaData;

int tmp_pin = 13;
int hmd_pin = 34;

//int a_pin = 36;
//int b_pin = 37;

float tmp = 0.0;
float hmd = 0.0;

int hmd_value = 0;

bool b = false;

GButton butt1(BTN_PIN);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);


void setup() {

  //initialize Serial Monitor
  Serial.begin(115200);


  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  pinMode(2, OUTPUT);

  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);


  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("LORA SENDER ");
  display.display();

  //  LoRa.setSyncWord(0x34);
  Serial.println("LoRa Sender Test");

  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.setTxPower(db_rate);

  Serial.println("LoRa Initializing OK!");
  display.setCursor(0, 10);
  display.print("LoRa Initializing OK!");
  display.display();


}



void loop() {

  butt1.tick();

  if (butt1.isSingle()) {
    Serial.println("Single");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Sender Mode 1");
    display.display();

    blink(1);

    struct Transmit {
      byte dv_id;
      byte sender;
    };

    struct Transmit d;

    while (true) {


      int packetSize = LoRa.parsePacket();

      if (packetSize) {

        while (LoRa.available()) {
          LoRaData = LoRa.read();
          String LoRaData = LoRa.readString();

          //          if (LoRaData == 255) {
          //
          //            d.dv_id = LoRa.read();
          //            d.sender = LoRa.read();
          //
          //            //            Serial.println("LoRaData " + String(LoRaData));
          //            //            Serial.println("device ID " + String(d.dv_id));
          //            //            Serial.println("Sender " + String(d.sender));
          //
          //            if (d.sender == 105) {
          //              if (d.dv_id == 1) {
          //
          //                tmp = get_tmp(tmp_pin);
          //                hmd = get_hmd(hmd_pin);
          //
          //                float tmp_median = median(tmp);
          //                float val = simpleKalman(tmp_median, 0.6, 0.09);
          //
          //                float hmd_median = median_hmd(hmd);
          //                float val_hmd = simpleKalman_hmd(hmd_median, 0.8, 0.09);
          //
          //                String n = String(val) + "&" + String(val_hmd);
          //
          //
          //                sendMessage(n, destination, localAddress);
          //              }
          //            }
          //          }
        }
      }

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Mode 1");
      display.setCursor(0, 10);
      display.println("SENDER " + String(db_rate) + " DB");
      display.setCursor(0, 30);
      display.setTextSize(1);
      display.print("packet sent.");
      display.setCursor(0, 40);
      display.println( String(LoRaData) );


      display.display();



    }
  }

  if (butt1.isDouble()) {
    blink(2);

    while (true) {
      tmp = get_tmp(tmp_pin);
      hmd = get_hmd(hmd_pin);

      String n = String(tmp) + "&" + String(hmd);

      sendMessage(n, destination, localAddress);

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Mode 2");
      display.setCursor(0, 10);
      display.println("SENDER " + String(db_rate) + " DB");
      display.setCursor(0, 30);
      display.setTextSize(1);
      display.print("packet sent.");
      display.setCursor(0, 40);
      display.println("C - " + String(tmp) + " hmd - " + String(hmd));

      display.display();
      delay(500);

    }
  }

  if (butt1.isTriple()) {
    blink(3);

    while (true) {

      tmp = get_tmp(tmp_pin);
      hmd = get_hmd(hmd_pin);

      float tmp_median = median(tmp);
      float val = simpleKalman(tmp_median, 0.6, 0.09);

      float hmd_median = median_hmd(hmd);
      float val_hmd = simpleKalman_hmd(hmd_median, 0.8, 0.09);
      //          float val_hmd = 69.0;
      Serial.print(hmd);
      Serial.print("\t");
      Serial.print(hmd_median);
      Serial.print("\t");
      Serial.println(val_hmd);

      //      Serial.print(hmd);
      //      Serial.print("\t");
      //      Serial.print(hmd_median);
      //      Serial.print("\t");
      //      Serial.println(val_hmd);

      String n = String(val) + "&" + String(val_hmd);

      sendMessage(n, destination, localAddress);

      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("Mode 2");
      display.setCursor(0, 10);
      display.println("SENDER " + String(db_rate) + " DB");
      display.setCursor(0, 30);
      display.setTextSize(1);
      display.print("packet sent.");
      display.setCursor(0, 40);
      display.println("C - " + String(val) + " hmd - " + String(val_hmd));

      display.display();
      delay(500);

    }
  }
}


void sendMessage(String outgoing, byte destination, byte localAddress) {

  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.print(outgoing);
  LoRa.endPacket();                     // finish packet and send it

}




float get_tmp(int sensor_pin) {
  int t_value = analogRead(sensor_pin);

  //  int tmp_mapped = map(t_value, 0, 4095, 0, 1023);
  float factor = 4.887 / 4095.0;
  float voltage = t_value * factor;
  float tmp = voltage * 24 - 65;


  return tmp;
}




float get_hmd(int sensor_pin) {
  hmd_value = analogRead(sensor_pin);
  //int hmd_mapped = map(hmd_value, 0, 4095, 0, 1023);
  float hmd_factor = 4.887 / 4095;

  float hmd_voltage = hmd_value * hmd_factor;
  float hmd = hmd_voltage * 20;

  return hmd;
}


// упрощённый калман вроде как
// примерный шум измерений
// скорость изменения значений 0.001-1, варьировать самому

float simpleKalman(float newVal, float _err_measure, float _q) {
  float _kalman_gain, _current_estimate;
  static float _err_estimate = _err_measure;
  static float _last_estimate;

  _kalman_gain = (float)_err_estimate / (_err_estimate + _err_measure);
  _current_estimate = _last_estimate + (float)_kalman_gain * (newVal - _last_estimate);
  _err_estimate =  (1.0 - _kalman_gain) * _err_estimate + fabs(_last_estimate - _current_estimate) * _q;
  _last_estimate = _current_estimate;

  return _current_estimate;
}

float simpleKalman_hmd(float newVal, float _err_measure, float _q) {
  float _kalman_gain, _current_estimate;
  static float _err_estimate = _err_measure;
  static float _last_estimate;

  _kalman_gain = (float)_err_estimate / (_err_estimate + _err_measure);
  _current_estimate = _last_estimate + (float)_kalman_gain * (newVal - _last_estimate);
  _err_estimate =  (1.0 - _kalman_gain) * _err_estimate + fabs(_last_estimate - _current_estimate) * _q;
  _last_estimate = _current_estimate;

  return _current_estimate;
}


float median(float newVal) {
  static float buf[3];
  static byte count = 0;
  buf[count] = newVal;
  if (++count >= 3) count = 0;

  float a = buf[0];
  float b = buf[1];
  float c = buf[2];

  float middle;
  if ((a <= b) && (a <= c)) {
    middle = (b <= c) ? b : c;
  } else {
    if ((b <= a) && (b <= c)) {
      middle = (a <= c) ? a : c;
    } else {
      middle = (a <= b) ? a : b;
    }
  }
  return middle;
}

float median_hmd(float newVal) {
  static float hmd_buf[3];
  static byte count = 0;
  hmd_buf[count] = newVal;
  if (++count >= 3) count = 0;

  float a = hmd_buf[0];
  float b = hmd_buf[1];
  float c = hmd_buf[2];

  float middle;
  if ((a <= b) && (a <= c)) {
    middle = (b <= c) ? b : c;
  } else {
    if ((b <= a) && (b <= c)) {
      middle = (a <= c) ? a : c;
    } else {
      middle = (a <= b) ? a : b;
    }
  }
  return middle;
}

void blink(int len) {
  for (int i = 0; i < len; i++) {
    digitalWrite(2, HIGH);
    delay(150);
    digitalWrite(2, LOW);
    delay(150);
  }
}
