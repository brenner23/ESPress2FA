#pragma once

// Pins, buttons and battery per device: see BOARD.h (since 1.3.0)

// ── Display resolution (after rotation 1) ───────────────────
// Display size TFT_W / TFT_H: BOARD.h

// ── Colors ──────────────────────────────────────────────────
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_GREY    0x7BEF
#define C_DGREY   0x2104
#define C_CYAN    0x07FF
#define C_GREEN   0x07E0
#define C_YELLOW  0xFFE0
#define C_RED     0xF800

// ── Timezone ─────────────────────────────────────────────────
#define TZ_BERLIN  "CET-1CEST,M3.5.0,M10.5.0/3"

// ── Modes ────────────────────────────────────────────────────
#define MODE_CLOCK 0
#define MODE_TOTP  1

volatile int          currentMode      = MODE_CLOCK;
volatile unsigned long lastInteraction = 0;
const    unsigned long SCREENSAVER_TIMEOUT = 120000; // 2 minutes

// ── Clock ────────────────────────────────────────────────────
// 1 = blinking, -1 = steady on, 0 = completely off
int ClockBlinking = -1;

// ── External objects (created in main.cpp) ───────────────────
extern WebServer server;   // tft (frame buffer) + lcd live in main.cpp

// ── Brightness ───────────────────────────────────────────────
volatile int currentBrightness = 200;
volatile bool displayFlip = false;          // rotated 180 degrees (since 1.3.1), buttons are swapped too
// ── Language (1.5.0): 0 = English (default), 1 = German. Device texts via TR(en, de) ──
volatile uint8_t uiLang = 0;
#define TR(en, de) (uiLang ? (de) : (en))
volatile unsigned long holdSince = 0;     // button held since (for the progress bar, 1.4.5)
volatile bool holdActive  = false;
volatile bool holdShown   = false;          // bar was drawn -> redraw the screen afterwards
volatile int  flipHoldS   = 3;              // hold the button this long -> rotate display (2..10 s, web)
volatile bool flipPending = false;          // display task applies the rotation
volatile int  otaPercent  = -1;             // firmware update via web: -1 = none, 0..100 = running
String        otaKey;                       // per-device OTA key: "<id>-<12 random>", NVS, never in web/backup

// ── TOTP accounts ────────────────────────────────────────────
#define MAX_ACCOUNTS 30

struct TotpAccount {
    char name[64];
    char issuer[32];
    char secret[64];
    bool active;
};

TotpAccount          accounts[MAX_ACCOUNTS];
int                  accountCount   = 0;
volatile int         currentTotpIdx = 0;
volatile bool        totpForceRedraw = true;    // full TOTP page redraw (after clock / reordering)
volatile unsigned long bootScreenUntil = 0;     // keep the IP screen until then (1.1.6: without blocking)
volatile bool        clockForceRedraw = true;   // full clock redraw (after IP screen / mode change)
volatile unsigned long lastIdxChange  = 0;     // last entry is stored in NVS with a delay
int                  savedTotpIdx    = -1;

// ── Account helpers ──────────────────────────────────────────
int getActiveAccountCount() {
    int n = 0;
    for (int i = 0; i < accountCount; i++) if (accounts[i].active) n++;
    return n;
}

// Returns the real index (in accounts[]) of the n-th active account
int getActiveAccountRealIdx(int activePos) {
    int n = 0;
    for (int i = 0; i < accountCount; i++) {
        if (accounts[i].active) {
            if (n == activePos) return i;
            n++;
        }
    }
    return -1;
}

// ── Draw-Sync (Dual-Core) ────────────────────────────────────
volatile bool      _drawRequest = false;
SemaphoreHandle_t  _drawMutex = nullptr;
void requestDraw() { _drawRequest = true; }

// ── NVS – brightness ─────────────────────────────────────────
void loadSettings() {
    Preferences p;
    p.begin("bigclock", true);
    currentBrightness = p.getInt("bright", 200);
    displayFlip       = p.getBool("flip", false);
    flipHoldS         = constrain(p.getInt("fliphold", 3), 2, 10);
    uiLang            = p.getUChar("lang", 0) ? 1 : 0;
    currentTotpIdx    = p.getInt("lastIdx", 0);   // last shown entry (since 1.1.0)
    savedTotpIdx      = currentTotpIdx;
    otaKey            = p.getString("otakey", "");
    p.end();
    // Key = device id + 12 random chars, e.g. "tds3-k7m2..." (since 1.3.4); id prefix missing -> generate a new one
    if (!otaKey.startsWith(BOARD_KEYID "-") || otaKey.length() < 12) {
        static const char* A = "abcdefghjkmnpqrstuvwxyz23456789";   // without 0/O/1/l/i
        otaKey = BOARD_KEYID "-";
        for (int i = 0; i < 12; i++) otaKey += A[esp_random() % 31];
        Preferences w;
        w.begin("bigclock", false);
        w.putString("otakey", otaKey);
        w.end();
    }
}
// Remember the last entry - only 5 s after the last button press (saves flash wear)
void saveLastIdxIfDue() {
    if (savedTotpIdx == currentTotpIdx || millis() - lastIdxChange < 5000) return;
    Preferences p;
    p.begin("bigclock", false);
    p.putInt("lastIdx", currentTotpIdx);
    p.end();
    savedTotpIdx = currentTotpIdx;
}
void saveLang(uint8_t l) {
    Preferences p;
    p.begin("bigclock", false);
    p.putUChar("lang", l);
    p.end();
}
void saveFlipHold(int s) {
    Preferences p;
    p.begin("bigclock", false);
    p.putInt("fliphold", s);
    p.end();
}
void saveFlip(bool f) {
    Preferences p;
    p.begin("bigclock", false);
    p.putBool("flip", f);
    p.end();
}
void saveBrightness(int v) {
    Preferences p;
    p.begin("bigclock", false);
    p.putInt("bright", v);
    p.end();
}

// ── NVS – 2FA accounts ───────────────────────────────────────
void loadAccounts() {
    Preferences p;
    p.begin("totp2fa", true);
    String json;
    if (p.isKey("accb")) {                 // since 1.1.0: large lists as blob (NVS strings max. ~4000 bytes)
        size_t n = p.getBytesLength("accb");
        char* buf = (char*)malloc(n + 1);
        if (buf) { p.getBytes("accb", buf, n); buf[n] = 0; json = buf; free(buf); }
    } else {
        json = p.getString("accs", "[]");
    }
    p.end();

    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, json) != DeserializationError::Ok) return;
    JsonArray arr = doc.as<JsonArray>();

    accountCount = 0;
    for (JsonObject obj : arr) {
        if (accountCount >= MAX_ACCOUNTS) break;
        TotpAccount& a = accounts[accountCount++];
        strlcpy(a.name,   obj["n"] | "", sizeof(a.name));
        strlcpy(a.issuer, obj["i"] | "", sizeof(a.issuer));
        strlcpy(a.secret, obj["s"] | "", sizeof(a.secret));
        a.active = obj["a"] | true;
    }
}

void saveAccounts() {
    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < accountCount; i++) {
        JsonObject obj = arr.createNestedObject();
        obj["n"] = accounts[i].name;
        obj["i"] = accounts[i].issuer;
        obj["s"] = accounts[i].secret;
        obj["a"] = accounts[i].active;
    }
    String json;
    serializeJson(doc, json);

    Preferences p;
    p.begin("totp2fa", false);
    if (json.length() < 3800) {
        p.putString("accs", json);
        if (p.isKey("accb")) p.remove("accb");
    } else if (p.putBytes("accb", json.c_str(), json.length()) == json.length()) {
        p.remove("accs");
    }
    p.end();
}

// ── NVS – WiFi (since 1.1.0, entered via the setup page) ─────
String wifiSsid, wifiPass;
void loadWifi() {
    Preferences p;
    p.begin("wlan", true);
    wifiSsid = p.getString("ssid", "");
    wifiPass = p.getString("pass", "");
    p.end();
}
void saveWifi(const String& ssid, const String& pass) {
    Preferences p;
    p.begin("wlan", false);
    p.putString("ssid", ssid);
    p.putString("pass", pass);
    p.end();
}
