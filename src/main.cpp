
#include <HX711.h>
#include <WiFi.h>
#include <BluetoothSerial.h>

#define HX711_EN 0
#define CALIB_EN 0
#define AUTO_CALIB 0
#define WEIGHT_CHECK 0
#define BATTERY_CHECK 0
#define WIFI_EN 0
#define BT_EN 0

#define DOUT 2
#define CLK 4
#define BTT_PIN 32
#define BTN 16

#if BT_EN
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;
#endif

const char *SSID = "UNISEM AIRTEL";
const char *PASSWORD = "9902330851";

float calibration_factor = 124725; //124725
double calibWeight = 0.71;

float Bgap = 1.001;
float Sgap = 0.010;

float tolrance = 0.010;

unsigned long firstv;
unsigned long secondv;

HX711 scale;

void loadCell_init();
int calib();
void tareCheck();
String weight();
double voltage_check();
void initWiFi();
void BT_serial();
void networkScan();
void up();
void down();

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
  if (Serial.available())
  {
    if (Serial.readString() == "tare")
      scale.tare();
  }
}

String weight()
{
  scale.set_scale(calibration_factor);
  return String(scale.get_units() * 0.453592, 3);
}

double voltage_check() //R1 3M, R2 1.13M
{
  double value = analogRead(BTT_PIN);
  return ((value * 3.3) / 4095);
}
#endif

#if WIFI_EN 
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
}
#endif

#if BT_EN
void BT_serial()
{
  if (Serial.available())
  {
    if (Serial.readString() == "send")
    {
      SerialBT.print("1. Config WiFi Crendential");
      delay(10);
      SerialBT.print("\n2. Tare Function");
      delay(10);
      SerialBT.print("\n3. Exit\n");
      delay(100);
    }
    //SerialBT.flush();
    while (1)
    {
      if (SerialBT.available())
      {
        String payload = SerialBT.readString();
        payload.trim();
        //Serial.println(payload);
        if (payload == "1")
        {
          Serial.println("WiFi Configuration Mode");
          networkScan();
        }
        if (payload == "2")
        {
          Serial.println("Tare Funtion");
        }
        if (payload == "3")
        {
          Serial.println("Exit Function");
        }
        delay(100);
      }
      delay(100);
    }
  }
  if (SerialBT.available())
  {
    Serial.print(SerialBT.readString());
  }
  delay(1000);
}

void networkScan()
{
  SerialBT.println("Scan Start");
  int n = WiFi.scanNetworks();
  if (n == 0)
  {
    SerialBT.println("No Network Found");
  }
  else
  {
    SerialBT.print(n);
    String ssidList[n];
    SerialBT.println(" Network Found\n");
    for (int i = 0; i < n; i++)
    {
      ssidList[i+1] = WiFi.SSID(i);
      SerialBT.print(i + 1);
      delay(10);
      SerialBT.print(": ");
      SerialBT.print(WiFi.SSID(i));
      delay(10);
      SerialBT.print(" (");
      SerialBT.print(WiFi.RSSI(i));
      delay(10);
      SerialBT.print(")");
      SerialBT.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(50);
    }
    delay(100);
    SerialBT.println("\nEnter Total count + 1 for scan again: ");
    SerialBT.print("\nPlease Enter Network number: ");
    SerialBT.flush();
    while (1)
    {
      if (SerialBT.available())
      {
        String payload = SerialBT.readString();
        payload.trim();
        int tempValue = payload.toInt();
        Serial.print("Selected value: ");
        Serial.println(tempValue);
        if (tempValue > 0 and tempValue <= n)
        {
          SerialBT.print("Selected SSID: ");
          SerialBT.println(ssidList[tempValue]);
          break;
        }
        else if (tempValue > n+1){
          SerialBT.print("Please Enter the number between: 1 to ");
          SerialBT.println(n);
          }
        else if(tempValue == n+1)
        {
          networkScan();
        }
      }
      delay(100);
    }
  }
  SerialBT.println();
  delay(2000);
}
#endif


void up()
{
  firstv = millis();
  Serial.println("UP function");
}

void down()
{
  secondv = millis();
  if((secondv - firstv)>=8000)
  {
    Serial.print(secondv-firstv);
    Serial.println(" Long Press");
    firstv = secondv;
  }else{
    //Serial.println("Short Press");
  }
}

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
  initWiFi();
#endif

#if BT_EN
  SerialBT.begin();
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
  Serial.print(scale.get_units() * 0.453592, 3);
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
  BT_serial();
  delay(100);
#endif
Serial.println("Interrupt Check code");
delay(5000);
}