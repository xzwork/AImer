#pragma once
#include <Windows.h>

class VirtualHidBackend final {
public:
    VirtualHidBackend() = default;
    ~VirtualHidBackend();
    VirtualHidBackend(const VirtualHidBackend&) = delete;
    VirtualHidBackend& operator=(const VirtualHidBackend&) = delete;

    bool Open();
    bool MoveRelative(int dx, int dy, DWORD& error) const;

private:
    HANDLE device = INVALID_HANDLE_VALUE;
    DWORD openError = ERROR_INVALID_HANDLE;
};
