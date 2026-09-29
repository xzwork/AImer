@echo off
setlocal
echo DEBUG TEST DRIVER ONLY - not the normal Windows release.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0VirtualHid.Debug.ps1" -Action Start
exit /b %errorlevel%
