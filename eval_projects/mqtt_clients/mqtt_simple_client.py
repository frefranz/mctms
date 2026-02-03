
# 1st trail version to get an mqtt client going, conceived by using the GitHub Copilot
# AI prompt "is it possible to write a mqtt client in python?"
# The result is the code below, and it is working fine.
#
# Simple MQTT client that connects to a broker, publishes a message, and disconnects.
#
# Prerequisites:
# Create a virtual environment and install paho-mqtt:
#   python3 -m venv venv
#   Linux: source venv/bin/activate
#   Windows powershell: Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass
#                       venv\Scripts\Activate.ps1
#   Windows cmd: venv\Scripts\activate.bat
# pip install paho-mqtt
# 
# Usage: python simple_client.py
# Optioanlly, when done deactivate the virtual environment:
#   Windows:  just type "deactivate" on the command line (no path, no nothing else)
#
# Note: Make sure an MQTT broker is running at the specified connection address.

import paho.mqtt.client as mqtt

# Create a client instance
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)

# Connect to the broker
client.connect("192.168.2.32", 1883, 60)

# Publish a message
client.publish("test/topic", "Hello MQTT")

# Disconnect
client.disconnect()

