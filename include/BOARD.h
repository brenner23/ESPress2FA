#pragma once

// ╔══════════════════════════════════════════════════════════╗
// ║  BOARD.h  –  everything that differs per device          ║
// ║  (since 1.3.0, one project for all 2FA displays)         ║
// ║  The PlatformIO env selects the board:                   ║
// ║  -DBOARD_T190 / -DBOARD_TDISPLAYS3 / -DBOARD_TDONGLES3   ║
// ╚══════════════════════════════════════════════════════════╝

#if !defined(BOARD_T190) && !defined(BOARD_TDISPLAYS3) && !defined(BOARD_TDONGLES3)
#define BOARD_T190 1                       // default: Heltec Vision Master T190
#endif

#if defined(BOARD_T190)
// ── Heltec Vision Master T190: ST7789 170x320 SPI, battery with switch (no LoRa chip) ──
#define BOARD_NAME      "T190"
#define BOARD_HOST      "t190-2fa"
#define BOARD_KEYID     "t190"             // prefix of the OTA key
#define TFT_CS      39
#define TFT_DC      47
#define TFT_SDA     48                     // MOSI
#define TFT_SCL     38                     // SCK
#define TFT_RES     40
#define TFT_BL      17
#define PIN_VEXT     7
//   GPIO  0 = boot button  → NEXT,  GPIO 21 = other button → PREV (USB label on the left)
#define PIN_BOOT    0
#define PIN_OTHER   21
// Battery per Meshtastic variant.h: GPIO 46 HIGH = measurement on, GPIO 6 ADC, divider ~4.9
#define BAT_ADC_CTRL   46
#define BAT_ADC_PIN    6
#define BAT_MULT       (4.9f * 1.03f)
#define BAT_ATTEN      ADC_2_5db

#elif defined(BOARD_TDISPLAYS3)
// ── LilyGO T-Display S3: ST7789 170x320 8-bit parallel (i80), battery connector ──
#define BOARD_NAME      "T-Display S3"
#define BOARD_HOST      "tdisplay-2fa"
#define BOARD_KEYID     "tds3"
#define LCD_D0 39
#define LCD_D1 40
#define LCD_D2 41
#define LCD_D3 42
#define LCD_D4 45
#define LCD_D5 46
#define LCD_D6 47
#define LCD_D7 48
#define LCD_WR  8
#define LCD_RD  9
#define LCD_DC  7
#define LCD_CS  6
#define LCD_RST 5
#define LCD_POWER 15                       // HIGH, otherwise no image on battery power
#define TFT_BL  38                         // AW9364: 16 levels via pulses
//   GPIO 0 = BOOT → NEXT, GPIO 14 = upper button → PREV
#define PIN_BOOT    0
#define PIN_OTHER   14
// Battery per LilyGO example GetBatteryVoltage: GPIO 4, divider 1:2, no enable pin
#define BAT_ADC_CTRL   -1
#define BAT_ADC_PIN    4
#define BAT_MULT       2.0f
#define BAT_ATTEN      ADC_11db

#elif defined(BOARD_TDONGLES3)
// ── LilyGO T-Dongle S3: ST7735S 80x160 SPI (landscape 160x80), ONE button, no battery, no PSRAM ──
// Pins/orientation per the LilyGO T-Dongle S3 pinout
#define BOARD_NAME      "T-Dongle S3"
#define BOARD_HOST      "dongle-2fa"
#define BOARD_KEYID     "dongle"
#define BOARD_SMALL     1                  // compact layout 160x80
#define TFT_W           160
#define TFT_H           80
#define LCD_MOSI 3
#define LCD_SCK  5
#define LCD_CS   4
#define LCD_DC   2
#define LCD_RST  1
#define TFT_BL   38                        // LOW = on (LEDC inverted)
#define PIN_BOOT    0                      // only button: short = next, long = back
#define PIN_OTHER   -1
#define BAT_ADC_CTRL   -1
#define BAT_ADC_PIN    -1                  // no battery
#define BAT_MULT       1.0f
#define BAT_ATTEN      ADC_11db
#endif

#ifndef TFT_W
#define TFT_W  320                         // T190 / T-Display S3: 320x170 landscape
#define TFT_H  170
#endif
#ifndef BOARD_SMALL
#define BOARD_SMALL 0
#endif
