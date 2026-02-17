# filepath: /home/franz/projects/github_repos/mctms/eval_projects/mqtt_recording_client/simple_client.py

#
# Note: Make sure an MQTT broker is running at the specified connection address.

import paho.mqtt.client as mqtt
import signal
import sys
import json
from datetime import datetime
import os

# Configuration
BROKER_ADDRESS = "192.168.2.32"
BROKER_PORT = 1883
SUBSCRIBE_TOPIC = "tmc01/sb01"  # Wildcard for all temperature sensors
#SUBSCRIBE_TOPIC = "#"  # Wildcard for all temperature sensors
OUTPUT_FILE = "temperature_data.jsonl"

# Global variables
sensor_readings = {}
client = None
running = True

def signal_handler(sig, frame):
    """Handle shutdown signals (SIGTERM, SIGINT)"""
    global running
    running = False
    print(f"\nShutdown signal received ({sig}). Disconnecting gracefully...")
    
    # Write any pending readings before shutdown
    if sensor_readings:
        write_record()
    
    if client and client.is_connected():
        client.disconnect()
    
    print("Data saved. Exiting.")
    sys.exit(0)

def on_connect(client, userdata, flags, rc, properties=None):
    """Callback when client connects to broker"""
    if rc == 0:
        print(f"Connected to broker at {BROKER_ADDRESS}")
        client.subscribe(SUBSCRIBE_TOPIC)
    else:
        print(f"Connection failed with code {rc}")

def on_message(client, userdata, msg):
    """Callback when message is received"""
    try:
        topic = msg.topic
        payload = (msg.payload.decode())
        
        # Store reading with topic as key
        sensor_readings[topic] = payload
        
        print(f"Received: {topic} = {payload}°C")
        
        # Write to file when we have all 32 sensors (adjust count as needed)
        #if len(sensor_readings) >= 3:
        write_record()
            
    except ValueError:
        print(f"Invalid payload on {msg.topic}: {msg.payload}")

def write_record():
    """Write sensor readings to JSON Lines file"""
    record = {
        "timestamp": datetime.now().isoformat(),
        **sensor_readings
    }
    
    try:
        with open(OUTPUT_FILE, "a") as f:
            f.write(json.dumps(record) + "\n")
        print(f"Recorded {len(sensor_readings)} readings at {record['timestamp']}")
    except IOError as e:
        print(f"Error writing to file: {e}")

def on_disconnect(client, userdata, rc, properties=None):
    """Callback when client disconnects"""
    print(f"Disconnected from broker (code: {rc})")

# Register signal handlers
signal.signal(signal.SIGTERM, signal_handler)  # kill -SIGTERM
signal.signal(signal.SIGINT, signal_handler)   # Ctrl+C

# Create client instance
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)

# Assign callbacks
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect

# Connect and loop
try:
    client.connect(BROKER_ADDRESS, BROKER_PORT, 60)
    print(f"Output file: {os.path.abspath(OUTPUT_FILE)}")
    print(f"Process ID: {os.getpid()}")
    print("To shutdown gracefully use: kill -SIGTERM <pid> or Ctrl+C")
    client.loop_forever()
except Exception as e:
    print(f"Error: {e}")
    sys.exit(1)
