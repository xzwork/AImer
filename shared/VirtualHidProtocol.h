#pragma once

// Include Windows.h + winioctl.h (user mode), or ntddk.h (kernel mode) first.
// Private, version-1 ABI. No pointers or platform-dependent fields.
#define AIMER_VHID_DEVICE_PATH L"\\\\.\\AImerVirtualMouse"
#define AIMER_VHID_NT_NAME L"\\Device\\AImerVirtualMouse"
#define AIMER_VHID_DOS_NAME L"\\DosDevices\\Global\\AImerVirtualMouse"
#define IOCTL_AIMER_MOVE_RELATIVE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_WRITE_DATA)
#define AIMER_VHID_MAX_DELTA 32767

typedef struct _AIMER_MOVE_RELATIVE {
    LONG dx;
    LONG dy;
} AIMER_MOVE_RELATIVE;

