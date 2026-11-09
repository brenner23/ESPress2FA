#pragma once

// ╔══════════════════════════════════════════════════════════╗
// ║  FUNC_BACKUP.h  –  backup / restore (1.1.0)              ║
// ║  Shared by web (/api/backup, /api/restore) and USB       ║
// ║  (serial JSON commands, tool tools/espress2fa_tool.py).        ║
// ║  A backup always contains EVERYTHING (accounts with      ║
// ║  secrets, settings, WiFi). Restore selectively:          ║
// ║  mode "all" | "nowifi" | "wifi".                         ║
// ╚══════════════════════════════════════════════════════════╝

void saveBrightness(int v);

String buildBackupJson() {
    DynamicJsonDocument doc(24576);
    doc["device"]  = "2FA Clock " BOARD_NAME;   // backups are interchangeable between all boards
    doc["version"] = FW_VERSION;
    doc["mac"]     = WiFi.macAddress();
    struct tm t;
    if (getLocalTime(&t, 50)) {
        char b[24];
        strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S", &t);
        doc["created"] = b;
    }
    JsonObject st = doc.createNestedObject("settings");
    st["bright"]  = (int)currentBrightness;
    st["lastIdx"] = (int)currentTotpIdx;
    JsonObject w = doc.createNestedObject("wifi");
    w["ssid"] = wifiSsid;
    w["pass"] = wifiPass;
    JsonArray arr = doc.createNestedArray("accounts");
    for (int i = 0; i < accountCount; i++) {
        JsonObject a = arr.createNestedObject();
        a["issuer"] = accounts[i].issuer;
        a["name"]   = accounts[i].name;
        a["secret"] = accounts[i].secret;
        a["active"] = accounts[i].active;
    }
    String out;
    serializeJson(doc, out);
    return out;
}

// Restores a backup. Returns nullptr = ok, otherwise an error text. restart = WiFi changed -> reboot required
const char* applyBackup(JsonVariantConst root, const String& mode, bool& restart, int& accN) {
    restart = false; accN = 0;
    const bool doAcc  = mode == "all" || mode == "nowifi";
    const bool doWifi = mode == "all" || mode == "wifi";
    if (!doAcc && !doWifi) return "mode: all | nowifi | wifi";
    if (!root["device"].as<String>().startsWith("2FA Clock")) return "not a 2FA Clock backup";

    if (doAcc) {
        JsonArrayConst arr = root["accounts"].as<JsonArrayConst>();
        if (arr.isNull()) return "accounts missing";
        const bool lock = _drawMutex != nullptr;   // the display task does not exist yet in the setup portal
        if (lock && xSemaphoreTake(_drawMutex, pdMS_TO_TICKS(1000)) != pdTRUE) return "display busy, try again";
        accountCount = 0;
        for (JsonObjectConst o : arr) {
            if (accountCount >= MAX_ACCOUNTS) break;
            TotpAccount& a = accounts[accountCount++];
            strlcpy(a.issuer, o["issuer"] | "", sizeof(a.issuer));
            strlcpy(a.name,   o["name"]   | "", sizeof(a.name));
            strlcpy(a.secret, o["secret"] | "", sizeof(a.secret));
            a.active = o["active"] | true;
        }
        accN = accountCount;
        saveAccounts();
        if (!root["settings"]["bright"].isNull()) {
            currentBrightness = constrain((int)root["settings"]["bright"], 10, 255);
            setBacklight(currentBrightness);
            saveBrightness(currentBrightness);
        }
        int li = root["settings"]["lastIdx"] | 0;
        currentTotpIdx  = (li >= 0 && li < getActiveAccountCount()) ? li : 0;
        lastIdxChange   = millis();
        totpForceRedraw = true;
        if (lock) xSemaphoreGive(_drawMutex);
        requestDraw();
    }
    if (doWifi) {
        String ssid = root["wifi"]["ssid"] | "";
        if (!ssid.length()) { if (!doAcc) return "no WiFi in this backup"; }
        else {
            String pass = root["wifi"]["pass"] | "";
            if (ssid != wifiSsid || pass != wifiPass) { saveWifi(ssid, pass); wifiSsid = ssid; wifiPass = pass; restart = true; }
        }
    }
    return nullptr;
}

// ── USB / serial commands (one JSON line, reply "@2FA {...}") ──
//  {"cmd":"info"}  {"cmd":"backup"}  {"cmd":"restore","mode":"nowifi","data":{...}}  {"cmd":"restart"}
static void serialReply(const String& json) { Serial.print("@2FA "); Serial.println(json); }

void handleSerialLine(const String& line) {
    DynamicJsonDocument doc(32768);
    if (deserializeJson(doc, line) != DeserializationError::Ok) { serialReply("{\"ok\":false,\"error\":\"bad JSON\"}"); return; }
    String cmd = doc["cmd"] | "";
    if (cmd == "info") {
        char b[260];
        snprintf(b, sizeof(b), "{\"ok\":true,\"device\":\"2FA Clock " BOARD_NAME "\",\"version\":\"%s\",\"ip\":\"%s\",\"ssid\":\"%s\",\"accounts\":%d,\"mac\":\"%s\"}",
                 FW_VERSION, WiFi.localIP().toString().c_str(), wifiSsid.c_str(), accountCount, WiFi.macAddress().c_str());
        serialReply(b);
    } else if (cmd == "getkey") {               // OTA key only via USB (device physically at hand)
        serialReply(String("{\"ok\":true,\"mac\":\"") + WiFi.macAddress() + "\",\"board\":\"" BOARD_NAME "\",\"key\":\"" + otaKey + "\"}");
    } else if (cmd == "backup") {
        serialReply("{\"ok\":true,\"data\":" + buildBackupJson() + "}");
    } else if (cmd == "restore") {
        bool restart; int n;
        const char* err = applyBackup(doc["data"], doc["mode"] | "nowifi", restart, n);
        if (err) { serialReply(String("{\"ok\":false,\"error\":\"") + err + "\"}"); return; }
        serialReply(String("{\"ok\":true,\"accounts\":") + n + ",\"restart\":" + (restart ? "true" : "false") + "}");
        if (restart) { delay(500); ESP.restart(); }
    } else if (cmd == "restart") {
        serialReply("{\"ok\":true}");
        delay(300); ESP.restart();
    } else {
        serialReply("{\"ok\":false,\"error\":\"cmd: info | getkey | backup | restore | restart\"}");
    }
}

void pollSerial() {
    static String line;
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            line.trim();
            if (line.startsWith("{")) handleSerialLine(line);
            line = "";
        } else if (line.length() < 30000) {
            line += c;
        }
    }
}
