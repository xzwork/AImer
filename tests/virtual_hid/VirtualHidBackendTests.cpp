#include <Windows.h>
#include <winioctl.h>
#include "../../shared/VirtualHidProtocol.h"
#include <climits>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

static void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
static bool openFails = false;
static DWORD openFailure = ERROR_FILE_NOT_FOUND;
static int opens = 0, closes = 0, attempts = 0, failAt = -1;
static std::vector<AIMER_MOVE_RELATIVE> reports;
static HANDLE fakeHandle = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(42));

static HANDLE WINAPI TestCreateFileW(LPCWSTR name, DWORD access, DWORD share,
    LPSECURITY_ATTRIBUTES attributes, DWORD disposition, DWORD flags, HANDLE templateFile) {
    ++opens;
    check(wcscmp(name, AIMER_VHID_DEVICE_PATH) == 0, "device path");
    check(access == GENERIC_WRITE && share == 0 && !attributes &&
          disposition == OPEN_EXISTING && flags == FILE_ATTRIBUTE_NORMAL && !templateFile, "open contract");
    if (openFails) { SetLastError(openFailure); return INVALID_HANDLE_VALUE; }
    return fakeHandle;
}
static BOOL WINAPI TestCloseHandle(HANDLE handle) {
    check(handle == fakeHandle, "close handle");
    ++closes;
    return TRUE;
}
static BOOL WINAPI TestDeviceIoControl(HANDLE handle, DWORD code, LPVOID input, DWORD inputSize,
    LPVOID output, DWORD outputSize, LPDWORD returned, LPOVERLAPPED overlapped) {
    check(handle == fakeHandle && code == IOCTL_AIMER_MOVE_RELATIVE, "IOCTL identity");
    check(input && inputSize == 8 && !output && !outputSize && returned && !overlapped, "IOCTL ABI");
    const auto move = *static_cast<AIMER_MOVE_RELATIVE*>(input);
    check(move.dx >= -32767 && move.dx <= 32767 && move.dy >= -32767 && move.dy <= 32767, "HID range");
    if (attempts++ == failAt) { SetLastError(ERROR_DEVICE_NOT_CONNECTED); return FALSE; }
    reports.push_back(move);
    *returned = 0;
    return TRUE;
}

// Substitute only the three OS operations in this test translation unit.
// Production uses the unmodified Windows APIs, with no test indirection.
#define CreateFileW TestCreateFileW
#define CloseHandle TestCloseHandle
#define DeviceIoControl TestDeviceIoControl
#include "../../src/controller/VirtualHidBackend.cpp"
#undef CreateFileW
#undef CloseHandle
#undef DeviceIoControl

int main() {
    try {
        DWORD error = 0;
        for (DWORD failure : {ERROR_FILE_NOT_FOUND, ERROR_ACCESS_DENIED, ERROR_SHARING_VIOLATION}) {
            openFails = true;
            openFailure = failure;
            VirtualHidBackend backend;
            check(!backend.Open(), "open failure must propagate");
            check(!backend.MoveRelative(1, 1, error) && error == failure, "preserve open error");
            check(attempts == 0 && closes == 0, "invalid handle must never be used");
        }
        openFails = false;
        {
            VirtualHidBackend backend;
            check(backend.Open() && backend.Open() && opens == 4, "idempotent open");
            check(backend.MoveRelative(0, 0, error) && reports.empty(), "zero is a no-op");
            for (const auto delta : {AIMER_MOVE_RELATIVE{17, -23}, {32767, -32767},
                                      {32768, -32768}, {INT_MAX, INT_MIN}}) {
                reports.clear();
                check(backend.MoveRelative(delta.dx, delta.dy, error) && !error, "valid movement");
                std::int64_t x = 0, y = 0;
                for (auto report : reports) { x += report.dx; y += report.dy; }
                check(x == delta.dx && y == delta.dy, "split must preserve exact totals");
            }
            reports.clear(); attempts = 0; failAt = 1;
            check(!backend.MoveRelative(100000, -100000, error), "partial IOCTL failure");
            check(error == ERROR_DEVICE_NOT_CONNECTED && attempts == 2 && reports.size() == 1,
                  "stop on first error without replay");
            failAt = -1;
            reports.clear();
            check(backend.MoveRelative(-1, 1, error) && reports.size() == 1, "later independent request");
        }
        check(closes == 1, "close exactly once");
        std::cout << "VirtualHidBackendTests PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
