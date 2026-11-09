# Changelog

## 1.5.0
- First public release as **ESPress2FA**
- Language **English / Deutsch** for the web interface and the device display (setting in the web)
- English comments, README in English and German, licenses
- Setup hotspot password is now `2fa-setup`

## 1.4.x
- **1.4.5** countdown bar while holding a button (shows when releasing will rotate)
- **1.4.4** fix: a short press sometimes rotated the display (`millis()|1` could lie 1 ms in the future →
  unsigned underflow); rotation is now decided on release
- **1.4.3** buttons decided on release, hold = rotate (hold time 2–10 s in the web)
- **1.4.2** T-Dongle: IP on the start screen in OCR-A 8 pt, no automatic text wrap
- **1.4.1** core debug output off (it broke USB answers), battery card hidden on boards without battery
- **1.4.0** **LilyGO T-Dongle S3** (160×80, one button); clicking an account in the web shows it on the device

## 1.3.x
- **1.3.4** OTA key starts with the board id (`t190-`, `tds3-`, `dongle-`)
- **1.3.3** random OTA key per device (USB only), network discovery (UDP 41900)
- **1.3.2** firmware update over the network (web upload)
- **1.3.1** rotate the display 180° (buttons swap roles)
- **1.3.0** one project for several boards (`include/BOARD.h`): Heltec T190 + **LilyGO T-Display S3**

## 1.2.0
- Battery level (web status + clock)

## 1.1.x
- Hardware SPI + frame buffer instead of software SPI (much faster display)
- Back to the last account after the clock screensaver, remembered in NVS
- Web: drag & drop sorting (HTML5), rename accounts
- Wi-Fi setup portal, no credentials in the source code
- JSON backup / restore (all / without Wi-Fi / Wi-Fi only), USB commands + PC tool
- IP shown for 5 s at start (without blocking)

## 1.0
- TOTP display for the Heltec Vision Master T190, Google Authenticator import, web interface, clock
