#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <esp_task_wdt.h>
#include <freertos/task.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <freertos/timers.h>
#include <DFRobotDFPlayerMini.h>

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

  if (topic_str == "Translate/ESP32/Word" && !StatusDone)
  {
    Serial.println("[" + topic_str + "]: " + payload_str);
    bool status = true;
    ExtractWord(payload_str);
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
void Sound_and_Oled(void *message)
{
  
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

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(Callbackfunc);

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
