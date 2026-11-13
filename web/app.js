import { ESPLoader, Transport } from "./vendor/esptool.js";

const FIRMWARE_URL = "../releases/esp_yt_subscriber_counter.merged.bin";
const FIRMWARE_HASH_URL = "../releases/esp_yt_subscriber_counter.merged.bin.sha256";

const startButton = document.getElementById("start");
const retryButton = document.getElementById("retry");
const statusEl = document.getElementById("status");
const logEl = document.getElementById("log");
const progressShell = document.querySelector(".progress-shell");
const progressBar = document.getElementById("progress-bar");
const progressText = document.getElementById("progress-text");
const browserMessage = document.getElementById("browser-message");

function log(message) {
  logEl.textContent += `${message}\n`;
  logEl.scrollTop = logEl.scrollHeight;
}

function setStatus(message) {
  statusEl.textContent = message;
}

function setProgress(percent) {
  const value = Math.max(0, Math.min(100, percent));
  progressBar.style.width = `${value}%`;
  progressBar.setAttribute("aria-valuenow", String(Math.round(value)));
  progressText.textContent = `${Math.round(value)}%`;
}

function setBusy(isBusy, canRetry = false) {
  startButton.disabled = isBusy;
  retryButton.hidden = !(canRetry && !isBusy);
}

function resetUi() {
  logEl.textContent = "";
  setProgress(0);
  progressShell.hidden = false;
}

function hex(buffer) {
  return Array.from(new Uint8Array(buffer), byte => byte.toString(16).padStart(2, "0")).join("");
}

async function fetchExpectedHash() {
  const response = await fetch(FIRMWARE_HASH_URL);
  if (!response.ok) throw new Error(`Could not read firmware checksum (${response.status}).`);
  const text = await response.text();
  const expected = text.trim().split(/\s+/)[0].toLowerCase();
  if (!/^[a-f0-9]{64}$/.test(expected)) throw new Error("Firmware checksum file is invalid.");
  return expected;
}

async function fetchVerifiedFirmware() {
  setStatus("Reading and verifying firmware…");
  log("Reading merged firmware image…");
  const [response, expectedHash] = await Promise.all([
    fetch(FIRMWARE_URL),
    fetchExpectedHash(),
  ]);

  if (!response.ok) throw new Error(`Could not read firmware image (${response.status}).`);
  const buffer = await response.arrayBuffer();
  const data = new Uint8Array(buffer);
  const actualHash = hex(await crypto.subtle.digest("SHA-256", buffer));

  if (actualHash !== expectedHash) {
    throw new Error("Firmware checksum does not match. Refusing to flash.");
  }

  log(`Firmware verified: ${actualHash}`);
  return data;
}

function makeTerminal() {
  return {
    clean() {
      logEl.textContent = "";
    },
    writeLine(data) {
      log(data);
    },
    write(data) {
      logEl.textContent += data;
      logEl.scrollTop = logEl.scrollHeight;
    },
  };
}

async function startFlash() {
  if (!("serial" in navigator)) {
    setStatus("This browser does not support Web Serial. Use Chrome or Edge on a desktop computer.");
    retryButton.hidden = false;
    return;
  }
  if (!window.isSecureContext) {
    setStatus("Open this page from http://127.0.0.1:8000. Web Serial is unavailable on a network IP over plain HTTP.");
    retryButton.hidden = false;
    return;
  }

  if (window.location.protocol === "file:") {
    setStatus("Open this page through the local server at http://127.0.0.1:8000/web/ instead of loading the HTML file directly.");
    retryButton.hidden = false;
    startButton.disabled = true;
    return;
  }

  resetUi();
  setBusy(true);
  startButton.textContent = "Flashing…";
  setStatus("Choose your XIAO ESP32-C6 in the browser prompt…");

  let transport = null;
  try {
    const port = await navigator.serial.requestPort();
    const firmware = await fetchVerifiedFirmware();

    transport = new Transport(port, false);
    const loader = new ESPLoader({
      transport,
      baudrate: 115200,
      terminal: makeTerminal(),
    });

    setStatus("Connecting to the ESP32…");
    const chipName = await loader.main();

    if (!chipName.toLowerCase().includes("esp32-c6")) {
      throw new Error(`This flasher expects an ESP32-C6. Detected ${chipName}.`);
    }

    log(`Detected ${chipName}.`);
    setStatus("Flashing subscriber-counter firmware…");

    await loader.writeFlash({
      fileArray: [{ data: firmware, address: 0x0 }],
      flashMode: "keep",
      flashFreq: "keep",
      flashSize: "keep",
      eraseAll: false,
      compress: true,
      reportProgress: (_fileIndex, written, total) => {
        setProgress((written / total) * 100);
      },
    });

    setProgress(100);
    setStatus("Firmware installed. Resetting the board…");
    await loader.after("hard_reset");
    await transport.disconnect();
    transport = null;

    log("Done.");
    setStatus("Flash complete. Unplug and reconnect the XIAO, then join ESP-YT-Setup.");
    startButton.textContent = "Flash again";
  } catch (error) {
    console.error(error);
    log(`Error: ${error.message || error}`);
    if (error instanceof DOMException && error.name === "NotFoundError") {
      setStatus("No port was selected. Connect the XIAO and click Try again.");
    } else {
      setStatus("Flash failed. Hold BOOT while plugging in the XIAO, then click Try again.");
    }
  } finally {
    if (transport) {
      try {
        await transport.disconnect();
      } catch (closeError) {
        console.warn("Serial cleanup failed:", closeError);
      }
    }
    setBusy(false, true);
    startButton.textContent = "Flash again";
  }
}

startButton.addEventListener("click", startFlash);
retryButton.addEventListener("click", startFlash);

if (window.location.protocol === "file:") {
  browserMessage.textContent = "This page must be opened through the local server, not directly from disk.";
  browserMessage.classList.add("error");
  startButton.disabled = true;
} else if ("serial" in navigator) {
  browserMessage.textContent = "Web Serial is supported in this browser.";
  browserMessage.classList.add("ok");
} else {
  browserMessage.textContent = "This browser does not support Web Serial. Please use Chrome or Edge on a desktop computer.";
  browserMessage.classList.add("error");
  startButton.disabled = true;
}
