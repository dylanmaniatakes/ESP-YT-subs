#include "TinyLCD9488.h"

TinyLCD9488::TinyLCD9488(int8_t cs, int8_t dc, int8_t rst, int8_t mosi,
                         int8_t sclk, int8_t miso)
    : Adafruit_GFX(480, 320),
      _cs(cs),
      _dc(dc),
      _rst(rst),
      _mosi(mosi),
      _sclk(sclk),
      _miso(miso),
      _frequency(10000000),
      _inTransaction(false),
      _rotated180(false) {}

void TinyLCD9488::begin(uint32_t frequency) {
  _frequency = frequency;
  pinMode(_cs, OUTPUT);
  pinMode(_dc, OUTPUT);
  pinMode(_rst, OUTPUT);
  digitalWrite(_cs, HIGH);
  digitalWrite(_dc, HIGH);
  digitalWrite(_rst, HIGH);

  SPI.begin(_sclk, _miso, _mosi, -1);
  initializeController();
}

void TinyLCD9488::setRotation180(bool rotated) {
  // This salvaged controller accepts the initial landscape MADCTL value but
  // does not reliably honor later MX/MY changes. Keep its proven hardware
  // orientation and rotate drawing coordinates in software instead.
  _rotated180 = rotated;
  const uint8_t memoryAccess[] = {0x28};
  command(0x36, memoryAccess, sizeof(memoryAccess));
}

void TinyLCD9488::startWrite() {
  if (_inTransaction) return;
  SPI.beginTransaction(SPISettings(_frequency, MSBFIRST, SPI_MODE0));
  digitalWrite(_cs, LOW);
  _inTransaction = true;
}

void TinyLCD9488::endWrite() {
  if (!_inTransaction) return;
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
  _inTransaction = false;
}

void TinyLCD9488::commandInTransaction(uint8_t value, const uint8_t *data,
                                       size_t length) {
  digitalWrite(_dc, LOW);
  SPI.transfer(value);
  if (length > 0) {
    digitalWrite(_dc, HIGH);
    for (size_t i = 0; i < length; ++i) SPI.transfer(data[i]);
  }
}

void TinyLCD9488::command(uint8_t value, const uint8_t *data, size_t length) {
  startWrite();
  commandInTransaction(value, data, length);
  endWrite();
}

void TinyLCD9488::hardwareReset() {
  digitalWrite(_rst, HIGH);
  delay(10);
  digitalWrite(_rst, LOW);
  delay(25);
  digitalWrite(_rst, HIGH);
  delay(150);
}

void TinyLCD9488::initializeController() {
  hardwareReset();
  command(0x01);
  delay(150);

  const uint8_t positiveGamma[] = {
      0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78,
      0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F};
  command(0xE0, positiveGamma, sizeof(positiveGamma));

  const uint8_t negativeGamma[] = {
      0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45,
      0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F};
  command(0xE1, negativeGamma, sizeof(negativeGamma));

  const uint8_t powerControl1[] = {0x17, 0x15};
  command(0xC0, powerControl1, sizeof(powerControl1));
  const uint8_t powerControl2[] = {0x41};
  command(0xC1, powerControl2, sizeof(powerControl2));
  const uint8_t vcomControl[] = {0x00, 0x12, 0x80};
  command(0xC5, vcomControl, sizeof(vcomControl));

  // Landscape orientation, BGR panel order, and 18-bit SPI pixel format.
  const uint8_t memoryAccess[] = {0x28};
  command(0x36, memoryAccess, sizeof(memoryAccess));
  const uint8_t pixelFormat[] = {0x66};
  command(0x3A, pixelFormat, sizeof(pixelFormat));

  const uint8_t interfaceMode[] = {0x00};
  command(0xB0, interfaceMode, sizeof(interfaceMode));
  const uint8_t frameRate[] = {0xA0};
  command(0xB1, frameRate, sizeof(frameRate));
  const uint8_t inversionControl[] = {0x02};
  command(0xB4, inversionControl, sizeof(inversionControl));
  const uint8_t displayFunction[] = {0x02, 0x02};
  command(0xB6, displayFunction, sizeof(displayFunction));
  const uint8_t imageFunction[] = {0x00};
  command(0xE9, imageFunction, sizeof(imageFunction));
  const uint8_t adjustControl[] = {0xA9, 0x51, 0x2C, 0x82};
  command(0xF7, adjustControl, sizeof(adjustControl));

  command(0x11);
  delay(150);
  command(0x29);
  delay(50);
}

void TinyLCD9488::setAddressWindow(int16_t x, int16_t y, int16_t w,
                                   int16_t h) {
  const uint16_t x1 = x + w - 1;
  const uint16_t y1 = y + h - 1;
  const uint8_t columns[] = {
      static_cast<uint8_t>(x >> 8), static_cast<uint8_t>(x),
      static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1)};
  const uint8_t rows[] = {
      static_cast<uint8_t>(y >> 8), static_cast<uint8_t>(y),
      static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1)};
  commandInTransaction(0x2A, columns, sizeof(columns));
  commandInTransaction(0x2B, rows, sizeof(rows));
  commandInTransaction(0x2C);
  digitalWrite(_dc, HIGH);
}

void TinyLCD9488::writeColor(uint16_t color, uint32_t count) {
  const uint8_t red5 = (color >> 11) & 0x1F;
  const uint8_t green6 = (color >> 5) & 0x3F;
  const uint8_t blue5 = color & 0x1F;
  const uint8_t red = (red5 << 3) | (red5 >> 2);
  const uint8_t green = (green6 << 2) | (green6 >> 4);
  const uint8_t blue = (blue5 << 3) | (blue5 >> 2);

  static constexpr uint16_t kChunkPixels = 96;
  uint8_t buffer[kChunkPixels * 3];
  while (count > 0) {
    const uint16_t chunk = min<uint32_t>(count, kChunkPixels);
    for (uint16_t i = 0; i < chunk; ++i) {
      buffer[i * 3 + 0] = red;
      buffer[i * 3 + 1] = green;
      buffer[i * 3 + 2] = blue;
    }
    SPI.transfer(buffer, chunk * 3);
    count -= chunk;
  }
}

void TinyLCD9488::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || y < 0 || x >= width() || y >= height()) return;
  startWrite();
  writePixel(x, y, color);
  endWrite();
}

void TinyLCD9488::writePixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || y < 0 || x >= width() || y >= height()) return;
  if (_rotated180) {
    x = width() - 1 - x;
    y = height() - 1 - y;
  }
  setAddressWindow(x, y, 1, 1);
  writeColor(color, 1);
}

void TinyLCD9488::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                                uint16_t color) {
  if (w <= 0 || h <= 0 || x >= width() || y >= height()) return;
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > width()) w = width() - x;
  if (y + h > height()) h = height() - y;
  if (w <= 0 || h <= 0) return;

  if (_rotated180) {
    x = width() - x - w;
    y = height() - y - h;
  }

  setAddressWindow(x, y, w, h);
  writeColor(color, static_cast<uint32_t>(w) * h);
}

void TinyLCD9488::writeFastHLine(int16_t x, int16_t y, int16_t w,
                                 uint16_t color) {
  writeFillRect(x, y, w, 1, color);
}

void TinyLCD9488::writeFastVLine(int16_t x, int16_t y, int16_t h,
                                 uint16_t color) {
  writeFillRect(x, y, 1, h, color);
}
