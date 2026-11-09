#pragma once

// ╔══════════════════════════════════════════════════════════╗
// ║  FUNC_CLOCK.h  –  clock screensaver + display dispatcher ║
// ╚══════════════════════════════════════════════════════════╝

// 1.4.0: sizes per display (Dongle 160x80 compact)
#if BOARD_SMALL
#define BOX_PAD   4
#define CLK_SIZE  2
#define CLK_GAP   10
#define CLK_DOT   4
#define DATE_SIZE 1
#define DATE_OFS  2
#else
#define BOX_PAD  12
#define CLK_SIZE  3
#define CLK_GAP   22
#define CLK_DOT   9
#define DATE_SIZE 3
#define DATE_OFS  15
#endif
#define C_BOX    0x2104   // dark grey – background box

void drawDisplay() {

    // ── Detect mode change → clear screen ───────────────────
    static int lastMode = -1;
    if (otaPercent >= 0) {                         // firmware update running (since 1.3.2)
        static int shown = -2;
        if (shown == otaPercent) return;
        shown = otaPercent;
        tft.fillScreen(C_BLACK);
        tft.setFont(nullptr);
        tft.setTextSize(BOARD_SMALL ? 1 : 3); tft.setTextColor(C_CYAN);
        tft.setCursor(BOARD_SMALL ? 2 : 8, BOARD_SMALL ? 4 : 20); tft.print("Firmware update");
        tft.setTextSize(BOARD_SMALL ? 1 : 2); tft.setTextColor(C_WHITE);
        tft.setCursor(BOARD_SMALL ? 2 : 8, BOARD_SMALL ? 24 : 70); tft.printf(TR("%d %%  - do not power off", "%d %%  - nicht ausschalten"), otaPercent);
        const int by = BOARD_SMALL ? 50 : 110, bh = BOARD_SMALL ? 14 : 24;
        tft.drawRect(BOARD_SMALL ? 2 : 8, by, TFT_W - (BOARD_SMALL ? 4 : 16), bh, C_GREY);
        tft.fillRect(BOARD_SMALL ? 4 : 10, by + 2, (TFT_W - (BOARD_SMALL ? 8 : 20)) * otaPercent / 100, bh - 4, C_GREEN);
        lastMode = -1;
        return;
    }
    if (bootScreenUntil) {                         // keep the boot screen with the IP for now
        if ((long)(millis() - bootScreenUntil) < 0) return;
        bootScreenUntil = 0;
        lastMode = -1;
        totpForceRedraw = true;
    }
    if (currentMode != lastMode) {
        clockForceRedraw = true;                   // draw the clock right after a mode change, not at the next minute
        tft.fillScreen(C_BLACK);
        lastMode = currentMode;
    }

    // ── Delegate TOTP mode to FUNC_TOTP.h ───────────────────
    if (currentMode == MODE_TOTP) {
        drawTOTPScreen();
        return;
    }

    // ════════════════════════════════════════════════════════
    // From here on: clock screensaver (MODE_CLOCK)
    // ════════════════════════════════════════════════════════

    static int  lastH = -1, lastM = -1, lastD = -1;
    if (clockForceRedraw) { clockForceRedraw = false; lastH = -1; }

    struct tm t;
    if (!getLocalTime(&t, 200)) return;

    int h = t.tm_hour, m = t.tm_min, s = t.tm_sec, d = t.tm_mday;

    bool colon;
    if      (ClockBlinking ==  1) colon = (s % 2 == 0);
    else if (ClockBlinking == -1) colon = true;
    else                          colon = false;

    static bool lastColon = false;
    bool timeChanged  = (h != lastH || m != lastM || d != lastD);
    bool colonChanged = (colon != lastColon);
    if (!timeChanged && !colonChanged) return;

    tft.setFont(&OCRA12pt7b);
    tft.setTextSize(CLK_SIZE);

    // ── Measure digit size ───────────────────────────────────
    int16_t gx, gy; uint16_t gw, gh;
    tft.getTextBounds("00", 0, 0, &gx, &gy, &gw, &gh);

    int boxW = gw + BOX_PAD * 2;
    int boxH = gh + BOX_PAD * 2;

    // Date font for the total height calculation
    tft.setFont(&OCRA5pt7b);
    tft.setTextSize(1);
    int16_t dx, dy; uint16_t dw, dh;
    tft.getTextBounds("2026-01-01 MON", 0, 0, &dx, &dy, &dw, &dh);

    int dateGap = 6;
    int totalH  = boxH + dateGap + dh;
    int startY  = (TFT_H - totalH) / 2;
    int boxY    = startY;
    int dateY   = startY + boxH + dateGap + dh;

    int gap    = CLK_GAP;
    int totalW = boxW + gap + boxW;
    int startX = (TFT_W - totalW) / 2;
    int box1X  = startX;
    int box2X  = startX + boxW + gap;

    // ── Full redraw when the time changes ────────────────────
    if (timeChanged) {
        tft.fillScreen(C_BLACK);

        tft.fillRoundRect(box1X, boxY, boxW, boxH, 6, C_BOX);
        tft.fillRoundRect(box2X, boxY, boxW, boxH, 6, C_BOX);

        tft.setFont(&OCRA12pt7b);
        tft.setTextSize(CLK_SIZE);
        tft.setTextColor(C_WHITE);

        char hStr[3], mStr[3];
        snprintf(hStr, sizeof(hStr), "%02d", h);
        snprintf(mStr, sizeof(mStr), "%02d", m);

        int textY = boxY + BOX_PAD + gh;
        tft.setCursor(box1X + BOX_PAD - gx, textY); tft.print(hStr);
        tft.setCursor(box2X + BOX_PAD - gx, textY); tft.print(mStr);

        // ── Date ──────────────────────────────────────────────
        const char* wdays[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
        char dateBuf[20];
        snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d %s",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                 wdays[t.tm_wday]);

        tft.setFont(&OCRA5pt7b);
        tft.setTextSize(DATE_SIZE);
        tft.setTextColor(C_GREY);

        int16_t dbx, dby; uint16_t dbw, dbh;
        tft.getTextBounds(dateBuf, 0, 0, &dbx, &dby, &dbw, &dbh);
        tft.setCursor((TFT_W - dbw) / 2 - dbx, dateY + DATE_OFS);
        tft.print(dateBuf);

        // Battery top right (1.2.0), red below 15 %
        if (batPct >= 0) {
            char bb[8];
            snprintf(bb, sizeof(bb), "%d%%", (int)batPct);
            tft.setFont(nullptr);
            tft.setTextSize(2);
            tft.setTextColor(batPct <= 15 ? C_RED : batPct <= 30 ? C_YELLOW : C_GREY);
            int16_t bx, by; uint16_t bw, bh;
            tft.getTextBounds(bb, 0, 0, &bx, &by, &bw, &bh);
            const int x = TFT_W - bw - 30, y = 6;
            tft.setCursor(x, y); tft.print(bb);
            tft.drawRect(TFT_W - 26, y, 20, 14, C_GREY);          // small battery icon
            tft.fillRect(TFT_W - 6, y + 4, 3, 6, C_GREY);
            tft.fillRect(TFT_W - 24, y + 2, 16 * batPct / 100, 10, batPct <= 15 ? C_RED : batPct <= 30 ? C_YELLOW : C_GREEN);
        }

        lastH = h; lastM = m; lastD = d;
    }

    // ── Colon (blinking) ─────────────────────────────────────
    if (timeChanged || colonChanged) {
        int dot   = CLK_DOT;
        int colX  = box1X + boxW + (gap - dot) / 2;
        int textY = boxY + BOX_PAD + gh;
        int dot1Y = textY - gh * 2 / 3;
        int dot2Y = textY - gh / 4;

        uint16_t c = colon ? C_GREY : C_BLACK;
        tft.fillRect(colX, dot1Y, dot, dot, c);
        tft.fillRect(colX, dot2Y, dot, dot, c);
        lastColon = colon;
    }
}
