#include "VirtualHidBackend.hpp"
#include <winioctl.h>
#include "../../shared/VirtualHidProtocol.h"
#include <algorithm>
#include <iostream>

static_assert(sizeof(AIMER_MOVE_RELATIVE) == 8);

VirtualHidBackend::~VirtualHidBackend() {
    if (device != INVALID_HANDLE_VALUE) CloseHandle(device);
}

bool VirtualHidBackend::Open() {
    if (device != INVALID_HANDLE_VALUE) return true;
    device = CreateFileW(AIMER_VHID_DEVICE_PATH, GENERIC_WRITE, 0, nullptr,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (device == INVALID_HANDLE_VALUE) {
        openError = GetLastError();
        std::cerr << "[mouse] backend=virtual_hid CreateFile failed; win32_error=" << openError;
        if (openError == ERROR_FILE_NOT_FOUND || openError == ERROR_PATH_NOT_FOUND)
            std::cerr << " (driver not installed, not started, or device unavailable)";
        else if (openError == ERROR_ACCESS_DENIED)
            std::cerr << " (access denied; run the controlled test as administrator)";
        else if (openError == ERROR_SHARING_VIOLATION)
            std::cerr << " (device already opened by another process)";
        std::cerr << "; movement disabled; install/start the driver and restart AImer" << std::endl;
        return false;
    }
    openError = ERROR_SUCCESS;
    std::cout << "[mouse] backend=virtual_hid device opened (relative movement only)" << std::endl;
    return true;
}

bool VirtualHidBackend::MoveRelative(int dx, int dy, DWORD& error) const {
    error = ERROR_SUCCESS;
    if (device == INVALID_HANDLE_VALUE) {
        error = openError;
        return false;
    }
    // Preserve the total delta instead of truncating to the signed 16-bit HID axes.
    // Stop on failure: previously accepted chunks must never be replayed.
    while (dx != 0 || dy != 0) {
        AIMER_MOVE_RELATIVE move{
            std::clamp(dx, -AIMER_VHID_MAX_DELTA, AIMER_VHID_MAX_DELTA),
            std::clamp(dy, -AIMER_VHID_MAX_DELTA, AIMER_VHID_MAX_DELTA)};
        DWORD returned = 0;
        if (!DeviceIoControl(device, IOCTL_AIMER_MOVE_RELATIVE, &move, sizeof(move),
                             nullptr, 0, &returned, nullptr)) {
            error = GetLastError();
            return false;
        }
        dx -= move.dx;
        dy -= move.dy;
    }
    return true;
}
