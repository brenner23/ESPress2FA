#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

// --- USB ---
#define USB_VID 0x303a
#define USB_PID 0x1001

// --- UART ---
static const uint8_t TX = 43;
static const uint8_t RX = 44;

// --- I2C (if a sensor is connected) ---
static const uint8_t SDA = 4;
static const uint8_t SCL = 3;

// --- SPI (default bus, not the TFT) ---
static const uint8_t SS   = 8;
static const uint8_t MOSI = 10;
static const uint8_t MISO = 11;
static const uint8_t SCK  = 9;

// --- TFT Display (ST7789, Software-SPI) ---
static const uint8_t TFT_SCL = 38;   // SPI Clock
static const uint8_t TFT_SDA = 48;   // SPI MOSI
static const uint8_t TFT_RES = 40;   // Reset
static const uint8_t TFT_DC  = 47;   // Data/Command
static const uint8_t TFT_CS  = 39;   // Chip Select
static const uint8_t TFT_BL  = 45;   // Backlight

// --- Power & User Interface ---
static const uint8_t Vext        = 7;   // display supply (see BOARD.h)
static const uint8_t LED_BUILTIN = 18;  // Onboard LED
#define BUILTIN_LED LED_BUILTIN
#define LED_BUILTIN LED_BUILTIN

// --- Analoge Pins ---
static const uint8_t A0  = 1;
static const uint8_t A1  = 2;
static const uint8_t A2  = 3;
static const uint8_t A3  = 4;
static const uint8_t A4  = 5;
static const uint8_t A5  = 6;
static const uint8_t A6  = 7;
static const uint8_t A7  = 8;
static const uint8_t A8  = 9;
static const uint8_t A9  = 10;
static const uint8_t A10 = 11;

// --- LoRa Pins (Dummy – kein LoRa auf HT-VMT190 ohne Suffix) ---
// needed so that Heltec library internals (gpio-board.c) compile
#define RADIO_NSS   8
#define RADIO_RESET 12
#define RADIO_BUSY  13
#define RADIO_DIO1  14

#endif /* Pins_Arduino_h */
