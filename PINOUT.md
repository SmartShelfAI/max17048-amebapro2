# AMB82-mini Pinout — Final Configuration

**Board:** AmebaPRO2-mini (RTL8735B)  
**Firmware:** Scale_Main + MAX17048 Battery Monitor  
**Status:** Tested & Confirmed

---

## Pin Assignment Table

| Pin # | GPIO | Primary Function | Current Usage | Status |
|:-----:|:----:|:-----------------|--------------|:------:|
| 0 | PF_5 | SPI1_MISO | ADS1256 DOUT | ✅ Occupied |
| 1 | PF_6 | SPI1_SCLK | ADS1256 SCLK | ✅ Occupied |
| 2 | PF_7 | SPI1_MOSI | ADS1256 DIN | ✅ Occupied |
| 3 | PF_8 | SPI1_SS | *Not used* — CS is GPIO-controlled via Pin 10 | 🔵 Free |
| 4 | PF_11 | GPIO / PWM | — | 🔵 Free |
| 5 | PF_12 | GPIO / PWM | — | 🔵 Free |
| 6 | PF_13 | GPIO / PWM | — | 🔵 Free |
| 7 | PF_14 | GPIO / PWM | ADS1256 PDWN | ✅ Occupied |
| 8 | PF_15 | GPIO / PWM | — | 🔵 Free |
| 9 | PF_2 | I2C1_SDA / A2 | ADS1256 DRDY | ✅ Occupied |
| 10 | PF_1 | I2C1_SCL / A1 | ADS1256 CS (GPIO-controlled) | ✅ Occupied |
| 11 | PF_0 | GPIO / A0 | — | 🔵 Free |
| **12** | **PE_4** | **I2C_SDA / SPI_SS** | **SHT3x SDA + MAX17048 SDA** | ✅ **Occupied (I2C)** |
| **13** | **PE_3** | **I2C_SCL / SPI_MOSI** | **SHT3x SCL + MAX17048 SCL** | ✅ **Occupied (I2C)** |
| 14 | PE_2 | SPI_MISO / SERIAL3_RX | — | 🔵 Free |
| 15 | PE_1 | SPI_SCLK / SERIAL3_TX | — | 🔵 Free |
| 16 | PD_18 | GPIO | — | 🔵 Free |
| 17 | PD_17 | GPIO | — | 🔵 Free |
| 18 | PD_16 | SERIAL2_RX | — | 🔵 Free |
| 19 | PD_15 | SERIAL2_TX | — | 🔵 Free |
| 20 | PD_14 | GPIO | — | 🔵 Free |
| 21 | PA_2 | SERIAL1_TX / A6 | — | 🔵 Free |
| 22 | PA_3 | SERIAL1_RX / A7 | — | 🔵 Free |
| 23 | PF_9 | LED_B (Blue) | On-board LED | ✅ Occupied |
| 24 | PE_6 | LED_G (Green) | On-board LED | ✅ Occupied |
| 25 | PF_4 | LOG_TX | — | 🔵 Free |
| 26 | PF_3 | LOG_RX / A3 | — | 🔵 Free |
| 27 | PA_1 | I2C2_SDA / A5 / SWD_CLK | — | 🔵 Free |
| 28 | PA_0 | I2C2_SCL / A4 / SWD_DATA | — | 🔵 Free |
| 29 | PF_10 | GPIO | — | 🔵 Free |

---

## Bus Summary

| Bus | Pins | Devices | Notes |
|-----|------|---------|-------|
| **SPI1** | 0, 1, 2 | ADS1256 (DOUT, SCLK, DIN) | Hardware SPI1 |
| **GPIO** | 7, 10 | ADS1256 (PDWN, CS) | Software-controlled |
| **GPIO** | 9 | ADS1256 (DRDY) | Interrupt input |
| **I2C (Wire)** | **12, 13** | **SHT3x + MAX17048** | Shared bus, both devices confirmed working |

---

## Key Points

- **No conflict** between SPI1 (pins 0–3) and I2C (pins 12–13) — they use different physical peripherals
- **Pin 9/10 cannot be used for MAX17048** without re-wiring ADS1256 DRDY/CS to free pins (3, 4, 5, 6, 8, 11)
- **Pin 3 (SPI1_SS)** is free — can be used as additional GPIO if needed
- **I2C bus on 12/13** supports multiple devices — currently SHT3x (temp/humidity) + MAX17048 (battery)
- All I2C pull-ups are provided by the MAX17048 breakout module (~4.7kΩ each)

---

## Wiring Diagram (ASCII)

```
AMB82-mini          MAX17048 Module         ADS1256
==========          ===============         =======
3.3V  ───────────── VCC
3.3V  ──┬─────────── SDA (via 4.7k pull-up)
        │
3.3V  ──┼─────────── SCL (via 4.7k pull-up)
        │
Pin 12 ─┴─────────── SDA
Pin 13 ───────────── SCL
GND   ────────────── GND
      ────────────── BAT+  → LiPo+
      ────────────── BAT-  → LiPo-

Pin 0  ───────────── DOUT
Pin 1  ───────────── SCLK
Pin 2  ───────────── DIN
Pin 7  ───────────── PDWN
Pin 9  ───────────── DRDY
Pin 10 ───────────── CS
GND   ────────────── GND
3.3V  ────────────── DVDD / AVDD
```
