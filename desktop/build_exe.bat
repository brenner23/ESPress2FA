@echo off
rem ESPress2FA Desktop - single EXE (no installation, runs from a USB stick)
rem 1) copy the web interface out of the firmware source  2) PyInstaller one-file build
cd /d "%~dp0"
python -c "s=open('../include/FUNC_WEBCONTENT.h',encoding='utf-8').read();import os;os.makedirs('web',exist_ok=True);open('web/index.html','w',encoding='utf-8').write(s[s.index('R\"rawhtml(')+10:s.index(')rawhtml\"')])"
python -m PyInstaller --noconfirm --onefile --noconsole --name ESPress2FA-Desktop --icon espress2fa.ico --add-data "web/index.html;web" --hidden-import qrcode.image.svg --exclude-module PIL --exclude-module numpy espress2fa_desktop.py
echo.
echo Done: dist\ESPress2FA-Desktop.exe
