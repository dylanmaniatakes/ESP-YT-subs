#pragma once

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <SPI.h>

class TinyLCD9488 : public Adafruit_GFX {
 public:
  TinyLCD9488(int8_t cs, int8_t dc, int8_t rst, int8_t mosi, int8_t sclk,
              int8_t miso = -1);

  void begin(uint32_t frequency = 10000000);
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void startWrite() override;
  void endWrite() override;
  void writePixel(int16_t x, int16_t y, uint16_t color) override;
  void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                     uint16_t color) override;
  void writeFastHLine(int16_t x, int16_t y, int16_t w,
                      uint16_t color) override;
  void writeFastVLine(int16_t x, int16_t y, int16_t h,
                      uint16_t color) override;
  void setRotation180(bool rotated);

 private:
  int8_t _cs;
  int8_t _dc;
  int8_t _rst;
  int8_t _mosi;
  int8_t _sclk;
  int8_t _miso;
  uint32_t _frequency;
  bool _inTransaction;
  bool _rotated180;

  void hardwareReset();
  void initializeController();
  void command(uint8_t command, const uint8_t *data = nullptr,
               size_t length = 0);
  void commandInTransaction(uint8_t command, const uint8_t *data = nullptr,
                            size_t length = 0);
  void setAddressWindow(int16_t x, int16_t y, int16_t w, int16_t h);
  void writeColor(uint16_t color, uint32_t count);
};
