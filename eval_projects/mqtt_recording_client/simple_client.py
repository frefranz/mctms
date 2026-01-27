import paho.mqtt.client as mqtt

# 1st trail version to get an mqtt client going
# AI prompt "is it possible to write a mqtt client in python?"
# The result is the code below, and it is working fine.
#
# Prerequisites:
# Create a virtual environment and install paho-mqtt:
#   python3 -m venv venv
#   source venv/bin/activate
#   pip install paho-mqtt
# Run the script: python simple_client.py"

# Create a client instance
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

# Connect to the broker
client.connect("192.168.2.32", 1883, 60)

# Publish a message
client.publish("test/topic", "Hello MQTT")

# Disconnect
client.disconnect()

