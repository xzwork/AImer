@echo off
setlocal
set "AIMER_INPUT_BACKEND=Win32"
set "AIMER_INPUT_TRACE=1"
if exist "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.8\bin" set "PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.8\bin;%PATH%"
cd /d "%~dp0"
echo AImer local mode: native Windows SendInput. No Virtual HID driver required.
AImer.exe
if errorlevel 1 pause
