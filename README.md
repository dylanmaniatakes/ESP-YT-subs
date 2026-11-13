# ESP YouTube Subscriber Counter

A compact, Wi-Fi-connected YouTube subscriber counter for a Seeed Studio **XIAO ESP32-C6** and a 480×320 SPI display with an ILI9488-compatible controller and XPT2046/ADS7846-compatible touch controller.

The device shows the public subscriber count, total views, public video count, connection status, and an Eastern-time clock. It stores Wi-Fi and YouTube API credentials only in the ESP32's internal preferences; no credentials are committed to this repository.

## What you need

- Seeed Studio XIAO ESP32-C6
- 480×320 SPI display with an ILI9488-compatible controller and XPT2046/ADS7846-compatible touch controller
- USB-C **data** cable — not a charge-only cable
- Jumper wires or the optional KiCad carrier board
- 3.3 V logic; do **not** connect a 5 V-only display directly

The display wiring is documented in [`hardware/xiao_c6_tinylcd_bottom_wiring.svg`](hardware/xiao_c6_tinylcd_bottom_wiring.svg).

## Choose how to install the firmware

There are three ways to put the firmware on the XIAO:

1. **Use the hosted web flasher** — easiest; no command line.
2. **Use the local web flasher** — useful offline or for testing.
3. **Build and upload with Arduino** — useful if you want to modify the firmware.

### Option 1: Hosted web flasher (recommended)

This is the easiest path if you only want to use the project. You do not need Arduino, a terminal, or a custom build.

Web Serial is the browser feature that lets a web page ask permission to use the XIAO’s USB serial connection. Chrome, Brave, and Edge support it.

Open this page on the computer connected to the XIAO:

[Open the ESP-YT web flasher](https://dylanmaniatakes.github.io/ESP-YT-subs/web/)

Then:

1. Connect the XIAO to the computer with a USB-C data cable.
2. Click **Start flashing**.
3. When the browser asks for a serial port, select the XIAO.
4. Wait for the progress bar to reach 100%.
5. Unplug and reconnect the XIAO.

A few things are normal:

- The browser may list other serial devices, including Bluetooth devices. Choose only the XIAO.
- If the XIAO does not appear, hold its **BOOT** button while plugging it in, then click **Try again**.
- The flasher checks the bundled firmware’s SHA-256 checksum and refuses to flash a non-ESP32-C6 chip.

Because the hosted page is served over HTTPS, the browser can open the normal Web Serial device chooser.

### Option 2: Local web flasher

Use this if you want an offline copy or do not want to use the hosted page.

1. Start a small local web server from the repository root:

   ```sh
   python3 -m http.server 8000
   ```

2. Open:

   <http://127.0.0.1:8000/web/>

3. Click **Start flashing** and choose the XIAO in the browser prompt.
4. Wait for the progress bar to finish, then unplug and reconnect the device.

Do **not** open `web/index.html` directly from Finder or VS Code. Web Serial needs the `http://127.0.0.1` origin, so the device chooser will not appear from a `file://` page.

### Option 3: Build and upload with Arduino

Use this if you want to change the firmware.

1. Install [Arduino IDE 2](https://www.arduino.cc/en/software) or [Arduino CLI](https://arduino.github.io/arduino-cli/).
2. Install the **esp32** board package.
3. Select the **XIAO_ESP32C6** board.
4. Use the bundled Adafruit GFX and Adafruit BusIO libraries in [`libraries/`](libraries/), or install those two libraries through the Arduino Library Manager.

#### Arduino IDE

1. Open `firmware/esp_yt_subscriber_counter/esp_yt_subscriber_counter.ino`.
2. Select **XIAO_ESP32C6**.
3. Click **Verify**, then **Upload**.

#### Arduino CLI

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

The firmware does not contain your Wi-Fi password, YouTube handle, or API key.

1. Power the counter.
2. Join the `ESP-YT-Setup` Wi-Fi hotspot.
3. Enter the hotspot password: `setup-subs`.
4. Open `http://192.168.4.1/`.
5. Select your Wi-Fi network, enter its password, enter your YouTube channel handle, and paste your YouTube Data API v3 key.
6. Save.

The counter reboots, joins your network, and starts showing your public channel statistics.

To reset the saved setup later, hold the XIAO’s **BOOT** button while powering it on. The device will clear its saved settings and return to the setup hotspot.

## Features

- Public YouTube channel statistics using YouTube Data API v3
- Captive Wi-Fi setup portal on first boot
- Automatic retry after network, DNS, time, or API failures
- Single tap to switch between the subscriber dashboard and clock
- Double tap to rotate the display 180°
- BOOT-button reset for saved settings
- Optional KiCad carrier board for a cleaner build

## API key notes

Create a YouTube Data API v3 key from [Google Cloud Console](https://console.cloud.google.com/), and restrict it to YouTube Data API v3.

Do not put the API key in the source code. The device asks for it during first-run setup and stores it locally.

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
releases/                           Prebuilt merged firmware for the web flasher
web/                                Local Web Serial flasher
libraries/                           Bundled Adafruit graphics libraries
```

## License

Project code and hardware files are released under the [MIT License](LICENSE). Bundled third-party libraries retain their own licenses in their respective folders.
