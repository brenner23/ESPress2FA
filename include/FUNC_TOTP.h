#pragma once
#include "TOTP.h"

// ╔══════════════════════════════════════════════════════════╗
// ║  FUNC_TOTP.h  –  2FA code display                        ║
// ╚══════════════════════════════════════════════════════════╝

// ── Base32 → Bytes ───────────────────────────────────────────
int base32Decode(const char* src, uint8_t* dst, int dstMax) {
    static const char* alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int buf = 0, bits = 0, cnt = 0;
    for (int i = 0; src[i] && cnt < dstMax; i++) {
        char c = toupper((unsigned char)src[i]);
        if (c == ' ' || c == '=' || c == '\r' || c == '\n') continue;
        const char* p = strchr(alpha, c);
        if (!p) continue;
        buf = (buf << 5) | (int)(p - alpha);
        bits += 5;
        if (bits >= 8) { bits -= 8; dst[cnt++] = (uint8_t)(buf >> bits) & 0xFF; }
    }
    return cnt;
}

// ── Calculate TOTP code ──────────────────────────────────────
String generateTOTP(const char* secretB32) {
    uint8_t key[64];
    int keyLen = base32Decode(secretB32, key, sizeof(key));
    if (keyLen == 0) return "------";
    TOTP totp(key, keyLen);
    time_t now; time(&now);
    char* code = totp.getCodeFromSteps((long)now / 30);
    return code ? String(code) : String("------");
}

#if BOARD_SMALL
// ── TOTP display compact (T-Dongle 160x80, since 1.4.0) ──────
// Header: issuer + "3/23", line 2: account (small), middle: code OCR-A 17 pt (26 px), bottom: time-left bar
#include "OCRA17pt7b.h"
static String fitText(const String& s, int maxChars) {
    return (int)s.length() <= maxChars ? s : s.substring(0, maxChars - 1) + ".";
}
void drawTOTPScreen() {
    int total = getActiveAccountCount();
    if (total == 0) {
        static bool emptyDrawn = false;
        if (emptyDrawn && !totpForceRedraw) return;
        totpForceRedraw = false;
        tft.fillScreen(C_BLACK);
        tft.setFont(nullptr); tft.setTextSize(1);
        tft.setTextColor(C_GREY);  tft.setCursor(4, 18); tft.print(TR("No 2FA accounts", "Keine 2FA-Konten"));
        tft.setTextColor(C_CYAN);  tft.setCursor(4, 38); tft.print("http://" + WiFi.localIP().toString());
        tft.setTextColor(C_DGREY); tft.setCursor(4, 58); tft.print(TR("Import tab: scan QR", "Import-Tab: QR scannen"));
        emptyDrawn = true;
        return;
    }
    if (currentTotpIdx < 0 || currentTotpIdx >= total) currentTotpIdx = 0;
    int ri = getActiveAccountRealIdx(currentTotpIdx);
    if (ri < 0) { currentTotpIdx = 0; return; }
    TotpAccount& acc = accounts[ri];

    time_t now; time(&now);
    int step = 30 - (int)(now % 30);
    String code = generateTOTP(acc.secret);

    static int lastIdx = -1, lastStep = -1;
    static String lastCode;
    const bool full = totpForceRedraw || currentTotpIdx != lastIdx || code != lastCode;
    if (!full && step == lastStep) return;
    totpForceRedraw = false;
    const uint16_t col = step <= 5 ? C_RED : step <= 10 ? C_YELLOW : C_GREEN;

    if (full) {
        tft.fillScreen(C_BLACK);
        tft.fillRect(0, 0, TFT_W, 14, C_DGREY);
        char idx[12];
        snprintf(idx, sizeof(idx), "%d/%d", currentTotpIdx + 1, total);
        tft.setFont(nullptr); tft.setTextSize(1);
        tft.setTextColor(C_GREY);
        tft.setCursor(TFT_W - 6 * strlen(idx) - 2, 3); tft.print(idx);
        const int room = (TFT_W - 6 * strlen(idx) - 8) / 6;
        tft.setTextColor(C_CYAN);
        tft.setCursor(2, 3); tft.print(fitText(acc.issuer[0] ? acc.issuer : acc.name, room));
        if (acc.issuer[0]) {
            tft.setTextColor(C_GREY);
            tft.setCursor(2, 17); tft.print(fitText(acc.name, TFT_W / 6));
        }
        // Code: two groups of three with a small gap (OCR-A is wide)
        tft.setFont(&OCRA17pt7b);
        tft.setTextColor(C_WHITE);
        const String a = code.substring(0, 3), b = code.substring(3);
        int16_t bx, by; uint16_t bw, bh;
        tft.getTextBounds(a.c_str(), 0, 0, &bx, &by, &bw, &bh);
        const int gap = 8, adv = 24 * 3;                         // 3 digits at 24 px each
        const int x0 = (TFT_W - (adv * 2 + gap)) / 2;
        tft.setCursor(x0, 56); tft.print(a);
        tft.setCursor(x0 + adv + gap, 56); tft.print(b);
        lastIdx = currentTotpIdx; lastCode = code;
    }
    // Time-left bar + seconds
    const int barX = 2, barY = 68, barW = TFT_W - 30, barH = 8;
    tft.fillRect(barX, barY, barW, barH, C_DGREY);
    tft.fillRect(barX, barY, barW * step / 30, barH, col);
    char sec[6]; snprintf(sec, sizeof(sec), "%2ds", step);
    tft.fillRect(TFT_W - 26, barY - 1, 26, 10, C_BLACK);
    tft.setFont(nullptr); tft.setTextSize(1); tft.setTextColor(col);
    tft.setCursor(TFT_W - 24, barY); tft.print(sec);
    lastStep = step;
}
#else
// ── TOTP Display ─────────────────────────────────────────────
void drawTOTPScreen() {
    int total = getActiveAccountCount();

    // ── No accounts ──────────────────────────────────────────
    if (total == 0) {
        static bool emptyDrawn = false;
        if (!emptyDrawn || totpForceRedraw) {
            totpForceRedraw = false;
            tft.fillScreen(C_BLACK);
            tft.setFont(nullptr);
            tft.setTextSize(2);
            tft.setTextColor(C_GREY, C_BLACK);
            tft.setCursor(20, 45); tft.println(TR("No 2FA accounts", "Keine 2FA Konten"));
            tft.setTextSize(1);
            tft.setTextColor(C_DGREY, C_BLACK);
            tft.setCursor(20, 78);  tft.println(TR("Open the web interface:", "Web-Interface aufrufen:"));
            tft.setCursor(20, 94);  tft.println("http://");
            tft.setTextColor(C_CYAN, C_BLACK);
            tft.setCursor(58, 94);  tft.println(WiFi.localIP().toString());
            tft.setTextColor(C_DGREY, C_BLACK);
            tft.setCursor(20, 118); tft.println(TR("Import tab  ->  scan QR", "Import-Tab  ->  QR scannen"));
            emptyDrawn = true;
        }
        return;
    }
    // Redraw on the next appearance
    // (controlled by the account-change detection below)

    // ── Keep index in valid range ────────────────────────────
    if (currentTotpIdx < 0 || currentTotpIdx >= total) currentTotpIdx = 0;
    int ri = getActiveAccountRealIdx(currentTotpIdx);
    if (ri < 0) { currentTotpIdx = 0; return; }
    TotpAccount& acc = accounts[ri];

    // ── Time info ────────────────────────────────────────────
    time_t now; time(&now);
    int step = 30 - (int)(now % 30);          // seconds until the code changes

    String code     = generateTOTP(acc.secret);
    String codeDisp = code.substring(0, 3) + " " + code.substring(3); // "123 456"

    // Color depending on urgency
    uint16_t codeColor = (step <= 5) ? C_RED :
                         (step <= 10) ? C_YELLOW : C_WHITE;
    uint16_t barColor  = (step <= 5) ? C_RED :
                         (step <= 10) ? C_YELLOW : C_GREEN;

    // ── Change-Detection ─────────────────────────────────────
    static int    lastTotpIdx = -1;
    static String lastCode    = "";
    static int    lastStep    = -1;

    bool idxChanged  = (currentTotpIdx != lastTotpIdx) || totpForceRedraw;   // force: after clock / web change
    totpForceRedraw  = false;
    bool codeChanged = (code != lastCode);
    bool stepChanged = (step != lastStep);

    if (!idxChanged && !codeChanged && !stepChanged) return;

    // ════════════════════════════════════════════════════════
    // On account change: redraw the whole screen
    // ════════════════════════════════════════════════════════
    if (idxChanged) {
        tft.fillScreen(C_BLACK);

        // ── Header-Leiste ────────────────────────────────────
        tft.fillRect(0, 0, TFT_W, 34, C_DGREY);

        // Issuer (or name if there is no issuer)
        const char* header = acc.issuer[0] ? acc.issuer : acc.name;
        // Long names (e.g. "Google My Long Account") one size smaller so "N / Total" stays clear
        tft.setFont(&OCRA5pt7b);
        tft.setTextSize(2);
        int16_t hx0, hy0; uint16_t hw0, hh0;
        tft.getTextBounds(header, 0, 0, &hx0, &hy0, &hw0, &hh0);
        const bool small = hw0 > TFT_W - 70;
        if (small) tft.setTextSize(1);
        tft.setTextColor(C_CYAN, C_DGREY);
        tft.setCursor(8, small ? 22 : 26);
        tft.print(header);

        // "N / Total" top right
        char idxBuf[12];
        snprintf(idxBuf, sizeof(idxBuf), "%d / %d", currentTotpIdx + 1, total);
        tft.setFont(nullptr);
        tft.setTextSize(1);
        tft.setTextColor(C_GREY, C_DGREY);
        int16_t bx, by; uint16_t bw, bh;
        tft.getTextBounds(idxBuf, 0, 0, &bx, &by, &bw, &bh);
        tft.setCursor(TFT_W - bw - 8, 14);
        tft.print(idxBuf);

        // ── Account name (small line) ────────────────────────
        tft.setFont(nullptr);
        tft.setTextSize(1);
        tft.setTextColor(C_GREY, C_BLACK);
        // Shorten if too long
        String nameStr = String(acc.name);
        if (nameStr.length() > 44) nameStr = nameStr.substring(0, 42) + "..";
        tft.setCursor(8, 47);
        tft.print(nameStr);

        // ── Navigation hint at the very bottom ───────────────
        tft.setTextColor(C_DGREY, C_BLACK);
        tft.setCursor(8, 160);
        tft.print("<  BACK");
        int16_t hx, hy; uint16_t hw, hh;
        tft.getTextBounds("NEXT  >", 0, 0, &hx, &hy, &hw, &hh);
        tft.setCursor(TFT_W - hw - 8, 160);
        tft.print("NEXT  >");

        lastTotpIdx = currentTotpIdx;
        lastCode    = "";    // force code redraw
        lastStep    = -1;    // force bar redraw
        codeChanged = true;
        stepChanged = true;
    }

    // ════════════════════════════════════════════════════════
    // Code area (only on a real code change, always white)
    // ════════════════════════════════════════════════════════
    if (codeChanged) {
        tft.fillRect(0, 56, TFT_W, 76, C_BLACK);

        tft.setFont(&OCRA12pt7b);
        tft.setTextSize(2);
        tft.setTextColor(C_WHITE, C_BLACK);   // always white, color only in the bar

        int16_t gx, gy; uint16_t gw, gh;
        tft.getTextBounds(codeDisp.c_str(), 0, 0, &gx, &gy, &gw, &gh);
        int cx = (TFT_W - gw) / 2 - gx;
        tft.setCursor(cx, 56 + gh + 6);
        tft.print(codeDisp);

        lastCode = code;
    }

    // ════════════════════════════════════════════════════════
    // Progress bar (every second, color: green → yellow → red)
    // ════════════════════════════════════════════════════════
    if (stepChanged) {
        const int barY = 140, barH = 11, barX = 8, barW = TFT_W - 56;
        tft.fillRect(barX, barY, barW, barH, C_DGREY);
        int fill = (int)((float)step / 30.0f * barW);
        tft.fillRect(barX, barY, fill, barH, barColor);

        // Seconds on the right (colored too)
        char secBuf[6];
        snprintf(secBuf, sizeof(secBuf), "%2ds", step);
        tft.setFont(nullptr);
        tft.setTextSize(1);
        tft.fillRect(TFT_W - 46, barY - 1, 44, 14, C_BLACK);
        tft.setTextColor(barColor, C_BLACK);
        tft.setCursor(TFT_W - 44, barY + 2);
        tft.print(secBuf);

        lastStep = step;
    }
}
#endif
