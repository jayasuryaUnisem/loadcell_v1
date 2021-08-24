#include <Arduino.h>
#include <HX711.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include <esp_wifi.h>
#include "driver/adc.h"
#include <EEPROM.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "time.h"

#define BT_ID "ESP32_CONN_TEST"
#define PUB_TOPIC "/data"
#define BTT_PIN 32
#define BTN_PIN 16
#define DOUT 2
#define CLK 4

#define WEIGHT_MEASURE 1
#define VOLTAGE_MEASURE 1

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

//BluetoothSerial SerialBT;
HX711 scale;

float calibration_factor = 124725; //124725

const char *mqtt_server = "192.168.1.15";
//const char* mqtt_server = "YOUR_MQTT_BROKER_IP_ADDRESS";

WiFiClient espClient;
PubSubClient client(espClient);

String SSID = "hack";
String PASSWORD = "virusvirus";
String MACC_ADD;

static int flag1 = 0;
unsigned long firstv;
unsigned long secondv;
static bool state = false;

long lastMsg = 0;
char msg[50];
int value = 0;


void disableWiFi();
void enableWiFi();
void BT_FUNCTION();
void BT_ENABLE();
void BT_DISABLE();
void EEPROM_WRITE(String, String);
void EEPROM_READ();
float VOLT_CHECK();
void LOADCELL_INIT();
String WEIGHT();
void TARE();
void reconnect();
void callback(char *, byte *, unsigned int);
void PACKET_SEND();
void TIMESTAMP_CONFIG();
String printLocalTime();

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
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Serial.print(".");
    if(++attempt>300)
      break;
  }
  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  MACC_ADD = WiFi.macAddress();
}

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

void change()
{
  if (flag1 == 0)
  {
    firstv = millis();
    //Serial.println("Count Start");
    flag1 = 1;
  }
  else
  {
    secondv = millis();
    //Serial.println(secondv - firstv);
    if (secondv - firstv >= 3000UL)
    {
      Serial.print(secondv - firstv);
      Serial.println(" Long Pressed");
      state = true;
    }
    //firstv = secondv;
    flag1 = 0;
  }
}

float VOLT_CHECK()
{
  return ((analogRead(BTT_PIN) * 3.3) / 4095);
}

void LOADCELL_INIT()
{

  scale.begin(DOUT, CLK);
  scale.set_scale(calibration_factor);
  TARE();
}

String WEIGHT()
{
  String payload = String(scale.get_units() * 0.453592, 3);
  return payload;
}

void TARE()
{
  scale.tare();
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client"))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void MQTT_INIT()
{
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void PACKET_SEND()
{
  int result = 0;
  enableWiFi();
  MQTT_INIT();
  while (result == 0)
  {
    StaticJsonDocument<200> doc;
    doc["CID"] = MACC_ADD;
    doc["timestamp"] = printLocalTime();
    doc["weight"] = WEIGHT();
    doc["voltage"] = VOLT_CHECK();
    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);
    Serial.println(jsonBuffer);
    if (!client.connected())
    {
      reconnect();
    }
    client.loop();
    int response = client.publish("data", jsonBuffer);
    if(response==1)
      Serial.println("Packet Send OK");
    else
      Serial.println("Packetr Send ERR");
    delay(2000);
  }
}

String printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return "\0";
  }
  char temp[25];
  strftime(temp, 25, "%d/%b/%Y %H:%M:%S\0", &timeinfo);
  return String(temp);
}

void TIMESTAMP_CONFIG()
{
  enableWiFi();
  configTime(4500, 4500, "pool.ntp.org");
  delay(1000);
  disableWiFi();

}

void setup()
{
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(BTN_PIN), change, CHANGE);
  EEPROM_READ();
  TIMESTAMP_CONFIG();
  delay(1000);
  LOADCELL_INIT();
}

void loop()
{
  //BT_FUNCTION();
  PACKET_SEND();
}