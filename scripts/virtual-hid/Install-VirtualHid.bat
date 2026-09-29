@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0VirtualHid.ps1" -Action Install
exit /b %errorlevel%
