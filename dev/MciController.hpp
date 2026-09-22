#pragma once
#include <Windows.h>
#include <ntddmou.h>
#include <cstdint>

typedef struct _MCI_MOUSE_CLASS_BUTTON_DEVICE_INFORMATION {
    USHORT UnitId;
} MCI_MOUSE_CLASS_BUTTON_DEVICE_INFORMATION;

typedef struct _MCI_MOUSE_CLASS_MOVEMENT_DEVICE_INFORMATION {
    USHORT UnitId;
    BOOLEAN AbsoluteMovement;
    BOOLEAN VirtualDesktop;
} MCI_MOUSE_CLASS_MOVEMENT_DEVICE_INFORMATION;

typedef struct _MCI_MOUSE_DEVICE_STACK_INFORMATION {
    MCI_MOUSE_CLASS_BUTTON_DEVICE_INFORMATION ButtonDevice;
    MCI_MOUSE_CLASS_MOVEMENT_DEVICE_INFORMATION MovementDevice;
} MCI_MOUSE_DEVICE_STACK_INFORMATION;

class MciController {
public:
    static MciController &getInstance();

    MciController(const MciController &) = delete;
    MciController &operator=(const MciController &) = delete;

    [[nodiscard]] bool isAvailable() const;

    void MoveRelative(int dx, int dy) const;

    void fire() const;

private:
    HANDLE _deviceHandle{INVALID_HANDLE_VALUE};
    MCI_MOUSE_DEVICE_STACK_INFORMATION _stackInfo{};
    bool _stackInitialized{false};

    MciController();
    ~MciController();

    bool _initializeDevice();

    bool _initializeStackContext();
};

#define FILE_DEVICE_MOUCLASS_INPUT_INJECTION 48781ul

#define IOCTL_INITIALIZE_MOUSE_DEVICE_STACK_CONTEXT \
    CTL_CODE(                                       \
        FILE_DEVICE_MOUCLASS_INPUT_INJECTION,       \
        2600,                                       \
        METHOD_BUFFERED,                            \
        FILE_ANY_ACCESS)

#define IOCTL_INJECT_MOUSE_BUTTON_INPUT         \
    CTL_CODE(                                   \
        FILE_DEVICE_MOUCLASS_INPUT_INJECTION,   \
        2850,                                   \
        METHOD_BUFFERED,                        \
        FILE_ANY_ACCESS)

#define IOCTL_INJECT_MOUSE_MOVEMENT_INPUT       \
    CTL_CODE(                                   \
        FILE_DEVICE_MOUCLASS_INPUT_INJECTION,   \
        2851,                                   \
        METHOD_BUFFERED,                        \
        FILE_ANY_ACCESS)

typedef struct _MCI_INJECT_MOUSE_BUTTON_INPUT_REQUEST {
    ULONG_PTR ProcessId;
    USHORT ButtonFlags;
    USHORT ButtonData;
} MCI_INJECT_MOUSE_BUTTON_INPUT_REQUEST;

typedef struct _MCI_INJECT_MOUSE_MOVEMENT_INPUT_REQUEST {
    ULONG_PTR ProcessId;
    USHORT IndicatorFlags;
    LONG MovementX;
    LONG MovementY;
} MCI_INJECT_MOUSE_MOVEMENT_INPUT_REQUEST;