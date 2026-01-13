# Hardware Setup

## Components

### Microcontroller: STM32F411CE (BlackPill)

The BlackPill is a development board based on the STM32F411CE microcontroller.

- ARM Cortex-M4 core at 100MHz
- 512KB Flash
- 128KB SRAM
- USB 2.0 Full Speed

### Display: TFT 2.4" 320x240 ST7789

A color TFT LCD display with SPI interface.

- Resolution: 320x240 pixels
- Driver: ST7789
- Interface: SPI
- Voltage: 3.3V

### Programmer: ST-Link V2

Used to upload firmware to the STM32.

## Wiring Diagram

```
STM32 BlackPill          TFT Display
--------------          -----------
     PA7  ------------->  MOSI
     PA5  ------------->  SCLK
     PA4  ------------->  CS
     PB1  ------------->  DC
     PB0  ------------->  RST
    3.3V  ------------->  VCC
     GND  ------------->  GND
```

## Assembly Notes

1. Connect the display to the STM32 using the SPI pins as shown above
2. Power the display with 3.3V (not 5V)
3. Connect the STM32 to the server via USB for serial communication
4. Use ST-Link V2 connected to SWDIO/SWCLK pins for programming

## Power Considerations

- The STM32 can be powered via USB
- The display draws approximately 20-40mA
- Total system power consumption is under 100mA
