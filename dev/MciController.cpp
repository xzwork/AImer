#include "MciController.hpp"
#include <iostream>
#include <ctime>

#define MCI_LOCAL_DEVICE_PATH_U L"\\\\.\\MouClassInputInjection"

MciController &MciController::getInstance() {
    static MciController instance;
    return instance;
}

MciController::MciController() {
    if (!_initializeDevice()) {
        return;
    }
    if (!_initializeStackContext()) {
        _stackInitialized = false;
    }
}

MciController::~MciController() {
    if (_deviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(_deviceHandle);
        _deviceHandle = INVALID_HANDLE_VALUE;
    }
}

bool MciController::isAvailable() const {
    return _deviceHandle != INVALID_HANDLE_VALUE && _stackInitialized;
}

bool MciController::_initializeDevice() {
    _deviceHandle = CreateFileW(
        MCI_LOCAL_DEVICE_PATH_U,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (_deviceHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    return true;
}

bool MciController::_initializeStackContext() {
    DWORD cbReturned = 0;
    const BOOL status = DeviceIoControl(
        _deviceHandle,
        IOCTL_INITIALIZE_MOUSE_DEVICE_STACK_CONTEXT,
        nullptr,
        0,
        &_stackInfo,
        sizeof(_stackInfo),
        &cbReturned,
        nullptr);
    if (!status) {
        std::cerr << "MCII: IOCTL_INITIALIZE_MOUSE_DEVICE_STACK_CONTEXT failed: "
                  << GetLastError() << std::endl;
        return false;
    }
    if (cbReturned < sizeof(_stackInfo)) {
        std::cerr << "MCII: init reply too small: " << cbReturned
                  << " < " << sizeof(_stackInfo) << std::endl;
        return false;
    }
    std::cout << "MCII: ButtonDevice.UnitId=" << _stackInfo.ButtonDevice.UnitId
              << " MovementDevice.UnitId=" << _stackInfo.MovementDevice.UnitId
              << " AbsoluteMovement=" << (int)_stackInfo.MovementDevice.AbsoluteMovement
              << " VirtualDesktop=" << (int)_stackInfo.MovementDevice.VirtualDesktop
              << std::endl;
    _stackInitialized = true;
    return true;
}

void MciController::MoveRelative(const int dx, const int dy) const {
    if (!isAvailable()) return;

    MCI_INJECT_MOUSE_MOVEMENT_INPUT_REQUEST request{};
    request.ProcessId = GetCurrentProcessId();
    request.IndicatorFlags = _stackInfo.MovementDevice.VirtualDesktop ? MOUSE_VIRTUAL_DESKTOP : 0;
    request.MovementX = dx;
    request.MovementY = dy;

    DWORD cbReturned = 0;
    const BOOL ok = DeviceIoControl(
        _deviceHandle,
        IOCTL_INJECT_MOUSE_MOVEMENT_INPUT,
        &request,
        sizeof(request),
        nullptr,
        0,
        &cbReturned,
        nullptr);
    if (!ok) {
        static int failCount = 0;
        if (++failCount <= 5) {
            std::cerr << "MCII: MoveRelative(" << dx << "," << dy
                      << ") failed: " << GetLastError()
                      << " sizeof(request)=" << sizeof(request)
                      << std::endl;
        }
    }
}

void MciController::fire() const {
    if (!isAvailable()) return;

    MCI_INJECT_MOUSE_BUTTON_INPUT_REQUEST requestDown{};
    requestDown.ProcessId = GetCurrentProcessId();
    requestDown.ButtonFlags = MOUSE_LEFT_BUTTON_DOWN;
    requestDown.ButtonData = 0;

    MCI_INJECT_MOUSE_BUTTON_INPUT_REQUEST requestUp{};
    requestUp.ProcessId = GetCurrentProcessId();
    requestUp.ButtonFlags = MOUSE_LEFT_BUTTON_UP;
    requestUp.ButtonData = 0;

    DWORD cbReturned = 0;
    DeviceIoControl(
        _deviceHandle,
        IOCTL_INJECT_MOUSE_BUTTON_INPUT,
        &requestDown,
        sizeof(requestDown),
        nullptr,
        0,
        &cbReturned,
        nullptr);

    DeviceIoControl(
        _deviceHandle,
        IOCTL_INJECT_MOUSE_BUTTON_INPUT,
        &requestUp,
        sizeof(requestUp),
        nullptr,
        0,
        &cbReturned,
        nullptr);
}