#pragma once

// ╔══════════════════════════════════════════════════════════╗
// ║  _CONFIG.h  –  general settings                          ║
// ╚══════════════════════════════════════════════════════════╝

// ── Firmware version (bump on every change, see CHANGELOG.md) ──
#define FW_VERSION  "1.5.0"

// ── WiFi ─────────────────────────────────────────────────────
// No credentials in the code: WiFi is entered on the setup page (own hotspot,
// see FUNC_PORTAL.h) and stored in NVS (namespace "wlan").
#define PORTAL_AP_PASS   "2fa-setup"     // password of the setup hotspot (min. 8 characters)
#define PORTAL_TIMEOUT   300000UL        // 5 min without input -> restart (retry the stored WiFi)
#define WIFI_CONNECT_MS  20000UL
#define DISCOVERY_PORT   41900           // network discovery: "2FA-DISCOVER" -> device answers with JSON

// ── Time (NTP) ───────────────────────────────────────────────
#define NTP_SERVER  "pool.ntp.org"
