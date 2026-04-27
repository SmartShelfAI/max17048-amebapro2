#include "BatteryMonitor.h"

BatteryMonitor::BatteryMonitor() : sensorAvailable(false) {
}

bool BatteryMonitor::begin() {
  // Wire.begin() вызывается в TemperatureSensor::begin() — НЕ вызывать здесь!
  // Только проверяем наличие MAX17048
  sensorAvailable = false;

  uint16_t ver = readRegister(MAX17048_VERSION_REG);
  if ((ver & 0xFFF0) != 0x0010) {
    return false;
  }

  // Сброс POR-бита
  uint16_t status = readRegister(0x00);
  if (status & 0x0002) {
    writeRegister(0x00, status & ~0x0002);
  }

  // QuickStart для калибровки SOC
  writeRegister(MAX17048_MODE_REG, 0x4000);
  delay(200);

  sensorAvailable = true;
  return true;
}

bool BatteryMonitor::read(float &voltage, float &percentage) {
  if (!sensorAvailable) {
    return false;
  }

  uint16_t vcellRaw = readRegister(MAX17048_VCELL_REG);
  uint16_t socRaw   = readRegister(MAX17048_SOC_REG);

  if (vcellRaw == 0xFFFF || socRaw == 0xFFFF) {
    return false;
  }

  voltage    = (float)vcellRaw * 78.125e-6f;
  percentage = (float)socRaw   / 256.0f;

  return true;
}

// ── Чтение регистра с workaround для AmebaPRO2 ──────────────────────────────
// ОБЯЗАТЕЛЬНО:
//   - requestFrom((int)addr, 2) — int, не uint8_t
//   - delay(1) после requestFrom() — SDK требует паузу
//   - Wire.begin() только один раз (в TemperatureSensor)
// ────────────────────────────────────────────────────────────────────────────
uint16_t BatteryMonitor::readRegister(uint8_t reg) {
  Wire.beginTransmission(MAX17048_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return 0xFFFF;
  }

  // AmebaPRO2 workaround: (int) вместо (uint8_t) + delay(1)
  Wire.requestFrom((int)MAX17048_ADDRESS, 2);
  delay(1);

  if (Wire.available() < 2) {
    return 0xFFFF;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  return ((uint16_t)msb << 8) | lsb;
}

void BatteryMonitor::writeRegister(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(MAX17048_ADDRESS);
  Wire.write(reg);
  Wire.write((uint8_t)(val >> 8));
  Wire.write((uint8_t)(val & 0xFF));
  Wire.endTransmission();
}
