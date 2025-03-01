#include <PubSubClient.h>
#include <WiFi.h>
#include <cstdlib>

#define MQTT_SERVER "20.243.148.107"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_NAME "ESP_Client"

WiFiClient client;
PubSubClient mqtt(client);

const char *ssid = "Iphone";
const char *pass = "tatty040347";
bool feed = true;
String Word[100];
void ShowMessage(String test)
{
  char num = test.charAt(0);
  int number = num - '0';
  // Serial.print("number is");
  // Serial.print(number);
  int pairing = 0;
  int counting = 0;
  String reload = "";
  for (int i = 1; i <= test.length(); i++)
  {
    if (isUpperCase(test.charAt(i)) or i == test.length())
    {
      if (pairing == 1)
      {
        Word[counting] = reload;
        reload = "";
        counting++;
        reload += test.charAt(i);
        pairing = 1;
      }
      else
      {
        pairing++;
        reload += test.charAt(i);

      }
    }
    else
    {
      reload += test.charAt(i);
    }
  }
  for (int i = 0; i < number; i++)
  {
    Serial.print("Word is ");
    Serial.println(Word[i]);
    delay(1000);
  }
  feed = true;
  
}
void callback(char *topic, byte *payload, unsigned int length)
{
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  String topic_str = topic;
  String payload_str = msg;
  if (topic_str == "Translate/ESP32/Word")
  {
    Serial.println("[" + topic_str + "]: " + payload_str);
  }
  if (topic_str == "Translate/ESP32/Word")
  {
    feed = false;
    ShowMessage(payload_str);
  }
}

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.printf("Connect sucessfully");
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
}
int newtime = 0;
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
  if (realtime - newtime >= 10000)
  {
    newtime = realtime;
    if (feed)
    {
      String status = "feed";
      mqtt.publish("Translate/Status", status.c_str());
      Serial.println("Sending sucessfully!!");
    }
  }
}
