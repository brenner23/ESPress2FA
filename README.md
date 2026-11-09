# 🔐 ESPress2FA

🇬🇧 **English** · 🇩🇪 [Deutsch](README.de.md)

A **2FA / TOTP code display for ESP32-S3 boards** – your two-factor codes (Google, GitHub, Discord, your web host …)
on a small display next to your keyboard, instead of digging out your phone. One firmware, three boards,
web interface, Wi-Fi setup from your phone, backups as JSON, updates over the network.

🔗 **Project page:** https://github.com/brenner23/ESPress2FA

> ## ⚠️ Read this first – security
> This device shows **valid login codes**. Treat it like a key.
> - **Only run it in your own, trusted home network.** Never in open or shared networks
>   (Freifunk, hotel, café, office/university Wi-Fi, guest networks): the web interface has **no login**,
>   so **everybody in the same network can open it and read all your 2FA codes**.
> - A **backup file contains all your 2FA secrets** (and your Wi-Fi password). Store it like a password safe export.
> - A 2FA code alone does not log anybody in – but together with a leaked password it does.
> - Hobby project, no warranty. Keep your phone authenticator (or the recovery codes) as a fallback.

## Supported boards

| Board | Display | Buttons | Battery | PlatformIO env |
|---|---|---|---|---|
| **Heltec Vision Master T190** (ESP32-S3, 16 MB, PSRAM) | ST7789 1.9" 320×170, SPI | 2 | ✅ voltage + % | `heltec_vision_master_t190` |
| **LilyGO T-Display S3** (ESP32-S3, 16 MB, PSRAM) | ST7789 1.9" 320×170, 8-bit parallel | 2 | ✅ voltage + % | `tdisplays3` |
| **LilyGO T-Dongle S3** (USB stick, ESP32-S3, 16 MB) | ST7735 0.96" 160×80, SPI | 1 | – | `tdongles3` |

All three are tested. Backups are interchangeable between the boards.

## Features

- **TOTP codes** (RFC 6238, 6 digits, 30 s) with countdown bar (green → yellow → red), up to 30 accounts
- **Import from Google Authenticator**: export QR code (`otpauth-migration://…`) or single `otpauth://totp/…` links –
  just scan with a USB QR reader into the import field, or paste
- **Web interface** (English / Deutsch): status, accounts, import, settings
  - reorder accounts by **drag & drop**, **rename** issuer / account (the secret stays untouched), enable/disable, delete
  - **click an account in the browser → the device shows exactly that code** and stays there while the code window is open
- **Clock screensaver** after 2 minutes; the **first button press returns to the last shown account**, the device
  remembers it even after a restart
- **Buttons** – decided on release:
  - two-button boards: short = next / previous, **hold = rotate the display 180°**
  - one button (T-Dongle): short = next, medium = previous, **hold = rotate**
  - hold time adjustable in the web (2–10 s, default 3 s); a **countdown bar** shows when releasing will rotate
- **Rotate 180°** also in the settings (buttons swap their roles accordingly)
- **Battery level** (T190, T-Display S3) on the web status page and on the clock
- **Fast display**: everything is drawn into a frame buffer and sent in one go (hardware SPI / 8-bit bus with DMA)
- **Wi-Fi setup portal** like a NerdMiner: no Wi-Fi stored (or not reachable) → own hotspot
  `2FA-T190-xxxx` / `2FA-TDS3-xxxx` / `2FA-DNG-xxxx`, password `2fa-setup`, page `http://192.168.4.1/`.
  Holding the button(s) during start forces the portal. **No credentials in the source code.**
- **Backup & restore** as JSON – download in the web; restore like on a Fritzbox: **all / without Wi-Fi / Wi-Fi only**
- **Firmware update over the network (OTA)** – in the web or with the PC tool. Every device creates its **own random
  OTA key** on first start (`t190-…`, `tds3-…`, `dongle-…`); it is shown on the start screen and can be read over USB,
  never over the web. Wrong key → rejected.
- **Network discovery**: the PC tool finds all ESPress2FA devices with one broadcast
- Device language **English / Deutsch** (display texts and web, switch in the settings)

## Build & flash

[PlatformIO](https://platformio.org/) (VS Code extension or CLI):

```bash
pio run -e heltec_vision_master_t190 -t upload     # Heltec Vision Master T190
pio run -e tdisplays3 -t upload                    # LilyGO T-Display S3
pio run -e tdongles3 -t upload                     # LilyGO T-Dongle S3
```

Connect the board **directly** to the PC (some USB 3 hubs don't detect ESP32-S3 boards).
After the first USB flash, further updates can go over the network (see below).

## First start

1. The device opens its hotspot (`2FA-…-xxxx`, password `2fa-setup`). Connect with your phone; the setup page opens
   (otherwise `http://192.168.4.1/`). Choose your network, enter the password, save.
2. The device restarts, connects and shows its **IP address** for 5 seconds (plus the OTA key).
3. Open `http://<IP>/` → **Import** → scan the Google Authenticator export QR code (or paste `otpauth://` links) →
   preview → import.
4. **Settings** → *Download backup* and keep the file in a safe place.

## PC tool (`tools/espress2fa_tool.py`)

Python 3 + `pip install pyserial`.

```bash
python tools/espress2fa_tool.py                      # menu (USB)
python tools/espress2fa_tool.py info                 # USB: device, version, IP, accounts
python tools/espress2fa_tool.py backup [file.json]   # USB: full backup
python tools/espress2fa_tool.py restore file.json --mode all|nowifi|wifi
python tools/espress2fa_tool.py getkey               # USB: read the OTA key once (stored in tools/ota_keys.json)
python tools/espress2fa_tool.py find                 # network: list all devices
python tools/espress2fa_tool.py ota firmware.bin --ip 192.168.x.y   # network update
```

The tool finds the device on USB by itself and opens the port **without resetting** the board.
`tools/ota_keys.json` holds your OTA keys – it is in `.gitignore`, don't share it.

## Desktop app (`desktop/`) – the same web interface on your PC

**ESPress2FA Desktop** shows exactly the device's web interface (accounts, drag & drop, rename, import, backup,
EN/DE) in a window on your PC – e.g. as a single EXE on a USB stick. The page is taken straight from the firmware
source, so device and desktop always look the same.

- All accounts live in **one encrypted vault file** (`ESPress2FA.vault` next to the program): **AES-256-GCM**,
  key derived from your **master password** with **scrypt** (deliberately slow against guessing).
  ⚠️ The master password **cannot be recovered** – keep the device (or the services' recovery codes) as fallback.
- Local server on **127.0.0.1 only** (port held exclusively), **no outgoing connections** at all.
- Unlock page, 🔒 lock button, **auto-lock after 5 minutes**; **click on a code = copy** (clipboard cleared after 30 s).
- **Export to a phone authenticator**: big QR codes in the Google Authenticator transfer format
  (8 accounts per code, several pages) – plus the link as text to paste into a device's import field.
- Import / export with the device's JSON backup (Settings → backup / restore).

```bash
cd desktop
pip install -r requirements.txt
python espress2fa_desktop.py              # app window (pywebview) - or --browser
build_exe.bat                             # Windows: single EXE in desktop/dist/ (no installation)
```

## Settings & data

Everything is stored in the device's NVS – accounts, Wi-Fi, brightness, rotation, hold time, language, last account,
OTA key. A firmware update (USB or OTA) **keeps** all of it. A brand-new board starts empty: restore your backup.

Time zone is Central Europe (`CET/CEST`), see `TZ_BERLIN` in `include/_VARIABLES.h`. Time comes from NTP
(`pool.ntp.org`) – TOTP needs the correct time.

## Project layout

```
include/BOARD.h        pins, buttons, battery and name per board
include/FUNC_*.h       TOTP, clock, web server + API, setup portal, backup, battery, buttons
include/FUNC_WEBCONTENT.h   the web interface (HTML/CSS/JS, EN + DE)
src/main.cpp           display drivers per board, includes
tools/espress2fa_tool.py    USB / network tool
```

## Licenses

ESPress2FA is MIT licensed (see [LICENSE](LICENSE)). Third-party parts: see [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).
