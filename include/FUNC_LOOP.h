#pragma once

// ╔══════════════════════════════════════════════════════════╗
// ║  FUNC_LOOP.h  –  dual core                               ║
// ║  Core 0: display  |  Core 1: web + logic                 ║
// ╚══════════════════════════════════════════════════════════╝

// Progress bar while a button is held (1.4.5): counts down to the rotate time, then turns green
void drawHoldOverlay() {
    if (!holdActive) return;
    const unsigned long held = millis() - holdSince;
    if (held < 800) return;                                   // short tap: no bar
    const unsigned long total = (unsigned long)flipHoldS * 1000UL;
    const int h = BOARD_SMALL ? 12 : 18, y = TFT_H - h;
    tft.fillRect(0, y, TFT_W, h, C_BLACK);
    tft.setFont(nullptr); tft.setTextSize(1);
    if (held >= total) {
        tft.fillRect(0, y + 1, TFT_W, h - 1, C_GREEN);
        tft.setTextColor(C_BLACK);
        tft.setCursor(4, y + (h - 8) / 2 + 1); tft.print(TR("release = rotate", "loslassen = drehen"));
    } else {
        const unsigned long rest = total - held;
        tft.fillRect(0, y + 1, (int)((long)TFT_W * rest / total), h - 1, C_CYAN);
        char t[24]; snprintf(t, sizeof(t), TR("Rotate in %lu s", "Drehen in %lu s"), (rest + 999) / 1000);
        tft.setTextColor(C_BLACK);
        tft.setCursor(4, y + (h - 8) / 2 + 1); tft.print(t);
    }
    holdShown = true;
}

void displayTask(void* param) {
    for (;;) {
        if (_drawRequest) {
            _drawRequest = false;
            if (xSemaphoreTake(_drawMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                if (flipPending) { flipPending = false; lcdSetFlip(displayFlip); }
                drawDisplay();
                drawHoldOverlay();
                lcdFlush();                     // since 1.1.0: send the finished frame to the panel in one go
                xSemaphoreGive(_drawMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void startDisplayTask() {
    _drawMutex = xSemaphoreCreateMutex();
    // Stack raised to 8192 – TOTP calculation (HMAC-SHA1) needs a bit more
    xTaskCreatePinnedToCore(displayTask, "display", 8192, nullptr, 1, nullptr, 0);
}

// Discovery: the tool sends "2FA-DISCOVER" via UDP broadcast to port 41900, every device replies
WiFiUDP discoveryUdp;
void pollDiscovery() {
    static bool started = false;
    if (!started) { started = discoveryUdp.begin(DISCOVERY_PORT); return; }
    const int n = discoveryUdp.parsePacket();
    if (n <= 0) return;
    char buf[32] = {0};
    discoveryUdp.read(buf, sizeof(buf) - 1);
    if (strncmp(buf, "2FA-DISCOVER", 12) != 0) return;
    char out[260];
    snprintf(out, sizeof(out),
             "{\"device\":\"2FA Clock " BOARD_NAME "\",\"board\":\"" BOARD_NAME "\",\"version\":\"" FW_VERSION "\","
             "\"mac\":\"%s\",\"ip\":\"%s\",\"host\":\"" BOARD_HOST "\",\"accounts\":%d,\"bat\":%d}",
             WiFi.macAddress().c_str(), WiFi.localIP().toString().c_str(), accountCount, (int)batPct);
    discoveryUdp.beginPacket(discoveryUdp.remoteIP(), discoveryUdp.remotePort());
    discoveryUdp.write((const uint8_t*)out, strlen(out));
    discoveryUdp.endPacket();
}

void loopDevice() {
    server.handleClient();
    pollDiscovery();
    pollSerial();                       // USB commands: backup / restore (espress2fa_tool.py)

    // ── Screensaver: TOTP → clock after 2 minutes ───────────
    if (currentMode == MODE_TOTP &&
        (millis() - lastInteraction > SCREENSAVER_TIMEOUT)) {
        currentMode = MODE_CLOCK;
        requestDraw();
    }

    // ── Periodic display refresh ─────────────────────────────
    static unsigned long lastTick = 0;
    if (millis() - lastTick >= 500) {
        lastTick = millis();
        requestDraw();
    }

    // Remember the last shown entry (delayed, since 1.1.0)
    saveLastIdxIfDue();
    batteryUpdate();                      // battery level every 10 s

    // ── Buttons (since 1.4.3): evaluated on RELEASE, hold time decides ──
    // Two buttons (T190, T-Display): short = browse (BOOT next, other back), long = rotate display
    // One button (Dongle): short = next, 0.7 s up to rotate time = back, long = rotate display
    // Rotate time configurable via web (2..10 s, default 3 s)
    // From the clock, the first short press returns to the last shown entry.
    {
        const unsigned long FLIP_MS = (unsigned long)flipHoldS * 1000UL;
        // 1.4.4: decision only on RELEASE: held >= rotate time -> rotate, otherwise browse.
        // Repeated tapping can never rotate. Press start is a separate flag (previously millis()|1 -> underflow bug).
        const int pins[2] = { PIN_BOOT, PIN_OTHER };
        static bool          isDown[2] = { false, false };
        static unsigned long downAt[2] = { 0, 0 };
        for (int k = 0; k < 2; k++) {
            if (pins[k] < 0) continue;
            const bool down = digitalRead(pins[k]) == LOW;
            if (down && !isDown[k]) { isDown[k] = true; downAt[k] = millis(); holdSince = downAt[k]; holdActive = true; }
            if (down && isDown[k]) {                        // refresh the bar while holding
                static unsigned long lastBar = 0;
                if (millis() - downAt[k] >= 800 && millis() - lastBar >= 100) { lastBar = millis(); requestDraw(); }
            }
            if (!down && isDown[k]) {
                isDown[k] = false;
                holdActive = false;
                if (holdShown) {                            // bar gone -> full redraw
                    holdShown = false;
                    totpForceRedraw = true;
                    clockForceRedraw = true;
                    tft.fillRect(0, TFT_H - (BOARD_SMALL ? 12 : 18), TFT_W, BOARD_SMALL ? 12 : 18, C_BLACK);
                    requestDraw();
                }
                const unsigned long held = millis() - downAt[k];
                if (held < 30) continue;                    // debounce
                lastInteraction = millis();
                if (held >= FLIP_MS) {                      // held long -> rotate display
                    displayFlip = !displayFlip;
                    saveFlip(displayFlip);
                    flipPending = true;
                    totpForceRedraw = true;
                    clockForceRedraw = true;
                } else if (currentMode == MODE_CLOCK) {
                    currentMode = MODE_TOTP;                // back to the last entry, not from the start
                    totpForceRedraw = true;
                } else {
                    const int total = getActiveAccountCount();
                    if (total > 0) {
#if PIN_OTHER < 0
                        const bool next = held < 700;       // Dongle: short = next, medium = back
#else
                        // BOOT = next, other = back; rotated: swap roles (top stays "back")
                        const bool next = (k == 0) != displayFlip;
#endif
                        currentTotpIdx = next ? (currentTotpIdx + 1) % total : (currentTotpIdx - 1 + total) % total;
                        lastIdxChange = millis();
                    }
                }
                requestDraw();
            }
        }
    }

    delay(10);
}
