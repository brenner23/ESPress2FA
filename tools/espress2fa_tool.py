#!/usr/bin/env python3
"""espress2fa_tool.py - 2FA Clock T190: backup / restore over USB (serial JSON commands, firmware >= 1.1.0)

Usage:
    python espress2fa_tool.py                          menu
    python espress2fa_tool.py info    [--port COMx]
    python espress2fa_tool.py backup  [--port COMx] [file.json]
    python espress2fa_tool.py restore [--port COMx] file.json --mode all|nowifi|wifi
    python espress2fa_tool.py getkey  [--port COMx]      remember the device's OTA key (USB, once per device)
    python espress2fa_tool.py find                       broadcast: list all 2FA Clocks in the network
    python espress2fa_tool.py ota firmware.bin [--ip X | --mac M]   firmware update over the network

The backup always contains everything (accounts with secrets, settings, WiFi).
Restore modes like a Fritzbox: all = everything, nowifi = accounts + settings, wifi = only WiFi.
Needs: pip install pyserial
"""
import argparse, datetime, json, os, sys, time

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("pyserial missing: pip install pyserial")

ESP_VID = 0x303A


_conn = {}   # port -> open serial (opening the port may restart the board, so keep it open)


def _open(port, wait=45):
    """Open the port once; if the board restarted, wait until it answers {"cmd":"info"}."""
    if port in _conn:
        return _conn[port]
    # RTS low + DTR high: the board does NOT restart, and it sends answers (DTR = "terminal connected")
    s = serial.Serial()
    s.port, s.baudrate, s.timeout, s.write_timeout = port, 115200, 0.2, 2
    s.dtr = False
    s.rts = False
    s.open()
    s.dtr = True
    time.sleep(0.2)
    _conn[port] = s
    t0 = time.time()
    while time.time() - t0 < wait:
        try:
            r = _send(s, {"cmd": "info"}, timeout=2)
            if r.get("device"):
                return s
        except (TimeoutError, serial.SerialTimeoutException):
            pass
        print("  waiting for the device ...", end="\r")
    raise TimeoutError("no answer from the device")


def _send(s, obj, timeout):
    line = (json.dumps(obj, ensure_ascii=False, separators=(",", ":")) + "\n").encode("utf-8")
    s.reset_input_buffer()
    for i in range(0, len(line), 32):   # USB-CDC drops characters on long lines -> small chunks
        s.write(line[i:i + 32])
        s.flush()
        time.sleep(0.02)
    # The answer is "@2FA {json}". Log lines of the Arduino core may cut it into pieces
    # -> search the whole received text for "@2FA" and decode the JSON object that follows.
    dec = json.JSONDecoder()
    buf, t0 = "", time.time()
    while time.time() - t0 < timeout:
        buf += s.read(4096).decode("utf-8", "replace")
        i = buf.find("@2FA")
        while i >= 0:
            rest = buf[i + 4:]
            j = rest.find("{")
            if j >= 0:
                try:
                    obj, _ = dec.raw_decode(rest[j:])
                    return obj
                except ValueError:
                    # maybe a log line sits inside the JSON: drop "[  123][E]..." lines and retry
                    cleaned = "\n".join(l for l in rest[j:].splitlines() if not l.lstrip().startswith("["))
                    try:
                        obj, _ = dec.raw_decode(cleaned.replace("\n", ""))
                        return obj
                    except ValueError:
                        pass
            nxt = buf.find("@2FA", i + 4)
            if nxt < 0:
                break
            i = nxt
    raise TimeoutError("no answer from the device")


def find_port():
    ports = [p for p in list_ports.comports() if p.vid == ESP_VID]
    for p in ports:
        try:
            r = command(p.device, {"cmd": "info"})
            if str(r.get("device", "")).startswith("2FA Clock"):
                return p.device
        except Exception:
            pass
    sys.exit("No 2FA Clock found on USB (ESP32-S3 ports: %s)" % (", ".join(p.device for p in ports) or "-"))


def command(port, obj, timeout=10):
    return _send(_open(port), obj, timeout)


def do_info(port):
    r = command(port, {"cmd": "info"})
    print("Device  :", r.get("device"), "v" + str(r.get("version")))
    print("IP      :", r.get("ip"), " WiFi:", r.get("ssid"))
    print("Accounts:", r.get("accounts"))


def do_backup(port, path=None):
    r = command(port, {"cmd": "backup"}, timeout=15)
    if not r.get("ok"):
        sys.exit("Backup failed: %s" % r.get("error"))
    data = r["data"]
    if not path:
        path = "2FA-T190_Sicherung_%s.json" % datetime.datetime.now().strftime("%Y-%m-%d_%H%M")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)
    print("Backup saved: %s (%d accounts, WiFi %s)" % (path, len(data.get("accounts", [])),
                                                      "yes" if data.get("wifi", {}).get("ssid") else "no"))


def do_restore(port, path, mode):
    with open(path, encoding="utf-8") as f:
        data = json.load(f)
    r = command(port, {"cmd": "restore", "mode": mode, "data": data}, timeout=15)
    if not r.get("ok"):
        sys.exit("Restore failed: %s" % r.get("error"))
    print("Restored (%s): %s accounts%s" % (mode, r.get("accounts"), ", device restarts" if r.get("restart") else ""))


def menu():
    port = find_port()
    print("2FA Clock on", port)
    do_info(port)
    while True:
        print("\n1 = Backup   2 = Restore (all)   3 = Restore without WiFi   4 = Restore only WiFi   0 = Exit")
        c = input("> ").strip()
        if c == "1":
            do_backup(port)
        elif c in ("2", "3", "4"):
            files = sorted([f for f in os.listdir(".") if f.lower().endswith(".json")], reverse=True)
            for i, f in enumerate(files):
                print(" %d) %s" % (i + 1, f))
            sel = input("File number or path: ").strip()
            path = files[int(sel) - 1] if sel.isdigit() and 0 < int(sel) <= len(files) else sel
            do_restore(port, path, {"2": "all", "3": "nowifi", "4": "wifi"}[c])
        elif c == "0":
            return


# ── Network: discovery (UDP broadcast), per-device OTA key, firmware update ──
KEYS_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ota_keys.json")   # local only, never share
DISCOVERY_PORT = 41900


def load_keys():
    try:
        with open(KEYS_FILE, encoding="utf-8") as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def do_find(timeout=2.0, quiet=False):
    """Broadcast "2FA-DISCOVER" - every 2FA Clock in the network answers with its JSON."""
    import socket
    # PCs with several networks (Wi-Fi/LAN/VPN/VirtualBox): Windows sends a broadcast out of ONE of them only
    # -> one socket per local IPv4 address, each calls on its own network (plus subnet broadcast x.y.z.255)
    ips = sorted({i[4][0] for i in socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET)
                  if not i[4][0].startswith("127.")})
    socks = []
    for ip in ips:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            s.bind((ip, 0))
            s.setblocking(False)
            socks.append((s, ip.rsplit(".", 1)[0] + ".255"))
        except OSError:
            pass
    found, t0, nxt = {}, time.time(), 0.0
    while time.time() - t0 < timeout:
        if time.time() - t0 >= nxt:                       # repeat a few times (Wi-Fi power save may drop one)
            nxt += 0.5
            for s, bc in socks:
                for target in ("255.255.255.255", bc):
                    try:
                        s.sendto(b"2FA-DISCOVER", (target, DISCOVERY_PORT))
                    except OSError:
                        pass
        for s, _ in socks:
            try:
                data, addr = s.recvfrom(1024)
                d = json.loads(data.decode("utf-8", "replace"))
                found[d.get("mac", addr[0])] = d
            except (BlockingIOError, OSError, ValueError):
                pass
        time.sleep(0.02)
    for s, _ in socks:
        s.close()
    keys = load_keys()
    if not quiet:
        if not found:
            print("No 2FA Clock answered.")
        for d in found.values():
            print("%-15s %-13s v%-6s %s  %s  accounts %s  battery %s%%  key %s" % (
                d.get("ip"), d.get("board"), d.get("version"), d.get("mac"), d.get("host"),
                d.get("accounts"), d.get("bat"), "known" if d.get("mac") in keys else "-"))
    return list(found.values())


def do_getkey(port):
    """Read the device's OTA key over USB and remember it (by MAC) in ota_keys.json."""
    r = command(port, {"cmd": "getkey"})
    if not r.get("ok"):
        sys.exit("getkey failed: %s" % r.get("error"))
    keys = load_keys()
    keys[r["mac"]] = {"key": r["key"], "board": r.get("board")}
    with open(KEYS_FILE, "w", encoding="utf-8") as f:
        json.dump(keys, f, indent=2)
    print("OTA key of %s (%s) saved in %s" % (r["mac"], r.get("board"), KEYS_FILE))


def _http(ip, path, key, body=None, ctype=None, extra=None, timeout=10):
    import urllib.request
    req = urllib.request.Request("http://%s%s" % (ip, path), data=body if body is not None else b"", method="POST")
    req.add_header("X-OTA-Key", key)
    if ctype:
        req.add_header("Content-Type", ctype)
    for k, v in (extra or {}).items():
        req.add_header(k, v)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, r.read().decode("utf-8", "replace")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8", "replace")


def _pick(ip, mac):
    devs = do_find(quiet=True)
    for d in devs:
        if (ip and d.get("ip") == ip) or (mac and d.get("mac", "").lower() == mac.lower()):
            return d
    if not ip and not mac and len(devs) == 1:
        return devs[0]
    sys.exit("Device not found - use --ip or --mac (found: %s)" % ", ".join(d.get("ip", "?") for d in devs))


def do_ota(binfile, ip=None, mac=None):
    d = _pick(ip, mac)
    key = load_keys().get(d["mac"], {}).get("key")
    if not key:
        sys.exit("No OTA key for %s - run 'getkey' once over USB" % d["mac"])
    st, txt = _http(d["ip"], "/api/otacheck", key)      # is it the right device / right key?
    if st != 200:
        sys.exit("Key rejected by %s (%s) - wrong device?" % (d["ip"], d["mac"]))
    data = open(binfile, "rb").read()
    boundary = "----2fa%d" % int(time.time())
    body = (("--%s\r\nContent-Disposition: form-data; name=\"firmware\"; filename=\"%s\"\r\n"
             "Content-Type: application/octet-stream\r\n\r\n") % (boundary, os.path.basename(binfile))).encode() \
        + data + ("\r\n--%s--\r\n" % boundary).encode()
    print("Uploading %s (%d KB) to %s %s v%s ..." % (os.path.basename(binfile), len(data) // 1024,
                                                     d["board"], d["ip"], d.get("version")))
    st, txt = _http(d["ip"], "/update", key, body, "multipart/form-data; boundary=" + boundary,
                    {"X-FW-Size": str(len(data))}, timeout=180)
    print("HTTP %d: %s" % (st, txt.strip()))
    if st != 200:
        sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description="2FA Clock: backup/restore over USB, find + OTA over network")
    ap.add_argument("action", nargs="?", choices=["info", "backup", "restore", "find", "getkey", "ota"])
    ap.add_argument("file", nargs="?")
    ap.add_argument("--port")
    ap.add_argument("--ip")
    ap.add_argument("--mac")
    ap.add_argument("--mode", default="nowifi", choices=["all", "nowifi", "wifi"])
    a = ap.parse_args()
    if not a.action:
        return menu()
    if a.action == "find":
        return do_find()
    if a.action == "ota":
        if not a.file:
            sys.exit("ota needs a firmware .bin")
        return do_ota(a.file, a.ip, a.mac)
    port = a.port or find_port()
    if a.action == "getkey":
        return do_getkey(port)
    if a.action == "info":
        do_info(port)
    elif a.action == "backup":
        do_backup(port, a.file)
    else:
        if not a.file:
            sys.exit("restore needs a file")
        do_restore(port, a.file, a.mode)


if __name__ == "__main__":
    main()
