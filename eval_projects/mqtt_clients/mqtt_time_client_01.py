import paho.mqtt.client as mqtt
import time
from datetime import datetime

# MQTT Broker Einstellungen
BROKER_ADDRESS = "192.168.2.32"
BROKER_PORT = 1883
TOPIC_REQUEST = "time/timestamp/request"
TOPIC_TIMESTAMP = "time/timestamp"

# Funktion zum Formatieren der aktuellen Zeit
def get_formatted_time():
    return datetime.now().strftime("%Y-%m-%d %H:%M:%S")

# Callback bei Empfang einer Nachricht
def on_message(client, userdata, msg):
    if msg.topic == TOPIC_REQUEST:
        # Wenn eine Anfrage empfangen wird, aktuellen Timestamp veröffentlichen
        current_time = get_formatted_time()
        client.publish(TOPIC_TIMESTAMP, current_time)
        print(f"Anfrage erhalten, Timestamp gesendet: {current_time}")
    elif msg.topic == TOPIC_TIMESTAMP:
        # Timestamp von anderen Clients erhalten
        print(f"Empfangener Timestamp: {msg.payload.decode()}")

# MQTT Client initialisieren
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)

# Callback setzen
client.on_message = on_message

# Verbindung zum Broker herstellen
client.connect(BROKER_ADDRESS, BROKER_PORT, keepalive=60)

# Subscribe auf die Anfrage- und Timestamp-Topics
client.subscribe([(TOPIC_REQUEST, 0), (TOPIC_TIMESTAMP, 0)])

# Netzwerk-Loop im Hintergrund starten
client.loop_start()

try:
    while True:
        # Hier können Sie entscheiden, wann eine Anfrage gesendet werden soll.
        # Zum Beispiel alle 10 Sekunden eine Anfrage stellen:
        time.sleep(10)
        # Sende eine Anfrage, um den aktuellen Timestamp zu erhalten
        client.publish(TOPIC_REQUEST, "Bitte aktuellen Timestamp senden")
        print("Anfrage gesendet: Bitte aktuellen Timestamp senden")
except KeyboardInterrupt:
    print("Beende Script...")
finally:
    client.loop_stop()
    client.disconnect()
