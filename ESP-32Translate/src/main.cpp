#include <DFRobotDFPlayerMini.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <cstdlib>
#include<string.h>
#include<map>

using std::string;

#define MQTT_SERVER "20.243.148.107"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_NAME "ESP_Client"
#define RXD2 16 
#define TXD2 17

WiFiClient client;
PubSubClient mqtt(client);
HardwareSerial mySerial(2); 
DFRobotDFPlayerMini myDFPlayer;

const char *ssid = "อิอิ";
const char *pass = "44444444";
bool feed = true;
String Word[100] = {""};
void print_word(int count)
{
  for(int i =0;i<count;i++)
  {
    Serial.println(Word[i]);
  }
}
void ShowMessage(String test)
{
  std::map<string,int> TransProtocals =
  {
    {"Hello",1},
    {"My",2},
    {"Name",3},
    {"Is",4},
    {"Chayathon",5},
    {"Rungrueang",6},
    {"Kasetsartuniversity",7},
  };

  int number = test.charAt(0) - '0';  
  String reload = "";
  int counting =0;
  for (int i = 1; i <= test.length(); i++)
  {
    if (isUpperCase(test.charAt(i)) || i == test.length())
    {
      if (!reload.isEmpty()) {
        Word[counting++] = reload;
        if (counting >= 100) break;  
      }
      reload = String(test.charAt(i));
    }
    else
    {
      reload += test.charAt(i);
    }
  }

  for (int i = 0; i < number ; i++)
  {
    if (TransProtocals.find(Word[i].c_str()) != TransProtocals.end())
    {
      int PointNumber = TransProtocals[Word[i].c_str()];
      Serial.println(PointNumber);
      if (myDFPlayer.available())
      {
        myDFPlayer.play(PointNumber);
        Serial.print("Playing: ");
        Serial.println(Word[i]);
        delay(1000);
      }
    }
    else
    {
      Serial.print("Error: No sound file for ");
      Serial.println(Word[i]);
    }
  }

  feed = true;
  print_word(counting);
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
  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);  
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }
  if (!myDFPlayer.begin(mySerial)) {  
    Serial.println("DFPlayer Nah ready");
    while (true);
    {
      Serial.print(".");
    }
  }


  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
  myDFPlayer.volume(30);

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
