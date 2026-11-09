#pragma once

// ╔══════════════════════════════════════════════════════════╗
// ║  FUNC_PORTAL.h  –  WiFi setup (since 1.1.0)              ║
// ║  Like NerdMiner: no / wrong WiFi -> own hotspot          ║
// ║  "2FA-T190-XXXX", page http://192.168.4.1/               ║
// ║  Pick network, enter password, save, reboot.             ║
// ╚══════════════════════════════════════════════════════════╝

void dispBoot(const char* l1, const char* l2);

static const char PORTAL_PAGE[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="%LANG%"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>2FA Clock – WiFi</title>
<style>
*{box-sizing:border-box}body{background:#000;color:#ccc;font-family:'Courier New',monospace;margin:0;padding:18px}
.c{max-width:420px;margin:0 auto;background:#0a0a0a;border:1px solid #0ff;border-radius:8px;padding:18px}
h1{color:#0ff;font-size:18px;margin:0 0 12px}label{display:block;font-size:11px;color:#666;margin:12px 0 4px;text-transform:uppercase}
input,select{width:100%;padding:9px;background:#111;border:1px solid #333;color:#fff;border-radius:4px;font-family:inherit;font-size:14px}
button{margin-top:16px;width:100%;padding:11px;background:#0a0;color:#000;border:0;border-radius:4px;font-weight:bold;font-size:14px;cursor:pointer}
.h{font-size:11px;color:#555;margin-top:10px;line-height:1.5}
</style></head><body><div class="c">
<h1>🔐 2FA Clock – %T_HEAD%</h1>
<form method="POST" action="/save">
<label>%T_NET%</label><select id="sel" onchange="document.getElementById('ssid').value=this.value">%OPTIONS%</select>
<label>SSID</label><input id="ssid" name="ssid" value="%SSID%" required>
<label>%T_PASS%</label><input name="pass" type="password" autocomplete="off">
<button type="submit">💾 %T_SAVE%</button>
</form>
<div class="h">%T_DEV% %MAC% · Firmware %VER%<br>%T_KEEP%</div>
</div></body></html>)rawhtml";

static String htmlEsc(const String& s) {
    String o;
    for (char c : s) {
        if (c == '<') o += "&lt;"; else if (c == '>') o += "&gt;";
        else if (c == '"') o += "&quot;"; else if (c == '&') o += "&amp;"; else o += c;
    }
    return o;
}

void runWifiPortal() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apName[24];
    #if defined(BOARD_TDONGLES3)
    snprintf(apName, sizeof(apName), "2FA-DNG-%02X%02X", mac[4], mac[5]);
#elif defined(BOARD_TDISPLAYS3)
    snprintf(apName, sizeof(apName), "2FA-TDS3-%02X%02X", mac[4], mac[5]);
#else
    snprintf(apName, sizeof(apName), "2FA-T190-%02X%02X", mac[4], mac[5]);
#endif

    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP_STA);
    int n = WiFi.scanNetworks();
    String opts = String("<option value=''>– ") + TR("please choose", "bitte wählen") + " –</option>";
    for (int i = 0; i < n; i++) {
        String s = WiFi.SSID(i);
        if (!s.length() || opts.indexOf("'" + htmlEsc(s) + "'") >= 0) continue;
        opts += "<option value='" + htmlEsc(s) + "'>" + htmlEsc(s) + " (" + WiFi.RSSI(i) + " dBm)</option>";
    }
    WiFi.softAP(apName, PORTAL_AP_PASS);

    char l2[120];
    snprintf(l2, sizeof(l2), TR("Hotspot: %s\nPassword: %s\nhttp://192.168.4.1/", "Hotspot: %s\nPasswort: %s\nhttp://192.168.4.1/"), apName, PORTAL_AP_PASS);   // Dongle: lines fall back to small font
    dispBoot(TR("WiFi setup", "WLAN einrichten"), l2);
    Serial.printf("[Portal] AP %s, http://192.168.4.1/\n", apName);

    WebServer portal(80);
    portal.on("/", HTTP_GET, [&]() {
        String page = FPSTR(PORTAL_PAGE);
        page.replace("%OPTIONS%", opts);
        page.replace("%SSID%", htmlEsc(wifiSsid));
        page.replace("%MAC%", WiFi.macAddress());
        page.replace("%VER%", FW_VERSION);
        page.replace("%LANG%", TR("en", "de"));
        page.replace("%T_HEAD%", TR("WiFi setup", "WLAN einrichten"));
        page.replace("%T_NET%", TR("Network", "Netzwerk"));
        page.replace("%T_PASS%", TR("Password", "Passwort"));
        page.replace("%T_SAVE%", TR("Save &amp; restart", "Speichern &amp; Neustart"));
        page.replace("%T_DEV%", TR("Device", "Gerät"));
        page.replace("%T_KEEP%", TR("Your 2FA accounts are kept.", "Die 2FA-Konten bleiben erhalten."));
        portal.send(200, "text/html", page);
    });
    portal.on("/save", HTTP_POST, [&]() {
        String ssid = portal.arg("ssid"); ssid.trim();
        if (!ssid.length()) { portal.send(400, "text/plain", TR("SSID missing", "SSID fehlt")); return; }
        saveWifi(ssid, portal.arg("pass"));
        portal.send(200, "text/html",
            String("<meta charset='UTF-8'><body style='background:#000;color:#0f0;font-family:monospace;padding:20px'>")
            + TR("Saved – restarting ...", "Gespeichert – Neustart ...") + "</body>");
        dispBoot(TR("WiFi saved", "WLAN gespeichert"), TR("Restarting ...", "Neustart ..."));
        delay(1500);
        ESP.restart();
    });
    portal.onNotFound([&]() {                       // phone captive-portal checks -> setup page
        portal.sendHeader("Location", "http://192.168.4.1/");
        portal.send(302, "text/plain", "");
    });
    portal.begin();

    const unsigned long t0 = millis();
    for (;;) {
        portal.handleClient();
        pollSerial();                              // also in the portal: WiFi can be restored via USB backup
        // Reboot after 5 min without input if a WiFi is already stored (router was only temporarily gone)
        if (wifiSsid.length() && millis() - t0 > PORTAL_TIMEOUT) ESP.restart();
        delay(5);
    }
}
