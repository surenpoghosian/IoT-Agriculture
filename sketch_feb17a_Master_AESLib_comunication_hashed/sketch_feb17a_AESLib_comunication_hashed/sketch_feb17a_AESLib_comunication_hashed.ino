#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "GyverButton.h"
#include <RTClib.h>
#include <AESLib.h>

#define BTN_PIN 12

//       LoRa      Pin
#define  LoRa_SCK    5
#define  LoRa_MISO  19
#define  LoRa_MOSI  27
#define  LoRa_CS    18
#define  LoRa_RST   14
#define  DI0        26
#define  BAND    915E6

#define  SD_SCK      5
#define  SD_MISO    19
#define  SD_MOSI    27
#define  SD_CS      13

#define  Select    LOW
#define  DeSelect  HIGH

#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

AESLib aesLib;

char cleartext[256];
char ciphertext[512];

char cleartext_2[256];
char ciphertext_2[512];

// AES Encryption Key
byte aes_key[] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };

// General initialization vector (you must use your own IV's in production for full security!!!)
byte aes_iv[N_BLOCK] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };




//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

GButton butt1(BTN_PIN);

//byte LoRaData;
String localAddress = "105";
String destination = "1";
String garden_id = "1|1";
uint8_t destination_array[2] = {0x01, 0x03};
String watering_id = "9";
String info = "";
String all_data = "";
String rx_data = "";
String request_data = "";

int Count = 0;
int msgCount = 0;
int lot = 0;
int interval = 2000;
int l = 0;
long lastSendTime = 0;

//const char* ssid     = "Cryptex";
//const char* password = "1234567890";

const char* ssid     = "ICN_000264";
const char* password = "83bjd86f2v";

const char* serverName_registrate = "http://213.136.78.202:8090/registrate";
const char* serverName_get_data = "http://213.136.78.202:8090/get";

bool mark = false;

RTC_DS1307 rtc;

unsigned long loopcount = 0;
byte enc_iv[N_BLOCK] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // iv_block gets written to, provide own fresh copy...
byte dec_iv[N_BLOCK] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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


  pinMode(SD_CS, OUTPUT);
  pinMode(LoRa_CS, OUTPUT);
  digitalWrite(LoRa_CS, DeSelect);

  if (!rtc.begin()) {

    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    //    // following line sets the RTC to the date & time this sketch was compiled
    //    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  }
  //  // following line sets the RTC to the date & time this sketch was compiled
  //  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //  else {
  //    // following line sets the RTC to the date & time this sketch was compiled
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //  }



  //aes_init();
  aesLib.set_paddingmode(paddingMode::CMS);

  char b64in[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  char b64out[base64_enc_len(sizeof(aes_iv))];
  base64_encode(b64out, b64in, 16);

  char b64enc[base64_enc_len(10)];
  base64_encode(b64enc, (char*) "0123456789", 10);

  char b64dec[ base64_dec_len(b64enc, sizeof(b64enc))];
  base64_decode(b64dec, b64enc, sizeof(b64enc));



  Serial.println("Receiver Test");
  WiFi.begin(ssid, password);
  Serial.println("Connecting");


  int c = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    c++;
    if (c > 13) {
      return;
    }
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Initializing OK!");
  display.setCursor(0, 10);
  display.println("Initializing OK!");
  display.display();



}



void loop() {

  if (mark == false) {

    digitalWrite(LoRa_CS, Select);
    SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_CS);
    LoRa.setPins(LoRa_CS, LoRa_RST, DI0);

    if (!LoRa.begin(BAND)) {
      Serial.println("LoRa Starting failed!");
      //while (1);
      return;
    }
    mark = true;
  }



  if (millis() - lastSendTime > interval) {

    byte flag = 0xFF;
    String data_to_send = String(flag) + "|" + String(destination_array[lot]) + "&" + String(localAddress);

    sprintf(cleartext, "%s", data_to_send.c_str()); // must not exceed 255 bytes; may contain a newline

    // Encrypt
    uint16_t clen = String(cleartext).length();
    String encrypted = encrypt(cleartext, clen, enc_iv);
    sprintf(ciphertext, "%s", encrypted.c_str());
    Serial.print("Ciphertext: ");
    Serial.println(encrypted);
    uint16_t enc_length = encrypted.length();
    sendMessage(encrypted);


    Serial.println("Sending " + String(flag) + " " + String(destination_array[lot]));
    lastSendTime = millis();            // timestamp the message
    interval = random(2000) + 500;    // delay
    l++;
    if (l > 69) {
      ESP.restart();
    }
    if (lot >= (sizeof(destination_array) - 1))
    {
      lot = 0;
    }
    else {
      lot++;
    }
  }




  int packetSize = LoRa.parsePacket();
  if (packetSize > 16) {
    Serial.println("packetSize= " + String(packetSize));


    while (LoRa.available()) {

      String LoRaData = LoRa.readString();
      Serial.println(LoRaData);
      uint16_t dlen = LoRaData.length();
      sprintf(ciphertext, "%s", LoRaData.c_str());

      String decrypted = decrypt( ciphertext, dlen, dec_iv);
      Serial.print("Cleartext: ");
      Serial.println(decrypted);
      String w = getValue(decrypted, '$', 0);
      String dest = getValue(w, '|', 0);
      String sender = getValue(w, '|', 1);
      Serial.println(w);
      Serial.println(dest);
      Serial.println(sender);
      if (dest == localAddress) {
        String tm = get_time();
        String data_request = getValue(decrypted, '$', 1);
        info =   sender + "|" + data_request + "?" + tm;
        Serial.println(info);
      }

    }


    String t = "";
    bool b = false;

    if (WiFi.status() == WL_CONNECTED) {
      if (sd_check(SD, "/datalog.txt")) {
        Serial.println("File is not empty");
        delay(200);
        t =  sd_data(SD, "/datalog.txt");
        if (t != "Fail") {
          Serial.println(t);
          b = true;
        }
      }

      WiFiClient client;
      HTTPClient http;
      http.begin(client, serverName_registrate);
      http.addHeader("Content-Type", "text/plain");


      if (b == true) {
        int httpResponseCode = http.POST(t);

        if (httpResponseCode == 200 || httpResponseCode == -2) {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          sd_clean(SD, "/datalog.txt");
        }
      }

      int httpResponseCode = http.POST(info);


      if (httpResponseCode == 200 || httpResponseCode == -2) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
      }
      else {

        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        mark = false;
        String sd_info = info + " ";
        sd(sd_info);

      }

      http.end();






      http.begin(client, serverName_get_data);
      int httpResponseCode_2 = http.POST(garden_id);

      if (httpResponseCode_2 > 0) {

        String payload = http.getString();
        String yes_no = getValue(payload, '|', 0);

        if (yes_no == "yes") {
          Serial.println("in if statement->" + payload);

          String intrvl = getValue(payload, '|', 1);
          Serial.println(intrvl);

          Serial.println(intrvl);
          String text = watering_id + "|" + localAddress + "&" + intrvl;
          Serial.println(text);
          sprintf(cleartext_2, "%s", text.c_str()); // must not exceed 255 bytes; may contain a newline

          // Encrypt
          uint16_t clen = String(cleartext_2).length();
          String encrypted = encrypt(cleartext_2, clen, enc_iv);

          sprintf(ciphertext_2, "%s", encrypted.c_str());

          Serial.print("Ciphertext for watering system: ");
          Serial.println(encrypted);
          uint16_t enc_length = encrypted.length();
          sendMessage(encrypted);

        }
        else {
          Serial.println(payload);
        }




      }
      else {
        Serial.println("An error occurred sending the request");
      }

      http.end();



    }
    else {
      mark = false;
      String sd_info = info + " ";
      sd(sd_info);
    }
    String x=getValue(info,'|',1);
    String y=getValue(x,'?',0);
    String z=getValue(y,'&',1);
    String z_1=getValue(x,'?',1);
    
    int rssi = LoRa.packetRssi();
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("RECEIVER");
    display.setCursor(0, 30);
    display.print("HMD->" + String(z));
    display.setCursor(0, 40);
    display.print("TIME->" + String(z_1));
    
    display.display();
  }
}










bool sd_check(fs::FS & fs, const char * path) {

  digitalWrite(LoRa_CS, DeSelect);

  Serial.println("\nInitializing SD card...");
  digitalWrite(SD_CS, Select);

  SPIClass spi = SPIClass(VSPI);
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, spi, 80000000)) {
    Serial.println("Card Mount Failed");
    return false;
  }

  else {
    Serial.println("initialization done.\n");

  }


  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }

  Serial.print("Read from file: ");

  if (file.available()) {
    Serial.println("not empty");
    file.close();
    digitalWrite(SD_CS, DeSelect);    //  DESELECT (high) SD Card SPI
    SD.end();
    return true;
  }

  else {
    Serial.println("empty");
    digitalWrite(SD_CS, DeSelect);    //  DESELECT (high) SD Card SPI
    SD.end();
    return false;
  }


}





String sd_data(fs::FS & fs, const char * path) {

  Serial.println("\nInitializing SD card...");
  digitalWrite(SD_CS, Select);

  SPIClass spi = SPIClass(VSPI);
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, spi, 80000000)) {
    Serial.println("Card Mount Failed");
    return "Fail";
  }

  else {
    Serial.println("initialization done.\n");

  }
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "Fail";
  }

  Serial.print("Read from file: ");

  String D = "";

  while (file.available()) {
    String t = String(char(file.read()));
    D += t;
  }
  //  Serial.println(D);
  file.close();
  digitalWrite(SD_CS, DeSelect);    //  DESELECT (high) SD Card SPI
  SD.end();
  return D;
}







void sd_clean(fs::FS & fs, const char * path) {

  Serial.println("\nInitializing SD card...");
  digitalWrite(SD_CS, Select);

  SPIClass spi = SPIClass(VSPI);
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, spi, 80000000)) {
    Serial.println("Card Mount Failed");
    return;
  }

  else {
    Serial.println("initialization done.\n");

  }

  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print("")) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
  digitalWrite(SD_CS, DeSelect);    //  DESELECT (high) SD Card SPI
  SD.end();
}








void sd(String text) {

  digitalWrite(LoRa_CS, DeSelect);

  Serial.print("Initializing SD card...");
  digitalWrite(SD_CS, Select);

  SPIClass spi = SPIClass(VSPI);
  spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, spi, 80000000)) {
    Serial.println("Card Mount Failed");
    return;
  }
  else {
    Serial.println("initialization done.");

  }


  appendFile(SD, "/datalog.txt",  text.c_str());

  digitalWrite(SD_CS, DeSelect);    //  DESELECT (high) SD Card SPI
  SD.end();

}




void sendMessage(String outgoing) {

  LoRa.beginPacket();                   // start packet
  LoRa.print(outgoing);
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




void writeFile(fs::FS & fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}






void appendFile(fs::FS & fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}






void readFile(fs::FS & fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.print("Read from file: ");

  while (file.available()) {
    Serial.write(file.read());
  }

  file.close();
}



String encrypt(char * msg, uint16_t msgLen, byte iv[]) {
  int cipherlength = aesLib.get_cipher64_length(msgLen);
  char encrypted[cipherlength]; // AHA! needs to be large, 2x is not enough
  aesLib.encrypt64(msg, msgLen, encrypted, aes_key, sizeof(aes_key), iv);
  Serial.print("encrypted = "); Serial.println(encrypted);
  return String(encrypted);
}

String decrypt(char * msg, uint16_t msgLen, byte iv[]) {
  char decrypted[msgLen];
  aesLib.decrypt64(msg, msgLen, decrypted, aes_key, sizeof(aes_key), iv);
  return String(decrypted);
}



String get_time() {

  String date = "";
  DateTime now = rtc.now();
  DateTime future (now + TimeSpan(0, 0, 0, 20));
  date += String(future.year(), DEC) + "/" + String(future.month(), DEC) + "/" + String(future.day(), DEC) + "_" + String(future.hour(), DEC) + ":" + String(future.minute(), DEC) + ":" + String(future.second(), DEC);
  Serial.println(date);

  return date;
}

void wait(unsigned long milliseconds) {
  unsigned long timeout = millis() + milliseconds;
  while (millis() < timeout) {
    yield();
  }
}
