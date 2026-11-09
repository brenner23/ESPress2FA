#pragma once

void startDisplayTask();

// Boot screen: line 1 large, line 2 small (multi-line with '\n'), then straight to the panel
void dispBoot(const char* l1, const char* l2 = "") {
    tft.fillScreen(C_BLACK);
    tft.setFont(nullptr);
    // 1.1.3: larger font for readability - title 3x, info lines 2x
    tft.setTextSize(BOARD_SMALL ? 2 : 3); tft.setTextColor(C_CYAN);
    tft.setCursor(BOARD_SMALL ? 2 : 8, BOARD_SMALL ? 2 : 12); tft.print(l1);
    tft.setTextColor(C_WHITE);
    int y = BOARD_SMALL ? 22 : 54;
    String s(l2);
    int a = 0;
    while (a <= (int)s.length()) {
        int b = s.indexOf('\n', a);
        if (b < 0) b = s.length();
        String line = s.substring(a, b);
        tft.setTextSize(line.length() * 12 <= TFT_W - 16 ? 2 : 1);   // 2x = 12 px per character
        tft.setCursor(BOARD_SMALL ? 2 : 8, y); tft.print(line);
        y += BOARD_SMALL ? 14 : 24; a = b + 1;
    }
    tft.setTextSize(1); tft.setTextColor(C_GREY);
    tft.setCursor(BOARD_SMALL ? 2 : 8, TFT_H - 10); tft.print("2FA Clock " BOARD_NAME " v" FW_VERSION);
    lcdFlush();
}

void setupDevice() {
    Serial.begin(115200);

    pinMode(PIN_BOOT,  INPUT_PULLUP);
    if (PIN_OTHER >= 0) pinMode(PIN_OTHER, INPUT_PULLUP);

    loadSettings();
    batteryInit();
    batteryUpdate(true);

    // Display + backlight per board (main.cpp)
    lcdBegin();
    tft.setTextWrap(false);                    // 1.4.2: never wrap automatically (clip instead)
    if (displayFlip) lcdSetFlip(true);
    backlightInit();
    setBacklight(currentBrightness);
    Serial.printf("[2FA] Firmware %s, frame buffer %s\n", FW_VERSION, tft.getBuffer() ? "OK" : "MISSING");
    dispBoot("2FA Clock", TR("Starting...", "Starte..."));
    delay(300);

    // ── Load 2FA accounts ─────────────────────────────────────
    loadAccounts();

    // ── WiFi: stored network, otherwise setup hotspot ────────
    loadWifi();
    // Force the setup portal: hold both buttons (Dongle: the single button) during boot
    const bool forcePortal = digitalRead(PIN_BOOT) == LOW && (PIN_OTHER < 0 || digitalRead(PIN_OTHER) == LOW);
    if (!wifiSsid.length() || forcePortal) runWifiPortal();

    dispBoot("2FA Clock", (String(TR("Connecting WiFi ...\n", "Verbinde WLAN ...\n")) + wifiSsid).c_str());
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(BOARD_HOST);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
    const unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_CONNECT_MS) delay(250);
    if (WiFi.status() != WL_CONNECTED) runWifiPortal();   // does not return (reboots)

    // ── NTP + Berlin time zone ───────────────────────────────
    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com");
    setenv("TZ", TZ_BERLIN, 1);
    tzset();

    struct tm t; int w = 0;
    while (!getLocalTime(&t, 500) && w++ < 20) delay(200);
    String msg = WiFi.localIP().toString() + " | " + String(accountCount) + " accounts";
    Serial.printf("[2FA] %s\n", msg.c_str());
#if BOARD_SMALL
    {   // show the IP for 5 s (Dongle compact)
        tft.fillScreen(C_BLACK);
        tft.setFont(nullptr); tft.setTextSize(1); tft.setTextColor(C_GREY);
        tft.setCursor(2, 2); tft.print("2FA Clock - IP:");
        tft.setFont(&OCRA8pt7b); tft.setTextColor(C_CYAN);       // 1.4.2: 8 pt (IP used to wrap)
        String ip = WiFi.localIP().toString();
        int16_t bx, by; uint16_t bw, bh;
        tft.getTextBounds(ip.c_str(), 0, 0, &bx, &by, &bw, &bh);
        if (bw > TFT_W - 4) { tft.setFont(nullptr); tft.setTextSize(1); tft.getTextBounds(ip.c_str(), 0, 0, &bx, &by, &bw, &bh); }
        tft.setCursor((TFT_W - bw) / 2 - bx, 34); tft.print(ip);
        tft.setFont(nullptr); tft.setTextSize(1); tft.setTextColor(C_GREY);
        tft.setCursor(2, 52); tft.print(String(accountCount) + TR(" accounts  v", " Konten  v") + String(FW_VERSION));
        tft.setTextColor(C_DGREY);
        tft.setCursor(2, 68); tft.print("OTA " + otaKey);
        lcdFlush();
    }
#else
    {   // show the IP large for 5 s (easy to copy down)
        tft.fillScreen(C_BLACK);
        tft.setFont(nullptr);
        tft.setTextSize(2); tft.setTextColor(C_GREY);
        tft.setCursor(10, 22); tft.print("2FA Clock - IP:");
        tft.setFont(&OCRA12pt7b); tft.setTextSize(1); tft.setTextColor(C_CYAN);
        String ip = WiFi.localIP().toString();
        int16_t bx, by; uint16_t bw, bh;
        tft.getTextBounds(ip.c_str(), 0, 0, &bx, &by, &bw, &bh);
        if (bw * 2 + 20 <= TFT_W) tft.setTextSize(2);
        tft.getTextBounds(ip.c_str(), 0, 0, &bx, &by, &bw, &bh);
        tft.setCursor((TFT_W - bw) / 2 - bx, 95);
        tft.print(ip);
        tft.setFont(nullptr); tft.setTextSize(1); tft.setTextColor(C_GREY);
        String l2 = "http://" + ip + "/   |   " + String(accountCount) + TR(" accounts   |   v", " Konten   |   v") + String(FW_VERSION);
        tft.setCursor(10, 140); tft.print(l2);
        tft.setTextColor(C_DGREY);
        tft.setCursor(10, 155); tft.print(String(TR("OTA key: ", "OTA-Schluessel: ")) + otaKey);
        lcdFlush();
    }
#endif
    bootScreenUntil = millis() + 5000;     // show the IP for 5 s, web + buttons are already active

    setupWebServer();
    startDisplayTask();
    requestDraw();
}
