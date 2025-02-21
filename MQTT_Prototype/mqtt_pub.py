import time
import paho.mqtt.client as mqtt

def on_publish(client, userdata, mid, reason_code, properties):
    # This callback will now handle the publish confirmation
    if reason_code != mqtt.MQTT_ERR_SUCCESS:
        print(f"Failed to publish message with mid {mid}. Reason code: {reason_code}")
    else:
        print(f"Message with mid {mid} published successfully.")

mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.on_publish = on_publish

mqttc.connect("")
mqttc.loop_start()

# Our application produces some messages
msg_info_list = []

for i in range(10):
    msg_info = mqttc.publish("test/iot", f"my message {i}", qos=0)
    msg_info_list.append(msg_info)

# Wait for all messages to be published
for msg_info in msg_info_list:
    msg_info.wait_for_publish()

# If you want to send another message
msg_info2 = mqttc.publish("test/iot", "my message2", qos=0)
msg_info2.wait_for_publish()

mqttc.disconnect()
mqttc.loop_stop()
