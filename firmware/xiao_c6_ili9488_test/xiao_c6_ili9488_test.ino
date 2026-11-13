#include <Arduino.h>
#include <SPI.h>

// TinyLCD 480C_TP -> Seeed Studio XIAO ESP32-C6
// Use the physical XIAO labels so the sketch follows the bench wiring directly.
static constexpr int PIN_LCD_MOSI = D10;
static constexpr int PIN_LCD_SCLK = D8;
static constexpr int PIN_LCD_CS   = D9;
static constexpr int PIN_LCD_DC   = D0;  // TinyLCD "RS"
static constexpr int PIN_LCD_RST  = D3;

// BL is deliberately not connected. Touch pins are also left disconnected.
static constexpr uint16_t LCD_WIDTH  = 480;
static constexpr uint16_t LCD_HEIGHT = 320;
static constexpr uint32_t LCD_SPI_HZ = 10000000;

SPISettings lcdSpiSettings(LCD_SPI_HZ, MSBFIRST, SPI_MODE0);

void lcdCommand(uint8_t command, const uint8_t *data = nullptr, size_t length = 0) {
  SPI.beginTransaction(lcdSpiSettings);
  digitalWrite(PIN_LCD_CS, LOW);
  digitalWrite(PIN_LCD_DC, LOW);
  SPI.transfer(command);

  if (length > 0) {
    digitalWrite(PIN_LCD_DC, HIGH);
    for (size_t i = 0; i < length; ++i) {
      SPI.transfer(data[i]);
    }
  }

  digitalWrite(PIN_LCD_CS, HIGH);
  SPI.endTransaction();
}

void lcdHardwareReset() {
  digitalWrite(PIN_LCD_RST, HIGH);
  delay(10);
  digitalWrite(PIN_LCD_RST, LOW);
  delay(25);
  digitalWrite(PIN_LCD_RST, HIGH);
  delay(150);
}

void initILI9488() {
  lcdHardwareReset();

  // Software reset, followed by a conservative ILI9488 initialization sequence.
  lcdCommand(0x01);
  delay(150);

  const uint8_t positiveGamma[] = {
      0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78,
      0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F};
  lcdCommand(0xE0, positiveGamma, sizeof(positiveGamma));

  const uint8_t negativeGamma[] = {
      0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45,
      0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F};
  lcdCommand(0xE1, negativeGamma, sizeof(negativeGamma));

  const uint8_t powerControl1[] = {0x17, 0x15};
  lcdCommand(0xC0, powerControl1, sizeof(powerControl1));

  const uint8_t powerControl2[] = {0x41};
  lcdCommand(0xC1, powerControl2, sizeof(powerControl2));

  const uint8_t vcomControl[] = {0x00, 0x12, 0x80};
  lcdCommand(0xC5, vcomControl, sizeof(vcomControl));

  // Landscape orientation with BGR color order.
  const uint8_t memoryAccessControl[] = {0x28};
  lcdCommand(0x36, memoryAccessControl, sizeof(memoryAccessControl));

  // ILI9488's SPI interface uses 18-bit (three-byte) pixel data.
  const uint8_t pixelFormat[] = {0x66};
  lcdCommand(0x3A, pixelFormat, sizeof(pixelFormat));

  const uint8_t interfaceMode[] = {0x00};
  lcdCommand(0xB0, interfaceMode, sizeof(interfaceMode));

  const uint8_t frameRate[] = {0xA0};
  lcdCommand(0xB1, frameRate, sizeof(frameRate));

  const uint8_t inversionControl[] = {0x02};
  lcdCommand(0xB4, inversionControl, sizeof(inversionControl));

  const uint8_t displayFunction[] = {0x02, 0x02};
  lcdCommand(0xB6, displayFunction, sizeof(displayFunction));

  const uint8_t imageFunction[] = {0x00};
  lcdCommand(0xE9, imageFunction, sizeof(imageFunction));

  const uint8_t adjustControl[] = {0xA9, 0x51, 0x2C, 0x82};
  lcdCommand(0xF7, adjustControl, sizeof(adjustControl));

  lcdCommand(0x11);  // Sleep out
  delay(150);
  lcdCommand(0x29);  // Display on
  delay(50);
}

void setAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  const uint8_t columns[] = {
      static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0),
      static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1)};
  lcdCommand(0x2A, columns, sizeof(columns));

  const uint8_t rows[] = {
      static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0),
      static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1)};
  lcdCommand(0x2B, rows, sizeof(rows));

  lcdCommand(0x2C);  // Begin memory write
}

void writeSolidRow(uint8_t red, uint8_t green, uint8_t blue) {
  static uint8_t row[LCD_WIDTH * 3];
  for (uint16_t x = 0; x < LCD_WIDTH; ++x) {
    row[x * 3 + 0] = red;
    row[x * 3 + 1] = green;
    row[x * 3 + 2] = blue;
  }
  SPI.transfer(row, sizeof(row));
}

void fillScreen(uint8_t red, uint8_t green, uint8_t blue) {
  setAddressWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

  SPI.beginTransaction(lcdSpiSettings);
  digitalWrite(PIN_LCD_CS, LOW);
  digitalWrite(PIN_LCD_DC, HIGH);
  for (uint16_t y = 0; y < LCD_HEIGHT; ++y) {
    writeSolidRow(red, green, blue);
  }
  digitalWrite(PIN_LCD_CS, HIGH);
  SPI.endTransaction();
}

void drawColorBars() {
  static constexpr uint8_t colors[6][3] = {
      {0xFF, 0x00, 0x00},  // red
      {0x00, 0xFF, 0x00},  // green
      {0x00, 0x00, 0xFF},  // blue
      {0xFF, 0xFF, 0x00},  // yellow
      {0x00, 0xFF, 0xFF},  // cyan
      {0xFF, 0x00, 0xFF}   // magenta
  };
  static uint8_t row[LCD_WIDTH * 3];

  setAddressWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
  SPI.beginTransaction(lcdSpiSettings);
  digitalWrite(PIN_LCD_CS, LOW);
  digitalWrite(PIN_LCD_DC, HIGH);
  for (uint16_t y = 0; y < LCD_HEIGHT; ++y) {
    // SPI.transfer(buffer, size) is full duplex and may overwrite the buffer
    // with received bytes. Rebuild every row so all 320 rows retain the bars.
    for (uint16_t x = 0; x < LCD_WIDTH; ++x) {
      uint8_t bar = min<uint8_t>(x / (LCD_WIDTH / 6), 5);
      row[x * 3 + 0] = colors[bar][0];
      row[x * 3 + 1] = colors[bar][1];
      row[x * 3 + 2] = colors[bar][2];
    }
    SPI.transfer(row, sizeof(row));
  }
  digitalWrite(PIN_LCD_CS, HIGH);
  SPI.endTransaction();
}

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("TinyLCD first-pass ILI9488 test starting");
  Serial.printf("Pins: MOSI=D10/GPIO%d SCLK=D8/GPIO%d CS=D9/GPIO%d DC=D0/GPIO%d RST=D3/GPIO%d\n",
                PIN_LCD_MOSI, PIN_LCD_SCLK, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST);

  pinMode(PIN_LCD_CS, OUTPUT);
  pinMode(PIN_LCD_DC, OUTPUT);
  pinMode(PIN_LCD_RST, OUTPUT);
  digitalWrite(PIN_LCD_CS, HIGH);
  digitalWrite(PIN_LCD_DC, HIGH);

  SPI.begin(PIN_LCD_SCLK, -1, PIN_LCD_MOSI, -1);
  initILI9488();
  Serial.println("ILI9488 initialization sent; starting visual test cycle");
}

void loop() {
  fillScreen(0xFF, 0x00, 0x00);
  delay(1500);
  fillScreen(0x00, 0xFF, 0x00);
  delay(1500);
  fillScreen(0x00, 0x00, 0xFF);
  delay(1500);
  fillScreen(0xFF, 0xFF, 0xFF);
  delay(1500);
  fillScreen(0x00, 0x00, 0x00);
  delay(1000);
  drawColorBars();
  delay(3000);
}
