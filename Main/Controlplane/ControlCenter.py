import paho.mqtt.client as mqtt
import time
import re

Feeding = False
Wake = False
Status = True
Spell = 0
Msg = ""
def ClassifyWord(Text):
    return re.findall(r'[A-z][a-z]*',Text)

def Connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    client.subscribe("Translate/Status")
    client.subscribe("Translate/Massage")
    client.subscribe("Translate/ESP32/Word")
    client.subscribe("Translate/Error")
    

def Message(client, userdata, msg):
    global Status
    global Feeding  
    global Wake
    global Spell 
    global Msg
    StringPayload = msg.payload.decode()
    # if str(msg.topic) == "Translate/Status":
    print("")
    print(f"Topic = {msg.topic} ")
    print(f"Payload = {StringPayload} ")

    if str(msg.topic) == "Translate/Status" and StringPayload == "idle":
        Feeding = True   
    elif str(msg.topic) == "Translate/Status" and (StringPayload == "ok" or StringPayload == "Wake" ):
        Wake = True
    elif str(msg.topic) == "Translate/Status" and StringPayload == "working" :
        Status = False
    elif str(msg.topic) == "Translate/Massage"  :
        Spell = int(StringPayload[0])
        Msg = ClassifyWord(StringPayload[1:])
        Status = False
        print(Msg)
        # print(Spell)


def CountingDown(timer,LastTime):
    now = time.time()
    elapsed_time = time.time() - LastTime
    if elapsed_time >= 1:
        print(f"Time left: {timer} seconds")
        timer -=1
        LastTime = now
    time.sleep(0.1)
    return timer,LastTime

def main():
    global Wake 
    global Feeding,Msg
    global Status
    global Spell
    CountDown = 50
    LastTime = time.time()
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = Connect
    client.on_message = Message
    client.connect("20.243.148.107", 1883, 60)
    client.loop_start()
    while True:
        while Status:
            Wake = False
            while CountDown > 0 and Status:
                if Feeding:
                    CountDown = 50
                    Feeding = False
                    print("Feeding time reset!")
                else:
                    CountDown,LastTime = CountingDown(CountDown,LastTime)

            while not Wake and Status:
                if client.is_connected():
                    client.publish("Translate/Error","NoRespond")
                    print("")
                    print("No Respond")
                    print("Sending sucessfully")
                time.sleep(5)

            if Wake and Status:
                print("Get Msg Reset timer")
                CountDown = 50
        print("")
        
        while not Status:
            if(Spell > 0):
                Status = True
            for word in Msg:
                client.publish("Translate/ESP32/Word",str(word))
                time.sleep(1)
                Spell = Spell-1


       



if __name__ == "__main__":
    main()
    