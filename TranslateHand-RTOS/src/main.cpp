#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <esp_task_wdt.h>
#include <freertos/task.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <freertos/timers.h>

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

const char *ssid = "Iphone";
const char *password = "tatty040347";
volatile bool mqttConnected = false;

QueueHandle_t Data_Queue;
QueueHandle_t Mailbox_status;
TimerHandle_t Feeding_Mqtt_Watchdog_timer;
WiFiClient client;
PubSubClient mqtt(client);

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
     bool status = true;
     Serial.println("[" + topic_str + "]: " + payload_str);
     if (xQueueSend(Data_Queue, &payload_str, portMAX_DELAY) != pdPASS)
     {
       Serial.println("Failed to send message to queue");
     }
     else 
     {
       Serial.println("Message received and added to queue");
       xQueueOverwrite(Mailbox_status, &status);  
     }
   }
   else if (StatusDone)
   {
     Serial.println("System is busy processing previous data");
   }
}
void Feeding(TimerHandle_t xTimer)
{
  String status = "feed";
  esp_task_wdt_reset();
  mqtt.publish("Translate/Status", status.c_str());
  Serial.println("Feeding Watchdog and Mqtt");
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
          if (xQueueReceive(Data_Queue, &message, 0) == pdTRUE)
          {
             
              long long int lockTimer = millis();
              
              Serial.println("\n========================");
              Serial.println("New data received!");
              Serial.print("Message: ");
              Serial.println(message);
              Serial.println("System locked for 5 seconds");
              Serial.println("========================\n");

              while (millis() - lockTimer < LOCK_PERIOD)
              {
                  vTaskDelay(pdMS_TO_TICKS(100));
              }
              
             
              status = false;
              xQueueOverwrite(Mailbox_status, &status);
              Serial.println("System unlocked - Ready for new data");
          }
      }

        // ตรวจสอบ timer
        long long int realtime = millis();
        if (realtime - planetimer >= 5000)
        {
            planetimer = realtime;
            Serial.println("Control Plane : Get command");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup()
{
  Serial.begin(115200);
  delay(2000);
  esp_task_wdt_init(WDT_TIMEOUT, true);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    esp_task_wdt_reset();
    Serial.print(".");
  }

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(Callbackfunc);

  Data_Queue = xQueueCreate(10, sizeof(String));
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
