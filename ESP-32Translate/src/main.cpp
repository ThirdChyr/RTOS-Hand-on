#include <DFRobotDFPlayerMini.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <cstdlib>
#include <EEPROM.h>
#include <string.h>
#include <esp_task_wdt.h>
#include <map>

// using std::map;
using std::string;

#define MQTT_SERVER "20.243.148.107"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_NAME "ESP_Client"
#define WDT_TIMEOUT 20 
#define RXD2 16
#define TXD2 17
#define EEPROM_SIZE 1024

WiFiClient client;
PubSubClient mqtt(client);
HardwareSerial mySerial(2);
DFRobotDFPlayerMini myDFPlayer;

const char *ssid = "Iphone";
const char *pass = "tatty040347";
int count_timer = 0;
int newtime = 0;
volatile bool isPlaying = true;
volatile bool watchdogfeed = false;
String Word[100] = {""};

void backupEEPROM(int length)
{
  EEPROM.write(0, length);
  EEPROM.commit();
  Serial.println("Save backup value");
  
}
void print_word(int count)
{
  for (int i = 0; i < count; i++)
  {
    Serial.println("Word : ");
    Serial.print(Word[i]);
  }
}
void OutController(String value)
{
  isPlaying = false;
  std::map<string, int> TransProtocals =
      {
          {"Hello", 1},
          {"My", 2},
          {"Name", 3},
          {"Is", 4},
          {"Chayathon", 5},
          {"Rungrueang", 6},
          {"Kasetsartuniversity", 7},
          {"Goodbye", 8},
          {"Thankyou", 9},
          {"SeeYouLater", 10},
          {"GoodMorning", 11},
          {"GoodNight", 12},
          {"GoodAfternoon", 13},
          {"GoodEvening", 14},
          {"HowAreYou", 15},
          {"ImFine", 16},
          {"NiceToMeetYou", 17},
          {"Welcome", 18},
          {"Sorry", 19},
          {"ExcuseMe", 20},
          {"Repeat", 999},
      };
  int number = 0;
  String reload = "";
  int counting = 0;
  if (value != "1Repeat")
  {
    number = value.charAt(0) - '0';
    //Serial.println("Flow this");
    for (int i = 1; i <= value.length(); i++)
    {
      if (isUpperCase(value.charAt(i)) || i == value.length())
      {
        if (!reload.isEmpty())
        {
          Word[counting++] = reload;
          if (counting >= 100)
            break;
        }
        reload = String(value.charAt(i));
      }
      else
      {
        reload += value.charAt(i);
      }
    }
  }
  else
  {
    number = (int)EEPROM.read(0);
  Serial.println("=====================================");
  Serial.print("Backup value :");
  Serial.println(number);
  Serial.println("=====================================");

  }
  // // Back up value
  backupEEPROM(value.charAt(0) - '0');
  Serial.println("=====================================");
  Serial.print("Backup value :");
  Serial.print(number);
  Serial.println("");
  Serial.println("=====================================");
  Serial.println("Playing Words..........");


  // // backup

  for (int i = 0; i < number; i++)
  {

    if (TransProtocals.find(Word[i].c_str()) != TransProtocals.end())
    {
      int PointNumber = TransProtocals[Word[i].c_str()];
      // Serial.println(PointNumber);
      if (myDFPlayer.available())
      {
        myDFPlayer.play(PointNumber);
        Serial.print("Playing : ");
        Serial.print(PointNumber);
        Serial.println(Word[i]);
      }
      delay(1000);
    }
    else
    {
      Serial.print("Error: No sound file for ");
      Serial.println(Word[i]);
    }
  }

  isPlaying = true;
  Serial.println("Playing Done");
  Serial.println("=====================================");
}

void Showsound(int number)
{
  if (myDFPlayer.available())
  {
    myDFPlayer.play(number);
    Serial.print("Playing: ");
    Serial.println(number);
    delay(1000);
  }
  else
  {
    Serial.print("Error: No sound file for ");
    Serial.println(number);
  }
}
void callback(char *topic, byte *payload, unsigned int length)
{
  
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';
  String topic_str = topic;
  String payload_str = msg;
  if (isPlaying)
  {
    if (topic_str == "Translate/ESP32/Word")
    {
      // feed = false;
      Serial.println("[" + topic_str + "]: " + payload_str);
      OutController(payload_str);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("Start Connect WiFi");
  WiFi.begin(ssid, pass);
  Serial.println("Out Flow WIFI");

  esp_task_wdt_deinit();
  esp_task_wdt_init(WDT_TIMEOUT, true); 
  esp_task_wdt_add(NULL); 

  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  EEPROM.begin(EEPROM_SIZE);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    //esp_task_wdt_reset();
    Serial.print(".");
  }
  Serial.println("WiFi Connected");
  if (!myDFPlayer.begin(mySerial))
  {
    Serial.println("DFPlayer Nah ready");
    while (true)
    {
      ESP.restart();
      Serial.print(".");
    }
  }

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
  myDFPlayer.volume(30);
}
void loop()
{
  long int realtime = millis();
  if (mqtt.connected() == false)
  {
    Serial.print("MQTT connection...");
    if (mqtt.connect(MQTT_NAME, MQTT_USERNAME, MQTT_PASSWORD))
    {
      Serial.println("Conteced");
      mqtt.subscribe("Translate/ESP32/Word");
      mqtt.subscribe("Translate/Status");
    }
    else
    {
      Serial.println("Connection error");
    }
  }
  mqtt.loop();

  if ((realtime - newtime >= 7000) and isPlaying)
  {
    count_timer++;
    newtime = realtime;

    if (mqtt.connected())
    {
      String status = "feed";
      esp_task_wdt_reset();
      mqtt.publish("Translate/Status", status.c_str());

      Serial.println("Normally feeding");
    }
    else if(mqtt.connected())
    {
      Serial.println("Not feeding waiting to reset");
    }
    else
    {
      Serial.println("playing");
      esp_task_wdt_reset();
    }
  }
}
