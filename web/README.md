# Local web flasher

This folder contains a small, offline-capable web page for flashing the ESP-YT subscriber counter to a Seeed Studio XIAO ESP32-C6.

## Start the local server

From the repository root:

```sh
python3 -m http.server 8000
```

Open:

<http://127.0.0.1:8000/web/>

Use Chrome or Edge on the same computer that has the XIAO connected.

## What the page does

1. Requests permission to use the XIAO’s serial port.
2. Detects the ESP32-C6 and refuses to flash other chips.
3. Verifies the included firmware’s SHA-256 checksum.
4. Writes the merged 4 MB image to flash at offset `0x0`.
5. Resets the board and tells you to join `ESP-YT-Setup`.

If the browser cannot open the board, hold the XIAO’s **BOOT** button while plugging it in, then click **Try again**.

The bundled `vendor/esptool.js` is the upstream Espressif `esptool-js` browser bundle, licensed under Apache 2.0; see `vendor/esptool-js-LICENSE.txt`.
