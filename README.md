# MaxLedControl

An Arduino library for MAX7219/MAX7221 LED drivers (matrices and 7-segment). Features Hardware SPI support, daisy-chaining, and full [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) integration.

## Installation

Install `MaxLedControl` and `Adafruit GFX Library` via the Arduino Library Manager.

## Configuration

Include the library and instantiate the object. Hardware SPI is recommended for performance.

### 1. Hardware SPI (Recommended)
Uses default SPI pins (SCK, MOSI) + a specific CS pin.

```cpp
// Syntax: LedControl(csPin, numDevices, [flipped], [spiClass])
LedControl matrix = LedControl(10, 4); 
````

### 2. Software SPI

Uses any digital pins. Slower, but flexible wiring.

```cpp
// Syntax: LedControl(dataPin, clkPin, csPin, numDevices, [flipped])
LedControl matrix = LedControl(12, 11, 10, 4);
```

**Parameters:**

  * `csPin`, `dataPin`, `clkPin`: Arduino pin numbers.
  * `numDevices`: Number of chained modules (default: 1).
  * `flipped`: `true` if display output is horizontally mirrored (default: false).
  * `spiClass`: Pointer to specific SPI bus (e.g., `&SPI1`) for advanced boards.

## Core API

### System Control

  * `begin(intensity)`: Initialise displays. `intensity` ranges 0-15 (default: 8).
  * `shutdown(addr, status)`: `true` to sleep, `false` to wake specific device `addr`.
  * `setIntensity(addr, intensity)`: Set brightness (0-15) for device `addr`.
  * `setScanLimit(addr, limit)`: Limit scanned rows (0-7) to increase brightness on small displays.
  * `clearDisplay(addr)`: Clear specific device.
  * `clear()`: Clear all devices.

### Native Drawing (Low Level)

  * `setLed(addr, row, col, state)`: Control single LED. `state` is boolean.
  * `setRow(addr, row, value)`: Set row (0-7) using a byte (e.g., `B10100000`).
  * `setColumn(addr, col, value)`: Set column (0-7) using a byte.

### 7-Segment Display

  * `setDigit(addr, digit, value, dp)`: Show hex number (0-15). `dp` enables decimal point.
  * `setChar(addr, digit, char, dp)`: Show char (0-9, A, E, F, H, L, P, -, ., space).

### Text Helper

  * `scroll(message, delay)`: Scrolls text across the matrix. `delay` is ms per frame (default: 50).

## Adafruit GFX Reference

This library inherits from `Adafruit_GFX`. Coordinates `(x, y)` span across all daisy-chained devices (e.g., 4 devices = 32x8 pixels).

**Common Methods:**

  * `drawPixel(x, y, color)`
  * `drawLine(x0, y0, x1, y1, color)`
  * `drawRect(x, y, w, h, color)` / `fillRect(...)`
  * `drawCircle(x, y, r, color)` / `fillCircle(...)`
  * `setCursor(x, y)`
  * `print("Text")`
  * `setRotation(0-3)`

*Note: `color` should always be 1 (ON) or 0 (OFF).*

## Complete Example

```cpp
#include <MaxLedControl.h>

// CS pin 10, 4 daisy-chained devices
LedControl matrix = LedControl(10, 4);

void setup() {
  matrix.begin(8); // Wake up, set brightness to 8
}

void loop() {
  // 1. GFX Graphics
  matrix.clear();
  matrix.drawLine(0, 0, 31, 7, 1); // Line across 4 devices
  matrix.drawCircle(15, 3, 3, 1);
  delay(1000);

  // 2. Native Control
  matrix.setLed(0, 0, 0, true); // Top-left pixel on device 0
  delay(500);

  // 3. Text Scrolling
  matrix.scroll("Hello World", 40);
}
```

## Troubleshooting

  * **No Display:** Check wiring (DIN-\>MOSI, CLK-\>SCK, CS-\>CS). Ensure `begin()` is called.
  * **Garbage/Flickering:** Check ground connections. Add a 10µF capacitor across VCC/GND near the display. Ensure power supply is sufficient.
  * **Mirrored Output:** Pass `true` as the `flipped` argument in the constructor.