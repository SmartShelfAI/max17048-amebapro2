/*
 * MAX17048 — Wire1 (PIN 9/10), тестовый скетч
 * AmebaPRO2-mini (AMB82-mini)
 *
 * Цель: проверить, починили ли Wire1 в свежей версии SDK.
 *
 * Подключение:
 *   MAX17048 SDA  -> Pin 9  (PF2, I2C1_SDA)
 *   MAX17048 SCL  -> Pin 10 (PF1, I2C1_SCL)
 *   MAX17048 VCC  -> 3.3V
 *   MAX17048 GND  -> GND
 *   BAT+/BAT-     -> аккумулятор
 *
 * Если сканер покажет ТОЛЬКО 0x36 (без "гирлянды" 0x37-0x7E)
 * и VERSION = 0x001x — значит Wire1 работает!
 *
 * Если снова фантомы или Bus Fault — Wire1 всё ещё сломан.
 * Используй SoftI2C вариант в этом же каталоге.
 */

#include <Wire.h>

#define MAX17048_ADDR 0x36

uint16_t readReg(uint8_t reg);
void writeReg(uint8_t reg, uint16_t val);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println(F("\n========================================"));
  Serial.println(F("  MAX17048  Wire1 (PIN 9/10) TEST"));
  Serial.println(F("========================================"));
  Serial.println(F("\nЕсли SDK починили — сканер покажет только 0x36"));
  Serial.println(F("Если нет — увидишь гирлянду 0x36...0x7E или краш\n"));

  // Инициализация Wire1 — один раз, только 100kHz
  Wire1.begin();
  Wire1.setClock(100000);
  delay(100);

  // --- Сканер ---
  Serial.println(F("--- I2C Scan ---"));
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire1.beginTransmission(a);
    if (Wire1.endTransmission() == 0) {
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

  // --- Чтение VERSION ---
  uint16_t ver = readReg(0x08);
  Serial.print(F("VERSION = 0x")); Serial.println(ver, HEX);

  if ((ver & 0xFFF0) != 0x0010) {
    Serial.println(F("[FAIL] Wire1 не работает корректно"));
    Serial.println(F("Попробуй обновить Ameba boards package в Arduino IDE"));
    Serial.println(F("Или используй SoftI2C вариант (92_2_MAX_17048_SoftI2C.ino)"));
    while (1) delay(1000);
  }

  Serial.println(F("[OK] Wire1 работает! SDK починили.\n"));

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

// ── Чтение с workaround для Ameba ──────────────────────────────────────────
uint16_t readReg(uint8_t reg) {
  Wire1.beginTransmission((int)MAX17048_ADDR);
  Wire1.write(reg);
  if (Wire1.endTransmission(false) != 0) return 0xFFFF;

  Wire1.requestFrom((int)MAX17048_ADDR, 2);
  delay(1);   // workaround

  if (Wire1.available() < 2) return 0xFFFF;

  uint8_t msb = Wire1.read();
  uint8_t lsb = Wire1.read();
  return ((uint16_t)msb << 8) | lsb;
}

void writeReg(uint8_t reg, uint16_t val) {
  Wire1.beginTransmission((int)MAX17048_ADDR);
  Wire1.write(reg);
  Wire1.write((uint8_t)(val >> 8));
  Wire1.write((uint8_t)(val & 0xFF));
  Wire1.endTransmission();
}
