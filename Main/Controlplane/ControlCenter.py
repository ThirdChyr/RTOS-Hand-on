import paho.mqtt.client as mqtt
import time

received_message = False

def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    client.subscribe("Translate/Status")
    client.subscribe("Translate/Spell")
    client.subscribe("Translate/Word")
    

def on_message(client, userdata, msg):
    global received_message  
    print(f"Message received: {msg.topic} {str(msg.payload)}")
    received_message = True
    return received_message

def counting_down(timer,LastTime):
    now = time.time()
    elapsed_time = time.time() - LastTime
    if elapsed_time >= 1:
        timer -=1
        LastTime = now
    return timer,LastTime

def main():
    timet = 15
    LastTime = time.time()
    global received_message
    mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    mqttc.on_connect = on_connect
    mqttc.on_message = on_message
    mqttc.connect("", 1883, 60)
    # time.sleep()
    while timet > 0:
        mqttc.loop()  
        if received_message:
            timet = 15
            print("Feeding time reset!")
            received_message = False
        else:
            timet,LastTime = counting_down(timet,LastTime)
            print(f"Time left: {timet} seconds") 
    while 1:
        print("Hello")
        


if __name__ == "__main__":
    main()
    # print(f"Time left: {timet} seconds") 