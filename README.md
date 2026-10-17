<div align="center">

# GPS Speedometer
### An AVR-based GPS speed display with a custom animated OLED interface

**Embedded C · ATmega328P · UART / NMEA · SPI OLED · u-blox GNSS**

![C](https://img.shields.io/badge/Language-C-A8B9CC?logo=c&logoColor=white)
![MCU](https://img.shields.io/badge/MCU-ATmega328P-1565C0)
![GPS](https://img.shields.io/badge/GNSS-u--blox%20NEO--M8U-1976D2)
![Display](https://img.shields.io/badge/Display-128%C3%9764%20OLED-7952B3)
![Project](https://img.shields.io/badge/Type-Embedded%20prototype-2E7D32)

*A hardware-focused embedded systems project built around GNSS data acquisition, interrupt-driven serial communication and low-level display control.*

</div>

---

## At a glance

The **GPS Speedometer** receives navigation data from a u-blox GNSS receiver, extracts speed and related information from NMEA sentences, and presents the result on a 128 × 64 monochrome OLED. The firmware is written in **C for the ATmega328P** and implements the UART, GPS configuration, data parsing, SPI display interface and animated visuals.

**What this project demonstrates**

- **Embedded communication:** interrupt-driven UART reception with a ring buffer and non-blocking reads.
- **Protocol handling:** NMEA **RMC** and **GGA** parsing and u-blox **UBX** configuration messages.
- **Measurement processing:** conversion from knots to km/h using integer-scaled values.
- **Bare-metal display programming:** direct SPI commands, font rendering and OLED page addressing.
- **Constrained graphics:** tiled parallax layers and a four-frame cyclist sprite stored in AVR program memory.
- **Debugging support:** sentence-rate and main-loop counters, plus LED status outputs.

> **Implementation note:** The current firmware displays the parsed GPS speed, but the parallax background is temporarily driven by a **fixed speed level**. See [Current limitations](#current-limitations) for details.

## System architecture

~~~mermaid
flowchart LR
    A["u-blox GNSS receiver"] -->|"NMEA over UART · 9600 baud"| B["ATmega328P"]
    B --> C["UART RX interrupt<br/>256-byte ring buffer"]
    C --> D["RMC / GGA parser"]
    D --> E["GPS data model<br/>speed · time · position · fix"]
    E --> F["OLED interface / graphics"]
    F -->|"SPI"| G["128 × 64 OLED"]
    B -->|"UBX configuration via UART TX"| A
~~~

### Data flow

1. The receiver transmits NMEA sentences over UART.
2. A UART receive interrupt places incoming bytes in a ring buffer.
3. The main loop assembles complete sentences and parses **RMC** and **GGA** fields.
4. Parsed values are stored in a shared GPS data structure.
5. The OLED code draws speed/time text and the animated background.

The GNSS configuration profile requests **RMC + GGA** messages at a nominal **2 Hz**, while disabling several unused NMEA message types. The actual receiver configuration depends on firmware compatibility and whether the UBX commands are accepted.

## Hardware and interfaces

| Component | Purpose | Implementation |
|:--|:--|:--|
| **ATmega328P** | Main microcontroller | 16 MHz target, AVR C firmware |
| **u-blox NEO-M8U** | GNSS / speed source | Model indicated by the repository's receiver-version log |
| **128 × 64 monochrome OLED** | Speed and graphical display | SPI, custom low-level display driver |
| **Status LEDs** | Reception / processing indicators | GPIO outputs PD2, PD3 and PD6 in firmware |

### Connections used by the firmware

| Signal | ATmega328P pin | Notes |
|:--|:--|:--|
| GNSS TX → MCU RX | **PD0 / RXD** | UART data, configured for 9600 baud |
| MCU TX → GNSS RX | **PD1 / TXD** | UBX configuration commands |
| OLED clock | **PB5 / SCK** | Hardware SPI |
| OLED data | **PB3 / MOSI** | Hardware SPI |
| OLED chip select | **PB2** | CS |
| OLED data/command | **PB1** | D/C |
| OLED reset | **PB0** | RES |
| Status outputs | **PD2, PD3, PD6** | Firmware-controlled LEDs |

**Electrical note:** Verify GNSS-module power and I/O voltage levels against the exact module and board being used. Connect grounds and use appropriate level shifting if required. The table describes **firmware pin assignments**, not a verified universal wiring schematic.

## Firmware highlights

### GPS speed processing

The RMC parser reads speed in knots and converts it into km/h. To avoid relying on floating-point calculations for the displayed value, the implementation uses scaled integers:

- Source value: **knots × 1000**
- Stored value: **km/h × 10**
- Display format: for example, a stored value of **123** represents **12.3 km/h**
- Values below **1.0 km/h** are currently displayed as zero.

RMC also supplies UTC time, date, course and navigation-validity information. GGA supplies fix quality, satellite count, position and altitude.

### OLED graphics

The display is driven directly over SPI rather than through an external graphics framework. The visual layer includes:

- Two horizontally scrolling background layers with a parallax effect.
- A cyclist character with **four animation frames**.
- Bitmap and sprite data stored in **PROGMEM** to conserve SRAM.
- A GPS information overlay containing speed and time.

The current main loop refreshes the background roughly every **50 ms** and advances the cyclist animation roughly every **200 ms**.

## Build and run

This repository contains a **Microchip Studio / Atmel Studio AVR-GCC project** rather than an Arduino sketch.

1. Clone the repository:

   ~~~bash
   git clone https://github.com/Stargaze99/Speedometer.git
   cd Speedometer
   ~~~

2. Open **Speedometer.atsln** in a compatible Microchip Studio / Atmel Studio installation.
3. Verify the target is **ATmega328P**, with a **16 MHz** clock configuration matching the firmware assumptions.
4. Connect the GNSS receiver and SPI OLED according to the pin table and the electrical specifications of your actual modules.
5. Build the project with AVR-GCC and program the microcontroller using a compatible AVR programmer.
6. Power the system and allow the GNSS receiver time to obtain valid satellite data, preferably in an environment with a clear view of the sky.

**Configuration:** Default UART baud rate and CPU clock are defined in [Speedometer/include/config.h](Speedometer/include/config.h). GNSS output settings are in [Speedometer/drivers/gps_cfg.c](Speedometer/drivers/gps_cfg.c).

## Project structure

~~~text
Speedometer/
├── Speedometer.atsln          # AVR IDE solution
└── Speedometer/
    ├── main.c                # System initialization and application loop
    ├── Speedometer.cproj     # Microchip/Atmel project configuration
    ├── drivers/
    │   ├── uart.c            # Interrupt-driven UART reception
    │   ├── gps_cfg.c         # GNSS message/rate configuration
    │   ├── ubx_tx.c          # UBX packet generation
    │   ├── spi.c             # Low-level SPI
    │   └── sys_time.c        # Timekeeping utilities
    ├── functions/
    │   ├── nmea_parse.c      # RMC / GGA parsing
    │   ├── data.c            # GPS data initialization
    │   ├── oled.c            # OLED display driver
    │   ├── font.c            # Display font
    │   └── background.c      # Parallax and sprite animation
    └── include/              # Public module headers
~~~

## Current limitations

This repository is a learning-oriented embedded prototype. The source code shows several areas still suitable for improvement:

- **Background animation:** The main loop currently forces <code>bg_speed_level = 2</code>, so scrolling is **not yet tied to live speed**, even though a speed-based update function exists.
- **GPS validation:** NMEA sentences are parsed without explicit checksum verification. A GPS fix flag is recorded, but invalid-fix handling should be strengthened before relying on displayed readings.
- **Time:** The current display code applies a fixed **UTC+2** offset; it does not implement daylight-saving changes or full date rollover.
- **Configuration feedback:** UBX configuration commands are sent without confirming successful acknowledgement.
- **Testing:** This repository does not include an automated test suite or documented measurement-accuracy results.

## Potential next steps

- Link the parallax speed level to validated GPS velocity.
- Add NMEA checksum checking and explicit no-fix / stale-data handling.
- Separate local-time conversion from display rendering.
- Add captured OLED photos, hardware wiring documentation and bench-test results.

---

<sub>**Yrgo · Electronics engineering / embedded systems project** · Source: [main.c](Speedometer/main.c) · [NMEA parser](Speedometer/functions/nmea_parse.c) · [Display graphics](Speedometer/functions/background.c)</sub>
