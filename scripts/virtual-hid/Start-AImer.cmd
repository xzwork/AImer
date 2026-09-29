@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0VirtualHid.ps1" -Action Start
exit /b %errorlevel%
