#!/usr/bin/env python3
"""ESPress2FA Desktop - the ESPress2FA web interface on your PC, with an encrypted vault.

- Serves the SAME web interface as the device (taken from include/FUNC_WEBCONTENT.h) on 127.0.0.1 only
  and implements the device API (/api/...) in Python.
- All accounts live in ONE encrypted vault file (AES-256-GCM, key from your master password via scrypt).
  Without the master password the file cannot be opened - keep a device backup / recovery codes as fallback.
- Unlock page, lock button, auto-lock after 5 minutes without activity, click on a code = copy
  (clipboard is cleared again after 30 s).
- Import / export: the device's JSON backup format (Settings -> backup / restore in the web interface),
  export to a phone authenticator as QR codes (Google Authenticator transfer format, several pages).

Usage:  python espress2fa_desktop.py [--vault FILE] [--browser] [--port N]
Needs:  pip install -r requirements.txt   (cryptography, qrcode, pywebview - pywebview optional: without it the default browser is used)
"""
import argparse, base64, datetime, hashlib, hmac, json, os, secrets, socket, struct, sys, threading, time, webbrowser
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.kdf.scrypt import Scrypt

APP_VERSION = "1.1.0"   # 1.1.0: export to a phone authenticator (otpauth-migration QR codes), port held exclusively. 1.0.1: new vault uses the system language
AUTO_LOCK_S = 300
HERE = os.path.dirname(os.path.abspath(sys.executable if getattr(sys, "frozen", False) else __file__))
BUNDLE = getattr(sys, "_MEIPASS", HERE)


# ── vault: AES-256-GCM, key = scrypt(master password) ─────────────────────────────────────────
class Vault:
    N, R, P = 2 ** 17, 8, 1          # scrypt cost (~0.5 s, 128 MB) - slows down password guessing on purpose

    def __init__(self, path):
        self.path = path
        self.key = None
        self.salt = None
        self.data = None             # {"accounts": [...], "wifi": {...}, "settings": {...}}

    def exists(self):
        return os.path.isfile(self.path)

    def _derive(self, password, salt):
        return Scrypt(salt=salt, length=32, n=self.N, r=self.R, p=self.P).derive(password.encode("utf-8"))

    def create(self, password):
        self.salt = secrets.token_bytes(16)
        self.key = self._derive(password, self.salt)
        self.data = {"accounts": [], "wifi": {"ssid": "", "pass": ""}, "settings": {"lang": system_lang(), "lastIdx": 0}}
        self.save()

    def unlock(self, password):
        with open(self.path, encoding="utf-8") as f:
            v = json.load(f)
        if v.get("format") != "ESPress2FA-vault":
            raise ValueError("not an ESPress2FA vault")
        salt = base64.b64decode(v["salt"])
        key = Scrypt(salt=salt, length=32, n=v["n"], r=v["r"], p=v["p"]).derive(password.encode("utf-8"))
        plain = AESGCM(key).decrypt(base64.b64decode(v["nonce"]), base64.b64decode(v["data"]), b"ESPress2FA")
        self.salt, self.key, self.data = salt, key, json.loads(plain.decode("utf-8"))   # wrong password -> InvalidTag

    def save(self):
        nonce = secrets.token_bytes(12)
        ct = AESGCM(self.key).encrypt(nonce, json.dumps(self.data, ensure_ascii=False).encode("utf-8"), b"ESPress2FA")
        out = {"format": "ESPress2FA-vault", "v": 1, "kdf": "scrypt", "n": self.N, "r": self.R, "p": self.P,
               "salt": base64.b64encode(self.salt).decode(), "nonce": base64.b64encode(nonce).decode(),
               "data": base64.b64encode(ct).decode()}
        tmp = self.path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as f:
            json.dump(out, f, indent=1)
        os.replace(tmp, self.path)                     # atomic: never a half-written vault

    def lock(self):
        self.key = None
        self.data = None

    @property
    def unlocked(self):
        return self.key is not None


def system_lang():
    """'de' if Windows / the system runs in German, else 'en' (used for a new vault)."""
    try:
        import ctypes, locale
        if hasattr(ctypes, "windll"):
            return "de" if (ctypes.windll.kernel32.GetUserDefaultUILanguage() & 0x3FF) == 0x07 else "en"
        return "de" if (locale.getlocale()[0] or "").lower().startswith("de") else "en"
    except Exception:
        return "en"


# ── TOTP (RFC 6238, SHA1, 6 digits, 30 s) ─────────────────────────────────────────────────────
def totp(secret_b32, t=None):
    s = "".join(c for c in secret_b32.upper() if c.isalnum())
    s += "=" * (-len(s) % 8)
    try:
        key = base64.b32decode(s)
    except Exception:
        return "------"
    counter = int((t or time.time()) // 30)
    h = hmac.new(key, struct.pack(">Q", counter), hashlib.sha1).digest()
    o = h[-1] & 0x0F
    return "%06d" % ((struct.unpack(">I", h[o:o + 4])[0] & 0x7FFFFFFF) % 1000000)


# ── export to a phone authenticator (Google Authenticator "transfer accounts" format) ─────────
# otpauth-migration://offline?data=<base64(MigrationPayload)>, protobuf:
#   MigrationPayload { repeated OtpParameters otp_parameters = 1; int32 version = 2; int32 batch_size = 3;
#                      int32 batch_index = 4; int32 batch_id = 5; }
#   OtpParameters    { bytes secret = 1; string name = 2; string issuer = 3; int32 algorithm = 4 (1 = SHA1);
#                      int32 digits = 5 (1 = six); int32 type = 6 (2 = TOTP); }
def _varint(n):
    out = bytearray()
    while True:
        b = n & 0x7F
        n >>= 7
        out.append(b | (0x80 if n else 0))
        if not n:
            return bytes(out)


def _field(no, wire, value):
    key = _varint((no << 3) | wire)
    if wire == 0:
        return key + _varint(value)
    return key + _varint(len(value)) + value            # wire 2: length-delimited


def migration_urls(accounts, per_code=8):
    """Accounts -> list of otpauth-migration URLs (several QR codes for many accounts, like the phone app)."""
    import urllib.parse
    usable = []
    for a in accounts:
        sec = "".join(c for c in a.get("secret", "").upper() if c.isalnum())
        try:
            raw = base64.b32decode(sec + "=" * (-len(sec) % 8))
        except Exception:
            continue
        usable.append((raw, a.get("name", ""), a.get("issuer", "")))
    batches = [usable[i:i + per_code] for i in range(0, len(usable), per_code)] or [[]]
    batch_id = secrets.randbits(31)
    urls = []
    for idx, batch in enumerate(batches):
        payload = b""
        for raw, name, issuer in batch:
            otp = (_field(1, 2, raw) + _field(2, 2, name.encode("utf-8")) + _field(3, 2, issuer.encode("utf-8"))
                   + _field(4, 0, 1) + _field(5, 0, 1) + _field(6, 0, 2))
            payload += _field(1, 2, otp)
        payload += _field(2, 0, 1) + _field(3, 0, len(batches)) + _field(4, 0, idx) + _field(5, 0, batch_id)
        data = base64.b64encode(payload).decode()
        urls.append(("otpauth-migration://offline?data=" + urllib.parse.quote(data, safe=""),
                     ["%s%s" % (n_i[2] + ": " if n_i[2] else "", n_i[1]) for n_i in batch]))
    return urls


def qr_svg(text):
    import qrcode, qrcode.image.svg
    q = qrcode.QRCode(error_correction=qrcode.constants.ERROR_CORRECT_L, box_size=10, border=4,
                      image_factory=qrcode.image.svg.SvgPathFillImage)
    q.add_data(text)
    q.make(fit=True)
    return q.make_image().to_string(encoding="unicode")


# ── web page: the device page + a small desktop add-on ────────────────────────────────────────
def load_device_page():
    for p in (os.path.join(BUNDLE, "web", "index.html"), os.path.join(HERE, "web", "index.html")):
        if os.path.isfile(p):
            return open(p, encoding="utf-8").read()
    src = os.path.join(HERE, "..", "include", "FUNC_WEBCONTENT.h")         # straight from the firmware source
    txt = open(src, encoding="utf-8").read()
    return txt[txt.index('R"rawhtml(') + len('R"rawhtml('):txt.index(')rawhtml"')]


DESKTOP_JS = r"""
<style>
#lockbtn{position:fixed;top:8px;right:12px;z-index:500;background:#0a0a0a;border:1px solid #0ff;color:#0ff;
  border-radius:4px;padding:4px 10px;font-family:'Courier New',monospace;font-size:13px;cursor:pointer}
#lockbtn:hover{background:#0ff;color:#000}
#m-code{cursor:pointer}
</style>
<button id="lockbtn" onclick="dkLock()" title="Lock / Sperren">🔒</button>
<script>
// ── ESPress2FA Desktop add-on ──
(function(){
  // hide device-only cards (brightness, rotation, firmware update, Wi-Fi reset)
  function hideDeviceCards(){
    ['br_title','flip_title','fw_title','wifi_title'].forEach(k=>{
      const e=document.querySelector('[data-t="'+k+'"]'); const c=e&&e.closest('.card'); if(c) c.style.display='none';
    });
  }
  document.addEventListener('DOMContentLoaded', hideDeviceCards); setTimeout(hideDeviceCards, 300);
  // locked (401) -> back to the unlock page
  const f0=window.fetch;
  window.fetch=function(){ return f0.apply(this,arguments).then(r=>{ if(r.status===401) location.reload(); return r; }); };
  // auto-lock after inactivity
  let last=Date.now();
  ['mousemove','keydown','click','wheel','touchstart'].forEach(ev=>document.addEventListener(ev,()=>{last=Date.now();},{passive:true}));
  setInterval(()=>{ if(Date.now()-last > __AUTOLOCK__*1000) dkLock(); }, 5000);
  // click on the big code = copy, clipboard cleared after 30 s
  document.addEventListener('click', e=>{
    const el=e.target.closest('#m-code'); if(!el) return;
    const code=el.textContent.replace(/\s/g,''); if(!/^\d{6}$/.test(code)) return;
    navigator.clipboard.writeText(code).then(()=>{
      if(window.toast) toast((document.documentElement.lang==='de'?'📋 Kopiert – wird in 30 s gelöscht':'📋 Copied – cleared in 30 s'));
      setTimeout(()=>navigator.clipboard.readText().then(t=>{ if(t===code) navigator.clipboard.writeText(''); }).catch(()=>navigator.clipboard.writeText('')),30000);
    }).catch(()=>{});
  });
})();
function dkLock(){ fetch('/api/lock',{method:'POST'}).finally(()=>location.reload()); }

// ── Export to a phone authenticator: QR codes (several pages for many accounts) ──
const QT={en:{title:'📲 Export to phone (Authenticator)',hint:'Creates QR codes in the Google Authenticator transfer format (up to 8 accounts per code). On the phone: Authenticator → Transfer accounts → Import accounts → scan the codes one after another.',
  only:'only active accounts',btn:'📲 Show QR codes',warn:'The QR codes contain ALL secrets of these accounts. Make sure nobody (and no camera) can see your screen. Show now?',
  page:'Code {0} of {1}',copy:'📋 Copy text',copied:'📋 Copied – paste it into the import field of a device',prev:'◀ Back',next:'Next ▶',close:'✕ Close',none:'No accounts to export'},
 de:{title:'📲 Export aufs Handy (Authenticator)',hint:'Erzeugt QR-Codes im Übertragungsformat des Google Authenticators (bis zu 8 Konten je Code). Am Handy: Authenticator → Konten übertragen → Konten importieren → die Codes nacheinander scannen.',
  only:'nur aktive Konten',btn:'📲 QR-Codes anzeigen',warn:'Die QR-Codes enthalten ALLE Geheimnisse dieser Konten. Achte darauf, dass niemand (und keine Kamera) deinen Bildschirm sieht. Jetzt anzeigen?',
  page:'Code {0} von {1}',copy:'📋 Text kopieren',copied:'📋 Kopiert – ins Import-Feld eines Geräts einfügen',prev:'◀ Zurück',next:'Weiter ▶',close:'✕ Schließen',none:'Keine Konten zum Exportieren'}};
function qt(k,...a){const L=document.documentElement.lang==='de'?'de':'en';let s=QT[L][k]||QT.en[k];a.forEach((v,i)=>s=s.replace('{'+i+'}',v));return s;}
let qrPages=[],qrIdx=0;
function qrCard(){
  const tab=document.getElementById('tab-settings'); if(!tab) return;
  let c=document.getElementById('dk-qr-card');
  if(!c){ c=document.createElement('div'); c.className='card'; c.id='dk-qr-card';
    const anchor=[...tab.querySelectorAll('.card')].find(x=>x.querySelector('[data-t="rs_title"]'));
    anchor ? anchor.after(c) : tab.appendChild(c); }
  c.innerHTML='<div class="ct">'+qt('title')+'</div><p class="hint">'+qt('hint')+'</p>'+
    '<label class="radio" style="font-size:13px"><input type="checkbox" id="dk-qr-act" checked> '+qt('only')+'</label>'+
    '<button class="btn bb" onclick="qrShow()">'+qt('btn')+'</button>';
}
function qrShow(){
  if(!confirm(qt('warn'))) return;
  fetch('/api/export_qr?active='+(document.getElementById('dk-qr-act').checked?1:0)).then(r=>r.json()).then(d=>{
    if(!d.pages||!d.accounts){ if(window.toast) toast(qt('none'),false); return; }
    qrPages=d.pages; qrIdx=0; qrDraw();
  });
}
function qrDraw(){
  let m=document.getElementById('dk-qr-modal');
  if(!m){ m=document.createElement('div'); m.id='dk-qr-modal';
    m.style.cssText='position:fixed;inset:0;background:rgba(0,0,0,.92);z-index:600;display:flex;align-items:center;justify-content:center';
    document.body.appendChild(m); }
  const p=qrPages[qrIdx];
  m.innerHTML='<div style="background:#0d0d0d;border:2px solid #0ff;border-radius:10px;padding:18px;max-width:94vw;text-align:center">'+
    '<div style="color:#0ff;font-size:15px;margin-bottom:10px">'+qt('page',qrIdx+1,qrPages.length)+'</div>'+
    '<div style="background:#fff;padding:10px;border-radius:6px;width:min(72vh,86vw);margin:0 auto">'+p.svg.replace(/<svg /,'<svg style="width:100%;height:auto;display:block" ')+'</div>'+
    '<div style="color:#888;font-size:11px;margin:10px 0;max-height:80px;overflow:auto">'+p.names.map(n=>n.replace(/[<>&]/g,'')).join(' · ')+'</div>'+
    '<textarea id="dk-qr-url" readonly rows="3" style="width:100%;margin:0 0 8px;font-size:10px" onclick="this.select()"></textarea>'+
    '<button class="btn bg" onclick="qrCopy()">'+qt('copy')+'</button>'+
    '<button class="btn bb" '+(qrIdx===0?'disabled':'')+' onclick="qrIdx--;qrDraw()">'+qt('prev')+'</button>'+
    '<button class="btn bb" '+(qrIdx>=qrPages.length-1?'disabled':'')+' onclick="qrIdx++;qrDraw()">'+qt('next')+'</button>'+
    '<button class="btn br" onclick="qrClose()">'+qt('close')+'</button></div>';
  document.getElementById('dk-qr-url').value=p.url;
}
function qrCopy(){
  const t=document.getElementById('dk-qr-url'); t.select();
  navigator.clipboard.writeText(t.value).then(()=>{ if(window.toast) toast(qt('copied')); })
    .catch(()=>{ document.execCommand('copy'); if(window.toast) toast(qt('copied')); });
}
function qrClose(){ const m=document.getElementById('dk-qr-modal'); if(m) m.remove(); qrPages=[]; }
document.addEventListener('keydown',e=>{ if(!document.getElementById('dk-qr-modal')) return;
  if(e.key==='Escape') qrClose(); if(e.key==='ArrowRight'&&qrIdx<qrPages.length-1){qrIdx++;qrDraw();} if(e.key==='ArrowLeft'&&qrIdx>0){qrIdx--;qrDraw();} });
setTimeout(qrCard,400);
// rebuild the card in the right language whenever the language changes
new MutationObserver(()=>qrCard()).observe(document.documentElement,{attributes:true,attributeFilter:['lang']});
</script>
</body>"""

UNLOCK_PAGE = r"""<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESPress2FA Desktop</title><style>
*{box-sizing:border-box}body{background:#000;color:#ccc;font-family:'Courier New',monospace;margin:0;padding:40px 18px}
.c{max-width:400px;margin:0 auto;background:#0a0a0a;border:1px solid #0ff;border-radius:8px;padding:22px}
h1{color:#0ff;font-size:18px;margin:0 0 6px}.s{color:#555;font-size:11px;margin-bottom:16px}
label{display:block;font-size:11px;color:#666;margin:12px 0 4px;text-transform:uppercase}
input{width:100%;padding:10px;background:#111;border:1px solid #333;color:#fff;border-radius:4px;font-family:inherit;font-size:15px}
input:focus{border-color:#0ff;outline:none}
button{margin-top:18px;width:100%;padding:11px;background:#0a0;color:#000;border:0;border-radius:4px;font-weight:bold;font-size:14px;cursor:pointer}
.e{color:#f55;font-size:12px;min-height:16px;margin-top:10px}.h{font-size:11px;color:#555;margin-top:14px;line-height:1.5}
</style></head><body><div class="c">
<h1>🔐 ESPress2FA Desktop</h1><div class="s" id="sub"></div>
<form onsubmit="go();return false">
<label id="l1"></label><input id="p1" type="password" autofocus autocomplete="current-password">
<div id="r2" style="display:none"><label id="l2"></label><input id="p2" type="password" autocomplete="new-password"></div>
<button id="b"></button></form><div class="e" id="err"></div><div class="h" id="hint"></div></div>
<script>
const NEW=__NEW__, DE=(navigator.language||'').toLowerCase().startsWith('de');
const T=DE?{sub:'Tresor: __VAULT__',l1:'Master-Passwort',l2:'Wiederholen',open:'🔓 Entsperren',create:'🔐 Tresor anlegen',
  wrong:'Falsches Passwort',diff:'Passwörter sind verschieden',short:'Mindestens 8 Zeichen',
  hnew:'Neuer Tresor. Das Master-Passwort kann NICHT wiederhergestellt werden – ohne es sind die Daten verloren. Danach im Tab Einstellungen die Sicherung vom Gerät einspielen.',
  hopen:'Alles bleibt auf diesem PC (nur 127.0.0.1). Sperrt sich nach __AL__ Minuten ohne Benutzung.'}
 :{sub:'Vault: __VAULT__',l1:'Master password',l2:'Repeat',open:'🔓 Unlock',create:'🔐 Create vault',
  wrong:'Wrong password',diff:'Passwords differ',short:'At least 8 characters',
  hnew:'New vault. The master password can NOT be recovered – without it the data is lost. Then restore the device backup in the Settings tab.',
  hopen:'Everything stays on this PC (127.0.0.1 only). Locks itself after __AL__ minutes without use.'};
document.getElementById('sub').textContent=T.sub; document.getElementById('l1').textContent=T.l1;
document.getElementById('l2').textContent=T.l2; document.getElementById('b').textContent=NEW?T.create:T.open;
document.getElementById('hint').textContent=NEW?T.hnew:T.hopen; if(NEW) document.getElementById('r2').style.display='';
function go(){
  const p1=document.getElementById('p1').value, p2=document.getElementById('p2').value, err=document.getElementById('err');
  if(NEW && p1.length<8){err.textContent=T.short;return} if(NEW && p1!==p2){err.textContent=T.diff;return}
  err.textContent='…';
  fetch('/unlock',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({password:p1,create:NEW})})
   .then(r=>{ if(r.ok) location.reload(); else { err.textContent=T.wrong; document.getElementById('p1').select(); } });
}
</script></body></html>"""


# ── HTTP server implementing the device API ───────────────────────────────────────────────────
class App:
    def __init__(self, vault_path):
        self.vault = Vault(vault_path)
        self.page = load_device_page()
        self.lock = threading.Lock()
        self.last_api = time.time()

    def accounts(self):
        return self.vault.data["accounts"]


def make_handler(app):
    class H(BaseHTTPRequestHandler):
        server_version = "ESPress2FA-Desktop/" + APP_VERSION

        def log_message(self, *a):
            pass

        def _send(self, code, body, ctype="application/json", extra=None):
            b = body.encode("utf-8") if isinstance(body, str) else body
            self.send_response(code)
            self.send_header("Content-Type", ctype + ("; charset=utf-8" if ctype.startswith(("text", "application/json")) else ""))
            self.send_header("Content-Length", str(len(b)))
            self.send_header("Cache-Control", "no-store")
            for k, v in (extra or {}).items():
                self.send_header(k, v)
            self.end_headers()
            self.wfile.write(b)

        def _json(self, code, obj):
            self._send(code, json.dumps(obj, ensure_ascii=False))

        def _body(self):
            n = int(self.headers.get("Content-Length") or 0)
            return self.rfile.read(n) if n else b""

        def _form(self):
            return {k: v[0] for k, v in parse_qs(self._body().decode("utf-8", "replace")).items()}

        # ---- routing ----
        def do_GET(self):
            self._route("GET")

        def do_POST(self):
            self._route("POST")

        def do_DELETE(self):
            self._route("DELETE")

        def _route(self, method):
            u = urlparse(self.path)
            path, q = u.path, {k: v[0] for k, v in parse_qs(u.query).items()}
            v = app.vault
            if path == "/" and method == "GET":
                if not v.unlocked:
                    page = UNLOCK_PAGE.replace("__NEW__", "false" if v.exists() else "true") \
                        .replace("__VAULT__", os.path.basename(v.path)).replace("__AL__", str(AUTO_LOCK_S // 60))
                    return self._send(200, page, "text/html")
                return self._send(200, app.page.replace("</body>", DESKTOP_JS.replace("__AUTOLOCK__", str(AUTO_LOCK_S)), 1), "text/html")
            if path == "/unlock" and method == "POST":
                try:
                    d = json.loads(self._body() or b"{}")
                    with app.lock:
                        if d.get("create") and not v.exists():
                            if len(d.get("password", "")) < 8:
                                return self._json(400, {"ok": False})
                            v.create(d["password"])
                        else:
                            v.unlock(d.get("password", ""))
                    return self._json(200, {"ok": True})
                except Exception:
                    time.sleep(1.0)                                   # slow down guessing a bit more
                    return self._json(403, {"ok": False})
            if not path.startswith("/api/"):
                return self._send(404, "Not found", "text/plain")
            if not v.unlocked:
                return self._send(401, "locked", "text/plain")
            with app.lock:
                return self._api(method, path, q)

        def _api(self, method, path, q):
            v = app.vault
            acc = app.accounts()
            st = v.data.setdefault("settings", {})
            if path == "/api/lock":
                v.lock()
                return self._send(200, "OK", "text/plain")
            if path == "/api/status":
                now = datetime.datetime.now()
                return self._json(200, {
                    "time": now.strftime("%H:%M:%S"), "date": now.strftime("%Y-%m-%d ") + now.strftime("%a").upper()[:3],
                    "ssid": "PC", "ip": "127.0.0.1", "bright": 0, "total": len(acc),
                    "active": sum(1 for a in acc if a.get("active", True)), "version": APP_VERSION, "board": "Desktop",
                    "lang": st.get("lang", "en"), "flip": False, "flip_s": 3, "has_bat": False, "bat_mv": 0, "bat_pct": -1})
            if path == "/api/accounts" and method == "GET":
                return self._json(200, [{"id": i, "name": a.get("name", ""), "issuer": a.get("issuer", ""),
                                         "active": a.get("active", True)} for i, a in enumerate(acc)])
            if path == "/api/code":
                i = int(q.get("i", -1))
                if not 0 <= i < len(acc):
                    return self._send(404, "not found", "text/plain")
                a = acc[i]
                st["lastIdx"] = i
                return self._json(200, {"code": totp(a.get("secret", "")), "remaining": 30 - int(time.time()) % 30,
                                        "issuer": a.get("issuer", ""), "name": a.get("name", "")})
            if path == "/api/accounts/import" and method == "POST":
                arr = json.loads(self._body() or b"[]")
                added = 0
                for o in arr:
                    if len(acc) >= 200:
                        break
                    acc.append({"issuer": o.get("issuer", ""), "name": o.get("name", ""),
                                "secret": o.get("secret", ""), "active": o.get("active", True)})
                    added += 1
                v.save()
                return self._json(200, {"added": added, "total": len(acc)})
            if path == "/api/accounts/reorder" and method == "POST":
                order = json.loads(self._body() or b"[]")
                new = [acc[i] for i in order if isinstance(i, int) and 0 <= i < len(acc)]
                if len(new) == len(acc):
                    v.data["accounts"] = new
                    v.save()
                return self._send(200, "OK", "text/plain")
            if path.startswith("/api/accounts/"):
                parts = path.split("/")
                try:
                    i = int(parts[3])
                except (IndexError, ValueError):
                    return self._send(404, "Not found", "text/plain")
                if not 0 <= i < len(acc):
                    return self._send(404, "Not found", "text/plain")
                if method == "DELETE":
                    del acc[i]
                elif path.endswith("/toggle"):
                    acc[i]["active"] = not acc[i].get("active", True)
                elif path.endswith("/edit"):
                    d = json.loads(self._body() or b"{}")
                    if isinstance(d.get("issuer"), str):
                        acc[i]["issuer"] = d["issuer"][:31]
                    if isinstance(d.get("name"), str):
                        acc[i]["name"] = d["name"][:63]
                else:
                    return self._send(404, "Not found", "text/plain")
                v.save()
                return self._send(200, "OK", "text/plain")
            if path == "/api/backup":
                fn = datetime.datetime.now().strftime("ESPress2FA_Desktop_%Y-%m-%d_%H%M.json")
                out = {"device": "2FA Clock Desktop", "version": APP_VERSION, "mac": "",
                       "created": datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                       "settings": {"bright": 128, "lastIdx": st.get("lastIdx", 0)},
                       "wifi": v.data.get("wifi", {"ssid": "", "pass": ""}), "accounts": acc}
                return self._send(200, json.dumps(out, ensure_ascii=False, indent=1), "application/json",
                                  {"Content-Disposition": 'attachment; filename="%s"' % fn})
            if path == "/api/restore" and method == "POST":
                try:
                    d = json.loads(self._body())
                except ValueError:
                    return self._send(400, "File is not valid JSON", "text/plain")
                mode = q.get("mode", "nowifi")
                if not str(d.get("device", "")).startswith("2FA Clock"):
                    return self._send(400, "not a 2FA Clock backup", "text/plain")
                n = 0
                if mode in ("all", "nowifi"):
                    v.data["accounts"] = [{"issuer": a.get("issuer", ""), "name": a.get("name", ""),
                                           "secret": a.get("secret", ""), "active": a.get("active", True)}
                                          for a in d.get("accounts", [])]
                    n = len(v.data["accounts"])
                if mode in ("all", "wifi") and d.get("wifi", {}).get("ssid"):
                    v.data["wifi"] = {"ssid": d["wifi"]["ssid"], "pass": d["wifi"].get("pass", "")}   # kept only to export it again
                v.save()
                return self._json(200, {"accounts": n, "restart": False})
            if path == "/api/export_qr":                     # desktop only: QR codes for the phone app
                only_active = q.get("active", "1") == "1"
                sel = [a for a in acc if a.get("active", True) or not only_active]
                pages = [{"svg": qr_svg(u), "names": names, "url": u} for u, names in migration_urls(sel)]
                return self._json(200, {"pages": pages, "accounts": len(sel)})
            if path == "/api/lang" and method == "POST":
                st["lang"] = "de" if self._form().get("v") == "de" else "en"
                v.save()
                return self._send(200, "OK", "text/plain")
            # device-only functions (brightness, rotation, OTA, Wi-Fi reset ...) do nothing here
            self._body()
            return self._send(200, "OK", "text/plain")

    return H


class LocalServer(ThreadingHTTPServer):
    """127.0.0.1 only, port held EXCLUSIVELY: http.server allows address reuse by default, and on Windows
    that lets a second program bind the same port and take over the requests."""
    allow_reuse_address = False
    daemon_threads = True

    def server_bind(self):
        if hasattr(socket, "SO_EXCLUSIVEADDRUSE"):
            self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_EXCLUSIVEADDRUSE, 1)
        super().server_bind()


def free_port(pref):
    for p in (pref, 0):
        try:
            s = socket.socket()
            s.bind(("127.0.0.1", p))
            port = s.getsockname()[1]
            s.close()
            return port
        except OSError:
            pass
    return 0


def main():
    ap = argparse.ArgumentParser(description="ESPress2FA Desktop")
    ap.add_argument("--vault", default=os.path.join(HERE, "ESPress2FA.vault"), help="vault file (default: next to the program)")
    ap.add_argument("--port", type=int, default=41901)
    ap.add_argument("--browser", action="store_true", help="use the default browser instead of an app window")
    ap.add_argument("--serve-only", action="store_true", help="only run the local server (no window, no browser)")
    a = ap.parse_args()

    app = App(os.path.abspath(a.vault))
    port = free_port(a.port)
    srv = LocalServer(("127.0.0.1", port), make_handler(app))       # 127.0.0.1 only - never reachable from the network
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    url = "http://127.0.0.1:%d/" % port
    print("ESPress2FA Desktop %s  -  %s  -  vault: %s" % (APP_VERSION, url, app.vault.path))

    if a.serve_only:
        try:
            while True:
                time.sleep(1)
        except KeyboardInterrupt:
            srv.shutdown()
        return
    if not a.browser:
        try:
            import webview
            webview.create_window("ESPress2FA Desktop", url, width=900, height=760, min_size=(420, 500))
            webview.start()
            app.vault.lock()
            srv.shutdown()
            return
        except Exception as e:
            print("App window not available (%s) - using the browser" % e)
    webbrowser.open(url)
    print("Close with Ctrl+C")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        app.vault.lock()
        srv.shutdown()


if __name__ == "__main__":
    main()
