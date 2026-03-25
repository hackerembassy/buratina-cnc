# Buratina CNC Spindle Tachometer

Optical tachometer for the Buratina CNC mill spindle, built on an ATmega328P (Arduino Mini) with a 16x2 I2C character LCD.

## How it works

An optical sensor on pin D2 generates one pulse per spindle revolution. The pulse triggers an ISR on the falling edge, which measures the interval between consecutive pulses. RPM is calculated as `60,000,000 / interval_us`. If no pulse is received for 1 second, RPM is zeroed (spindle stopped).

Statistics are updated every 250 ms on both the LCD and the serial port (115200 baud):
- Current RPM
- Total pulse count
- Pulse interval in microseconds

## Hardware

| Component | Connection |
|---|---|
| ATmega328P (Arduino Mini) | - |
| Optical sensor | D2 (INT0), INPUT_PULLUP |
| 16x2 HD44780 LCD (I2C) | A4 (SDA) / A5 (SCL), address 0x27 |

An SSD1306 128x64 OLED display was also tested via I2C (see `src/main_logo.cpp.bak`).

## Build

PlatformIO project. Build and upload:

```
pio run -t upload
pio device monitor
```

## Project files

| File | Purpose |
|---|---|
| `src/main_tacho.cpp` | Main tachometer firmware (active) |
| `src/tacho.cpp.stub` | Serial-only tachometer (no LCD) |
| `src/lcdtest.cpp.bak` | HD44780 LCD feature test |
| `src/main_logo.cpp.bak` | SSD1306 OLED display test |
| `src/main_i2cscan.cpp.bak` | I2C bus scanner utility |
| `src/main.cpp.blink.bak` | Basic LED blink test |

## Dependencies

- [U8g2](https://github.com/olikraus/u8g2) ^2.36.15 (OLED graphics)
- [LCD-I2C-HD44780](https://github.com/sstaub/LCD-I2C-HD44780) ^2.0.3 (character LCD)
