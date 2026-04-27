/*
 * MAX17048 — SoftI2C (bit-bang) на PIN 9/10
 * AmebaPRO2-mini (AMB82-mini)
 *
 * Используем, если Wire1 (аппаратный I2C1) всё ещё сломан в SDK.
 *
 * Подключение:
 *   MAX17048 SDA  -> Pin 9  (PF2)
 *   MAX17048 SCL  -> Pin 10 (PF1)
 *   MAX17048 VCC  -> 3.3V
 *   MAX17048 GND  -> GND
 *   BAT+/BAT-     -> аккумулятор
 *
 * Подтяжки: на модуле уже есть (~4.7k каждая, 9k между SDA-SCL).
 *            НЕ вызывать Wire1.begin() — пины 9/10 должны быть свободны для GPIO!
 */

#define SOFT_SDA  9
#define SOFT_SCL  10

#define MAX17048_ADDR 0x36

// ── SoftI2C с улучшенным open-drain ────────────────────────────────────────
// Пины всегда OUTPUT. HIGH = отпускаем (тянет внешний резистор).
// LOW = прижимаем. Для чтения переключаем в INPUT (без pullup).

class SoftI2C {
public:
  void begin(uint8_t sda, uint8_t scl) {
    _sda = sda;
    _scl = scl;
    pinMode(_scl, OUTPUT);
    pinMode(_sda, OUTPUT);
    digitalWrite(_scl, HIGH);
    digitalWrite(_sda, HIGH);
    delayMicroseconds(20);
  }

  inline void sdaHigh() { digitalWrite(_sda, HIGH); }
  inline void sdaLow()  { digitalWrite(_sda, LOW);  }
  inline void sdaInput()  { pinMode(_sda, INPUT); }
  inline void sdaOutput() { pinMode(_sda, OUTPUT); digitalWrite(_sda, HIGH); }

  void start() {
    sdaHigh();
    digitalWrite(_scl, HIGH);
    delayMicroseconds(10);
    sdaLow();
    delayMicroseconds(10);
    digitalWrite(_scl, LOW);
    delayMicroseconds(10);
  }

  void stop() {
    sdaLow();
    digitalWrite(_scl, LOW);
    delayMicroseconds(10);
    digitalWrite(_scl, HIGH);
    delayMicroseconds(10);
    sdaHigh();
    delayMicroseconds(10);
  }

  bool writeByte(uint8_t data) {
    for (int i = 7; i >= 0; i--) {
      if (data & (1 << i)) sdaHigh(); else sdaLow();
      delayMicroseconds(5);
      digitalWrite(_scl, HIGH);
      delayMicroseconds(10);
      digitalWrite(_scl, LOW);
      delayMicroseconds(5);
    }
    sdaInput();
    delayMicroseconds(5);
    digitalWrite(_scl, HIGH);
    delayMicroseconds(10);
    bool ack = (digitalRead(_sda) == LOW);
    digitalWrite(_scl, LOW);
    delayMicroseconds(5);
    sdaOutput();
    return ack;
  }

  uint8_t readByte(bool ack) {
    uint8_t data = 0;
    sdaInput();
    for (int i = 7; i >= 0; i--) {
      delayMicroseconds(5);
      digitalWrite(_scl, HIGH);
      delayMicroseconds(10);
      if (digitalRead(_sda) == HIGH) data |= (1 << i);
      digitalWrite(_scl, LOW);
      delayMicroseconds(5);
    }
    sdaOutput();
    if (ack) sdaLow(); else sdaHigh();
    delayMicroseconds(5);
    digitalWrite(_scl, HIGH);
    delayMicroseconds(10);
    digitalWrite(_scl, LOW);
    delayMicroseconds(5);
    return data;
  }

  uint16_t readReg(uint8_t addr, uint8_t reg) {
    start();
    if (!writeByte(addr << 1)) { stop(); return 0xFFFF; }
    if (!writeByte(reg)) { stop(); return 0xFFFF; }
    start();
    if (!writeByte((addr << 1) | 1)) { stop(); return 0xFFFF; }
    uint8_t msb = readByte(true);
    uint8_t lsb = readByte(false);
    stop();
    return ((uint16_t)msb << 8) | lsb;
  }

  void writeReg(uint8_t addr, uint8_t reg, uint16_t val) {
    start();
    writeByte(addr << 1);
    writeByte(reg);
    writeByte(val >> 8);
    writeByte(val & 0xFF);
    stop();
  }

private:
  uint8_t _sda, _scl;
};

SoftI2C i2c;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println(F("\n========================================"));
  Serial.println(F("  MAX17048  SoftI2C (SDA=9, SCL=10)"));
  Serial.println(F("========================================"));
  Serial.println(F("\nWire1 НЕ используется — чистый GPIO bit-bang\n"));

  i2c.begin(SOFT_SDA, SOFT_SCL);
  delay(100);

  uint16_t ver = i2c.readReg(MAX17048_ADDR, 0x08);
  Serial.print(F("VERSION = 0x")); Serial.println(ver, HEX);

  if ((ver & 0xFFF0) != 0x0010) {
    Serial.println(F("[FAIL] Не удалось прочитать VERSION"));
    Serial.println(F("Проверь: подтяжки, батарею, VCC=3.3V, провода"));
    while (1) delay(1000);
  }

  Serial.println(F("[OK] MAX17048 найден!\n"));

  // QuickStart
  i2c.writeReg(MAX17048_ADDR, 0x06, 0x4000);
  delay(200);

  Serial.println(F("--- Battery readings ---"));
}

void loop() {
  uint16_t vcell = i2c.readReg(MAX17048_ADDR, 0x02);
  uint16_t soc   = i2c.readReg(MAX17048_ADDR, 0x04);

  if (vcell == 0xFFFF || soc == 0xFFFF) {
    Serial.println(F("[ERROR] Read failed"));
  } else {
    float voltage = vcell * 78.125e-6f;
    float percent = soc / 256.0f;
    Serial.print(F("Voltage: ")); Serial.print(voltage, 3); Serial.print(F(" V"));
    Serial.print(F("  |  SOC: ")); Serial.print(percent, 1); Serial.println(F(" %"));
  }

  delay(3000);
}
