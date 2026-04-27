# MAX17048 Fuel Gauge — AmebaPRO2-mini (AMB82-mini)

Рабочая конфигурация мониторинга LiPo/Li-Ion аккумулятора на платформе **AmebaPRO2-mini (RTL8735B)** с использованием чипа **MAX17048**.

---

## Архитектура

### Интерфейс
- **I2C** (2-wire)
- Адрес: `0x36` (фиксированный, 7-bit)

### Пины (рабочие)

| MAX17048 | AmebaPRO2-mini | Примечание |
|----------|----------------|------------|
| **SDA** | **Pin 12** (PE4) | `Wire` (штатный I2C) |
| **SCL** | **Pin 13** (PE3) | `Wire` (штатный I2C) |
| **VCC** | **3.3V** | 2.5–5.5В допустимо |
| **GND** | **GND** | Общий |
| **BAT+** | + аккумулятора | Напрямую к ячейке LiPo |
| **BAT-** | − аккумулятора | Тот же GND |

### Подтяжки (pull-up)
На модуле MAX17048 (Adafruit/аналог) уже есть резисторы ~10kΩ. Дополнительные резисторы **не требуются**, если используется готовый модуль. Для голого чипа — нужны 4.7kΩ на SDA и SCL к 3.3V.

### Регистры и формулы

| Регистр | Адрес | Назначение | Формула |
|---------|-------|------------|---------|
| STATUS | `0x00` | Статус (POR-бит и др.) | — |
| VCELL | `0x02` | Напряжение ячейки | `raw × 78.125 µV` |
| SOC | `0x04` | Заряд (State of Charge) | `raw / 256.0 %` |
| MODE | `0x06` | Режим (QuickStart) | — |
| VERSION | `0x08` | Версия чипа | `0x001x` |

---

## Особенности AmebaPRO2-mini — ЧТО ПРИШЛОСЬ СДЕЛАТЬ

### 1. Использовать `Wire`, а НЕ `Wire1`
- `Wire` (Pin 12/13) — штатный I2C интерфейс AMB82-mini
- `Wire1` (Pin 9/10, PF1/PF2) — **конфликтует с DMIC** (звуковой подсистемой RTL8735B)
- При использовании `Wire1` SDK выдаёт `[MISC Err]Pin 2[26] is conflicted` и падает в **Bus Fault / HardFault**

### 2. `delay(1)` после `requestFrom()` — обязательный workaround
На AmebaPRO2 `requestFrom()` возвращает управление до того, как данные реально появятся в буфере. Без `delay(1)` `Wire.read()` возвращает `0x00` или мусор.

```cpp
Wire.requestFrom((int)MAX17048_ADDRESS, 2);
delay(1);  // <-- ОБЯЗАТЕЛЬНО для Ameba SDK
uint8_t msb = Wire.read();
uint8_t lsb = Wire.read();
```

### 3. `requestFrom((int)addr, 2)` — тип `int`, не `uint8_t`
SDK Ameba имеет перегрузку `requestFrom(int, int)`, но `requestFrom(uint8_t, uint8_t)` работает нестабильно или вызывает ambiguous call warning.

### 4. `Wire.setClock(100000)` — только 100kHz или 400kHz
Значения `50000` и другие произвольные частоты **валят SDK в Bus Fault**.

### 5. `Wire.begin()` — ТОЛЬКО ОДИН РАЗ в `setup()`
Повторный вызов `Wire.begin()` (например, в каждом методе чтения) приводит к `[MISC Err]Pin conflicted` и перезахвату пинов.

### 6. Проверка VERSION по маске
MAX17048 может иметь разные ревизии: `0x0010`, `0x0011`, `0x0012` и т.д. Жёсткая проверка `ver == 0x0010` отбраковывает рабочие чипы.

```cpp
if ((ver & 0xFFF0) != 0x0010) {  // любая 0x001x — OK
    // fail
}
```

---

## ЧЕГО ИЗБЕГАТЬ

| ❌ Не делай | ✅ Делай вместо |
|-------------|----------------|
| `Wire1` на Pin 9/10 | `Wire` на Pin 12/13 |
| `Wire.setClock(50000)` | `Wire.setClock(100000)` |
| `requestFrom((uint8_t)addr, (uint8_t)2)` | `requestFrom((int)addr, 2)` |
| `Wire.begin()` внутри методов чтения | `Wire.begin()` один раз в `setup()` |
| `read()` сразу после `requestFrom()` | `delay(1)` после `requestFrom()` |
| Жёсткая проверка `ver == 0x0010` | Маска `(ver & 0xFFF0) == 0x0010` |
| SoftI2C на Pin 9/10 (заблокированы SDK) | Аппаратный `Wire` (12/13) |
| Adafruit_MAX1704X (зависит от Adafruit_BusIO) | Свой минимальный драйвер на `Wire.h` |

---

## Структура файлов

```
92_MAX_17048/
├── README.md              # Этот файл
├── 92_MAX_17048.ino       # Standalone тестовый скетч
├── BatteryMonitor.h       # Модуль для Scale_Main
└── BatteryMonitor.cpp     # Модуль для Scale_Main
```

---

## Интеграция в Scale_Main

1. Убедись, что `TemperatureSensor.begin()` вызывается **до** `BatteryMonitor.begin()` (он делает `Wire.begin()`)
2. Замени `BatteryMonitor::readRegister()` на версию с `delay(1)` и `(int)`
3. Не вызывай `Wire.begin()` внутри `BatteryMonitor`
4. Используй `Wire` (Pin 12/13), разделяя шину с SHT3x

---

## История отладки (кратко)

- Сканер I2C на Ameba показывает 126 «фантомных» устройств — это нормальный баг SDK, не мешает работе
- `endTransmission(false)` (repeated start) — работает, но без `delay(1)` данные нулевые
- Проблема НЕ в подтяжках (они есть на модуле, 9k между SDA-SCL = 2×4.7k)
- Проблема НЕ в коде чипа MAX17048 — он отлично работает с правильными таймингами
- Основная проблема — специфика I2C реализации в SDK AmebaPRO2 4.0.9
