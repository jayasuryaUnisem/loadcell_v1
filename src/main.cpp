#include <Arduino.h>
#include <HX711.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
//#include <esp_wifi.h>
//#include "driver/adc.h"
#include <EEPROM.h>

#define HX711_EN 0
#define CALIB_EN 0
#define AUTO_CALIB 0
#define WEIGHT_CHECK 0
#define BATTERY_CHECK 1
#define WIFI_EN 1
#define BT_EN 1
#define EEPROM_EN 1
#define SEND_PACKET_EN 1


#define DOUT 2
#define CLK 4
#define BTT_PIN 32
#define BTN 16

#define BT_ID "ESP32_CONN_TEST" 

#if BT_EN
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;
#endif

String SSID = "hack";
String PASSWORD = "virusvirus";

float calibration_factor = 124725; //124725
double calibWeight = 0.71;

unsigned long firstv;
unsigned long secondv;

static bool state = false;

HX711 scale;

void loadCell_init();
int calib();
void tareCheck();
String weight();
float voltage_check();
void enableWiFi();
void disableWiFi();
void BT_FUNCTION();
void BT_ENABLE();
void BT_DISABLE();
void EEPROM_WRITE(String, String);
void EEPROM_READ();

static int flag1 = 0;

#if HX711_EN 
void loadCell_init()
{
  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  scale.tare();
}

int calib()
{
  scale.set_scale(calibration_factor);
  Serial.print("Reading: ");
  float weight = scale.get_units() * 0.453592;
  String value = String(weight, 3);
  Serial.print(value); //scale.get_units()*0.453592,3
  Serial.print("Kg");
  Serial.print(" Calibration_factor ");
  Serial.print(calibration_factor);
  Serial.println();
  //tareCheck();
  if (weight > calibWeight)
  {
    calibration_factor += 200;
  }
  if (weight < calibWeight)
  {
    calibration_factor -= 200;
  }
  else
  {
    //Serial.println("Clibd Done!!");
  }
  if (Serial.available())
  {
    String temp = Serial.readString();
    Serial.println("Im Here");
    if (temp == "stop")
      return 1;
  }
  delay(100);
  return 0;
}

void tareCheck()
{
  SerialBT.print("\nTare Function Triggered!");
  scale.tare();
}

String weight()
{
  scale.set_scale(calibration_factor);
  return String(scale.get_units() * 0.453592, 3);
}
#endif

#if BATTERY_CHECK
float voltage_check() //R1 3M, R2 1.13M
{
  return ((analogRead(BTT_PIN) * 3.3) / 4095);
}
#endif

#if WIFI_EN 
void disableWiFi()
{
  //adc_power_off();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi Disabled..");
}

void enableWiFi()
{
  //adc_power_on();
  WiFi.disconnect(false);
  WiFi.mode(WIFI_STA);

  Serial.println("WIFI Connecting");
  WiFi.begin(SSID.c_str(), PASSWORD.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}
#endif

#if BT_EN
void BT_FUNCTION()
{
  String ssid;
  String password;
  while (SerialBT.available() == 0)
    ;
  ;
  String payload = SerialBT.readString();
  payload.trim();
  if (payload == "start")
  {
    SerialBT.print("\n1. WiFi Config\n2. Tare Function\n3. Exit ");
    SerialBT.print("\nEnter the Option: ");
    delay(1000);
    while (SerialBT.available() == 0)
      ;
    ;
    //Serial.println();
    payload = SerialBT.readString();
    payload.trim();
    if (payload == "1")
    {
      SerialBT.println("\nWiFi Config Option Selected!!");
      SerialBT.print("\nEnter SSID: ");
      while (SerialBT.available() == 0)
        ;
      ;
      ssid = SerialBT.readString();
      ssid.trim();
      SerialBT.print("\nEnter the password: ");
      while (SerialBT.available() == 0)
        ;
      ;
      password = SerialBT.readString();
      password.trim();
      SerialBT.print("SSID: ");
      SerialBT.print(ssid);
      SerialBT.print("\nPassword: ");
      SerialBT.println(password);
      SerialBT.println("Config Done");
      EEPROM_WRITE(ssid, password);
    }
    else if (payload == "2")
    {
      SerialBT.println("Tare Function triggred");
    }
    else
    {
      SerialBT.println("Exit Pressed");
    }
  }
}

void BT_ENABLE()
{
  SerialBT.begin(BT_ID);
  Serial.println("Bluetooth Enabled..");
  SerialBT.println("Bluetooth Enabled");
}

void BT_DISABLE()
{
  SerialBT.println("Bluetooth Disabled..");
  Serial.println("Bluetooth Disabled..");
  delay(100);
  SerialBT.flush();
  SerialBT.end();
}
#endif

#if EEPROM_EN
void EEPROM_WRITE(String ssid, String password)
{
  EEPROM.begin(512);
  String con = ssid + "," + password;
  int len = con.length();
  EEPROM.put(0, len);
  int i;
  Serial.print("Write Len");
  Serial.println(len);
  for (i = 0; i < len; i++)
  {
    EEPROM.put(0x0F + i, con[i]);
    //Serial.println(ssid[i]);
  }
  EEPROM.commit();
  EEPROM.end();
  Serial.println("EEPROM Commit Done!");
  delay(1000);
  Serial.println("EEPROM Read start!!");
  EEPROM_READ();
}

void EEPROM_READ()
{
  int len;
  String ssid;
  String password;
  String payload;
  EEPROM.begin(512);
  EEPROM.get(0, len);
  Serial.print("Length: ");
  Serial.println(len);
  if (len > 1 and len < 50)
  {
    int i;
    for (i = 0; i < len; i++)
    {
      payload = payload + char(EEPROM.read(0x0f + i));
    }
    EEPROM.end();
    int in = payload.indexOf(",");
    ssid = payload.substring(0, in);
    password = payload.substring(in + 1);
    Serial.print("payload: ");
    Serial.println(payload);
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("PASSWORD: ");
    Serial.println(password);
    SSID = ssid;
    PASSWORD = password;
  }

  delay(100);
}
#endif

void change()
{
  if(flag1 == 0)
  {
    firstv = millis();
    //Serial.println("Count Start");
    flag1 = 1;
  }
  else
  {
    secondv = millis();
    //Serial.println(secondv - firstv);
    if(secondv-firstv >= 3000UL)
    {
      Serial.print(secondv-firstv);
      Serial.println(" Long Pressed");
      state = true;
    }
    //firstv = secondv;
    flag1 = 0;
  }
}

void setup()
{
  Serial.begin(38400);
  pinMode(BTN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BTN), change, CHANGE);
  // attachInterrupt(BTN, up, RISING);
  // attachInterrupt(BTN, down, FALLING);
#if HX711_EN
  loadCell_init();
#endif

#if WIFI_EN
  delay(100);
  EEPROM_READ();
  disableWiFi();
#endif

#if BT_EN
  BT_DISABLE();
#endif
}

void loop()
{

#if CALIB_EN
  Serial.println("Press 1 for start the calibration process\nPress 2 for skip the calibration process");
  while (1)
  {
    String payload = Serial.readString();
    if (payload == "1")
    {
      static int temp = 0;
      while (temp == 0)
      {
        temp = calib();
      }
      Serial.println("Calib Done");
      break;
    }
    else if (payload == "2")
    {
      Serial.println("Calibration Process Skipped");
      break;
    }
    delay(1000);
  }
  while (1)
  {
    Serial.print(weight());
    Serial.println(" kg");
    tareCheck();
    delay(1000);
  }
#endif
#if WEIGHT_CHECK
  Serial.print("Weight: ");
  Serial.print(weight());
  Serial.println("kg");
  tareCheck();
  delay(1000);
#endif

#if BATTERY_CHECK
  Serial.print("Battery Voltage: ");
  Serial.println(voltage_check());
  delay(1000);
#endif

#if BT_EN
if (state == true)
{
  BT_ENABLE();
  delay(1000);
  
  BT_FUNCTION();
  state = false;
  BT_DISABLE();
 
  delay(100);
}
#endif

#if SEND_PACKET_EN

#endif

Serial.println("Interrupt Check code");
delay(5000);
}