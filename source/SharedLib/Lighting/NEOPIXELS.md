# Adafruit_NeoPixel Library Reference

## Overview

Adafruit_NeoPixel controls addressable LED strips (WS2812B/NeoPixel type). It provides methods to set individual LED colors and push them to hardware.

---

## Constructor

```cpp
Adafruit_NeoPixel pixels(uint16_t numPixels, int16_t pin, neoPixelType type);
```

**Parameters:**
- `numPixels`: Total number of LEDs in the strip (e.g., 500)
- `pin`: GPIO pin for LED data line (e.g., 4)
- `type`: LED color order and speed (e.g., `NEO_RGB + NEO_KHZ800`)

**Example:**
```cpp
Adafruit_NeoPixel pixels(500, 4, NEO_RGB + NEO_KHZ800);
```

---

## Setup Methods

| Method | Parameters | Description |
|--------|-----------|-------------|
| `begin()` | None | Initialize the LED hardware. Call once at startup. |
| `setPin(int16_t p)` | Pin number | Change the GPIO pin used for LED data. |

**Example:**
```cpp
pixels.begin();
```

---

## Setting LED Colors

### Method 1: RGB Channels
```cpp
setPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
```
Set one LED using separate red, green, blue values (0-255 each).

**Example:**
```cpp
pixels.setPixelColor(0, 255, 0, 0);  // Set LED 0 to red
pixels.setPixelColor(1, 0, 255, 0);  // Set LED 1 to green
pixels.setPixelColor(2, 0, 0, 255);  // Set LED 2 to blue
```

### Method 2: Packed Color
```cpp
setPixelColor(uint16_t index, uint32_t packed_color)
```
Set one LED using a pre-packed uint32_t color.

**Example:**
```cpp
uint32_t red = pixels.Color(255, 0, 0);
pixels.setPixelColor(0, red);
```

---

## Getting LED Colors

```cpp
uint32_t getPixelColor(uint16_t index)
```
Returns the current color of one LED as a packed uint32_t.

**Example:**
```cpp
uint32_t current = pixels.getPixelColor(0);
```

---

## Bulk Operations

### Fill Range
```cpp
fill(uint32_t color, uint16_t first = 0, uint16_t count = 0)
```
Fill a range of LEDs with the same color. If `count=0`, fills all LEDs from `first` to the end.

**Examples:**
```cpp
// Fill all 500 LEDs with red
pixels.fill(pixels.Color(255, 0, 0), 0, 0);

// Fill LEDs 10-19 with green
pixels.fill(pixels.Color(0, 255, 0), 10, 10);

// Fill LEDs 0-99 with blue
pixels.fill(pixels.Color(0, 0, 255), 0, 100);
```

### Clear All LEDs
```cpp
clear()
```
Set all LEDs to black (off). Does NOT call `show()`.

**Example:**
```cpp
pixels.clear();
pixels.show();  // Must call show() to update hardware
```

---

## Displaying Changes

```cpp
show()
```
Send all LED colors to hardware. **Required after any color changes.** This is the only way changes are visible on the physical LEDs.

**Workflow:**
```cpp
pixels.setPixelColor(0, 255, 0, 0);   // Set in buffer
pixels.setPixelColor(1, 0, 255, 0);   // Set in buffer
pixels.show();                         // Send buffer to hardware
```

---

## Color Conversion (Static Methods)

### RGB to Packed Color
```cpp
static uint32_t Color(uint8_t r, uint8_t g, uint8_t b)
```
Convert separate RGB values to a packed uint32_t.

**How it works:**
- Takes three bytes: R, G, B
- Packs them into one 32-bit integer via bit-shifting
- Formula: `(R << 16) | (G << 8) | B`

**Examples:**
```cpp
uint32_t red = pixels.Color(255, 0, 0);      // 0xFF0000
uint32_t green = pixels.Color(0, 255, 0);    // 0x00FF00
uint32_t blue = pixels.Color(0, 0, 255);     // 0x0000FF
uint32_t white = pixels.Color(255, 255, 255); // 0xFFFFFF
uint32_t purple = pixels.Color(255, 0, 255); // 0xFF00FF
```

### RGBW to Packed Color (for RGBW strips)
```cpp
static uint32_t Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
```
Convert RGB plus white channel to a packed uint32_t (for RGBW LED strips).

**Example:**
```cpp
uint32_t color = pixels.Color(255, 0, 0, 100);  // Red with white channel
```

### HSV to Packed Color
```cpp
static uint32_t ColorHSV(uint16_t hue, uint8_t sat = 255, uint8_t val = 255)
```
Convert HSV (Hue, Saturation, Value) to packed uint32_t.

**Parameters:**
- `hue`: 0-65535 (0° to 360°)
- `sat`: 0-255 (0% to 100%, default 255=full)
- `val`: 0-255 (0% to 100%, default 255=full)

**Example:**
```cpp
uint32_t hueColor = pixels.ColorHSV(0, 255, 255);      // Red (hue 0°)
uint32_t greenHue = pixels.ColorHSV(21845, 255, 255);  // Green (hue 120°)
```

---

## Brightness Control

### Set Global Brightness
```cpp
setBrightness(uint8_t brightness)
```
Set global brightness for all LEDs. 0=off, 255=full brightness.

**Example:**
```cpp
pixels.setBrightness(128);  // 50% brightness
pixels.show();
```

### Get Current Brightness
```cpp
uint8_t getBrightness()
```
Returns the current brightness setting (0-255).

---

## Information Methods

| Method | Return Type | Description |
|--------|-----------|-------------|
| `numPixels()` | uint16_t | Total number of LEDs in the strip. |
| `getPin()` | int16_t | GPIO pin number used for data (-1 if not set). |
| `canShow()` | bool | Returns true if `show()` will send immediately, false if it will block waiting for latch time. |

**Example:**
```cpp
uint16_t count = pixels.numPixels();  // Returns 500
int16_t dataPin = pixels.getPin();    // Returns 4
```

---

## Complete Workflow Example

```cpp
// Initialize
Adafruit_NeoPixel pixels(500, 4, NEO_RGB + NEO_KHZ800);
pixels.begin();

// Set brightness
pixels.setBrightness(200);

// Set individual LEDs
pixels.setPixelColor(0, pixels.Color(255, 0, 0));    // Red
pixels.setPixelColor(1, pixels.Color(0, 255, 0));    // Green
pixels.setPixelColor(2, pixels.Color(0, 0, 255));    // Blue

// Fill a range
pixels.fill(pixels.Color(255, 255, 0), 3, 10);       // Yellow LEDs 3-12

// Update hardware
pixels.show();

// Later: clear all
pixels.clear();
pixels.show();
```

---

## Key Constraints

- **No external buffer access**: Adafruit_NeoPixel manages its own internal buffer. You cannot get a pointer to it.
- **Atomic updates**: All changes are buffered until `show()` is called.
- **One LED at a time**: Use `setPixelColor()` for individual LEDs or `fill()` for ranges. Loop if needed for bulk operations.
- **Latch time**: After `show()`, there is a ~300 microsecond "quiet time" before new data can be sent.

---

## Color Order Reference

| Constant | Order | Use Case |
|----------|-------|----------|
| `NEO_RGB` | Red, Green, Blue | Most common |
| `NEO_GRB` | Green, Red, Blue | Many WS2812B strips |
| `NEO_BRG` | Blue, Red, Green | Some variations |
| `NEO_GBR` | Green, Blue, Red | Some variations |
| `NEO_RBG` | Red, Blue, Green | Some variations |
| `NEO_BGR` | Blue, Green, Red | Some variations |

Combine with speed: `NEO_RGB + NEO_KHZ800` or `NEO_GRB + NEO_KHZ800`

---

## Speed Reference

- `NEO_KHZ800`: 800 KHz (standard, default)
- `NEO_KHZ400`: 400 KHz (older WS2811 chips, less common)
