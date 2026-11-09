#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>

#include "OCRA12pt7b.h"
#include "OCRA5pt7b.h"
#include "OCRA8pt7b.h"

#include "_CONFIG.h"
#include "BOARD.h"

// ── Display (frame buffer since 1.1.0, per board since 1.3.0) ──
// Everything is drawn into the frame buffer tft (320x170, landscape) first,
// lcdFlush() then sends the finished image to the panel in one go.
class FrameCanvas : public GFXcanvas16 {
public:
    FrameCanvas(uint16_t w, uint16_t h) : GFXcanvas16(w, h) {}
    volatile bool dirty = true;
    void drawPixel(int16_t x, int16_t y, uint16_t c) override { dirty = true; GFXcanvas16::drawPixel(x, y, c); }
    void fillScreen(uint16_t c) override { dirty = true; GFXcanvas16::fillScreen(c); }
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) override { dirty = true; GFXcanvas16::drawFastVLine(x, y, h, c); }
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c) override { dirty = true; GFXcanvas16::drawFastHLine(x, y, w, c); }
};
FrameCanvas tft(TFT_W, TFT_H);
WebServer   server(80);

#if defined(BOARD_T190)
// T190: hardware SPI (HSPI) on the display pins, 40 MHz (software SPI was very slow)
#include "HT_st7789spi.h"
SPIClass    lcdSpi(HSPI);
HT_ST7789   lcd(170, 320, &lcdSpi, TFT_CS, TFT_DC, TFT_RES);

void lcdBegin() {
    pinMode(PIN_VEXT, OUTPUT); digitalWrite(PIN_VEXT, LOW);
    delay(100);
    lcdSpi.begin(TFT_SCL, -1, TFT_SDA, TFT_CS);
    lcd.init(170, 320);
    lcd.setSPISpeed(40000000);
    lcd.setRotation(1);
}
void lcdFlush() {
    if (!tft.dirty || !tft.getBuffer()) return;
    tft.dirty = false;
    lcd.drawRGBBitmap(0, 0, tft.getBuffer(), 320, 170);
}
void lcdSetFlip(bool f) { lcd.setRotation(f ? 3 : 1); tft.dirty = true; }   // 180 degrees
void backlightInit() {
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
}
void setBacklight(int v) { ledcWrite(0, constrain(v, 0, 255)); }

#elif defined(BOARD_TDISPLAYS3)
// T-Display S3: 8-bit parallel bus (i80) with DMA, settings taken from the LilyGO factory example
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_heap_caps.h>
#include <freertos/semphr.h>
static esp_lcd_panel_handle_t s_panel = nullptr;
static SemaphoreHandle_t      s_done  = nullptr;
static uint16_t*              s_chunk = nullptr;
static const int CHUNK_LINES = 17;           // 10 transfers per frame

static bool IRAM_ATTR lcdTransDone(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t*, void*) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_done, &woken);
    return woken == pdTRUE;
}

void lcdBegin() {
    pinMode(LCD_POWER, OUTPUT); digitalWrite(LCD_POWER, HIGH);
    pinMode(LCD_RD, OUTPUT);    digitalWrite(LCD_RD, HIGH);
    s_chunk = (uint16_t*)heap_caps_malloc(320 * CHUNK_LINES * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_done  = xSemaphoreCreateBinary();

    esp_lcd_i80_bus_handle_t bus = nullptr;
    esp_lcd_i80_bus_config_t bc = {};
    bc.dc_gpio_num = LCD_DC;
    bc.wr_gpio_num = LCD_WR;
    bc.clk_src = LCD_CLK_SRC_PLL160M;
    const int data[8] = { LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7 };
    for (int i = 0; i < 8; i++) bc.data_gpio_nums[i] = data[i];
    bc.bus_width = 8;
    bc.max_transfer_bytes = 320 * CHUNK_LINES * 2;
    esp_lcd_new_i80_bus(&bc, &bus);

    esp_lcd_panel_io_handle_t io = nullptr;
    esp_lcd_panel_io_i80_config_t ic = {};
    ic.cs_gpio_num = LCD_CS;
    ic.pclk_hz = 16 * 1000 * 1000;           // as in the factory example (faster -> mosaic artifacts)
    ic.trans_queue_depth = 4;
    ic.on_color_trans_done = lcdTransDone;
    ic.lcd_cmd_bits = 8;
    ic.lcd_param_bits = 8;
    ic.dc_levels.dc_idle_level = 0;
    ic.dc_levels.dc_cmd_level = 0;
    ic.dc_levels.dc_dummy_level = 0;
    ic.dc_levels.dc_data_level = 1;
    esp_lcd_new_panel_io_i80(bus, &ic, &io);

    esp_lcd_panel_dev_config_t pc = {};
    pc.reset_gpio_num = LCD_RST;
    pc.color_space = ESP_LCD_COLOR_SPACE_RGB;
    pc.bits_per_pixel = 16;
    esp_lcd_new_panel_st7789(io, &pc, &s_panel);
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    esp_lcd_panel_invert_color(s_panel, true);
    esp_lcd_panel_swap_xy(s_panel, true);
    esp_lcd_panel_mirror(s_panel, false, true);   // USB on the left
    esp_lcd_panel_set_gap(s_panel, 0, 35);
    esp_lcd_panel_disp_on_off(s_panel, true);
}
void lcdFlush() {
    if (!tft.dirty || !tft.getBuffer() || !s_panel || !s_chunk) return;
    tft.dirty = false;
    const uint16_t* fb = tft.getBuffer();
    for (int y = 0; y < 170; y += CHUNK_LINES) {
        const int lines = (170 - y) < CHUNK_LINES ? (170 - y) : CHUNK_LINES;
        const uint16_t* src = fb + y * 320;
        const int n = lines * 320;
        for (int i = 0; i < n; i++) s_chunk[i] = __builtin_bswap16(src[i]);   // panel expects big-endian
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, 320, y + lines, s_chunk);
        xSemaphoreTake(s_done, pdMS_TO_TICKS(100));
    }
}
void lcdSetFlip(bool f) {                       // 180 degrees (factory example: USB right = mirror(true,false))
    if (!s_panel) return;
    if (f) esp_lcd_panel_mirror(s_panel, true, false);
    else   esp_lcd_panel_mirror(s_panel, false, true);
    esp_lcd_panel_set_gap(s_panel, 0, 35);
    tft.dirty = true;
}
// Backlight AW9364: 16 levels via pulses (as in the factory example); brightness 10..255 -> level 1..16
static uint8_t s_blLevel = 0;
void backlightInit() { pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, LOW); s_blLevel = 0; }
void setBacklight(int v) {
    const uint8_t steps = 16;
    uint8_t value = v <= 0 ? 0 : (uint8_t)constrain(1 + (v * 15) / 255, 1, 16);
    if (value == s_blLevel) return;
    if (value == 0) { digitalWrite(TFT_BL, 0); delay(3); s_blLevel = 0; return; }
    if (s_blLevel == 0) { digitalWrite(TFT_BL, 1); s_blLevel = steps; delayMicroseconds(30); }
    const int from = steps - s_blLevel, to = steps - value;
    const int num = (steps + to - from) % steps;
    for (int i = 0; i < num; i++) { digitalWrite(TFT_BL, 0); digitalWrite(TFT_BL, 1); }
    s_blLevel = value;
}

#elif defined(BOARD_TDONGLES3)
// T-Dongle S3: ST7735S via the ST7789 driver, SPI2 40 MHz, rotation/offset for the 160x80 panel
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <driver/spi_master.h>
#include <esp_heap_caps.h>
#include <freertos/semphr.h>
static esp_lcd_panel_handle_t s_panel = nullptr;
static SemaphoreHandle_t      s_done  = nullptr;
static uint16_t*              s_chunk = nullptr;
static const int CHUNK_LINES = 40;           // 2 transfers per frame

static bool IRAM_ATTR lcdTransDone(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t*, void*) {
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_done, &woken);
    return woken == pdTRUE;
}

void lcdBegin() {
    s_chunk = (uint16_t*)heap_caps_malloc(TFT_W * CHUNK_LINES * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_done  = xSemaphoreCreateBinary();
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = LCD_MOSI;
    buscfg.miso_io_num = -1;
    buscfg.sclk_io_num = LCD_SCK;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = TFT_W * CHUNK_LINES * 2;
    spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_handle_t io = nullptr;
    esp_lcd_panel_io_spi_config_t ic = {};
    ic.cs_gpio_num = LCD_CS;
    ic.dc_gpio_num = LCD_DC;
    ic.spi_mode = 0;
    ic.pclk_hz = 40 * 1000 * 1000;
    ic.trans_queue_depth = 4;
    ic.on_color_trans_done = lcdTransDone;
    ic.lcd_cmd_bits = 8;
    ic.lcd_param_bits = 8;
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI2_HOST, &ic, &io);

    esp_lcd_panel_dev_config_t pc = {};
    pc.reset_gpio_num = LCD_RST;
    pc.color_space = ESP_LCD_COLOR_SPACE_RGB;
    pc.bits_per_pixel = 16;
    esp_lcd_new_panel_st7789(io, &pc, &s_panel);
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    esp_lcd_panel_invert_color(s_panel, true);
    esp_lcd_panel_swap_xy(s_panel, true);
    esp_lcd_panel_mirror(s_panel, true, false);
    esp_lcd_panel_set_gap(s_panel, 1, 26);        // ST7735S landscape
    esp_lcd_panel_disp_on_off(s_panel, true);
}
void lcdFlush() {
    if (!tft.dirty || !tft.getBuffer() || !s_panel || !s_chunk) return;
    tft.dirty = false;
    const uint16_t* fb = tft.getBuffer();
    for (int y = 0; y < TFT_H; y += CHUNK_LINES) {
        const int lines = (TFT_H - y) < CHUNK_LINES ? (TFT_H - y) : CHUNK_LINES;
        const uint16_t* src = fb + y * TFT_W;
        const int n = lines * TFT_W;
        for (int i = 0; i < n; i++) s_chunk[i] = __builtin_bswap16(src[i]);
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, TFT_W, y + lines, s_chunk);
        xSemaphoreTake(s_done, pdMS_TO_TICKS(100));
    }
}
void lcdSetFlip(bool f) {
    if (!s_panel) return;
    if (f) esp_lcd_panel_mirror(s_panel, false, true);
    else   esp_lcd_panel_mirror(s_panel, true, false);
    esp_lcd_panel_set_gap(s_panel, 1, 26);
    tft.dirty = true;
}
// Backlight: LEDC, active LOW (255 = off)
void backlightInit() { ledcSetup(0, 5000, 8); ledcAttachPin(TFT_BL, 0); }
void setBacklight(int v) { ledcWrite(0, 255 - constrain(v, 0, 255)); }
#endif

// Order matters: TOTP before CLOCK (CLOCK calls drawTOTPScreen)
#include "_VARIABLES.h"
#include "FUNC_BATTERY.h"     // battery level (1.2.0)
#include "FUNC_WEBCONTENT.h"
#include "FUNC_BACKUP.h"      // backup/restore (web + USB)
#include "FUNC_TOTP.h"        // before FUNC_WEB.h – generateTOTP() is needed there
#include "FUNC_WEB.h"
#include "FUNC_CLOCK.h"
#include "FUNC_PORTAL.h"
#include "FUNC_SETUP.h"
#include "FUNC_LOOP.h"

void setup() { setupDevice(); }
void loop()  { loopDevice();  }
