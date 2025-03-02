#include <DFRobotDFPlayerMini.h>

#define RXD2 16 
#define TXD2 17  

HardwareSerial mySerial(2); 
DFRobotDFPlayerMini myDFPlayer;

void setup() {
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, RXD2, TXD2);  

  if (!myDFPlayer.begin(mySerial)) {  
    Serial.println("DFPlayer Nah ready");
    while (true);
  }
  Serial.print("Ready to show");
  delay(5000);
  Serial.println("DFPlayer Ready!");
  myDFPlayer.volume(30);  
  myDFPlayer.play(1);     

}

void loop() {
  Serial.println("DFPlayer Ready");
  delay(1000);
}
