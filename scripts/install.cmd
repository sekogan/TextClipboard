@echo off

set EXE=text_clipboard.exe
set EXEFULLPATH=%~dp0%EXE%

if not exist "%EXEFULLPATH%" (
    echo Cannot find %EXE%
    goto Abort
)
taskkill /IM %EXE% >NUL 2>&1
start "" /B "%EXEFULLPATH%"
reg add HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Run /v TextClipboard /d "%EXEFULLPATH%" /f
if errorlevel 1 goto Abort
echo Done!
exit /b 0

:Abort
call uninstall
echo Installation aborted.
exit /b 1
