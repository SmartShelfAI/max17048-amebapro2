/*
 * MAX17048 — Wire (SDA=Pin 12, SCL=Pin 13)
 * AMB82-mini / AmebaPRO2-mini (RTL8735B)
 *
 * Правила для Ameba SDK (подтверждено workaround'ами):
 *   1. Использовать ТОЛЬКО Wire (не Wire1 — конфликтует с DMIC)
 *   2. Wire.begin() — ОДИН раз в setup()
 *   3. setClock(100000) или (400000), НЕ другие значения
 *   4. delay(1) после requestFrom() — обязательный workaround
 *   5. requestFrom((int)addr, 2) — int, не uint8_t
 *
 * Подключение:
 *   MAX17048 SDA  -> Pin 12 (PE4, I2C_SDA)
 *   MAX17048 SCL  -> Pin 13 (PE3, I2C_SCL)
 *   MAX17048 VCC  -> 3.3V
 *   MAX17048 GND  -> GND
 *   BAT+/BAT-     -> аккумулятор
 *
 * Подтяжки 4.7k уже есть на модуле (измерено: 9k между SDA-SCL).
 */

#include <Wire.h>

#define MAX17048_ADDR 0x36

uint16_t readReg(uint8_t reg);
void writeReg(uint8_t reg, uint16_t val);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println(F("\n========================================"));
  Serial.println(F("  MAX17048  Wire (SDA=12, SCL=13)"));
  Serial.println(F("  AmebaPRO2-mini workaround edition"));
  Serial.println(F("========================================\n"));

  // 1. Один раз инициализируем Wire
  Wire.begin();
  Wire.setClock(100000);   // только 100k или 400k!
  delay(50);

  // Сканер (медленный, с паузой)
  Serial.println(F("--- I2C Scan ---"));
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  0x")); if (a < 16) Serial.print('0');
      Serial.print(a, HEX);
      if (a == MAX17048_ADDR) Serial.print(F(" <-- MAX17048"));
      Serial.println();
      found++;
    }
    delay(2);
  }
  Serial.print(F("Found: ")); Serial.println(found);
  Serial.println();

  // Читаем VERSION
  uint16_t ver = readReg(0x08);
  Serial.print(F("VERSION = 0x")); Serial.println(ver, HEX);

  // Мягкая проверка: любая 0x001x — это MAX17048/49
  if ((ver & 0xFFF0) != 0x0010) {
    Serial.println(F("[FAIL] MAX17048 not found or bad VERSION"));
    Serial.println(F("Check: battery connected, VCC=3.3V, wiring"));
    while (1) delay(1000);
  }

  Serial.println(F("[OK] MAX17048 found!\n"));

  // Сброс POR-бита
  uint16_t status = readReg(0x00);
  if (status & 0x0002) {
    writeReg(0x00, status & ~0x0002);
  }

  // QuickStart
  writeReg(0x06, 0x4000);
  delay(200);

  Serial.println(F("--- Battery readings ---"));
}

void loop() {
  uint16_t vcell = readReg(0x02);
  uint16_t soc   = readReg(0x04);

  if (vcell == 0xFFFF || soc == 0xFFFF) {
    Serial.println(F("[ERROR] Read failed"));
  } else {
    float voltage = vcell * 78.125e-6f;
    float percent = soc / 256.0f;
    Serial.print(F("Voltage: ")); Serial.print(voltage, 3); Serial.print(F(" V"));
    Serial.print(F("  |  SOC: ")); Serial.print(percent, 1); Serial.println(F(" %"));
  }

  delay(2000);
}

// ── Чтение регистра (workaround: delay(1) после requestFrom) ───────────────
uint16_t readReg(uint8_t reg) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return 0xFFFF;

  // (int) важен для Ameba SDK — не использовать (uint8_t)
  Wire.requestFrom((int)MAX17048_ADDR, 2);

  delay(1);   // <-- ОФИЦИАЛЬНЫЙ workaround для AmebaPro2

  if (Wire.available() < 2) return 0xFFFF;

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  return ((uint16_t)msb << 8) | lsb;
}

// ── Запись 16-битного регистра ─────────────────────────────────────────────
void writeReg(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(MAX17048_ADDR);
  Wire.write(reg);
  Wire.write((uint8_t)(val >> 8));
  Wire.write((uint8_t)(val & 0xFF));
  Wire.endTransmission();
}
