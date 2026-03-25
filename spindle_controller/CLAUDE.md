# ESP32-C3 Spindle Tachometer

## Hardware Configuration
- OLED: SSD1306 72x40 on I2C (SDA=5, SCL=6)
- Tachometer sensor: GPIO 7 (INPUT_PULLUP, FALLING edge interrupt)
- PWM pass-through: GPIO 10 (in) → GPIO 9 (out)
- Probe relay output: GPIO 8 (HIGH when spindle running)

## Key Implementation Details
- RPM calculation: 60,000,000 / pulse_interval_microseconds (1 pulse per revolution)
- Motor stopped detection: no pulse for 1 second → RPM = 0
- Display updates at 10Hz, serial output at 250ms intervals
