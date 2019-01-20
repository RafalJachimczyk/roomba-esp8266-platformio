#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <Wire.h>

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Roomba.h>
#include <SimpleTimer.h>

#include "config.h"

#define OLED_RESET 0 //GPIO0
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 25
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH 16

#define BRC_PIN 14

uint32_t freeMem;
int lastWakeupTime = 0;
int lastStateMsgTime = 0;

SimpleTimer timer;

// ---------- ROOMBA SETUP ---------//

// Roomba setup
Roomba roomba(&Serial, Roomba::Baud115200);

long battery_Current_mAh = 0;
long battery_Voltage = 0;
long battery_Total_mAh = 0;
long battery_percent = 0;
char battery_percent_send[50];
char battery_Current_mAh_send[50];
uint8_t tempBuf[10];

// ---------- ROOMBA SETUP ---------//

// ---------- MQTT SETUP -----------//
WiFiClient espClient;
PubSubClient client(espClient);
// ---------- MQTT SETUP -----------//

// ---------- UTIL -----------------//

void prepareOledDisplay()
{
  display.clearDisplay();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
}

char getFreeHeapMem()
{
  freeMem = system_get_free_heap_size();
  char tmp[32];
  snprintf(tmp, sizeof(tmp), "%d", freeMem);
  return *tmp;
}

// ---------- UTIL -----------------//

// ---------- NETWORKING -----------//

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password))
    {
      //    if (client.connect("ESP8266Client")) {
      Serial.println("MQTT connected");
      // Once connected, publish an announcement...
      client.publish("roomba/debug", "{\"message\":\"roomba connected\"}");
      client.subscribe("roomba/commands");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in a second");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void setupWiFi()
{
  WiFi.mode(WIFI_STA); // added 300716
  WiFi.begin(ssid, password);
  Serial.println();
  Serial.print("Connecting");
  prepareOledDisplay();
  display.print("Connecting");
  display.display();

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
  }

  Serial.println();
  display.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  display.println("Connected\nIP address: ");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
  display.clearDisplay();
}

void setupOTA()
{
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      Serial.println("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      Serial.println("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      Serial.println("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      Serial.println("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

// ---------- NETWORKING -----------//

// ---------- ROOMBA --------------//

void roombaStartClean()
{
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  Serial.write(135);
  client.publish("roomba/debug", "{\"message\":\"roomba cleaning\"}");
  client.publish("roomba/status", "CLEANING");
  Serial.println("Roomba cleaning");
  display.println("Roomba cleaning");
  display.display();
}

void roombaGoToDock()
{
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  Serial.write(143);
  client.publish("roomba/debug", "{\"message\":\"roomba going to dock\"}");
  client.publish("roomba/status", "RETURNING");
  Serial.println("Roomba going to dock");
  display.println("Roomba going to dock");
  display.display();
}

void roombaPowerOff()
{
  Serial.write(128);
  delay(50);
  Serial.write(131);
  delay(50);
  Serial.write(133);
  client.publish("roomba/debug", "{\"message\":\"roomba powered off\"}");
  Serial.println("Roomba powered off");
  display.println("Roomba powered off");
  display.display();
}

void pulseBrc()
{
  digitalWrite(BRC_PIN, HIGH);
  delay(100);
  digitalWrite(BRC_PIN, LOW);
  delay(500);
  digitalWrite(BRC_PIN, HIGH);
  delay(2000);
}

void wakeup()
{
  pinMode(BRC_PIN, OUTPUT);
  digitalWrite(BRC_PIN, LOW);
  delay(200);
  pinMode(BRC_PIN, INPUT);
  delay(200);
  Serial.write(128); // Start
}

void roombaWakeUpOnDock()
{
  wakeup();
#ifdef ROOMBA_650_SLEEP_FIX
  // Some black magic from @AndiTheBest to keep the Roomba awake on the dock
  // See https://github.com/johnboiles/esp-roomba-mqtt/issues/3#issuecomment-402096638
  delay(10);
  Serial.write(135); // Clean
  delay(150);
  Serial.write(143); // Dock
#endif
  client.publish("roomba/debug", "{\"message\":\"roomba wake up brc\"}");
  Serial.println("Roomba wake up brc");
  display.println("Roomba wake up brc");
  display.display();
}

void roombaWakeUpOffDock()
{
  Serial.write(131); // Safe mode
  delay(300);
  Serial.write(130); // Passive mode
  client.publish("roomba/debug", "{\"message\":\"roomba wake up off dock\"}");
  Serial.println("Roomba wake up off dock");
  display.println("Roomba wake up off dock");
  display.display();
}

void sendInfoRoomba()
{
  roomba.start();
  roomba.getSensors(21, tempBuf, 1);
  battery_Voltage = tempBuf[0];
  delay(50);
  roomba.getSensors(25, tempBuf, 2);
  battery_Current_mAh = tempBuf[1] + 256 * tempBuf[0];
  delay(50);
  roomba.getSensors(26, tempBuf, 2);
  battery_Total_mAh = tempBuf[1] + 256 * tempBuf[0];
  if (battery_Total_mAh != 0)
  {
    int nBatPcent = 100 * battery_Current_mAh / battery_Total_mAh;
    String temp_str2 = String(nBatPcent);
    temp_str2.toCharArray(battery_percent_send, temp_str2.length() + 1); //packaging up the data to publish to mqtt
    client.publish("roomba/battery", battery_percent_send);
  }
  if (battery_Total_mAh == 0)
  {
    client.publish("roomba/battery", "NO DATA");
  }
  String temp_str = String(battery_Voltage);
  temp_str.toCharArray(battery_Current_mAh_send, temp_str.length() + 1); //packaging up the data to publish to mqtt
  client.publish("roomba/charging", battery_Current_mAh_send);
}

// ---------- ROOMBA --------------//

void callback(char *topic, byte *payload, unsigned int length)
{
  prepareOledDisplay();

  String command;
  for (int i = 0; i < length; i++)
  {
    command += (char)payload[i];
  }

  if (command == "START")
  {
    roombaStartClean();
  }
  else if (command == "DOCK")
  {
    roombaGoToDock();
  }
  else if (command == "STOP")
  {
    roombaPowerOff();
  }
  else if (command == "WAKE")
  {
    roombaWakeUpOffDock();
  }
  else if (command == "WAKE_BRC")
  {
    roombaWakeUpOnDock();
  }
}

void setup()
{
  // High-impedence on the BRC_PIN
  pinMode(BRC_PIN, INPUT);
  //Serial.begin(115200);

  setupWiFi();
  setupOTA();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  timer.setInterval(5000, sendInfoRoomba);
}

void loop()
{
  ArduinoOTA.handle();

  if (!client.connected())
  {
    reconnect();
  }

  long now = millis();

  // Wakeup the roomba at fixed intervals
  if (now - lastWakeupTime > 10000)
  {
    sendInfoRoomba();
    client.publish("roomba/debug", "{\"message\":\"50s keep-alive\"}");
    Serial.println("Roomba 50s keep-alive");
    prepareOledDisplay();
    display.println("Roomba 50s keep-alive");
    display.display();
    lastWakeupTime = now;

    wakeup();
  }

  client.loop();
}
