# Third-party licenses

## Included in this repository

| Part | Files | License |
|---|---|---|
| **OCR-A font** (bitmap conversions for Adafruit GFX) | `include/OCRA5pt7b.h`, `OCRA8pt7b.h`, `OCRA12pt7b.h`, `OCRA17pt7b.h` | **Public domain** – font file by Matthew Skala (2011-12), based on code by Richard B. Wales (1988-89) and Tor Lillqvist. Skala: *"Any copyright claims by Matthew Skala on these files are hereby released to the public domain."* (https://tsukurimashou.org/ocr.pdf). Also distributed by Debian as `fonts-ocr-a` (public domain). |
| **HT_ST7789** display driver (Heltec, based on Adafruit_ST7789) | `lib/HT_ST7789/` | **MIT** – Adafruit Industries, written by Limor Fried / Ladyada; the license notice is kept in the source files. |
| **Board definitions** | `boards/*.json`, `variants/heltec_t190/pins_arduino.h` | PlatformIO / Arduino-ESP32 board definitions (Apache-2.0 / LGPL as in their upstream projects), adapted. |

## Downloaded by PlatformIO at build time (not part of this repository)

| Library | License |
|---|---|
| [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library) | BSD |
| [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO) | MIT |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | MIT |
| [TOTP-Arduino](https://github.com/lucadentella/TOTP-Arduino) (Luca Dentella) | Apache-2.0 |
| Arduino-ESP32 core / ESP-IDF (Espressif) | LGPL-2.1 / Apache-2.0 |

## Desktop app (`desktop/`, installed with pip, not part of this repository)

| Library | License |
|---|---|
| [cryptography](https://github.com/pyca/cryptography) | Apache-2.0 OR BSD-3-Clause |
| [qrcode](https://github.com/lincolnloop/python-qrcode) | BSD |
| [pywebview](https://github.com/r0x0r/pywebview) | BSD-3-Clause |
| [PyInstaller](https://github.com/pyinstaller/pyinstaller) (only to build the EXE) | GPL-2.0 with bootloader exception |

Battery pin information for the Heltec T190 comes from the public Meshtastic firmware
(`variants/esp32s3/heltec_vision_master_t190/variant.h`, GPL-3.0) – only the pin numbers / divider factor were used as
hardware facts, no code was copied.
