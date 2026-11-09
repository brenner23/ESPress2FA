#pragma once

// ╔══════════════════════════════════════════════════════════╗
// ║  FUNC_WEB.h  –  web server routes                        ║
// ║  GET  /api/status                                        ║
// ║  GET  /api/accounts           (list, without secrets)    ║
// ║  POST /api/accounts/import    (add accounts)             ║
// ║  POST /api/accounts/reorder   (order)                    ║
// ║  POST /api/accounts/N/toggle  (active on/off)            ║
// ║  DEL  /api/accounts/N         (delete account)           ║
// ║  POST /api/brightness/live    (preview)                  ║
// ║  POST /api/brightness/save    (NVS)                      ║
// ╚══════════════════════════════════════════════════════════╝

// ── Escape a JSON string (no DynamicJsonDocument needed) ───
String jsonEsc(const char* s) {
    String out;
    for (int i = 0; s[i]; i++) {
        char c = s[i];
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    return out;
}

void setupWebServer() {

    // ── Web-UI ───────────────────────────────────────────────
    server.on("/", HTTP_GET, []() {
        server.send_P(200, "text/html", PAGE_ROOT);
    });

    // ── Status ───────────────────────────────────────────────
    server.on("/api/status", HTTP_GET, []() {
        struct tm t;
        char timeBuf[10] = "--:--:--";
        char dateBuf[20] = "---";
        if (getLocalTime(&t, 100)) {
            const char* wd[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
            snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
                     t.tm_hour, t.tm_min, t.tm_sec);
            snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d %s",
                     t.tm_year+1900, t.tm_mon+1, t.tm_mday, wd[t.tm_wday]);
        }
        int activeAccs = 0;
        for (int i = 0; i < accountCount; i++) if (accounts[i].active) activeAccs++;
        char json[420];
        snprintf(json, sizeof(json),
            "{\"time\":\"%s\",\"date\":\"%s\","
            "\"ssid\":\"%s\",\"ip\":\"%s\","
            "\"bright\":%d,\"total\":%d,\"active\":%d,\"version\":\"" FW_VERSION "\",\"board\":\"" BOARD_NAME "\",\"lang\":\"%s\",\"flip\":%s,\"flip_s\":%d,\"has_bat\":%s,"
            "\"bat_mv\":%d,\"bat_pct\":%d}",
            timeBuf, dateBuf,
            WiFi.status() == WL_CONNECTED ? WiFi.SSID().c_str() : "offline",
            WiFi.localIP().toString().c_str(),
            (int)currentBrightness,
            accountCount,
            activeAccs,
            uiLang ? "de" : "en",
            displayFlip ? "true" : "false",
            (int)flipHoldS,
            BAT_ADC_PIN >= 0 ? "true" : "false",
            (int)batMv, (int)batPct);
        server.send(200, "application/json", json);
    });

    // ── Account list (no secrets, no TOTP calculation) ──────
    // Codes are ONLY calculated by the display task (core 0).
    // Concurrent access to the TOTP/SHA1 lib from core 1
    // would cause race conditions → leave codes out here.
    server.on("/api/accounts", HTTP_GET, []() {
        String json;
        json.reserve(accountCount * 120 + 4);
        json = "[";
        for (int i = 0; i < accountCount; i++) {
            if (i > 0) json += ",";
            json += "{\"id\":";        json += i;
            json += ",\"name\":\"";   json += jsonEsc(accounts[i].name);
            json += "\",\"issuer\":\""; json += jsonEsc(accounts[i].issuer);
            json += "\",\"active\":";  json += accounts[i].active ? "true" : "false";
            json += "}";
        }
        json += "]";
        server.send(200, "application/json", json);
    });

    // ── Import (append accounts, with secret) ────────────────
    server.on("/api/accounts/import", HTTP_POST, []() {
        String body = server.arg("plain");
        DynamicJsonDocument doc(8192);
        if (deserializeJson(doc, body) != DeserializationError::Ok) {
            server.send(400, "text/plain", "Bad JSON");
            return;
        }
        JsonArray arr = doc.as<JsonArray>();
        int added = 0;
        for (JsonObject obj : arr) {
            if (accountCount >= MAX_ACCOUNTS) break;
            TotpAccount& a = accounts[accountCount++];
            strlcpy(a.name,   obj["name"]   | "", sizeof(a.name));
            strlcpy(a.issuer, obj["issuer"] | "", sizeof(a.issuer));
            strlcpy(a.secret, obj["secret"] | "", sizeof(a.secret));
            a.active = obj["active"] | true;
            added++;
        }
        saveAccounts();
        requestDraw();

        char resp[48];
        snprintf(resp, sizeof(resp),
                 "{\"added\":%d,\"total\":%d}", added, accountCount);
        server.send(200, "application/json", resp);
    });

    // ── Brightness: preview ──────────────────────────────────
    server.on("/api/brightness/live", HTTP_POST, []() {
        if (server.hasArg("v")) {
            int v = constrain(server.arg("v").toInt(), 10, 255);
            currentBrightness = v;
            setBacklight(v);
            server.send(200, "text/plain", "OK");
        } else {
            server.send(400, "text/plain", "missing v");
        }
    });

    // ── Firmware update via web (OTA, since 1.3.2) ──────────
    // New firmware goes into the second app partition; it is only activated after a successful finish.
    // Accounts / WiFi / settings (NVS) are kept.
    static bool otaDenied = false;
    server.on("/update", HTTP_POST, []() {
        if (otaDenied) { otaDenied = false; server.send(403, "text/plain", "Wrong OTA key"); return; }
        const bool ok = !Update.hasError();
        server.send(ok ? 200 : 500, "text/plain", ok ? "OK - restarting" : String("Error: ") + Update.errorString());
        delay(800);
        if (ok) ESP.restart();
        otaPercent = -1;
        requestDraw();
    }, []() {
        HTTPUpload& up = server.upload();
        static size_t total = 0;
        if (up.status == UPLOAD_FILE_START) {
            otaDenied = server.header("X-OTA-Key") != otaKey;
            if (otaDenied) { Serial.println("[OTA] wrong key - rejected"); return; }
            total = server.header("X-FW-Size").toInt();
            Serial.printf("[OTA] %s (%u Byte)\n", up.filename.c_str(), (unsigned)total);
            otaPercent = 0; requestDraw();
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
        } else if (otaDenied) {
            return;                                   // no key -> write nothing
        } else if (up.status == UPLOAD_FILE_WRITE) {
            if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
            if (total) { otaPercent = (int)(100ULL * Update.progress() / total); requestDraw(); }
        } else if (up.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { Serial.printf("[OTA] done, %u bytes\n", up.totalSize); otaPercent = 100; requestDraw(); }
            else Update.printError(Serial);
        } else if (up.status == UPLOAD_FILE_ABORTED) {
            Update.abort(); otaPercent = -1; requestDraw();
        }
    });

    // ── Check key ("Is this the right device?") ────────────
    server.on("/api/otacheck", HTTP_POST, []() {
        const bool ok = server.header("X-OTA-Key") == otaKey;
        server.send(ok ? 200 : 403, "application/json",
                    String("{\"ok\":") + (ok ? "true" : "false") + ",\"mac\":\"" + WiFi.macAddress() + "\"}");
    });

    // ── Language EN/DE (web UI + device texts) ──────────────
    server.on("/api/lang", HTTP_POST, []() {
        uiLang = server.arg("v") == "de" ? 1 : 0;
        saveLang(uiLang);
        totpForceRedraw = true;
        clockForceRedraw = true;
        tft.dirty = true;
        requestDraw();
        server.send(200, "text/plain", "OK");
    });

    // ── Hold time for "rotate display" via button (2..10 s) ─
    server.on("/api/fliphold", HTTP_POST, []() {
        flipHoldS = constrain(server.arg("s").toInt(), 2, 10);
        saveFlipHold(flipHoldS);
        server.send(200, "text/plain", "OK");
    });

    // ── Rotate display 180 degrees (immediate + saved) ───────
    server.on("/api/flip", HTTP_POST, []() {
        displayFlip = server.arg("v") == "1";
        saveFlip(displayFlip);
        flipPending = true;
        totpForceRedraw = true;
        clockForceRedraw = true;
        tft.dirty = true;
        requestDraw();
        server.send(200, "text/plain", "OK");
    });

    // ── Brightness: save to NVS ──────────────────────────────
    server.on("/api/brightness/save", HTTP_POST, []() {
        saveBrightness(currentBrightness);
        server.send(200, "text/plain", "OK");
    });

    // ── Single TOTP code for the modal ───────────────────────
    // GET /api/code?i=N
    // Mutex-protected: prevents a race with the display task (core 0)
    server.on("/api/code", HTTP_GET, []() {
        if (!server.hasArg("i")) {
            server.send(400, "text/plain", "missing i"); return;
        }
        int idx = server.arg("i").toInt();
        if (idx < 0 || idx >= accountCount) {
            server.send(404, "text/plain", "not found"); return;
        }
        // 1.4.0: code dialog open in the web UI -> display shows exactly this entry and stays there
        if (accounts[idx].active) {
            int pos = 0;
            for (int i = 0; i < idx; i++) if (accounts[i].active) pos++;
            if (currentMode != MODE_TOTP || currentTotpIdx != pos) {
                currentTotpIdx = pos;
                currentMode = MODE_TOTP;
                totpForceRedraw = true;
                lastIdxChange = millis();
                requestDraw();
            }
            lastInteraction = millis();            // while the dialog is open: no screensaver
        }
        time_t now; time(&now);
        int remaining = 30 - (int)(now % 30);

        // Take the mutex so the display task does not compute TOTP at the same time
        String code = "------";
        if (xSemaphoreTake(_drawMutex, pdMS_TO_TICKS(300)) == pdTRUE) {
            code = generateTOTP(accounts[idx].secret);
            xSemaphoreGive(_drawMutex);
        }

        String json = "{\"code\":\"" + code +
                      "\",\"remaining\":" + remaining +
                      ",\"issuer\":\"" + jsonEsc(accounts[idx].issuer) +
                      "\",\"name\":\"" + jsonEsc(accounts[idx].name) + "\"}";
        server.send(200, "application/json", json);
    });

    // ── Change order ─────────────────────────────────────────
    // Body: [2, 0, 3, 1, ...]  → new order as an index array
    server.on("/api/accounts/reorder", HTTP_POST, []() {
        String body = server.arg("plain");
        DynamicJsonDocument doc(1024);
        if (deserializeJson(doc, body) != DeserializationError::Ok) {
            server.send(400, "text/plain", "Bad JSON"); return;
        }
        JsonArray arr = doc.as<JsonArray>();
        const int selReal = getActiveAccountRealIdx(currentTotpIdx);   // keep the displayed entry
        static TotpAccount tmp[MAX_ACCOUNTS];
        int newSel = -1;
        int newCount = 0;
        for (JsonVariant v : arr) {
            int idx = v.as<int>();
            if (idx >= 0 && idx < accountCount && newCount < MAX_ACCOUNTS) {
                if (idx == selReal) newSel = newCount;
                tmp[newCount++] = accounts[idx];
            }
        }
        for (int i = 0; i < newCount; i++) accounts[i] = tmp[i];
        accountCount = newCount;
        currentTotpIdx = 0;
        if (newSel >= 0) {                       // position among the active accounts
            int pos = 0;
            for (int i = 0; i < newSel; i++) if (accounts[i].active) pos++;
            if (accounts[newSel].active) currentTotpIdx = pos;
        }
        lastIdxChange   = millis();
        totpForceRedraw = true;
        saveAccounts();
        requestDraw();
        server.send(200, "text/plain", "OK");
    });

    // ── Download backup (always everything) ─────────────────
    server.on("/api/backup", HTTP_GET, []() {
        char fn[64];
        struct tm t;
        if (getLocalTime(&t, 50)) strftime(fn, sizeof(fn), "2FA-T190_Sicherung_%Y-%m-%d_%H%M.json", &t);
        else strlcpy(fn, "2FA-T190_Sicherung.json", sizeof(fn));
        server.sendHeader("Content-Disposition", String("attachment; filename=\"") + fn + "\"");
        server.send(200, "application/json", buildBackupJson());
    });

    // ── Restore backup: ?mode=all|nowifi|wifi, body = JSON file ──
    server.on("/api/restore", HTTP_POST, []() {
        DynamicJsonDocument doc(24576);
        if (deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
            server.send(400, "text/plain", "File is not valid JSON"); return;
        }
        bool restart; int n;
        const char* err = applyBackup(doc.as<JsonVariantConst>(), server.arg("mode"), restart, n);
        if (err) { server.send(400, "text/plain", err); return; }
        server.send(200, "application/json",
                    String("{\"accounts\":") + n + ",\"restart\":" + (restart ? "true" : "false") + "}");
        if (restart) { delay(800); ESP.restart(); }
    });

    // ── Reset WiFi: clear credentials, reboot into the hotspot ──
    server.on("/api/wifi/reset", HTTP_POST, []() {
        saveWifi("", "");
        server.send(200, "text/plain", "OK");
        delay(800);
        ESP.restart();
    });

    // ── Dynamic routes: toggle + delete + edit ──────────────
    // /api/accounts/N/toggle  (POST)
    // /api/accounts/N         (DELETE)
    server.onNotFound([]() {
        String uri    = server.uri();
        HTTPMethod m  = server.method();

        // POST /api/accounts/N/toggle
        if (m == HTTP_POST && uri.startsWith("/api/accounts/") &&
            uri.endsWith("/toggle")) {
            String numStr = uri.substring(strlen("/api/accounts/"),
                                          uri.length() - strlen("/toggle"));
            int idx = numStr.toInt();
            if (idx >= 0 && idx < accountCount) {
                accounts[idx].active = !accounts[idx].active;
                int active = getActiveAccountCount();
                if (currentTotpIdx >= active) currentTotpIdx = 0;
                totpForceRedraw = true;
                saveAccounts();
                requestDraw();
                server.send(200, "text/plain", "OK");
            } else {
                server.send(404, "text/plain", "Not found");
            }
            return;
        }

        // POST /api/accounts/N/edit  Body {"issuer":"Google","name":"My Account"} (secret is kept)
        if (m == HTTP_POST && uri.startsWith("/api/accounts/") && uri.endsWith("/edit")) {
            int idx = uri.substring(strlen("/api/accounts/"), uri.length() - strlen("/edit")).toInt();
            DynamicJsonDocument doc(512);
            if (idx < 0 || idx >= accountCount ||
                deserializeJson(doc, server.arg("plain")) != DeserializationError::Ok) {
                server.send(400, "text/plain", "Bad request"); return;
            }
            if (xSemaphoreTake(_drawMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                if (doc["issuer"].is<const char*>()) strlcpy(accounts[idx].issuer, doc["issuer"], sizeof(accounts[idx].issuer));
                if (doc["name"].is<const char*>())   strlcpy(accounts[idx].name,   doc["name"],   sizeof(accounts[idx].name));
                totpForceRedraw = true;
                xSemaphoreGive(_drawMutex);
            }
            saveAccounts();
            requestDraw();
            server.send(200, "text/plain", "OK");
            return;
        }

        // DELETE /api/accounts/N
        if (m == HTTP_DELETE && uri.startsWith("/api/accounts/")) {
            int idx = uri.substring(strlen("/api/accounts/")).toInt();
            if (idx >= 0 && idx < accountCount) {
                // Remove account (shift the array forward)
                for (int i = idx; i < accountCount - 1; i++)
                    accounts[i] = accounts[i + 1];
                accountCount--;
                if (currentTotpIdx >= accountCount && currentTotpIdx > 0)
                    currentTotpIdx = accountCount - 1;
                totpForceRedraw = true;
                saveAccounts();
                requestDraw();
                server.send(200, "text/plain", "OK");
            } else {
                server.send(404, "text/plain", "Not found");
            }
            return;
        }

        server.send(404, "text/plain", "Not found");
    });

    const char* hdrs[] = { "X-FW-Size", "X-OTA-Key" };
    server.collectHeaders(hdrs, 2);
    server.begin();
}
