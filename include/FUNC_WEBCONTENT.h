#pragma once

const char PAGE_ROOT[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>2FA Clock T190</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#000;color:#ccc;font-family:'Courier New',monospace;min-height:100vh}
nav{display:flex;background:#080808;border-bottom:2px solid #0ff;padding:0 12px;flex-wrap:wrap}
nav a{padding:10px 16px;color:#555;font-size:13px;text-decoration:none;border-bottom:3px solid transparent;margin-bottom:-2px;transition:.2s;white-space:nowrap}
nav a.on,nav a:hover{color:#0ff;border-bottom-color:#0ff}
.w{max-width:820px;margin:0 auto;padding:20px 14px}
h1{color:#0ff;font-size:18px;margin-bottom:16px;padding-bottom:8px;border-bottom:1px solid #0ff}
.card{background:#0a0a0a;border:1px solid #1c1c1c;border-radius:6px;padding:16px;margin-bottom:14px}
.ct{color:#0ff;font-size:11px;text-transform:uppercase;letter-spacing:1.5px;margin-bottom:14px}
.row{display:flex;gap:22px;flex-wrap:wrap}
.si label{display:block;font-size:10px;color:#444;text-transform:uppercase;letter-spacing:1px;margin-bottom:3px}
.si .v{font-size:14px;font-weight:bold;color:#0f0}
.vw{color:#fff!important}.vc{color:#0ff!important}.vy{color:#ff0!important}
textarea{width:100%;background:#0d0d0d;border:1px solid #333;color:#bbb;
  font-family:'Courier New',monospace;font-size:11px;padding:10px;
  border-radius:4px;resize:vertical;outline:none}
textarea:focus{border-color:#0ff}
input[type=range]{flex:1;accent-color:#0ff;cursor:pointer;height:6px}
.slv{color:#0ff;min-width:34px;text-align:right;font-size:15px}
.btn{padding:7px 16px;border:none;border-radius:4px;font-family:'Courier New',monospace;
  font-size:12px;cursor:pointer;font-weight:bold;margin:4px 4px 0 0;transition:.15s}
.bg{background:#0a0;color:#000}.bg:hover{background:#0c0}
.br{background:#900;color:#fff}.br:hover{background:#b00}
.bb{background:#005577;color:#fff}.bb:hover{background:#0077aa}
.by{background:#554400;color:#ff0;border:1px solid #554400}.by:hover{background:#776600}
.bsel{background:#0a0a0a;color:#555;border:1px solid #2a2a2a}.bsel:hover{color:#0ff;border-color:#0ff}
.bsel.on{background:#001a1a;color:#0ff;border-color:#0ff}
.tab{display:none}.tab.on{display:block}
table{width:100%;border-collapse:collapse;font-size:12px;margin-top:4px}
th{color:#444;font-size:10px;text-transform:uppercase;letter-spacing:1px;
   padding:5px 8px;text-align:left;border-bottom:1px solid #1a1a1a}
td{padding:7px 8px;border-bottom:1px solid #111;vertical-align:middle}
tr:hover td{background:#0d0d0d}
.badge{display:inline-block;padding:2px 8px;border-radius:10px;font-size:10px;font-weight:bold}
.bon{background:#001a1a;color:#0ff;border:1px solid #0ff}
.boff{background:#141414;color:#444;border:1px solid #2a2a2a}
.mbtn{background:#0d0d0d;border:1px solid #2a2a2a;color:#555;padding:2px 7px;
  cursor:pointer;border-radius:3px;font-size:11px;line-height:1.4;transition:.15s}
.mbtn:hover:not(:disabled){color:#0ff;border-color:#0ff}
.mbtn:disabled{opacity:0.18;cursor:default}
.sort-bar{display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-bottom:10px}
.hint{font-size:11px;color:#444;line-height:1.6;margin:6px 0}
#toast{position:fixed;bottom:20px;right:20px;background:#0a0a0a;border:1px solid #0ff;
  color:#0ff;padding:10px 18px;border-radius:4px;font-size:12px;
  opacity:0;transition:.3s;pointer-events:none;z-index:99}
#toast.show{opacity:1}
.preview-header{color:#0f0;margin:12px 0 6px;font-size:12px}
.empty{color:#444;font-style:italic;padding:12px 0}
.slider-row{display:flex;align-items:center;gap:14px;margin:12px 0 8px}
.grip{cursor:grab;color:#555;font-size:18px;user-select:none;touch-action:none;padding:0 4px}
.grip:hover{color:#0ff}
tr.dragging{opacity:.4}
tr.drag-over td{border-top:2px solid #ff0}
tr.drag-under td{border-bottom:2px solid #ff0}
#acctab{user-select:none;-webkit-user-select:none}
#acctab tr[data-pos]{cursor:grab}
.edit-in{width:100%;padding:8px;background:#111;border:1px solid #333;color:#fff;border-radius:4px;
  font-family:'Courier New',monospace;font-size:13px;margin:4px 0 10px}
.edit-in:focus{border-color:#0ff;outline:none}
.radio{display:block;margin:6px 0;font-size:12px;cursor:pointer}
.radio input{accent-color:#0ff;margin-right:6px}
</style>
</head>
<body>
<nav>
  <a href="#" class="on" onclick="tab('start',this);return false" data-t="nav_status">🕐 Status</a>
  <a href="#" onclick="tab('konten',this);return false" data-t="nav_accounts">🔐 Accounts</a>
  <a href="#" onclick="tab('import',this);return false" data-t="nav_import">📲 Import</a>
  <a href="#" onclick="tab('settings',this);return false" data-t="nav_settings">⚙️ Settings</a>
</nav>
<div class="w">
<h1>🔐 2FA Clock &nbsp;<span style="color:#555;font-size:13px"><span id="board">…</span> · v<span id="ver">…</span></span></h1>

<!-- ═══════════════ STATUS ═══════════════ -->
<div id="tab-start" class="tab on">
  <div class="card" id="batcard">
    <div class="ct" data-t="bat_title">🔋 Battery</div>
    <div style="display:flex;align-items:center;gap:18px;flex-wrap:wrap">
      <div id="batpct" style="font-size:44px;font-weight:bold;color:#0f0;min-width:120px">--</div>
      <div style="flex:1;min-width:180px">
        <div style="background:#1a1a1a;border:1px solid #333;border-radius:6px;height:26px;overflow:hidden">
          <div id="batbar" style="height:100%;width:0;background:#0f0;transition:width .6s,background .6s"></div>
        </div>
        <div id="battxt" style="font-size:14px;color:#aaa;margin-top:6px">...</div>
      </div>
    </div>
  </div>
  <div class="card">
    <div class="ct" data-t="dev_status">📡 Device status</div>
    <div class="row">
      <div class="si"><label data-t="lbl_time">Time</label><div class="v vw" id="st">--:--:--</div></div>
      <div class="si"><label data-t="lbl_date">Date</label><div class="v vw" id="sd">---</div></div>
      <div class="si"><label data-t="lbl_wifi">WiFi</label><div class="v vc" id="ss">...</div></div>
      <div class="si"><label data-t="lbl_ip">IP address</label><div class="v vc" id="sip">...</div></div>
    </div>
    <div class="row" style="margin-top:14px">
      <div class="si"><label data-t="lbl_bright">Brightness</label><div class="v" id="sb">...</div></div>
      <div class="si"><label data-t="lbl_total">Total accounts</label><div class="v vw" id="stot">...</div></div>
      <div class="si"><label data-t="lbl_active">Active</label><div class="v" id="sact">...</div></div>
    </div>
  </div>
</div>

<!-- ═══════════════ ACCOUNTS ═══════════════ -->
<div id="tab-konten" class="tab">
  <div class="card">
    <div class="ct" data-t="acc_title">🔐 Manage 2FA accounts</div>
    <div id="acclist"><div class="empty" data-t="loading">Loading...</div></div>
  </div>
</div>

<!-- ═══════════════ IMPORT ═══════════════ -->
<div id="tab-import" class="tab">
  <div class="card">
    <div class="ct" data-t="imp_title">📲 Google Authenticator Import</div>
    <p class="hint" data-t-html="imp_hint"></p>
    <textarea id="qrin" rows="5" data-t-ph="imp_ph" oninput="onQrChange()"></textarea>
    <div>
      <button class="btn bg" onclick="parseQR()" data-t="imp_parse">🔍 Parse &amp; preview</button>
      <button class="btn br" onclick="clearImport()" data-t="imp_clear">✕ Clear</button>
    </div>
    <div id="preview"></div>
  </div>
</div>

<!-- ═══════════════ SETTINGS ═══════════════ -->
<div id="tab-settings" class="tab">
  <div class="card">
    <div class="ct">🌐 Language / Sprache</div>
    <button class="btn bsel" id="lang-en" onclick="setLang('en')">English</button>
    <button class="btn bsel" id="lang-de" onclick="setLang('de')">Deutsch</button>
    <p class="hint" data-t="lang_hint">Also used for the texts on the device display.</p>
  </div>
  <div class="card">
    <div class="ct" data-t="br_title">💡 Display brightness</div>
    <div class="slider-row">
      <input type="range" id="sl" min="10" max="255" value="200" oninput="liveUpdate(this.value)">
      <span class="slv" id="sv">200</span>
    </div>
    <p class="hint" data-t="br_hint">← Drag the slider = instant preview on the display</p>
    <button class="btn bg" onclick="saveBright()" data-t="br_save">💾 Save to NVS (kept after restart)</button>
  </div>
  <div class="card">
    <div class="ct" data-t="flip_title">🔄 Rotate display</div>
    <label class="radio" style="font-size:14px"><input type="checkbox" id="flip" onchange="setFlip(this.checked)"> <span data-t="flip_lbl">Rotate display 180° (USB port on the other side)</span></label>
    <p class="hint" data-t="flip_hint">Takes effect immediately and is saved. The buttons are swapped too: top = back, bottom = next.</p>
    <div class="slider-row">
      <span style="font-size:12px;color:#888;white-space:nowrap" data-t="fh_lbl">Hold button to rotate:</span>
      <input type="range" id="fh" min="2" max="10" value="3" oninput="document.getElementById('fhv').textContent=this.value+' s'" onchange="setFlipHold(this.value)">
      <span class="slv" id="fhv">3 s</span>
    </div>
    <p class="hint" data-t="fh_hint">Hold a button this long and the display rotates (Dongle: short = next, medium = back).</p>
  </div>
  <div class="card">
    <div class="ct" data-t="bk_title">💾 Backup</div>
    <p class="hint" data-t-html="bk_hint"></p>
    <button class="btn bg" onclick="location.href='/api/backup'" data-t="bk_btn">⬇ Download backup (JSON)</button>
  </div>
  <div class="card">
    <div class="ct" data-t="rs_title">♻️ Restore backup</div>
    <input type="file" id="rsf" accept=".json,application/json" style="color:#888;font-size:12px;margin-bottom:10px">
    <label class="radio"><input type="radio" name="rsm" value="all"> <span data-t="rs_all">Everything (accounts, settings + WiFi)</span></label>
    <label class="radio"><input type="radio" name="rsm" value="nowifi" checked> <span data-t="rs_nowifi">Without WiFi (accounts + settings only)</span></label>
    <label class="radio"><input type="radio" name="rsm" value="wifi"> <span data-t="rs_wifi">WiFi only</span></label>
    <p class="hint" data-t-html="rs_hint"></p>
    <button class="btn by" onclick="doRestore()" data-t="rs_btn">♻️ Restore</button>
  </div>
  <div class="card">
    <div class="ct" data-t="fw_title">⬆️ Firmware update</div>
    <input type="file" id="fwf" accept=".bin" style="color:#888;font-size:12px;margin-bottom:10px">
    <input class="edit-in" id="fwkey" data-t-ph="fw_key_ph" autocomplete="off">
    <div style="background:#1a1a1a;border:1px solid #333;border-radius:4px;height:16px;overflow:hidden;margin:6px 0">
      <div id="fwbar" style="height:100%;width:0;background:#0a0;transition:width .2s"></div></div>
    <div id="fwtxt" class="hint"><b style="color:#ccc" id="fwboard"></b></div>
    <button class="btn by" onclick="doUpdate()" data-t="fw_btn">⬆️ Upload &amp; restart</button>
  </div>
  <div class="card">
    <div class="ct" data-t="wifi_title">📶 WiFi</div>
    <p class="hint" data-t-html="wifi_hint"></p>
    <button class="btn br" onclick="wifiReset()" data-t="wifi_btn">📶 Set up WiFi again</button>
  </div>
</div>

</div><!-- .w -->
<div id="toast"></div>

<!-- ═══════════════ EDIT MODAL ═══════════════ -->
<div id="emodal" onclick="if(event.target===this)closeEdit()"
     style="display:none;position:fixed;inset:0;background:rgba(0,0,0,.88);z-index:210;align-items:center;justify-content:center">
  <div style="background:#0d0d0d;border:2px solid #0ff;border-radius:10px;padding:24px 26px;width:min(380px,92vw)">
    <div class="ct" data-t="ed_title">✎ Rename entry</div>
    <label class="hint" style="margin:0" data-t="ed_issuer">Issuer (large line at the top of the display)</label>
    <input class="edit-in" id="e-issuer" maxlength="31" data-t-ph="ed_issuer_ph">
    <label class="hint" style="margin:0" data-t="ed_name">Account (small line)</label>
    <input class="edit-in" id="e-name" maxlength="63" data-t-ph="ed_name_ph">
    <p class="hint" data-t="ed_hint">The secret key stays unchanged.</p>
    <button class="btn bg" onclick="saveEdit()" data-t="ed_save">💾 Save</button>
    <button class="btn br" onclick="closeEdit()" data-t="ed_cancel">✕ Cancel</button>
  </div>
</div>

<!-- ═══════════════ CODE MODAL ═══════════════ -->
<div id="modal" onclick="if(event.target===this)closeModal()"
     style="display:none;position:fixed;inset:0;background:rgba(0,0,0,.88);
            z-index:200;align-items:center;justify-content:center">
  <div style="background:#0d0d0d;border:2px solid #0ff;border-radius:10px;
              padding:32px 36px;min-width:270px;max-width:340px;
              text-align:center;position:relative;box-shadow:0 0 40px #00ffff22">
    <button onclick="closeModal()"
            style="position:absolute;top:10px;right:14px;background:none;
                   border:none;color:#333;font-size:20px;cursor:pointer;
                   line-height:1;transition:.2s"
            onmouseover="this.style.color='#f55'"
            onmouseout="this.style.color='#333'">✕</button>

    <div id="m-issuer" style="font-size:18px;font-weight:bold;color:#0ff;
                              margin-bottom:6px;letter-spacing:1px"></div>
    <div id="m-name"   style="font-size:11px;color:#555;margin-bottom:28px;
                              word-break:break-all"></div>

    <div id="m-code"   style="font-size:42px;font-weight:bold;color:#fff;
                              letter-spacing:8px;font-family:'Courier New',monospace;
                              margin-bottom:24px;min-height:52px"></div>

    <div style="background:#1a1a1a;border-radius:4px;height:10px;margin-bottom:10px;overflow:hidden">
      <div id="m-bar" style="height:10px;border-radius:4px;
                             transition:width .9s linear,background .5s"></div>
    </div>
    <div id="m-sek" style="font-size:12px;letter-spacing:1px"></div>
  </div>
</div>

<script>
// ══════════════════════════════════════════════════════
// Language / translations (English = base, German = translation)
// ══════════════════════════════════════════════════════
const I18N = {
  en: {
    nav_status: '🕐 Status',
    nav_accounts: '🔐 Accounts',
    nav_import: '📲 Import',
    nav_settings: '⚙️ Settings',
    bat_title: '🔋 Battery',
    bat_none: 'no battery detected',
    bat_usb: ' – on USB / charging',
    bat_low: ' – please charge!',
    bat_soon: ' – charge soon',
    dec_sep: '.',
    dev_status: '📡 Device status',
    lbl_time: 'Time',
    lbl_date: 'Date',
    lbl_wifi: 'WiFi',
    lbl_ip: 'IP address',
    lbl_bright: 'Brightness',
    lbl_total: 'Total accounts',
    lbl_active: 'Active',
    st_pcs: '{0} pcs',
    st_active: '{0} active',
    acc_title: '🔐 Manage 2FA accounts',
    loading: 'Loading...',
    load_err: 'Error while loading',
    acc_empty: 'No accounts yet – use the Import tab 📲',
    sort_issuer: '↕ A–Z issuer',
    sort_name: '↕ A–Z account',
    acc_help: '⠿ drag or ▲▼ = order on display · ✎ = rename',
    th_pos: 'Pos.',
    th_issuer: 'Issuer',
    th_account: 'Account',
    th_status: 'Status',
    th_acc_name: 'Account / name',
    badge_on: '● Active',
    badge_off: '○ Inactive',
    rename: 'Rename',
    imp_title: '📲 Google Authenticator Import',
    imp_hint: '1. In Google Authenticator: <b style="color:#ccc">Settings → Export accounts → Create QR code</b><br>' +
              '2. Point the USB QR reader at <b style="color:#ccc">this text field</b>, click/tab into it → scan QR<br>' +
              '3. With several QR codes, scan them one after another – all are recognized<br>' +
              '4. Single <span style="color:#0ff">otpauth://totp/...</span> links are supported too',
    imp_ph: 'Let the USB QR reader scan in here (otpauth-migration://offline?data=...)\nOr paste single otpauth://totp/... lines',
    imp_parse: '🔍 Parse & preview',
    imp_clear: '✕ Clear',
    imp_none: '⚠ No valid QR content found',
    imp_found: '✅ {0} account(s) found – select and import:',
    imp_btn: '📥 Import selected',
    imp_cancel: '✕ Cancel',
    imp_nothing: 'Nothing selected',
    imp_done: '✅ {0} account(s) imported, total: {1}',
    imp_err: '❌ Import error',
    err_nodata: 'no data parameter',
    parse_err: 'Parse error: {0}',
    lang_hint: 'Also used for the texts on the device display.',
    lang_set: '🌐 Language: English',
    br_title: '💡 Display brightness',
    br_hint: '← Drag the slider = instant preview on the display',
    br_save: '💾 Save to NVS (kept after restart)',
    br_saved: '✅ Brightness saved',
    flip_title: '🔄 Rotate display',
    flip_lbl: 'Rotate display 180° (USB port on the other side)',
    flip_hint: 'Takes effect immediately and is saved. The buttons are swapped too: top = back, bottom = next.',
    fh_lbl: 'Hold button to rotate:',
    fh_hint: 'Hold a button this long and the display rotates (Dongle: short = next, medium = back).',
    fh_saved: '✅ Rotate after holding {0} s',
    flip_on: '🔄 Rotated',
    flip_off: '🔄 Normal',
    bk_title: '💾 Backup',
    bk_hint: 'Contains <b style="color:#ccc">everything</b>: all 2FA accounts with keys, names, order,\n' +
             '      brightness and WiFi credentials. Keep the file safe – whoever has it has your codes.',
    bk_btn: '⬇ Download backup (JSON)',
    rs_title: '♻️ Restore backup',
    rs_all: 'Everything (accounts, settings + WiFi)',
    rs_nowifi: 'Without WiFi (accounts + settings only)',
    rs_wifi: 'WiFi only',
    rs_hint: 'Accounts will be <b style="color:#f88">replaced</b>. If the WiFi changes, the device restarts.',
    rs_btn: '♻️ Restore',
    rs_nofile: '❌ Choose a backup file first',
    rs_all_c: 'EVERYTHING (accounts are replaced + WiFi)',
    rs_nowifi_c: 'Accounts + settings (accounts are replaced)',
    rs_wifi_c: 'WiFi only',
    rs_confirm: 'Restore backup: {0} ?',
    rs_done: '✅ Restored',
    rs_accounts: ': {0} accounts',
    rs_restart: ' – restarting ...',
    fw_title: '⬆️ Firmware update',
    fw_key_ph: 'OTA key (shown on the display at startup / via USB)',
    fw_hint: 'Choose the file <b style="color:#ccc">firmware.bin</b> for <b style="color:#ccc" id="fwboard">{0}</b>. Accounts, WiFi and settings are kept.',
    fw_this_device: 'this device',
    fw_btn: '⬆️ Upload & restart',
    fw_nofile: '❌ Choose a .bin file first',
    fw_confirm: 'Flash firmware "{0}" ({1} KB)?',
    fw_progress: 'Uploading ... {0} %',
    fw_done: '✅ Done – device is restarting, page will reload shortly ...',
    fw_lost: '❌ Connection lost',
    wifi_title: '📶 WiFi',
    wifi_hint: 'Deletes the WiFi data and starts the setup hotspot <b style="color:#ccc">2FA-…-xxxx</b>\n' +
               '      (password t190setup, page http://192.168.4.1/). Accounts are kept.',
    wifi_btn: '📶 Set up WiFi again',
    wifi_confirm: 'Delete WiFi data and start the setup hotspot?',
    wifi_restart: '📶 Restarting into the hotspot ...',
    ed_title: '✎ Rename entry',
    ed_issuer: 'Issuer (large line at the top of the display)',
    ed_issuer_ph: 'e.g. Google Fillers Channel',
    ed_name: 'Account (small line)',
    ed_name_ph: 'e.g. name@gmail.com',
    ed_hint: 'The secret key stays unchanged.',
    ed_save: '💾 Save',
    ed_cancel: '✕ Cancel',
    saved: '✅ Saved',
    err: '❌ Error',
    conn_err: '❌ Connection error',
    order_saved: '✅ Order saved',
    order_err: '❌ Error while saving',
    still_saving: '⏳ Still saving ...',
    del_confirm: 'Really delete this account?',
    deleted: '🗑 Account deleted',
    seconds: '{0} seconds'
  },
  de: {
    nav_status: '🕐 Status',
    nav_accounts: '🔐 Konten',
    nav_import: '📲 Import',
    nav_settings: '⚙️ Einstellungen',
    bat_title: '🔋 Akku',
    bat_none: 'kein Akku erkannt',
    bat_usb: ' – am USB / lädt',
    bat_low: ' – bitte laden!',
    bat_soon: ' – bald laden',
    dec_sep: ',',
    dev_status: '📡 Gerätestatus',
    lbl_time: 'Uhrzeit',
    lbl_date: 'Datum',
    lbl_wifi: 'WLAN',
    lbl_ip: 'IP-Adresse',
    lbl_bright: 'Helligkeit',
    lbl_total: 'Konten gesamt',
    lbl_active: 'Davon aktiv',
    st_pcs: '{0} Stk',
    st_active: '{0} aktiv',
    acc_title: '🔐 2FA Konten verwalten',
    loading: 'Lädt...',
    load_err: 'Fehler beim Laden',
    acc_empty: 'Noch keine Konten – Import-Tab nutzen 📲',
    sort_issuer: '↕ A–Z Aussteller',
    sort_name: '↕ A–Z Konto',
    acc_help: '⠿ ziehen oder ▲▼ = Reihenfolge auf Display · ✎ = umbenennen',
    th_pos: 'Pos.',
    th_issuer: 'Aussteller',
    th_account: 'Konto',
    th_status: 'Status',
    th_acc_name: 'Konto / Name',
    badge_on: '● Aktiv',
    badge_off: '○ Inaktiv',
    rename: 'Umbenennen',
    imp_title: '📲 Google Authenticator Import',
    imp_hint: '1. In Google Authenticator: <b style="color:#ccc">Einstellungen → Konten exportieren → QR-Code erstellen</b><br>' +
              '2. USB QR-Leser auf <b style="color:#ccc">dieses Textfeld</b> richten, Tab-Klick → QR scannen<br>' +
              '3. Bei mehreren QR-Codes nacheinander scannen – alle werden erkannt<br>' +
              '4. Auch einzelne <span style="color:#0ff">otpauth://totp/...</span> Links werden unterstützt',
    imp_ph: 'USB QR-Leser hier einscannen lassen (otpauth-migration://offline?data=...)\nOder einzelne otpauth://totp/... Zeilen einfügen',
    imp_parse: '🔍 Parsen & Vorschau',
    imp_clear: '✕ Löschen',
    imp_none: '⚠ Kein gültiger QR-Inhalt gefunden',
    imp_found: '✅ {0} Konto(s) gefunden – auswählen und importieren:',
    imp_btn: '📥 Ausgewählte importieren',
    imp_cancel: '✕ Abbrechen',
    imp_nothing: 'Nichts ausgewählt',
    imp_done: '✅ {0} Konto(s) importiert, gesamt: {1}',
    imp_err: '❌ Import-Fehler',
    err_nodata: 'kein data-Parameter',
    parse_err: 'Parse-Fehler: {0}',
    lang_hint: 'Gilt auch für die Texte auf dem Gerätedisplay.',
    lang_set: '🌐 Sprache: Deutsch',
    br_title: '💡 Display-Helligkeit',
    br_hint: '← Slider ziehen = sofortige Vorschau auf dem Display',
    br_save: '💾 In NVS speichern (bleibt nach Neustart)',
    br_saved: '✅ Helligkeit gespeichert',
    flip_title: '🔄 Display drehen',
    flip_lbl: 'Display 180° drehen (USB-Anschluss auf der anderen Seite)',
    flip_hint: 'Wirkt sofort und bleibt gespeichert. Die Tasten werden mitgetauscht: oben = zurück, unten = weiter.',
    fh_lbl: 'Taste halten zum Drehen:',
    fh_hint: 'So lange eine Taste gedrückt halten, dann dreht sich das Display (Dongle: kurz = weiter, mittel = zurück).',
    fh_saved: '✅ Drehen nach {0} s halten',
    flip_on: '🔄 Gedreht',
    flip_off: '🔄 Normal',
    bk_title: '💾 Sicherung',
    bk_hint: 'Enthält <b style="color:#ccc">alles</b>: alle 2FA-Konten mit Schlüsseln, Namen, Reihenfolge,\n' +
             '      Helligkeit und WLAN-Zugangsdaten. Datei gut aufbewahren – wer sie hat, hat deine Codes.',
    bk_btn: '⬇ Sicherung herunterladen (JSON)',
    rs_title: '♻️ Sicherung einspielen',
    rs_all: 'Alles (Konten, Einstellungen + WLAN)',
    rs_nowifi: 'Ohne WLAN (nur Konten + Einstellungen)',
    rs_wifi: 'Nur WLAN',
    rs_hint: 'Konten werden dabei <b style="color:#f88">ersetzt</b>. Bei geändertem WLAN startet das Gerät neu.',
    rs_btn: '♻️ Einspielen',
    rs_nofile: '❌ Erst eine Sicherungsdatei wählen',
    rs_all_c: 'ALLES (Konten werden ersetzt + WLAN)',
    rs_nowifi_c: 'Konten + Einstellungen (Konten werden ersetzt)',
    rs_wifi_c: 'nur WLAN',
    rs_confirm: 'Sicherung einspielen: {0} ?',
    rs_done: '✅ Eingespielt',
    rs_accounts: ': {0} Konten',
    rs_restart: ' – Neustart ...',
    fw_title: '⬆️ Firmware-Update',
    fw_key_ph: 'OTA-Schlüssel (steht beim Start auf dem Display / per USB)',
    fw_hint: 'Datei <b style="color:#ccc">firmware.bin</b> für <b style="color:#ccc" id="fwboard">{0}</b> wählen. Konten, WLAN und Einstellungen bleiben erhalten.',
    fw_this_device: 'dieses Gerät',
    fw_btn: '⬆️ Hochladen & neu starten',
    fw_nofile: '❌ Erst eine .bin-Datei wählen',
    fw_confirm: 'Firmware "{0}" ({1} KB) aufspielen?',
    fw_progress: 'Lade hoch ... {0} %',
    fw_done: '✅ Fertig – Gerät startet neu, Seite lädt gleich neu ...',
    fw_lost: '❌ Verbindung abgebrochen',
    wifi_title: '📶 WLAN',
    wifi_hint: 'Löscht die WLAN-Daten und startet in den Einrichtungs-Hotspot <b style="color:#ccc">2FA-…-xxxx</b>\n' +
               '      (Passwort t190setup, Seite http://192.168.4.1/). Konten bleiben erhalten.',
    wifi_btn: '📶 WLAN neu einrichten',
    wifi_confirm: 'WLAN-Daten löschen und in den Einrichtungs-Hotspot starten?',
    wifi_restart: '📶 Neustart in den Hotspot ...',
    ed_title: '✎ Eintrag umbenennen',
    ed_issuer: 'Aussteller (große Zeile oben am Display)',
    ed_issuer_ph: 'z. B. Google Fillers Channel',
    ed_name: 'Konto (kleine Zeile)',
    ed_name_ph: 'z. B. name@gmail.com',
    ed_hint: 'Der geheime Schlüssel bleibt unverändert.',
    ed_save: '💾 Speichern',
    ed_cancel: '✕ Abbrechen',
    saved: '✅ Gespeichert',
    err: '❌ Fehler',
    conn_err: '❌ Verbindungsfehler',
    order_saved: '✅ Reihenfolge gespeichert',
    order_err: '❌ Fehler beim Speichern',
    still_saving: '⏳ Speichere noch ...',
    del_confirm: 'Konto wirklich löschen?',
    deleted: '🗑 Konto gelöscht',
    seconds: '{0} Sekunden'
  }
};
let curLang = 'en';
let langLoaded = false;   // language is taken from /api/status once
let boardName = '';

// Translate key, replace {0}, {1}, ... with arguments (falls back to English, then to the key)
function t(key, ...args) {
  let s = I18N[curLang] && I18N[curLang][key];
  if (s === undefined) s = I18N.en[key];
  if (s === undefined) return key;
  return s.replace(/\{(\d+)\}/g, (m, i) => args[i] !== undefined ? args[i] : m);
}

// Firmware hint contains the board name (element #fwboard); skip while the upload text is shown
function fwHint() {
  if (!document.getElementById('fwboard')) return;
  document.getElementById('fwtxt').innerHTML = t('fw_hint', boardName ? esc(boardName) : t('fw_this_device'));
}

// Fill all static texts of the page in the current language
function applyLang() {
  document.documentElement.lang = curLang;
  document.querySelectorAll('[data-t]').forEach(e => { e.textContent = t(e.getAttribute('data-t')); });
  document.querySelectorAll('[data-t-ph]').forEach(e => { e.placeholder = t(e.getAttribute('data-t-ph')); });
  document.querySelectorAll('[data-t-title]').forEach(e => { e.title = t(e.getAttribute('data-t-title')); });
  document.querySelectorAll('[data-t-html]').forEach(e => { e.innerHTML = t(e.getAttribute('data-t-html')); });
  fwHint();
  ['en', 'de'].forEach(l => document.getElementById('lang-' + l).classList.toggle('on', l === curLang));
}

// Language switch on the settings tab (also used by the firmware for the device display)
function setLang(l) {
  if (l !== 'en' && l !== 'de') return;
  curLang = l;
  langLoaded = true;
  applyLang();
  if (accLoaded) renderTable();
  if (previewData.length && document.getElementById('preview').innerHTML) showPreview(previewData);
  fetch('/api/lang', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'v=' + l })
    .then(r => r.ok ? toast(t('lang_set')) : toast(t('err'), false))
    .catch(() => toast(t('conn_err'), false));
}

// ══════════════════════════════════════════════════════
// Tabs
// ══════════════════════════════════════════════════════
let kontenTimer = null;
function tab(n, el) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('on'));
  document.querySelectorAll('nav a').forEach(a => a.classList.remove('on'));
  document.getElementById('tab-' + n).classList.add('on');
  el.classList.add('on');
  // Start / stop auto refresh for the accounts tab
  clearInterval(kontenTimer);
  kontenTimer = null;
  if (n === 'konten') {
    loadAccounts();
    // no auto refresh – the table is for management only, no live codes
  }
  if (n === 'import') setTimeout(() => document.getElementById('qrin').focus(), 80);
}

// ══════════════════════════════════════════════════════
// Toast
// ══════════════════════════════════════════════════════
function toast(msg, ok = true) {
  const t = document.getElementById('toast');
  t.textContent = msg;
  t.style.color = ok ? '#0ff' : '#f55';
  t.style.borderColor = ok ? '#0ff' : '#f55';
  t.classList.add('show');
  setTimeout(() => t.classList.remove('show'), 2800);
}

// ══════════════════════════════════════════════════════
// Status polling
// ══════════════════════════════════════════════════════
let firstLoad = true;
function showBattery(p, mv) {
  const pe = document.getElementById('batpct'), bar = document.getElementById('batbar'), tx = document.getElementById('battxt');
  if (p === undefined || p < 0) { pe.textContent = '--'; pe.style.color = '#555'; bar.style.width = '0';
    tx.textContent = t('bat_none'); return; }
  const col = p <= 15 ? '#f33' : p <= 30 ? '#fd0' : '#0f0';
  pe.textContent = p + ' %'; pe.style.color = col;
  bar.style.width = p + '%'; bar.style.background = col;
  const v = (mv / 1000).toFixed(2).replace('.', t('dec_sep')) + ' V';
  tx.textContent = v + (mv >= 4250 ? t('bat_usb') : p <= 15 ? t('bat_low') : p <= 30 ? t('bat_soon') : '');
  tx.style.color = p <= 15 ? '#f66' : '#aaa';
}
function updateStatus() {
  fetch('/api/status').then(r => r.json()).then(d => {
    if (!langLoaded) {
      curLang = d.lang === 'de' ? 'de' : 'en';
      langLoaded = true;
      applyLang();
      if (accLoaded) renderTable();
    }
    document.getElementById('st').textContent   = d.time;
    document.getElementById('sd').textContent   = d.date;
    document.getElementById('ss').textContent   = d.ssid;
    document.getElementById('sip').textContent  = d.ip;
    document.getElementById('sb').textContent   = d.bright;
    document.getElementById('stot').textContent = t('st_pcs', d.total);
    document.getElementById('sact').textContent = t('st_active', d.active);
    if (d.version) document.getElementById('ver').textContent = d.version;
    if (d.board) {
      boardName = d.board;
      document.getElementById('board').textContent = d.board;
      const fb = document.getElementById('fwboard');
      if (fb) fb.textContent = d.board;
      document.title = '2FA Clock ' + d.board;
    }
    document.getElementById('batcard').style.display = d.has_bat === false ? 'none' : '';
    showBattery(d.bat_pct, d.bat_mv);
    if (firstLoad) {
      document.getElementById('flip').checked = !!d.flip;
      if (d.flip_s) { document.getElementById('fh').value = d.flip_s; document.getElementById('fhv').textContent = d.flip_s + ' s'; }
      document.getElementById('sl').value = d.bright;
      document.getElementById('sv').textContent = d.bright;
      firstLoad = false;
    }
  }).catch(() => {});
}

// ══════════════════════════════════════════════════════
// Brightness / rotation / firmware update
// ══════════════════════════════════════════════════════
let lt = null;
function liveUpdate(v) {
  document.getElementById('sv').textContent = v;
  clearTimeout(lt);
  lt = setTimeout(() => fetch('/api/brightness/live', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: 'v=' + v
  }), 80);
}
function doUpdate() {
  const f = document.getElementById('fwf').files[0];
  if (!f) { toast(t('fw_nofile'), false); return; }
  if (!confirm(t('fw_confirm', f.name, Math.round(f.size / 1024)))) return;
  const fd = new FormData(); fd.append('firmware', f, f.name);
  const x = new XMLHttpRequest();
  const bar = document.getElementById('fwbar'), tx = document.getElementById('fwtxt');
  x.upload.onprogress = e => { if (e.lengthComputable) { const p = Math.round(e.loaded * 100 / e.total);
    bar.style.width = p + '%'; tx.textContent = t('fw_progress', p); } };
  x.onload = () => { if (x.status === 200) { tx.textContent = t('fw_done');
      setTimeout(() => location.reload(), 12000); } else { tx.textContent = '❌ ' + x.responseText; bar.style.background = '#a00'; } };
  x.onerror = () => { tx.textContent = t('fw_lost'); };
  x.open('POST', '/update');
  x.setRequestHeader('X-FW-Size', f.size);
  x.setRequestHeader('X-OTA-Key', document.getElementById('fwkey').value.trim());
  x.send(fd);
}
function setFlipHold(v) {
  fetch('/api/fliphold', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 's=' + v })
    .then(r => r.ok ? toast(t('fh_saved', v)) : toast(t('err'), false));
}
function setFlip(v) {
  fetch('/api/flip', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'v=' + (v ? 1 : 0) })
    .then(r => r.ok ? toast(v ? t('flip_on') : t('flip_off')) : toast(t('err'), false));
}
function saveBright() {
  fetch('/api/brightness/save', { method: 'POST' })
    .then(r => r.ok ? toast(t('br_saved')) : toast(t('err'), false));
}

// ══════════════════════════════════════════════════════
// Account list – data, sorting, moving
// ══════════════════════════════════════════════════════
let accountData = [];
let accLoaded   = false;
let saveTimer   = null;

function esc(s) {
  return String(s).replace(/[&<>"']/g,
    c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
}

function loadAccounts() {
  fetch('/api/accounts').then(r => r.json()).then(data => {
    accountData = data.map(a => ({ ...a })); // id now comes from the server
    accLoaded = true;
    renderTable();
  }).catch(() => {
    document.getElementById('acclist').innerHTML =
      '<div class="empty" style="color:#f55">' + t('load_err') + '</div>';
  });
}

// Codes only on the device – no TOTP call from the web handler
function refreshCodes() { /* not used */ }

// ── Render table ──────────────────────────────────────
function renderTable() {
  const el = document.getElementById('acclist');
  if (!accountData.length) {
    el.innerHTML = '<div class="empty">' + t('acc_empty') + '</div>';
    return;
  }
  const n = accountData.length;
  let html = `
    <div class="sort-bar">
      <button class="btn bb" onclick="sortBy('issuer')">${t('sort_issuer')}</button>
      <button class="btn bb" onclick="sortBy('name')">${t('sort_name')}</button>
      <span style="color:#333;font-size:11px;align-self:center">${t('acc_help')}</span>
    </div>
    <table id="acctab">
      <tr>
        <th style="width:24px"></th>
        <th style="width:28px"></th>
        <th style="width:52px;text-align:center">${t('th_pos')}</th>
        <th>${t('th_issuer')}</th>
        <th>${t('th_account')}</th>
        <th style="text-align:center">${t('th_status')}</th>
        <th></th>
      </tr>`;

  accountData.forEach((a, pos) => {
    html += `<tr data-pos="${pos}" draggable="true" onclick="openModal(${a.id})">
      <td class="grip">⠿</td>
      <td style="text-align:center" onclick="event.stopPropagation()">
        <input type="checkbox" ${a.active?'checked':''} onchange="toggleAcc(${a.id},this)">
      </td>
      <td style="text-align:center;white-space:nowrap" onclick="event.stopPropagation()">
        <button class="mbtn" onclick="moveAcc(${pos},-1)" ${pos===0?'disabled':''}>▲</button>
        <button class="mbtn" onclick="moveAcc(${pos},+1)" ${pos===n-1?'disabled':''}>▼</button>
      </td>
      <td style="color:#ccc">${esc(a.issuer||'–')}</td>
      <td style="color:#888">${esc(a.name)}</td>
      <td style="text-align:center">
        <span class="badge ${a.active?'bon':'boff'}">${a.active?t('badge_on'):t('badge_off')}</span>
      </td>
      <td style="text-align:right" onclick="event.stopPropagation()">
        <button class="btn bb" style="padding:3px 10px;margin:0 4px 0 0" onclick="editAcc(${pos})" title="${t('rename')}">✎</button>
        <button class="btn br" style="padding:3px 10px;margin:0" onclick="delAcc(${a.id})">✕</button>
      </td>
    </tr>`;
  });
  html += '</table>';
  el.innerHTML = html;
  attachDnD();
}

// ── Sort ──────────────────────────────────────────────
function sortBy(field) {
  accountData.sort((a, b) =>
    (a[field]||'').toLowerCase().localeCompare((b[field]||'').toLowerCase()));
  renderTable();
  saveOrderDebounced();
}

// ── Move ──────────────────────────────────────────────
function moveAcc(pos, dir) {
  if (orderBusy) return;
  const to = pos + dir;
  if (to < 0 || to >= accountData.length) return;
  [accountData[pos], accountData[to]] = [accountData[to], accountData[pos]];
  renderTable();
  saveOrderDebounced();
}

// ── Save order (debounced) ────────────────────────────
function saveOrderDebounced() {
  clearTimeout(saveTimer);
  saveTimer = setTimeout(saveOrder, 600);
}
// The positions (id) are only valid for the last loaded state -> until the fresh list has arrived,
// allow no further change (otherwise the device assigns wrongly, bug up to 1.1.5)
let orderBusy = false;
function saveOrder() {
  if (orderBusy) return;
  orderBusy = true;
  const order = accountData.map(a => a.id);
  fetch('/api/accounts/reorder', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(order)
  }).then(r => {
    if (r.ok) toast(t('order_saved'));
    else toast(t('order_err'), false);
    return fetch('/api/accounts').then(r => r.json()).then(d => { accountData = d; renderTable(); });
  }).catch(() => toast(t('conn_err'), false))
    .finally(() => { orderBusy = false; });
}

// ── Drag & drop (1.1.7) – as in the Stream-Radio project: the browser's built-in HTML5 dragging ──
// The browser itself shows an image of the row under the mouse, the target gets a yellow line.
let dragSrc = null;
function attachDnD() {
  const rows = document.querySelectorAll('#acctab tr[data-pos]');
  const clear = () => rows.forEach(r => r.classList.remove('drag-over', 'drag-under'));
  rows.forEach(tr => {
    const pos = +tr.dataset.pos;
    tr.addEventListener('dragstart', e => {
      if (orderBusy) { e.preventDefault(); toast(t('still_saving')); return; }
      dragSrc = pos;
      tr.classList.add('dragging');
      e.dataTransfer.effectAllowed = 'move';
      e.dataTransfer.setData('text/plain', String(pos));          // Firefox needs data, otherwise no dragging
    });
    tr.addEventListener('dragend', () => { tr.classList.remove('dragging'); clear(); dragSrc = null; });
    tr.addEventListener('dragover', e => {
      if (dragSrc === null) return;
      e.preventDefault();
      e.dataTransfer.dropEffect = 'move';
      clear();
      if (pos !== dragSrc) tr.classList.add(pos > dragSrc ? 'drag-under' : 'drag-over');
    });
    tr.addEventListener('drop', e => {
      e.preventDefault();
      clear();
      if (dragSrc === null || dragSrc === pos) return;
      const moved = accountData.splice(dragSrc, 1)[0];          // insert at the target's position
      accountData.splice(pos, 0, moved);
      dragSrc = null;
      renderTable();
      clearTimeout(saveTimer);
      saveOrder();                                                // save immediately
    });
  });
}

// ── Rename ──
let editPos = -1;
function editAcc(pos) {
  editPos = pos;
  const a = accountData[pos];
  document.getElementById('e-issuer').value = a.issuer || '';
  document.getElementById('e-name').value = a.name || '';
  document.getElementById('emodal').style.display = 'flex';
  setTimeout(() => document.getElementById('e-issuer').focus(), 50);
}
function closeEdit() { document.getElementById('emodal').style.display = 'none'; editPos = -1; }
function saveEdit() {
  if (editPos < 0) return;
  const a = accountData[editPos];
  const body = { issuer: document.getElementById('e-issuer').value.trim(),
                 name:   document.getElementById('e-name').value.trim() };
  fetch('/api/accounts/' + a.id + '/edit', { method: 'POST',
    headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) })
    .then(r => { if (r.ok) { toast(t('saved')); closeEdit(); loadAccounts(); } else toast(t('err'), false); })
    .catch(() => toast(t('conn_err'), false));
}

// ── Restore backup (like the Fritzbox: everything / without WiFi / WiFi only) ──
function doRestore() {
  const f = document.getElementById('rsf').files[0];
  if (!f) { toast(t('rs_nofile'), false); return; }
  const mode = document.querySelector('input[name=rsm]:checked').value;
  const txt = { all: t('rs_all_c'), nowifi: t('rs_nowifi_c'), wifi: t('rs_wifi_c') }[mode];
  if (!confirm(t('rs_confirm', txt))) return;
  f.text().then(body => fetch('/api/restore?mode=' + mode, { method: 'POST',
      headers: { 'Content-Type': 'application/json' }, body }))
    .then(async r => {
      const tx = await r.text();
      if (!r.ok) { toast('❌ ' + tx, false); return; }
      const d = JSON.parse(tx);
      toast(t('rs_done') + (d.accounts ? t('rs_accounts', d.accounts) : '') + (d.restart ? t('rs_restart') : ''));
      loadAccounts();
    }).catch(() => toast(t('conn_err'), false));
}
function wifiReset() {
  if (!confirm(t('wifi_confirm'))) return;
  fetch('/api/wifi/reset', { method: 'POST' }).then(() => toast(t('wifi_restart')));
}

function toggleAcc(id, cb) {
  fetch('/api/accounts/' + id + '/toggle', { method: 'POST' })
    .then(r => { if (r.ok) loadAccounts(); else { toast(t('err'),false); cb.checked=!cb.checked; } });
}
function delAcc(id) {
  if (!confirm(t('del_confirm'))) return;
  fetch('/api/accounts/' + id, { method: 'DELETE' })
    .then(r => r.ok ? (toast(t('deleted')), loadAccounts()) : toast(t('err'), false));
}

// ══════════════════════════════════════════════════════
// Code modal
// ══════════════════════════════════════════════════════
let modalTimer = null;
let modalId    = -1;

function openModal(id) {
  modalId = id;
  const m = document.getElementById('modal');
  m.style.display = 'flex';
  document.getElementById('m-code').textContent = '··· ···';
  document.getElementById('m-issuer').textContent = '';
  document.getElementById('m-name').textContent = '';
  fetchModalCode();
  clearInterval(modalTimer);
  modalTimer = setInterval(fetchModalCode, 1000);
}

function closeModal() {
  clearInterval(modalTimer);
  modalTimer = null;
  modalId    = -1;
  document.getElementById('modal').style.display = 'none';
}

function fetchModalCode() {
  if (modalId < 0) return;
  fetch('/api/code?i=' + modalId)
    .then(r => r.json())
    .then(d => {
      const code = d.code || '------';
      const rem  = d.remaining || 0;
      const col  = rem <= 5 ? '#f55' : rem <= 10 ? '#ff0' : '#0f0';

      document.getElementById('m-issuer').textContent = d.issuer || d.name || '?';
      document.getElementById('m-name').textContent   = d.name   || '';
      document.getElementById('m-code').textContent   = code.substring(0,3) + ' ' + code.substring(3);
      document.getElementById('m-bar').style.width      = (rem / 30 * 100) + '%';
      document.getElementById('m-bar').style.background = col;
      document.getElementById('m-sek').textContent      = t('seconds', rem);
      document.getElementById('m-sek').style.color      = col;
    })
    .catch(() => {
      document.getElementById('m-code').textContent = '– – –';
    });
}

// Escape key closes the modal
document.addEventListener('keydown', e => {
  if (e.key === 'Escape') { closeModal(); closeEdit(); }
  if (e.key === 'Enter' && editPos >= 0) saveEdit();
});

// ══════════════════════════════════════════════════════
// Import – QR parser
// ══════════════════════════════════════════════════════
let qrTimer = null;
function onQrChange() {
  clearTimeout(qrTimer);
  qrTimer = setTimeout(parseQR, 1000);
}
function clearImport() {
  document.getElementById('qrin').value = '';
  document.getElementById('preview').innerHTML = '';
}

function parseQR() {
  const raw = document.getElementById('qrin').value.trim();
  if (!raw) { document.getElementById('preview').innerHTML = ''; return; }

  const lines = raw.split(/[\n\r]+/).map(s => s.trim()).filter(Boolean);
  let all = [];

  for (const line of lines) {
    if (line.startsWith('otpauth-migration://')) {
      const parsed = parseMigration(line);
      if (parsed) all = all.concat(parsed);
    } else if (line.startsWith('otpauth://totp/') || line.startsWith('otpauth://hotp/')) {
      const acc = parseSingleOtpauth(line);
      if (acc) all.push(acc);
    }
  }

  if (!all.length) {
    document.getElementById('preview').innerHTML =
      '<p style="color:#f55;margin-top:12px">' + t('imp_none') + '</p>';
    return;
  }
  showPreview(all);
}

// ── Parse Google Authenticator migration URL ──────────────
function parseMigration(url) {
  try {
    const m = url.match(/[?&]data=([^&\s]+)/);
    if (!m) throw new Error(t('err_nodata'));
    const b64 = decodeURIComponent(m[1]);
    // atob needs + and /, convert base64url if necessary
    const raw = atob(b64.replace(/-/g, '+').replace(/_/g, '/'));
    const bytes = new Uint8Array(raw.length);
    for (let i = 0; i < raw.length; i++) bytes[i] = raw.charCodeAt(i);
    return decodeProtobuf(bytes);
  } catch (e) {
    toast(t('parse_err', e.message), false);
    return null;
  }
}

// ── Single otpauth://totp/... link ───────────────────────
function parseSingleOtpauth(url) {
  try {
    const u = new URL(url);
    const raw = decodeURIComponent(u.pathname.replace(/^\/\/[^\/]*/, '').replace(/^\//, ''));
    let issuer = u.searchParams.get('issuer') || '';
    let name   = raw;
    if (raw.includes(':')) {
      const parts = raw.split(':');
      if (!issuer) issuer = parts[0].trim();
      name = parts.slice(1).join(':').trim();
    }
    const secret = (u.searchParams.get('secret') || '').toUpperCase().replace(/\s/g, '');
    if (!secret) return null;
    return { name, issuer, secret, active: true };
  } catch (e) { return null; }
}

// ══════════════════════════════════════════════════════
// Minimal protobuf decoder for the migration format
// ══════════════════════════════════════════════════════
function decodeProtobuf(data) {
  let pos = 0;

  const varint = () => {
    let v = 0, s = 0;
    while (pos < data.length) {
      const b = data[pos++];
      v |= (b & 0x7F) << s;
      if (!(b & 0x80)) break;
      s += 7;
    }
    return v;
  };

  const lenDelim = () => {
    const len = varint();
    const sl = data.slice(pos, pos + len);
    pos += len;
    return sl;
  };

  const accounts = [];
  while (pos < data.length) {
    const tag = varint();
    const fn = tag >> 3, wt = tag & 7;
    if      (fn === 1 && wt === 2) { const msg = lenDelim(); const a = decodeOtpParams(msg); if (a && a.secret) accounts.push(a); }
    else if (wt === 0) varint();
    else if (wt === 2) lenDelim();
    else if (wt === 5) pos += 4;
    else if (wt === 1) pos += 8;
    else break; // unknown wire type, abort
  }
  return accounts;
}

function decodeOtpParams(data) {
  const td = new TextDecoder();
  let pos = 0;
  const acc = { name: '', issuer: '', secret: '', active: true };

  const varint = () => {
    let v = 0, s = 0;
    while (pos < data.length) {
      const b = data[pos++];
      v |= (b & 0x7F) << s;
      if (!(b & 0x80)) break;
      s += 7;
    }
    return v;
  };
  const lenDelim = () => {
    const len = varint();
    const sl = data.slice(pos, pos + len);
    pos += len;
    return sl;
  };

  while (pos < data.length) {
    const tag = varint();
    const fn = tag >> 3, wt = tag & 7;
    if (wt === 2) {
      const bytes = lenDelim();
      if      (fn === 1) acc.secret = bytesToBase32(bytes);
      else if (fn === 2) acc.name   = td.decode(bytes);
      else if (fn === 3) acc.issuer = td.decode(bytes);
    } else if (wt === 0) varint();
    else if (wt === 5) pos += 4;
    else if (wt === 1) pos += 8;
    else break;
  }

  // Extract issuer from the name if missing ("Google:user@...")
  if (!acc.issuer && acc.name.includes(':')) {
    const parts = acc.name.split(':');
    acc.issuer = parts[0].trim();
    acc.name   = parts.slice(1).join(':').trim();
  }
  acc.secret = acc.secret.toUpperCase().replace(/\s/g, '');
  return acc;
}

// ── Bytes → Base32 ───────────────────────────────────────
const B32 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ234567';
function bytesToBase32(bytes) {
  let bits = 0, val = 0, result = '';
  for (const b of bytes) {
    val  = (val << 8) | b;
    bits += 8;
    while (bits >= 5) {
      result += B32[(val >>> (bits - 5)) & 31];
      bits   -= 5;
    }
  }
  if (bits > 0) result += B32[(val << (5 - bits)) & 31];
  return result;
}

// ══════════════════════════════════════════════════════
// Preview + import
// ══════════════════════════════════════════════════════
let previewData = [];

function showPreview(accounts) {
  previewData = accounts;
  const el = document.getElementById('preview');

  let html = `<p class="preview-header">${t('imp_found', accounts.length)}</p>
  <table>
    <tr>
      <th style="width:36px"><input type="checkbox" id="chk-all" checked onchange="toggleAll(this.checked)"></th>
      <th>${t('th_issuer')}</th>
      <th>${t('th_acc_name')}</th>
    </tr>`;
  accounts.forEach((a, i) => {
    html += `<tr>
      <td style="text-align:center"><input type="checkbox" id="imp${i}" checked></td>
      <td style="color:#ccc">${esc(a.issuer || '–')}</td>
      <td style="color:#888">${esc(a.name)}</td>
    </tr>`;
  });
  html += `</table>
  <div style="margin-top:12px">
    <button class="btn bg" onclick="doImport()">${t('imp_btn')}</button>
    <button class="btn by" onclick="clearImport()">${t('imp_cancel')}</button>
  </div>`;
  el.innerHTML = html;
}

function toggleAll(v) {
  previewData.forEach((_, i) => {
    const cb = document.getElementById('imp' + i);
    if (cb) cb.checked = v;
  });
}

function doImport() {
  const toImport = previewData.filter((_, i) => {
    const cb = document.getElementById('imp' + i);
    return cb && cb.checked;
  });
  if (!toImport.length) { toast(t('imp_nothing'), false); return; }

  fetch('/api/accounts/import', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(toImport)
  }).then(r => r.json()).then(d => {
    toast(t('imp_done', d.added, d.total));
    clearImport();
    updateStatus();
  }).catch(() => toast(t('imp_err'), false));
}

// ══════════════════════════════════════════════════════
// Start: English texts until /api/status reports the stored language
// ══════════════════════════════════════════════════════
applyLang();
updateStatus();
setInterval(updateStatus, 2000);
</script>
</body>
</html>
)rawhtml";
