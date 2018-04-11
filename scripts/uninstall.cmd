@echo off

taskkill /IM text_clipboard.exe >NUL 2>&1
reg delete HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Run /v TextClipboard /f >NUL 2>&1

echo Uninstalled.
