# 🔐 ESPress2FA

🇬🇧 [English](README.md) · 🇩🇪 **Deutsch**

Eine **2FA-/TOTP-Code-Anzeige für ESP32-S3-Boards** – deine Zwei-Faktor-Codes (Google, GitHub, Discord, Webhoster …)
auf einem kleinen Display neben der Tastatur, statt jedes Mal das Handy zu suchen. Eine Firmware, drei Boards,
Weboberfläche, WLAN-Einrichtung per Handy, Sicherung als JSON, Updates übers Netz.

🔗 **Projektseite:** https://github.com/brenner23/ESPress2FA

> ## ⚠️ Zuerst lesen – Sicherheit
> Das Gerät zeigt **gültige Anmelde-Codes**. Behandle es wie einen Schlüssel.
> - **Nur im eigenen, vertrauenswürdigen Heimnetz betreiben.** Nie in offenen oder geteilten Netzen
>   (Freifunk, Hotel, Café, Firmen-/Uni-WLAN, Gästenetze): Die Weboberfläche hat **kein Login** –
>   **jeder im selben Netz kann sie öffnen und alle deine 2FA-Codes lesen**.
> - Eine **Sicherungsdatei enthält alle 2FA-Geheimnisse** (und dein WLAN-Passwort). Aufbewahren wie den Export eines Passwort-Tresors.
> - Ein 2FA-Code allein meldet niemanden an – zusammen mit einem gestohlenen Passwort aber schon.
> - Hobbyprojekt ohne Gewähr. Behalte die Authenticator-App (oder die Wiederherstellungscodes) als Rückfallebene.

## Unterstützte Boards

| Board | Display | Tasten | Akku | PlatformIO-Umgebung |
|---|---|---|---|---|
| **Heltec Vision Master T190** (ESP32-S3, 16 MB, PSRAM) | ST7789 1,9" 320×170, SPI | 2 | ✅ Spannung + % | `heltec_vision_master_t190` |
| **LilyGO T-Display S3** (ESP32-S3, 16 MB, PSRAM) | ST7789 1,9" 320×170, 8-Bit parallel | 2 | ✅ Spannung + % | `tdisplays3` |
| **LilyGO T-Dongle S3** (USB-Stick, ESP32-S3, 16 MB) | ST7735 0,96" 160×80, SPI | 1 | – | `tdongles3` |

Alle drei sind getestet. Sicherungen sind zwischen den Boards austauschbar.

## Funktionen

- **TOTP-Codes** (RFC 6238, 6 Ziffern, 30 s) mit Restzeit-Balken (grün → gelb → rot), bis zu 30 Konten
- **Import aus dem Google Authenticator**: Export-QR-Code (`otpauth-migration://…`) oder einzelne `otpauth://totp/…`-Links –
  einfach mit einem USB-QR-Leser ins Importfeld scannen oder einfügen
- **Weboberfläche** (English / Deutsch): Status, Konten, Import, Einstellungen
  - Konten per **Ziehen sortieren**, Aussteller/Konto **umbenennen** (der Schlüssel bleibt unberührt), an/aus, löschen
  - **Klick auf ein Konto im Browser → das Gerät zeigt genau diesen Code** und bleibt dort, solange das Code-Fenster offen ist
- **Uhr als Bildschirmschoner** nach 2 Minuten; der **erste Tastendruck führt zum zuletzt angezeigten Konto zurück**,
  auch nach einem Neustart
- **Tasten** – entschieden beim Loslassen:
  - Zwei-Tasten-Boards: kurz = weiter / zurück, **halten = Display um 180° drehen**
  - eine Taste (T-Dongle): kurz = weiter, mittel = zurück, **halten = drehen**
  - Haltezeit im Web einstellbar (2–10 s, Standard 3 s); eine **Ablauf-Leiste** zeigt, wann Loslassen dreht
- **180° drehen** auch in den Einstellungen (die Tasten tauschen ihre Rollen mit)
- **Akkustand** (T190, T-Display S3) im Web-Status und auf der Uhr
- **Schnelles Display**: alles wird in einen Bildpuffer gezeichnet und in einem Rutsch übertragen (Hardware-SPI bzw. 8-Bit-Bus mit DMA)
- **WLAN-Einrichtung** wie beim NerdMiner: kein WLAN gespeichert (oder nicht erreichbar) → eigener Hotspot
  `2FA-T190-xxxx` / `2FA-TDS3-xxxx` / `2FA-DNG-xxxx`, Passwort `2fa-setup`, Seite `http://192.168.4.1/`.
  Taste(n) beim Start gedrückt halten erzwingt das Portal. **Keine Zugangsdaten im Quelltext.**
- **Sichern & Wiederherstellen** als JSON – Download im Web; Einspielen wie bei der Fritzbox: **alles / ohne WLAN / nur WLAN**
- **Firmware-Update übers Netz (OTA)** – im Web oder mit dem PC-Tool. Jedes Gerät erzeugt beim ersten Start einen
  **eigenen zufälligen OTA-Schlüssel** (`t190-…`, `tds3-…`, `dongle-…`); er steht im Startbild und ist per USB lesbar,
  nie übers Web. Falscher Schlüssel → abgelehnt.
- **Suche im Netz**: Das PC-Tool findet alle ESPress2FA-Geräte mit einem Rundruf
- Gerätesprache **English / Deutsch** (Display-Texte und Web, Umschalter in den Einstellungen)

## Bauen & Flashen

[PlatformIO](https://platformio.org/) (VS-Code-Erweiterung oder Kommandozeile):

```bash
pio run -e heltec_vision_master_t190 -t upload     # Heltec Vision Master T190
pio run -e tdisplays3 -t upload                    # LilyGO T-Display S3
pio run -e tdongles3 -t upload                     # LilyGO T-Dongle S3
```

Board **direkt** am PC anschließen (manche USB-3-Hubs erkennen ESP32-S3-Boards nicht).
Nach dem ersten Flashen per USB gehen Updates auch übers Netz (siehe unten).

## Erster Start

1. Das Gerät öffnet seinen Hotspot (`2FA-…-xxxx`, Passwort `2fa-setup`). Mit dem Handy verbinden; die Einrichtungsseite
   öffnet sich (sonst `http://192.168.4.1/`). Netz wählen, Passwort eingeben, speichern.
2. Das Gerät startet neu, verbindet sich und zeigt 5 Sekunden lang seine **IP-Adresse** (dazu den OTA-Schlüssel).
3. `http://<IP>/` öffnen → **Import** → den Export-QR-Code des Google Authenticators scannen (oder `otpauth://`-Links
   einfügen) → Vorschau → importieren.
4. **Einstellungen** → *Sicherung herunterladen* und die Datei sicher aufbewahren.

## PC-Tool (`tools/espress2fa_tool.py`)

Python 3 + `pip install pyserial`.

```bash
python tools/espress2fa_tool.py                      # Menü (USB)
python tools/espress2fa_tool.py info                 # USB: Gerät, Version, IP, Konten
python tools/espress2fa_tool.py backup [datei.json]  # USB: komplette Sicherung
python tools/espress2fa_tool.py restore datei.json --mode all|nowifi|wifi
python tools/espress2fa_tool.py getkey               # USB: OTA-Schlüssel einmal lesen (landet in tools/ota_keys.json)
python tools/espress2fa_tool.py find                 # Netz: alle Geräte auflisten
python tools/espress2fa_tool.py ota firmware.bin --ip 192.168.x.y   # Update übers Netz
```

Das Tool findet das Gerät am USB selbst und öffnet den Port **ohne Neustart** des Boards.
`tools/ota_keys.json` enthält deine OTA-Schlüssel – sie steht in `.gitignore`, nicht weitergeben.

## Desktop-Programm (`desktop/`) – dieselbe Weboberfläche am PC

**ESPress2FA Desktop** zeigt genau die Weboberfläche des Geräts (Konten, Ziehen, Umbenennen, Import, Sicherung,
EN/DE) in einem Fenster am PC – z. B. als einzelne EXE auf dem USB-Stick. Die Seite kommt direkt aus dem
Firmware-Quelltext, Gerät und PC sehen also immer gleich aus.

- Alle Konten liegen in **einer verschlüsselten Tresor-Datei** (`ESPress2FA.vault` neben dem Programm):
  **AES-256-GCM**, Schlüssel aus deinem **Master-Passwort** per **scrypt** (absichtlich langsam gegen Raten).
  ⚠️ Das Master-Passwort ist **nicht wiederherstellbar** – Gerät bzw. Wiederherstellungscodes der Dienste als Rückfallebene behalten.
- Lokaler Server **nur auf 127.0.0.1** (Port exklusiv), **keinerlei Verbindungen nach außen**.
- Entsperr-Seite, 🔒-Knopf, **Auto-Sperre nach 5 Minuten**; **Klick auf den Code = kopieren** (Zwischenablage nach 30 s geleert).
- **Export aufs Handy**: große QR-Codes im Übertragungsformat des Google Authenticators
  (8 Konten je Code, mehrere Seiten) – dazu der Link als Text zum Einfügen ins Import-Feld eines Geräts.
- Import/Export mit der JSON-Sicherung des Geräts (Einstellungen → Sicherung / Einspielen).

```bash
cd desktop
pip install -r requirements.txt
python espress2fa_desktop.py              # App-Fenster (pywebview) – oder --browser
build_exe.bat                             # Windows: einzelne EXE in desktop/dist/ (ohne Installation)
```

## Einstellungen & Daten

Alles liegt im NVS des Geräts – Konten, WLAN, Helligkeit, Drehung, Haltezeit, Sprache, letztes Konto, OTA-Schlüssel.
Ein Firmware-Update (USB oder OTA) **behält** alles. Ein ganz neues Board startet leer: Sicherung einspielen.

Zeitzone ist Mitteleuropa (`CET/CEST`), siehe `TZ_BERLIN` in `include/_VARIABLES.h`. Die Uhrzeit kommt per NTP
(`pool.ntp.org`) – TOTP braucht die richtige Zeit.

## Lizenzen

ESPress2FA steht unter der MIT-Lizenz (siehe [LICENSE](LICENSE)). Fremde Bestandteile: siehe [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).
