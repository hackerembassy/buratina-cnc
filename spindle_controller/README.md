# Buratina CNC Spindle Controller

Spindle tachometer and control daughterboard for the Buratina CNC mill, built on an **ESP32-C3 board with built-in 0.42" OLED** ([espboards.dev](https://www.espboards.dev/esp32/esp32-c3-oled-042/)).

## What it does

- Measures spindle RPM via an optical sensor (1 pulse/revolution) and displays it on the OLED
- Passes through the PWM spindle-speed signal from the CNC controller to the spindle driver
- Drives a probe relay output (HIGH when spindle is running) so the CNC controller can detect spindle state
- Streams RPM telemetry over serial at 250 ms intervals

## Board Pinout & Wiring

```
                        ┌──── USB-C ────┐
                        │  [ESP32-C3]   │
                        │  ┌────────┐   │
                        │  │  OLED  │   │
                        │  │ 72×40  │   │
                        │  └────────┘   │
                   5V ──┤■              ■├── GPIO10  ◄── PWM IN (from CNC)
                  GND ──┤■              ■├── GPIO21
                GPIO0 ──┤■              ■├── GPIO20
                GPIO1 ──┤■              ■├── GPIO7   ◄── TACHO SENSOR
                GPIO2 ──┤■              ■├── GPIO6   ──► I2C SCL (OLED)
                GPIO3 ──┤■              ■├── GPIO9   ──► PWM OUT (to spindle)
                GPIO4 ──┤■              ■├── GPIO8   ──► PROBE RELAY
  I2C SDA (OLED) ◄──── GPIO5 ──┤■    [BOOT] [RST]  │
                        └────────────────┘

  ◄── input    ──► output
```

Note: The built-in OLED is wired to GPIO8/GPIO9 by default on this board.
This project remaps I2C to **GPIO5 (SDA) / GPIO6 (SCL)** via `Wire.begin(5, 6)`,
freeing GPIO8 and GPIO9 for the probe relay and PWM output.

| Function | GPIO | Direction | Notes |
|---|---|---|---|
| I2C SDA (OLED) | 5 | Out | Remapped from default GPIO8 |
| I2C SCL (OLED) | 6 | Out | Remapped from default GPIO9 |
| Tachometer sensor | 7 | In | INPUT_PULLUP, FALLING edge interrupt |
| Probe relay | 8 | Out | HIGH = spindle running |
| PWM out | 9 | Out | Copied from PWM in via ISR |
| PWM in | 10 | In | From CNC controller |

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
cd spindle_controller
pio run                  # build
pio run -t upload        # flash
pio device monitor       # serial monitor (115200 baud)
```

## Dependencies

Managed by PlatformIO (`platformio.ini`):

- [U8g2](https://github.com/olikraus/u8g2) -- OLED graphics library
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) -- JSON serialization (reserved for future use)

## How RPM is calculated

An IRAM interrupt fires on each falling edge from the optical sensor. The interval between consecutive pulses gives instantaneous RPM:

```
RPM = 60,000,000 / pulse_interval_us
```

Pulses faster than 65 000 RPM equivalent (~923 us) are discarded as noise. If no pulse arrives for 1 second the spindle is considered stopped.

## Display

The OLED shows a large 5-digit RPM reading (with leading zeros) and a status line ("Running" / "Stopped"), refreshed at 10 Hz.
