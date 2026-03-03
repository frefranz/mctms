
#
# This is a display client for the mqtt driven mctms project using a 64x64 pixel led matrix display 
#
# The code below currently is just a placedholder
#
# Note: Make sure an MQTT broker is running at the specified connection address.

"""
MQTT-driven display client for 64x64 LED matrix.

    - Subscribes to a JSON measurements topic (default `tmc1/sb0`).
    - Expects payload in the new format: {"client":"tmc0","sb_nr":0,"ds_nr":0,"ts_dat": {"s0":20.0, "s1":21.1}}
    - Displays two configured sensor values (default `s0` and `s1`) on the matrix.

Requirements:
  - rpi-rgb-led-matrix (Python bindings)
  - paho-mqtt

Example:
  sudo python3 mqtt_ledm_display_client.py --broker 192.168.2.32 --topic sbc0/measurements --key1 sbc0/owb0/ts0 --key2 sbc0/owb0/ts1
"""

import argparse
import json
import signal
import sys
import threading
import time
import yaml
import paho.mqtt.client as mqtt

try:
    from rgbmatrix import RGBMatrix, RGBMatrixOptions, graphics
except Exception as e:
    raise SystemExit(f"Failed to import rpi-rgb-led-matrix: {e}\nMake sure the library is installed and you're running this on the Raspberry Pi.")

# program version follows semantic 3-number scheme
VERSION = "0.1.0"


def make_matrix(options_kwargs):
    opts = RGBMatrixOptions()
    for k, v in options_kwargs.items():
        setattr(opts, k, v)
    return RGBMatrix(options=opts)


class LEDMDisplay:
    def __init__(self, broker, port, topic, key1, key2, font, brightness=20, hwmap="adafruit-hat"):
        self.broker = broker
        self.port = port
        self.topic = topic
        self.key1 = key1
        self.key2 = key2
        self.font_path = font
        self.brightness = brightness
        self.hwmap = hwmap

        # matrix setup
        options = {
            'rows': 64,
            'cols': 64,
            'chain_length': 1,
            'parallel': 1,
            'hardware_mapping': self.hwmap,
            'brightness': self.brightness,
            'gpio_slowdown': 2,
        }
        self.matrix = make_matrix(options)
        self.canvas = self.matrix.CreateFrameCanvas()
        self.font = graphics.Font()
        self.font.LoadFont(self.font_path)
        self.color = graphics.Color(255, 255, 0)

        # mqtt
        self.client = mqtt.Client(client_id="ledm-display")
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message

        self.connected = threading.Event()
        self.current_values = {self.key1: "--", self.key2: "--"}
        self._stop = threading.Event()

    def _on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            print("Connected to broker")
            self.client.subscribe(self.topic)
            self.connected.set()
        else:
            print(f"Failed to connect rc={rc}")

    def _on_disconnect(self, client, userdata, rc, properties=None):
        print("Disconnected")
        self.connected.clear()

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode())

            # support new payload format: { ..., "ts_dat": {"s0": 20.0, ...} }
            ts_dat = None
            if isinstance(payload, dict) and "ts_dat" in payload and isinstance(payload["ts_dat"], dict):
                ts_dat = payload["ts_dat"]
            else:
                # fallback: payload may be a flat mapping of keys -> values
                ts_dat = payload if isinstance(payload, dict) else {}

            def find_value(key: str):
                # try exact key in ts_dat
                if isinstance(ts_dat, dict) and key in ts_dat:
                    return ts_dat[key]
                # try last path segment (in case callers pass full topic-like keys)
                last = key.split("/")[-1]
                if isinstance(ts_dat, dict) and last in ts_dat:
                    return ts_dat[last]
                # finally, try top-level payload
                if isinstance(payload, dict) and key in payload:
                    return payload[key]
                return None

            v1 = find_value(self.key1)
            v2 = find_value(self.key2)

            if v1 is not None:
                self.current_values[self.key1] = v1
            if v2 is not None:
                self.current_values[self.key2] = v2

            self._draw()
        except Exception as e:
            print(f"Failed to process message: {e}")

    def _draw(self):
        # clear canvas
        self.canvas.Clear()
        # draw two lines: key (short) and value
        # Line 1
        text1 = f"{self.key1.split('/')[-1]}: {self.current_values[self.key1]}"
        text2 = f"{self.key2.split('/')[-1]}: {self.current_values[self.key2]}"
        # positions
        x = 1
        y1 = 24
        y2 = 48
        graphics.DrawText(self.canvas, self.font, x, y1, self.color, text1)
        graphics.DrawText(self.canvas, self.font, x, y2, self.color, text2)
        self.matrix.SwapOnVSync(self.canvas)

    def start(self):
        try:
            self.client.connect(self.broker, self.port, keepalive=60)
            self.client.loop_start()
            # wait for connection
            self.connected.wait(timeout=5.0)
            print(f"Subscribed to {self.topic}")
            # initial draw
            self._draw()
            while not self._stop.is_set():
                time.sleep(1)
        except KeyboardInterrupt:
            pass
        finally:
            self.stop()

    def stop(self):
        self._stop.set()
        try:
            self.client.loop_stop()
            self.client.disconnect()
        except Exception:
            pass
        self.matrix.Clear()


def main():
    # parse config file path first so config values can provide defaults
    cfg_parser = argparse.ArgumentParser(add_help=False)
    cfg_parser.add_argument("-c", "--config", default="mqtt_ledm_display_client_config.yml", help="path to yaml config file")
    known_args, _ = cfg_parser.parse_known_args()

    # load configuration if file exists
    def load_config(path: str):
        try:
            with open(path, "r") as f:
                data = yaml.safe_load(f)
                return data or {}
        except FileNotFoundError:
            return {}

    cfg = load_config(known_args.config)

    parser = argparse.ArgumentParser(description="MQTT LED matrix display client")
    parser.add_argument("-c", "--config", default=known_args.config, help="path to yaml config file")
    parser.add_argument("-v", "--version", action="version", version=VERSION, help="show program version and exit")
    parser.add_argument("--broker", default=cfg.get("broker", "127.0.0.1"), help="MQTT broker host")
    parser.add_argument("--port", type=int, default=cfg.get("port", 1883), help="MQTT broker port")
    parser.add_argument("--topic", default=cfg.get("topic", "tmc1/sb0"), help="Topic to subscribe for measurement JSON payloads")
    parser.add_argument("--key1", default=cfg.get("key1", "s0"), help="JSON key for first temperature value (e.g. s0)")
    parser.add_argument("--key2", default=cfg.get("key2", "s1"), help="JSON key for second temperature value (e.g. s1)")
    parser.add_argument("--font", default=cfg.get("font", "rpi-rgb-led-matrix/fonts/7x13.bdf"), help="Path to BDF font")
    parser.add_argument("--brightness", type=int, default=cfg.get("brightness", 20), help="Brightness 0..100")
    parser.add_argument("--hwmap", default=cfg.get("hwmap", "adafruit-hat"), help="Hardware mapping name (e.g. adafruit-hat)")

    args = parser.parse_args()

    disp = LEDMDisplay(broker=args.broker, port=args.port, topic=args.topic, key1=args.key1, key2=args.key2, font=args.font, brightness=args.brightness, hwmap=args.hwmap)

    def handle_sig(signum, frame):
        print("Shutdown requested")
        disp.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, handle_sig)
    signal.signal(signal.SIGTERM, handle_sig)

    disp.start()


if __name__ == "__main__":
    main()

