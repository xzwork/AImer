@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0VirtualHid.Debug.ps1" -Action EnableTestMode
exit /b %errorlevel%
