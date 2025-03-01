import paho.mqtt.client as mqtt
import time
import re

Feeding = False
Wake = False
Status = True
Spell = 0
Msg = ""


def ClassifyWord(Text):
    return re.findall(r"[A-z][a-z]*", Text)


def Connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    client.subscribe("Translate/Status")
    client.subscribe("Translate/Massage")
    client.subscribe("Translate/ESP32/Word")
    client.subscribe("Translate/Error")


def Message(client, userdata, msg):
    global Status, Feeding, Wake, Spell, Msg
    StringPayload = msg.payload.decode()

    print("\n")
    print(f"Topic = {msg.topic}")
    print(f"Payload = {StringPayload}")

    if msg.topic == "Translate/Status":
        if StringPayload == "feed":
            Feeding = True
            Wake = True 
        elif StringPayload in ["ok", "Wake"]:
            Wake = True
        elif StringPayload == "working":
            Status = False

    elif msg.topic == "Translate/Massage":
        Msg = StringPayload
        Status = False
        


def CountingDown(timer, LastTime):
    now = time.time()
    elapsed_time = now - LastTime
    if elapsed_time >= 1:
        print(f"Time left: {timer} seconds")
        timer -= 1
        LastTime = now
    time.sleep(0.1)
    return timer, LastTime


def main():
    global Wake, Feeding, Msg, Status, Spell
    CountDown = 50
    LastTime = time.time()
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = Connect
    client.on_message = Message
    client.connect("20.243.148.107", 1883, 60)
    client.loop_start()
    print(Status);
    while True:
        while Status:
            while CountDown > 0 and Status:
                #print("flow this")
                if Feeding:
                    CountDown = 50
                    Feeding = False
                    print("Feeding time reset!")
                else:
                    CountDown, LastTime = CountingDown(CountDown, LastTime)
            Wake = False
            while not Wake and Status:
                print("hello")
                if not Feeding: 
                    if client.is_connected():
                        client.publish("Translate/Error", "NoRespond")
                        print("\nNo Respond\nSending successfully")
                time.sleep(5)

            if Wake and Status:
                print("Get Msg Reset timer")
                CountDown = 50

        print("\nProcessing Message...")
        while not Status:
            client.publish("Translate/ESP32/Word", Msg)
            print("Send Back to Esp32")
            Status = True
            time.sleep(1)


if __name__ == "__main__":
    main()
