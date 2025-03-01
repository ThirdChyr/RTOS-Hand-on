
#include <PubSubClient.h>
#include <WiFi.h>


#define MQTT_SERVER "20.243.148.107"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_NAME "ESP_Client"


WiFiClient client;
PubSubClient mqtt(client);

const char* ssid= "Iphone";
const char* pass = "tatty040347";

void callback(char* topic, byte* payload, unsigned int length)
{
  char msg[length + 1];  
  memcpy(msg, payload, length);
  msg[length] = '\0';   

  String topic_str = topic;
  String payload_str = msg; 
  if(topic_str == "Translate/ESP32/Word")
  {
   Serial.println("[" + topic_str + "]: " + payload_str);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid,pass);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

Serial.printf("Connect sucessfully");
mqtt.setServer(MQTT_SERVER,MQTT_PORT);
mqtt.setCallback(callback);
}
int newtime = 0;
void loop() 
{
long int realtime = millis();
if(mqtt.connected() == false)
  {
    Serial.print("MQTT connection...");
    if(mqtt.connect(MQTT_NAME,MQTT_USERNAME,MQTT_PASSWORD))
    {
      Serial.println("Conteced");
      mqtt.subscribe("Translate/ESP32/Word");
      mqtt.subscribe("Translate/Status");

    }
    else
    {
      Serial.println("Connection error");
      delay(5000);
    }
  }
  mqtt.loop();
if(realtime - newtime >=10000)
{
  newtime = realtime;
  String status = "feed";
  mqtt.publish("Translate/Status",status.c_str());
  Serial.println("Sending sucessfully!!");
}
  
}
