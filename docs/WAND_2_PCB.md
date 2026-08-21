<h1><span class="logo-icon"></span> GPStar Wand || PCB Hookup</h1>

&starf; For a comparison of the original GPStar and GPStar II controllers please see [this comparison guide](https://gpstartechnologies.com/blogs/gpstar-blog/gpstar-ii-vs-gpstar) on the main GPStar website.

Welcome to the second generation of GPStar Proton Wand controllers, intended for users who wish to fully replace the stock lighting of their HasLab Neutrona Wand or are building a DIY wand. This device improves on several core feature changes based on real-world use and customer requests.

**Significant Features/Changes:**

- Processor change to ESP32-S3 SoC instead of ATMega2560, offering dual CPU cores operating at a faster clock speed.
- Integrates a WiFi radio for direct web-enabled controls and firmware updates.
- Features a 6-axis gyroscope and acceleration sensor for motion response.
- Dedicated sockets for optional components and future expansion.
    - An infrared LED add-on is available to support integration with the Ghost Trap.

**Discontinued Hardware:**

In an effort to make room for the new PCB components, the decision to deprecate several Hasbro-specific devices was made. Please note the following wand-specific devices which should be used or replaced if using the GPStar II Neutrona Wand PCB as a standalone upgrade. If purchasing a new GPStar II kit, all components included with a bundled kit will be fully compatible.

Support for the stock, analog Vent/Top lights has been REMOVED in favor of addressable LED models.

  - ✅ SUPPORTED MODELS:
    - [GPStar RGB LED Vent Light](https://gpstartechnologies.com/products/gpstar-rgb-vent-lights) 
  - ❌ NO LONGER COMPATIBLE:
    - Frutto Technology "high intensity" vent light

Support for the stock, analog 5-LED bargraph has been REMOVED in favor of a true multi-segment bargraph.

  - ✅ SUPPORTED MODELS:
    - [GPStar Neutrona Wand Bargraph](https://gpstartechnologies.com/products/gpstar-neutrona-wand-bargraph) with a blue PCB color (all light colors supported)
    - Frutto Technology bargraphs with a black, blue, or purple PCB color; or clearly labelled "v3.0" or later
  - ❌ NO LONGER COMPATIBLE:
    - Frutto Technology bargraphs with the green PCB color, possibly labelled with "v2.0" (or earlier)

**Other Hardware Changes:**

- Programming pins were replaced with a USB-C connector for direct connectivity to your computer for firmware updates.
- Introduction of the newest GPStar Wand II JST model supports the new GPStar Neutrona Barrel II (more info below).

**IMPORTANT: The GPStar Wand II controller is NOT backwards compatible with the original GPStar Pack controller! DO NOT attempt to mix and match wands as you can potentially damage your devices.**

![Product View](images/GPStarWandII.jpg)

This guide is part of the kit approach to providing a minimally-invasive upgrade to the stock HasLab controllers. For the Neutrona Wand all available connections for JST-PH wiring is present, though a significant amount of wire-cutting will be required to separate the stock controller and re-attach using terminal blocks on the new PCB.

**NOTICE:** This device REQUIRES the [GPStar Neutrona Wand Bargraph](https://gpstartechnologies.com/products/gpstar-neutrona-wand-bargraph) and [GPStar RGB LED Vent Light](https://gpstartechnologies.com/products/gpstar-rgb-vent-lights). Support for the stock 5-LED bargraph and stock 2-LED top/vent light assembly has been discontinued as these devices required significant wiring connections on the controller.

![Bargraph PCB Connections](images/Wand2PCB-Standard.png)

### UPDATED: GPStar Wand II JST

Same device, simpler connections! This model supports the new [GPStar Neutrona Barrel II](https://gpstartechnologies.com/products/gpstar-neutrona-barrel-ii) which packs 48 RGB LEDs + 1 RGB LED tip, with a built-in infrared transmitter LED at the tip. That new barrel design utilizes the 4-pin connection built into the JST model wand controller.

> For non-JST GPStar Wand II owners, a special 4-pin to 3+1 pin splitter cable is available for supporting the GPStar Neutrona Barrel II device using the existing 3-pin barrel connector and 3-pin Infrared connector.

![Bargraph PCB Connections](images/Wand2PCB-JST-Standard.png)

## Neutrona Wand - Connection Details

Connections for the wand should be made according to the tables below.

- Ordering aligns with PCB labels or when viewed left-to-right with the connector keyhole at the bottom.
- Pins denoted A#/D# correspond to the internal code and connection to the controller chip.
- Ground may be designated as "GND" or simply "-".

![](images/Wand2PCB-Labels.png)

### Stock Connectors (JST-PH)

| Label | Pins | Notes |
|-------|------|-------|
| 5V-IN | +/\- | 2-pin JST-PH for power from Proton Pack.<br/>**This MUST be a regulated 5V source!** |
| Q2 | VCC/D10/GND | 3-pin JST-PH connection for addressable barrel LEDs |
| SW45/SW4 | GND/D2/GND/D3 | 4-pin JST-PH connection for the Intensify button and Activate toggle |
| SW6 | GND/A6 | 2-pin JST-PH connection for the orange wand-end mode/alt switch |

**Note:** The 3-pin connector for the Barrel LEDs is compatible with the [GPStar 50-LED Neutrona Barrel](https://gpstartechnologies.com/products/gpstar-neutrona-barrel) or the [GPStar Barrel LED Mini](https://gpstartechnologies.com/products/gpstar-barrel-led-mini).

### Stock Connectors (Terminal Blocks)

| Label/Pin | Color | Notes |
|-----------|-------|-------|
| A7 | <font color="orange">Orange</font> | Barrel extension switch (wire order does not matter) |
| GND | <font color="orange">Orange</font> | Barrel extension switch (wire order does not matter) |
| D8 | <font color="red">Red</font> | Slo-Blo VCC |
| GND | Black | Slo-Blo GND |
| D4 | <font color="brown">Brown</font> | Lower-right Toggle (wire order does not matter) |
| GND | <font color="brown">Brown</font> | Lower-right Toggle (wire order does not matter) |
| A0 | <font color="red">Red</font> | Upper-right Toggle (wire order does not matter) |
| GND | <font color="red">Red</font> | Upper-right Toggle (wire order does not matter) |
| 5V+ | <font color="red">Red</font> | Clippard LED (Top Left) VCC |
| D9 | <font color="gold">Yellow</font> | Clippard LED (Top Left) GND |
| R+ | <font color="red">Red</font> | Rumble (vibration) motor VCC |
| R- | Black | Rumble (vibration) motor GND |
| GND | Black | Ground for RGB vent light |
| D12 | <font color="gray">White</font> | RGB vent light data |
| VL+ | <font color="red">Red</font> | VCC for RGB vent light |
| D7 | <font color="orange">Orange</font> | Rotary encoder B |
| D6 | <font color="red">Red</font> | Rotary encoder A |
| ROT- | <font color="brown">Brown</font> | Ground for rotary encoder |

### Special Connectors

| Label | Pins | Notes |
|-------|------|-------|
| PACK (Serial) | TX1/RX1 | Serial communication to the Proton Pack. **IMPORTANT: The GPStar Wand II controller is NOT backwards compatible with the original GPStar Pack controller! DO NOT attempt to mix and match wands as you can potentially damage your devices.**<br><br>`Connector type: JST-PH` |
| AUDIO BOARD | GND/NC/VCC/TX/RX/NC | Communication and Power for the wands's GPStar Audio or WAV Trigger.<br><br>`Connector type: JST-PH` |
| USB-C | Socket | This controller now comes with a standard USB-C socket for programming, though it is now possible to update using over-the-air (OTA) process via the WiFi and web interface |

### Optional Connectors

| Label | Pins | Notes |
|-------|------|-------|
| HAT1 | GND/D22 | Connection for top of the barrel tip hat LED.<br><br>The left (top in the photo) pin is GND, the right (bottom in the photo) pin is D22 which provides 5V and has a 150Ω resistor connected to it.<br><br>`Connector type: JST-PH`<br><br>`Do not draw more than 40mA from this connector.` |
| HAT2 | GND/D23 | Connection for the wand box hat LED.<br><br>The left (top in the photo) pin is GND, the right (bottom in the photo) pin is D23 provides 5V and has a 150Ω resistor connected to it.<br><br>`Connector type: JST-PH`<br><br>`Do not draw more than 40mA from this connector.` |
| BARREL-LED | GND/D24 | Connection for white wand tip light.<br><br>The left (top in the photo) pin is GND, the right (bottom in the photo) pin is D24 which provides 5V and has a 100Ω resistor connected to it.<br><br>`Connector type: JST-PH`<br><br>`Do not draw more than 40mA from this connector.` |
| 5V-OUT | +/\- | Power for additional accessories, intended for the 28-segment or 30-segment bargraph.<br><br>`Connector type: JST-PH` |
| SCL/SDA | SCL/SDA | Expansion serial port using I2C, intended for the 28-segment or 30-segment bargraph.<br><br>`Connector type: JST-PH` |
| INFRARED | 5V/D17/GND | Dedicated port for an infrared LED circuit (port outputs 5V with a signal line).<br><br>`Connector type: JST-PH` |

### Hardware Calibration

In order to get accurate readings from your wand's magnetometer you must first perform magnetic calibration based on your specific installation and hardware. This means the calibration must be performed AFTER the controller has been fully installed with the speaker and any other accessories within your wand. Please see the [Wand Calibration Guide](WAND_CALIBRATION.md) for more information.
