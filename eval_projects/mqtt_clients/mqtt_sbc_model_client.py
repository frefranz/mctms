import argparse
import json
import signal
import sys
import threading
import time
from datetime import datetime

import paho.mqtt.client as mqtt

class SBCModel:
    def __init__(self, name, broker_host='127.0.0.1', broker_port=1883, publish_interval=2.0, cycle_seconds=20.0):
        self.name = name  # e.g. "sbc0"
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.publish_interval = publish_interval
        self.cycle_seconds = cycle_seconds

        self.client = mqtt.Client(client_id=f"{self.name}-sim")
        self.client_connected = threading.Event()
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect

        # Prepare waveform steps for triangular wave between 0.0 and 0.4
        self.steps = max(2, int(round(self.cycle_seconds / self.publish_interval)))
        # ensure an even number for symmetrical triangular shape
        if self.steps % 2 != 0:
            self.steps += 1
        half = self.steps // 2
        # generate fraction values 0..1..0
        self.fracs = [i / half for i in range(half + 1)] + [i / half for i in range(half - 1, 0, -1)]
        # scale to 0..0.4
        self.amps = [f * 0.4 for f in self.fracs]
        self.step_idx = 0

        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._publish_loop, daemon=True)

    def _on_connect(self, client, userdata, flags, rc):
        # rc == 0 means success
        if rc == 0:
            self.client_connected.set()
        else:
            print(f"{self.name}: connect failed rc={rc}")

    def _on_disconnect(self, client, userdata, rc):
        self.client_connected.clear()

    def start(self):
        self.client.connect(self.broker_host, self.broker_port, keepalive=60)
        self.client.loop_start()
        # wait a short time for connection
        self.client_connected.wait(timeout=5.0)
        self._stop.clear()
        self._thread.start()
        print(f"{self.name}: started, publishing every {self.publish_interval}s to {self.broker_host}:{self.broker_port}")

    def stop(self):
        self._stop.set()
        if self._thread.is_alive():
            self._thread.join(timeout=5.0)
        try:
            self.client.loop_stop()
            self.client.disconnect()
        except Exception:
            pass
        print(f"{self.name}: stopped")

    def _generate_payload(self):
        # build a dict with keys "sbcX/owbY/tsZ" for owb0 and owb1, ts0..ts7
        amp = self.amps[self.step_idx % len(self.amps)]
        payload = {}
        for owb in (0, 1):
            for ts in range(8):
                topic_key = f"{self.name}/owb{owb}/ts{ts}"
                # value = base (ts) + small oscillation (amp)
                value = round(ts + amp, 3)
                payload[topic_key] = value
        return payload

    def _publish_loop(self):
        while not self._stop.is_set():
            payload = self._generate_payload()
            topic = f"{self.name}/measurements"
            # include timestamp in payload
            envelope = {"timestamp": datetime.utcnow().isoformat() + "Z", **payload}
            body = json.dumps(envelope)
            try:
                # publish QoS 0
                self.client.publish(topic, body)
            except Exception as e:
                print(f"{self.name}: publish error: {e}")
            # advance step
            self.step_idx = (self.step_idx + 1) % len(self.amps)
            # sleep with early exit
            if self._stop.wait(self.publish_interval):
                break

def main():
    parser = argparse.ArgumentParser(description="Simulate two SBC temperature publishers (sbc0, sbc1).")
    parser.add_argument("--broker", default="127.0.0.1", help="MQTT broker host")
    parser.add_argument("--port", type=int, default=1883, help="MQTT broker port")
    parser.add_argument("--interval", type=float, default=2.0, help="publish interval in seconds (default 2.0)")
    args = parser.parse_args()

    sbc0 = SBCModel("sbc0", broker_host=args.broker, broker_port=args.port, publish_interval=args.interval)
    sbc1 = SBCModel("sbc1", broker_host=args.broker, broker_port=args.port, publish_interval=args.interval)

    def handle_stop(signum, frame):
        print("shutdown signal received, stopping sbc models...")
        sbc0.stop()
        sbc1.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, handle_stop)
    signal.signal(signal.SIGTERM, handle_stop)

    sbc0.start()
    sbc1.start()

    # keep main thread alive while workers run
    try:
        while True:
            time.sleep(1.0)
    except KeyboardInterrupt:
        handle_stop(None, None)

if __name__ == "__main__":
    main()