# Hardware Notes: ESP32-C3 OLED 0.42" Module

Board reference: https://www.espboards.dev/esp32/esp32-c3-oled-042/

## Pin Assignment

| Function | GPIO | Rationale |
|---|---|---|
| Optical sensor (tachometer) | IO3 | Safe GPIO, interrupt-capable, ADC-capable if analog filtering needed |
| OLED SDA | IO8 | Built-in, not reassignable |
| OLED SCL | IO9 | Built-in, not reassignable |

## Pin Selection Considerations

### Why GPIO3 for the sensor

On the ATmega328 version the sensor used D2 (INT0). GPIO2 on the ESP32-C3 is a boot-strapping pin (must be high during boot) -- connecting a sensor that may pull it low could prevent the board from starting. GPIO3 is the safest alternative:

- No boot-strap function
- No JTAG/USB conflict
- Supports external interrupts (all ESP32-C3 GPIOs do)
- Has ADC channel if analog threshold detection is ever needed
- Adjacent to power pins on the header for clean sensor wiring

### Pins to avoid

| GPIO | Reason |
|---|---|
| IO2 | Boot strapping -- must be high at boot |
| IO4, IO5, IO6, IO7 | JTAG interface (IO4/5 also SPI) |
| IO8, IO9 | Occupied by built-in OLED I2C |
| IO20, IO21 | USB/UART0 (serial monitor) |

### Safe GPIOs for future expansion

IO0, IO1, IO10 remain available. IO0 and IO1 have ADC channels. IO10 is UART1 TX by default but can be used as general GPIO if UART1 is not needed.

## Power

The optical sensor runs on 3.3V from the board's regulator. No level shifting needed -- ESP32-C3 is a 3.3V device natively. If the sensor is a 5V-only type, power it from the 5V pin but add a voltage divider or level shifter on the signal line.

## Built-in OLED

- Controller: SSD1306
- Resolution: 72x40
- Interface: I2C (GPIO8 SDA, GPIO9 SCL)
- U8g2 constructor: `U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);`
