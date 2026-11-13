# ESP YouTube Subscriber Counter

A compact, Wi-Fi-connected YouTube subscriber counter for a Seeed Studio **XIAO ESP32-C6** and a 480×320 SPI display with an ILI9488-compatible controller and XPT2046/ADS7846-compatible touch controller.

The counter shows the public subscriber count, total views, public video count, connection status, and an Eastern-time clock. It stores Wi‑Fi and YouTube API credentials only in the ESP32's internal preferences; no credentials are committed to this repository.

## Features

- Public YouTube channel statistics using YouTube Data API v3
- Captive Wi‑Fi setup portal on first boot
- Automatic retry after network, DNS, time, or API failures
- Single tap to switch between the subscriber dashboard and clock
- Double tap to rotate the display 180°
- BOOT-button reset for saved settings
- Optional KiCad carrier board for a cleaner build

## Hardware you need

- Seeed Studio XIAO ESP32-C6
- 480×320 SPI display with an ILI9488-compatible controller and XPT2046/ADS7846-compatible touch controller
- USB-C data cable
- Jumper wires or the optional KiCad carrier board
- 3.3 V logic; do **not** connect a 5 V-only display directly

The display wiring is documented in [`hardware/xiao_c6_tinylcd_bottom_wiring.svg`](hardware/xiao_c6_tinylcd_bottom_wiring.svg).

## Software setup

1. Install [Arduino IDE 2](https://www.arduino.cc/en/software) or [Arduino CLI](https://arduino.github.io/arduino-cli/).
2. Install the **esp32** board package.
3. Select the **XIAO_ESP32C6** board.
4. Use the bundled Adafruit GFX and Adafruit BusIO libraries in [`libraries/`](libraries/), or install those two libraries through the Arduino Library Manager.
5. Create a YouTube Data API v3 key from [Google Cloud Console](https://console.cloud.google.com/), and restrict it to YouTube Data API v3.

Do not put your API key in the source code. The device asks for it during first-run setup and stores it locally.

## Build and upload

### Arduino IDE

1. Open `firmware/esp_yt_subscriber_counter/esp_yt_subscriber_counter.ino`.
2. Select **XIAO_ESP32C6**.
3. Click **Verify**, then **Upload**.

### Arduino CLI

```sh
arduino-cli compile \
  --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app \
  --libraries libraries \
  --output-dir build/esp_yt_subscriber_counter \
  firmware/esp_yt_subscriber_counter

arduino-cli upload \
  --port /dev/YOUR_SERIAL_PORT \
  --fqbn esp32:esp32:XIAO_ESP32C6:PartitionScheme=huge_app \
  --input-dir build/esp_yt_subscriber_counter
```

Replace `/dev/YOUR_SERIAL_PORT` with your actual serial port.

## First-run setup

1. Power the counter.
2. Join the `ESP-YT-Setup` Wi‑Fi hotspot with password `setup-subs`.
3. Open `http://192.168.4.1/`.
4. Select your Wi‑Fi network, enter its password, enter your YouTube channel handle, and paste your API key.
5. Save. The counter reboots, joins your network, and begins displaying your public channel statistics.

Hold the BOOT button while powering on to erase saved settings and return to the setup hotspot.

## Optional KiCad carrier board

The editable KiCad project is in [`hardware/tinylcd_xiao_ups_carrier_r2/`](hardware/tinylcd_xiao_ups_carrier_r2/README.md).

- KiCad schematic ERC and PCB DRC pass with no reported errors.
- The routed PCB and manufacturing files are included.
- **This board has not been fully fabricated or bench-tested yet.** Treat it as an unverified design until you have built and tested your own copy.
- If you use the 18650 backup-power circuit, follow the safety procedure in the hardware README.

## Repository layout

```text
firmware/esp_yt_subscriber_counter/  Counter firmware
firmware/xiao_c6_ili9488_test/       Display diagnostic firmware
hardware/                            Wiring diagram and KiCad carrier project
libraries/                           Bundled Adafruit graphics libraries
```

## License

Project code and hardware files are released under the [MIT License](LICENSE). Bundled third-party libraries retain their own licenses in their respective folders.
