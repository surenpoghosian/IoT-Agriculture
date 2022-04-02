#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


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


int tmp_pin = 13;
int hmd_pin = 34;

float tmp = 0.0;
float hmd = 0.0;

int hmd_value = 0;

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

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  Serial.println("LoRa Sender Test");


}


void loop() {

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

  //  String n = String(val) + "&" + String(val_hmd);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(25, 25);
  display.println(String(val_hmd) + "%");
  display.display();
  delay(1000);
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
