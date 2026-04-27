#pragma once

#include <Arduino.h>
#include <Wire.h>

#define MAX17048_ADDRESS     0x36
#define MAX17048_VCELL_REG   0x02
#define MAX17048_SOC_REG     0x04
#define MAX17048_MODE_REG    0x06
#define MAX17048_VERSION_REG 0x08

class BatteryMonitor {
public:
  BatteryMonitor();
  bool begin();
  bool read(float &voltage, float &percentage);
  bool isAvailable() const { return sensorAvailable; }

private:
  bool sensorAvailable;
  uint16_t readRegister(uint8_t reg);
  void writeRegister(uint8_t reg, uint16_t val);
};
