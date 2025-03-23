#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <Adafruit_SSD1306.h>
#include <esp_task_wdt.h>
#include <freertos/task.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <freertos/timers.h>
#include <DFRobotDFPlayerMini.h>
#include <map>
#include <EEPROM.h>

using std::string;

#define MQTT_SERVER "20.243.148.107"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_NAME "ESP_Client"

#define RXD2 16
#define TXD2 17

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define EEPROM_SIZE 1024
#define WDT_TIMEOUT 20
#define MAX_WORD_LENGTH 32

const char *ssid = "Iphone";
const char *password = "tatty040347";
volatile int words = 0;

TaskHandle_t SoundOled = NULL;
QueueHandle_t Data_Queue;
QueueHandle_t Mailbox_status;
TimerHandle_t Feeding_Mqtt_Watchdog_timer;
WiFiClient client;
PubSubClient mqtt(client);
HardwareSerial mySerial(2);
DFRobotDFPlayerMini myDFPlayer;
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT, &Wire);

String get_backup()
{
  int start = 0;
  int word_lenght = EEPROM.read(start);
  start++;
  String value = "";
  for (int i = 0; i < word_lenght; i++)
  {
    value += (char)EEPROM.read(start + i);
  }
  Serial.println("Backup value: " + value);
  return value;
}
void EEPORM_Backup(String value)
{
  int start = 0;
  int word_lenght = value.length();
  EEPROM.write(start, word_lenght);
  start++;
  for (int i = 0; i < word_lenght; i++)
  {
    EEPROM.write(start + i, value.charAt(i));
  }
  if(EEPROM.commit())
  {
    Serial.println("Backup Done");
  }
  else
  {
    Serial.println("Backup Failed");
  }
}
void print_word(int count)
{
  for (int i = 0; i < count; i++)
  {
    String value = "";
    if (xQueueReceive(Data_Queue, &value, 0) == pdTRUE)
    {
      Serial.print("Word[");
      Serial.print(i);
      Serial.print("]: ");
      Serial.println(value);
    }
  }
}
void Feeding(TimerHandle_t xTimer)
{
  String status = "feed";
  mqtt.publish("Translate/Status", status.c_str());
  Serial.println("Feeding Watchdog and Mqtt");
}
void FeedingWatchdog()
{
  esp_task_wdt_reset();
}
void conllect_Queue(String word)
{
  if (xQueueSend(Data_Queue, &word, portMAX_DELAY) == pdPASS)
  {
    Serial.printf("Added word: %s", word.c_str());
  }
}
void ExtractWord(String message)
{
  Serial.println("Extracting words from message");
  int count = 0;
  String word = "";

  for (int i = 0; i <= message.length(); i++)
  {
    if (isUpperCase(message.charAt(i)) || i == message.length())
    {
      if (!word.isEmpty())
      {
        if (count < 100)
        {
          conllect_Queue(word);
        }
        else
        {
          Serial.println("Error: Word array overflow");
          break;
        }
      }
      word = String(message.charAt(i));
    }
    else
    {
      word += message.charAt(i);
    }
  }
}

void Callbackfunc(char *topic, byte *payload, unsigned int length)
{
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';
  String topic_str = topic;
  String payload_str = msg;

  bool StatusDone = false;
  xQueuePeek(Mailbox_status, &StatusDone, 0);

  if (topic_str == "Translate/ESP32/Word" && !StatusDone && payload_str != "1Repeat")
  {
    Serial.println("[" + topic_str + "]: " + payload_str);
    bool status = true;
    EEPORM_Backup(payload_str);
    ExtractWord(payload_str);
    xQueueOverwrite(Mailbox_status, &status);
  }
  else if(topic_str == "Translate/ESP32/Word" && payload_str == "1Repeat")
  {
   
      bool status = true;
      String word = get_backup();
      ExtractWord(word);
      xQueueOverwrite(Mailbox_status, &status);
  }
  else if (topic_str == "Translate/Status" && payload_str == "feed")
  {
    FeedingWatchdog();
  }
  else if (StatusDone)
  {
    Serial.println("System is busy processing previous data");
  }
}
void Sound_and_Oled(void *pvParameters)
{
  int count = 0;
  String word ;
  std::map<string, int> TransProtocals =
      {
          {"Hello", 4},
          {"Good", 1},
          {"Bad", 2},
          {"I", 3},
          {"Repeat", 5},
      };

  while (1)
  {
    if (xQueueReceive(Data_Queue, &word, 0) == pdTRUE)
    {
      count = word.toInt();
      Serial.println("Playing Words..........");
      for (int i = 0; i < count; i++)
      {
        if(xQueueReceive(Data_Queue, &word, 0) == pdTRUE)
        {
          string stdWord = string(word.c_str());
          if(TransProtocals.find(stdWord) != TransProtocals.end())
          {
            int pointnumber = TransProtocals[stdWord];
            if(myDFPlayer.available())
            {
              Serial.printf("Playing sound %d for word: %s\n", pointnumber, word.c_str());
              myDFPlayer.play(pointnumber);
            }
            else
            {
              Serial.println("DFPlayer is not available");
            }
          }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
      Serial.println("Playing Done");
      vTaskDelete(SoundOled);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
 

void ControlPlane(void *pvParameters)
{
  int planetimer = 0;
  bool status = false;
  String message;
  const int LOCK_PERIOD = 5000;
  while (1)
  {
    if (xQueuePeek(Mailbox_status, &status, 0) == pdTRUE && status)
    {

      xTaskCreate(Sound_and_Oled,"ProcessData",4096,NULL,1,&SoundOled);
      vTaskDelay(pdMS_TO_TICKS(5000));

      status = false;
      xQueueOverwrite(Mailbox_status, &status);
      Serial.println("System unlocked - Ready for new data");
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(2000);
  EEPROM.begin(EEPROM_SIZE);
  esp_task_wdt_deinit();
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    esp_task_wdt_reset();
    Serial.print(".");
  }

  if (!myDFPlayer.begin(mySerial))
  {
    Serial.println("DFPlayer Nah ready");
    while (true)
    {
      Serial.print(".");
      delay(1000);
    }
  }

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(Callbackfunc);
  myDFPlayer.volume(30);

  Data_Queue = xQueueCreate(100, MAX_WORD_LENGTH);
  Mailbox_status = xQueueCreate(1, sizeof(bool));

  bool initial_status = false;
  xQueueOverwrite(Mailbox_status, &initial_status);
  Feeding_Mqtt_Watchdog_timer = xTimerCreate("Feeding_Mqtt_Watchdog_timer", pdMS_TO_TICKS(10000), pdTRUE, (void *)0, Feeding);
  xTimerStart(Feeding_Mqtt_Watchdog_timer, 0);
  xTaskCreate(ControlPlane, "ControlPlane", 4096, NULL, 2, NULL);
}

void loop()
{
  if (!mqtt.connected())
  {
    Serial.print("MQTT connection...");
    if (mqtt.connect(MQTT_NAME, MQTT_USERNAME, MQTT_PASSWORD))
    {
      Serial.println("Connected");
      mqtt.subscribe("Translate/ESP32/Word");
      mqtt.subscribe("Translate/Status");
    }
    else
    {
      Serial.println("Connection error");
    }
  }
  mqtt.loop();
  vTaskDelay(pdMS_TO_TICKS(100));
}
