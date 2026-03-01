#!/usr/bin/env python3
"""Modelled Temperature Measurement Client (mtmc).

Reads a YAML configuration file that describes a set of virtual temperature sensors
and periodically publishes the measurements to an MQTT broker.  A single instance of
this script represents one tmc device (e.g. "tmc0", "tmc1" etc); to run multiple
models simply start the program several times with different configuration files.

The configuration file format is described in <repo_root>/doc/requirements/tmcm_config_yml.txt
and the JSON payload layout in <repo_root>/doc/requirements/payload_json.txt.

Example invocation::

    python mqtt_tmc_model.py -c my_client.yml

Required packages: paho-mqtt, PyYAML

The script handles ctrl+c (SIGINT) and cleanly disconnects from the broker.
It also subscribes to "<client_name>/#" so that you can send commands or monitor
activity directed at this modelled client.
"""

import argparse
import json
import signal
import sys
import time
from typing import Any, Dict, List

# program version follows semantic 3-number scheme
VERSION = "0.1.0"

import yaml

import paho.mqtt.client as mqtt
from paho.mqtt.client import CallbackAPIVersion


# ---------------------------------------------------------------------------
# configuration helpers
# ---------------------------------------------------------------------------

def load_config(path: str) -> Dict[str, Any]:
    with open(path, "r") as f:
        return yaml.safe_load(f)


# ---------------------------------------------------------------------------
# MQTT callbacks
# ---------------------------------------------------------------------------

def on_connect(client: mqtt.Client, userdata: Any, flags: Dict[str, int], rc: int, *args, **kwargs) -> None:
    # newer versions of paho-mqtt pass a properties argument; accept
    # arbitrary extra parameters to avoid TypeError.
    print(f"connected to broker, rc={rc}")
    # let the user know the model is active
    print("tmc model up and running, type ctrl-c for model shutdown")


def on_message(client: mqtt.Client, userdata: Any, msg: mqtt.MQTTMessage) -> None:
    # print any message that is published to topics we subscribe to
    if getattr(userdata, "verbose", False):
        print(f"[received] {msg.topic}: {msg.payload.decode('utf-8')}" )


# ---------------------------------------------------------------------------
# model implementation
# ---------------------------------------------------------------------------

class SensorBank:
    def __init__(self, ts_dat: Dict[str, List[float]]):
        # keep a copy of the sequences and an index for each sensor
        self._ts_dat = ts_dat
        self._indices = {name: 0 for name in ts_dat}

    def next_values(self) -> Dict[str, float]:
        """Return the next measurement for every sensor and advance the index."""
        result: Dict[str, float] = {}
        for name, values in self._ts_dat.items():
            idx = self._indices[name]
            result[name] = values[idx]
            self._indices[name] = (idx + 1) % len(values)
        return result


class TmcModel:
    def __init__(self, config: Dict[str, Any], verbose: bool = False) -> None:
        self.verbose = verbose
        self.broker_ip = config.get("broker_ip", "localhost")
        self.broker_port = int(config.get("broker_port", 1883))
        self.client_name = config.get("client_name", "tmc0")
        self.sb_cnt = int(config.get("sb_cnt", 1))
        self.meas_delay = int(config.get("meas_delay", 2))
        self.ds_nr = int(config.get("ds_nr", 0))

        # expect ts_dat to be a dict of sensor-name -> list of values
        raw_ts_dat = config.get("ts_dat", {})
        if not isinstance(raw_ts_dat, dict):
            raise ValueError("ts_dat section of configuration must be a mapping")

        # create a bank for each requested sensor bank; share the same ts_dat
        # (the sample configuration does not distinguish between banks).
        self.banks = [SensorBank(raw_ts_dat) for _ in range(self.sb_cnt)]

        # create mqtt client.  the default callback API version (1) is
        # deprecated and triggers a warning; request version 2 explicitly.
        # our callbacks already accept ``*args, **kwargs`` so the extra
        # parameters that version 2 passes are handled gracefully.
        self.mqtt = mqtt.Client(
            client_id=self.client_name,
            callback_api_version=CallbackAPIVersion.VERSION2,
        )
        # store reference in userdata so callbacks can inspect verbosity
        self.mqtt.user_data_set(self)
        self.mqtt.on_connect = on_connect
        self.mqtt.on_message = on_message

        self._stop = False

    def connect(self) -> None:
        self.mqtt.connect(self.broker_ip, self.broker_port)
        # subscribe to our own namespace so that commands can be sent
        self.mqtt.subscribe(f"{self.client_name}/#")
        # start network loop in background thread
        self.mqtt.loop_start()

    def disconnect(self) -> None:
        self.mqtt.loop_stop()
        self.mqtt.disconnect()

    def run(self) -> None:
        print(f"starting model '{self.client_name}' ({self.sb_cnt} bank(s))")
        try:
            while not self._stop:
                for sb_nr, bank in enumerate(self.banks):
                    ts_values = bank.next_values()
                    payload = {
                        "client": self.client_name,
                        "sb_nr": sb_nr,
                        "ds_nr": self.ds_nr,
                        "ts_dat": ts_values,
                    }

                    topic = f"{self.client_name}/sb{sb_nr}"
                    msg = json.dumps(payload)
                    self.mqtt.publish(topic, msg)

                    # also publish each sensor individually so that someone
                    # can subscribe to tmcX/sbY/sensorName.  Build a small
                    # payload that follows the same JSON schema as the
                    # full bank message, making it easier for clients that
                    # expect JSON to parse.
                    for name, value in ts_values.items():
                        single_payload = {
                            "client": self.client_name,
                            "sb_nr": sb_nr,
                            "ds_nr": self.ds_nr,
                            "ts_dat": {name: value},
                        }
                        self.mqtt.publish(f"{topic}/{name}", json.dumps(single_payload))

                    if self.verbose:
                        print(f"[published] {topic} {msg}")

                # increment data set counter and wrap at 65535
                self.ds_nr = (self.ds_nr + 1) & 0xFFFF
                time.sleep(self.meas_delay)
        except KeyboardInterrupt:
            pass
        finally:
            print("shutting down")
            self.disconnect()

    def stop(self) -> None:
        self._stop = True


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Start a modelled temperature measurement client."
    )
    parser.add_argument(
        "-V",
        "--version",
        action="version",
        version=VERSION,
        help="show program version and exit",
    )
    parser.add_argument(
        "-c",
        "--config",
        help="path to yaml configuration file",
        default="tmcm_config.yml",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="enable verbose output (show every publish/receive)",
    )
    args = parser.parse_args()

    config = load_config(args.config)
    model = TmcModel(config, verbose=args.verbose)

    # catch signals for graceful shutdown
    def handler(sig, frame):
        model.stop()

    signal.signal(signal.SIGINT, handler)
    signal.signal(signal.SIGTERM, handler)

    model.connect()
    model.run()


if __name__ == "__main__":
    main()

