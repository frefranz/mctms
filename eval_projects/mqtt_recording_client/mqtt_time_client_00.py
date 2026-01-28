import paho.mqtt.client as mqtt
import datetime
import time

# MQTT-Konfiguration
BROKER_ADDRESS = "192.168.2.32"  # Adresse des MQTT-Brokers
BROKER_PORT = 1883            # Standardport
TOPIC = "time/timestamp"

# Funktion, um die aktuelle Systemzeit im gewünschten Format zu erhalten
def get_formatted_time():
    now = datetime.datetime.now()
    return now.strftime("%Y-%m-%d %H:%M:%S")

# Callback, wenn eine Nachricht empfangen wird
def on_message(client, userdata, msg):
    payload = msg.payload.decode('utf-8')
    print(f"Empfangene Nachricht auf '{msg.topic}': {payload}")

# Hauptfunktion
def main():
    # MQTT-Client erstellen
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

    # Callback für empfangene Nachrichten setzen
    client.on_message = on_message

    # Verbindung zum Broker herstellen
    client.connect(BROKER_ADDRESS, BROKER_PORT, keepalive=60)

    # Subscription zum Topic, um empfangene Zeiten zu sehen
    client.subscribe(TOPIC)

    # Start des Netzwerk-Loop im Hintergrund
    client.loop_start()

    try:
        while True:
            # Aktuelle Zeit im gewünschten Format
            current_time = get_formatted_time()

            # Payload
            payload = current_time

            # Publish auf das Topic
            client.publish(TOPIC, payload)
            print(f"Gesendet: {payload}")

            # Sekundentakt
            time.sleep(1)
    except KeyboardInterrupt:
        print("Beende Script...")
    finally:
        client.loop_stop()
        client.disconnect()

if __name__ == "__main__":
    main()
