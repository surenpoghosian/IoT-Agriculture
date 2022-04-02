#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "GyverButton.h"
#include <AESLib.h>

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

String localAddress = "1";     // address of this device
String srvr_device = "105";

int flag;

byte LoRaData;

int tmp_pin = 13;
int hmd_pin = 34;
int l = 0;
//int a_pin = 36;
//int b_pin = 37;

float tmp = 0.0;
float hmd = 0.0;

int hmd_value = 0;

bool b = false;

GButton butt1(BTN_PIN);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

AESLib aesLib;

char cleartext[256];
char ciphertext[512];

// AES Encryption Key
byte aes_key[] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };

// General initialization vector (you must use your own IV's in production for full security!!!)
byte aes_iv[N_BLOCK] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

unsigned long loopcount = 0;
byte enc_iv[N_BLOCK] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // iv_block gets written to, provide own fresh copy...
byte dec_iv[N_BLOCK] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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
  aesLib.set_paddingmode(paddingMode::CMS);

  char b64in[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  char b64out[base64_enc_len(sizeof(aes_iv))];
  base64_encode(b64out, b64in, 16);

  char b64enc[base64_enc_len(10)];
  base64_encode(b64enc, (char*) "0123456789", 10);

  char b64dec[ base64_dec_len(b64enc, sizeof(b64enc))];
  base64_decode(b64dec, b64enc, sizeof(b64enc));

  Serial.println("LoRa Initializing OK!");
  display.setCursor(0, 10);
  display.print("LoRa Initializing OK!");
  display.display();


}


void loop() {

  //  struct Transmit {
  //    byte dv_id;
  //    byte sender;
  //  };
  //
  //  struct Transmit d;



  int packetSize = LoRa.parsePacket();

  if (packetSize > 16) {
//    l++;
//    if (l > 42) {
//      ESP.restart();
//    }
    while (LoRa.available()) {
      String LoRaData = LoRa.readString();
//      Serial.println(LoRaData);

      uint16_t dlen = LoRaData.length();
      sprintf(ciphertext, "%s", LoRaData.c_str());

      String decrypted = decrypt( ciphertext, dlen, dec_iv);
//      Serial.print("Cleartext: ");
//      Serial.println(decrypted);

      String flag = getValue(decrypted, '|', 0);
      String w = getValue(decrypted, '|', 1);

      String dest = getValue(w, '&', 0);
      String sender = getValue(w, '&', 1);

//      Serial.println(w);
//      Serial.println(dest);
//      Serial.println(sender);


      if (flag == "255") {
        if (dest == localAddress) {
          if (sender == srvr_device) {
            tmp = get_tmp(tmp_pin);
            hmd = get_hmd(hmd_pin);

            float tmp_median = median(tmp);
            float val = simpleKalman(tmp_median, 0.6, 0.09);

            float hmd_median = median_hmd(hmd);
            float val_hmd = simpleKalman_hmd(hmd_median, 0.8, 0.09);
            Serial.print(hmd);
            Serial.print("\t");
            Serial.print(hmd_median);
            Serial.print("\t");
            Serial.println(val_hmd);

            String n = String(val) + "&" + String(val_hmd);
            display.clearDisplay();
            display.setCursor(0, 10);
            display.println("Sensor");
            display.setTextSize(1);
            display.setCursor(0, 40);
            display.println("HMD - " + String(val_hmd));


            display.display();
            String plaintext = srvr_device + "|" + localAddress + "$" + n;

            sprintf(cleartext, "%s", plaintext.c_str()); // must not exceed 255 bytes; may contain a newline

            // Encrypt
            uint16_t clen = String(cleartext).length();
            String encrypted = encrypt(cleartext, clen, enc_iv);
            sprintf(ciphertext, "%s", encrypted.c_str());
            //            Serial.print("Ciphertext: ");
            //            Serial.println(encrypted);
            uint16_t enc_length = encrypted.length();
            //            sendMessage(encrypted, enc_length, destination, localAddress);
            sendMessage(encrypted);

            //            uint16_t dlen = encrypted.length();
            //            String decrypted = decrypt( ciphertext, dlen, dec_iv);
            //            Serial.print("Cleartext: ");
            //            Serial.println(decrypted);
            //
            //            if (decrypted.equals(cleartext)) {
            //              Serial.println("SUCCES");
            //            }
            //            else
            //            {
            //              Serial.println("FAILURE");
            //
            //            }
          }
        }
      }







    }
  }



}



void sendMessage(String outgoing) {

  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);
  LoRa.endPacket();                     // finish packet and send it

}



String encrypt(char * msg, uint16_t msgLen, byte iv[]) {
  int cipherlength = aesLib.get_cipher64_length(msgLen);
  char encrypted[cipherlength]; // AHA! needs to be large, 2x is not enough
  aesLib.encrypt64(msg, msgLen, encrypted, aes_key, sizeof(aes_key), iv);
  // Serial.print("encrypted = "); Serial.println(encrypted);
  return String(encrypted);
}

String decrypt(char * msg, uint16_t msgLen, byte iv[]) {
  char decrypted[msgLen];
  aesLib.decrypt64(msg, msgLen, decrypted, aes_key, sizeof(aes_key), iv);
  return String(decrypted);
}

void wait(unsigned long milliseconds) {
  unsigned long timeout = millis() + milliseconds;
  while (millis() < timeout) {
    yield();
  }
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
void blink(int len) {
  for (int i = 0; i < len; i++) {
    digitalWrite(2, HIGH);
    delay(150);
    digitalWrite(2, LOW);
    delay(150);
  }
}
