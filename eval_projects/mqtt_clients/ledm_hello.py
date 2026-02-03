#!/usr/bin/env python3
"""
Simple hello-world for a 64x64 RGB LED matrix using rpi-rgb-led-matrix.

Requirements:
  pip install paho-mqtt (only for MQTT client script)
  sudo apt-get install -y python3-dev python3-pil libfreetype6-dev libjpeg-dev
  Follow rpi-rgb-led-matrix installation instructions: https://github.com/hzeller/rpi-rgb-led-matrix

Example:
  sudo python3 ledm_hello.py --text "Hello World" --font ./fonts/7x13.bdf --brightness 20

This script draws static text centered on the matrix.
"""

import argparse
import time

try:
    from rgbmatrix import RGBMatrix, RGBMatrixOptions, graphics
except Exception as e:
    raise SystemExit(f"Failed to import rpi-rgb-led-matrix: {e}\nMake sure the library is installed and you're running this on the Raspberry Pi.")


def main():
    parser = argparse.ArgumentParser(description="Hello world text for 64x64 RGB matrix")
    parser.add_argument("--text", default="Hello World", help="Text to display")
    parser.add_argument("--font", default="/home/pi/rpi-rgb-led-matrix/fonts/7x13.bdf", help="Path to BDF font")
    parser.add_argument("--brightness", type=int, default=20, help="Brightness 0..100")
    parser.add_argument("--hardware-mapping", default="adafruit-hat", help="Hardware mapping (e.g. adafruit-hat)")
    parser.add_argument("--rotate", type=int, default=0, help="Rotate degrees (0/90/180/270) - not all setups support rotation via software")
    args = parser.parse_args()

    options = RGBMatrixOptions()
    options.rows = 64
    options.cols = 64
    options.chain_length = 1
    options.parallel = 1
    options.hardware_mapping = args.hardware_mapping
    options.brightness = args.brightness
    # tweak as needed for your hardware
    options.gpio_slowdown = 2

    matrix = RGBMatrix(options=options)
    offscreen_canvas = matrix.CreateFrameCanvas()

    import os

    def find_font(path):
        tried = []
        # direct path
        if path:
            tried.append(path)
            if os.path.exists(path) and os.access(path, os.R_OK):
                return path
        # same filename next to this script (project fonts folder)
        local = os.path.join(os.path.dirname(__file__), "fonts", os.path.basename(path or ""))
        tried.append(local)
        if os.path.exists(local) and os.access(local, os.R_OK):
            return local
        # some system locations (allow user to add more if needed)
        candidates = [
            "/usr/local/share/fonts/7x13.bdf",
            "/usr/share/fonts/7x13.bdf",
        ]
        for p in candidates:
            tried.append(p)
            if os.path.exists(p) and os.access(p, os.R_OK):
                return p
        # failed - build a helpful diagnostic message
        details = [f"Effective UID: {os.geteuid()}", "Tried these paths:"] + tried
        details_s = "\n  ".join(details)
        raise SystemExit(
            f"Failed to locate a readable font file.\n{details_s}\n\nIf the font is on a user-mounted filesystem (sshfs/FUSE) root may not have access.\nWorkarounds: run without sudo, copy the font to /usr/local/share/fonts and set readable permissions, or pass an explicit accessible path via --font.")

    font_path = find_font(args.font)

    font = graphics.Font()
    try:
        font.LoadFont(font_path)
    except Exception as e:
        raise SystemExit(f"Failed to load font '{font_path}': {e}")

    text_color = graphics.Color(255, 255, 0)

    # Rough centering: start x at 1 and draw
    x = 1
    y = 32  # baseline roughly at middle for 64px high

    graphics.DrawText(offscreen_canvas, font, x, y, text_color, args.text)
    matrix.SwapOnVSync(offscreen_canvas)

    # keep displayed until Ctrl+C
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        matrix.Clear()


if __name__ == "__main__":
    main()
