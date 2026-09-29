#include <ntddk.h>
#include <wdf.h>
#include <hidport.h>
#include <vhf.h>
#include "../../shared/VirtualHidProtocol.h"

typedef struct _DEVICE_CONTEXT {
    VHFHANDLE Vhf;
} DEVICE_CONTEXT;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD MouseDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP MouseDeviceCleanup;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MouseIoControl;

// Mouse / Pointer, report 1: two signed 16-bit relative axes. No buttons/wheel.
static UCHAR MouseDescriptor[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x02,       // Usage (Mouse)
    0xA1, 0x01,       // Collection (Application)
    0x85, 0x01,       // Report ID (1)
    0x09, 0x01,       // Usage (Pointer)
    0xA1, 0x00,       // Collection (Physical)
    0x09, 0x30,       // Usage (X)
    0x09, 0x31,       // Usage (Y)
    0x16, 0x01, 0x80, // Logical Minimum (-32767)
    0x26, 0xFF, 0x7F, // Logical Maximum (32767)
    0x75, 0x10,       // Report Size (16)
    0x95, 0x02,       // Report Count (2)
    0x81, 0x06,       // Input (Data, Variable, Relative)
    0xC0, 0xC0
};

C_ASSERT(sizeof(AIMER_MOVE_RELATIVE) == 8);

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
    WDF_DRIVER_CONFIG config;
    WDF_DRIVER_CONFIG_INIT(&config, MouseDeviceAdd);
    return WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES,
                           &config, WDF_NO_HANDLE);
}

NTSTATUS MouseDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit) {
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_IO_QUEUE_CONFIG queue;
    VHF_CONFIG vhfConfig;
    NTSTATUS status;
    DEVICE_CONTEXT* context;
    DECLARE_CONST_UNICODE_STRING(name, AIMER_VHID_NT_NAME);
    DECLARE_CONST_UNICODE_STRING(link, AIMER_VHID_DOS_NAME);
    DECLARE_CONST_UNICODE_STRING(sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)");
    UNREFERENCED_PARAMETER(Driver);

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);
    WdfDeviceInitSetCharacteristics(DeviceInit, FILE_DEVICE_SECURE_OPEN, FALSE);
    WdfDeviceInitSetExclusive(DeviceInit, TRUE);
    status = WdfDeviceInitAssignName(DeviceInit, &name);
    if (!NT_SUCCESS(status)) return status;
    status = WdfDeviceInitAssignSDDLString(DeviceInit, &sddl);
    if (!NT_SUCCESS(status)) return status;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
    attributes.ExecutionLevel = WdfExecutionLevelPassive;
    attributes.EvtCleanupCallback = MouseDeviceCleanup;
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) return status;
    context = DeviceGetContext(device);

    VHF_CONFIG_INIT(&vhfConfig, WdfDeviceWdmGetDeviceObject(device),
                    (USHORT)sizeof(MouseDescriptor), MouseDescriptor);
    // VHF owns report buffering; a stack buffer may be reused after Submit returns.
    status = VhfCreate(&vhfConfig, &context->Vhf);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                   "AImerVirtualMouse: VhfCreate failed: 0x%08lX\n", (ULONG)status);
        return status;
    }
    status = VhfStart(context->Vhf);
    if (!NT_SUCCESS(status)) {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,
                   "AImerVirtualMouse: VhfStart failed: 0x%08lX\n", (ULONG)status);
        return status;
    }

    // Sequential, power-managed requests are drained by KMDF before device cleanup.
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queue, WdfIoQueueDispatchSequential);
    queue.EvtIoDeviceControl = MouseIoControl;
    status = WdfIoQueueCreate(device, &queue, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) return status;
    return WdfDeviceCreateSymbolicLink(device, &link);
}

VOID MouseDeviceCleanup(WDFOBJECT DeviceObject) {
    DEVICE_CONTEXT* context = DeviceGetContext(DeviceObject);
    if (context->Vhf != NULL) {
        VhfDelete(context->Vhf, TRUE);
        context->Vhf = NULL;
    }
}

VOID MouseIoControl(WDFQUEUE Queue, WDFREQUEST Request, size_t OutputBufferLength,
                    size_t InputBufferLength, ULONG IoControlCode) {
    AIMER_MOVE_RELATIVE* move;
    HID_XFER_PACKET packet = {0};
    UCHAR report[5]; // ID + little-endian X + little-endian Y; no structure padding.
    USHORT x, y;
    NTSTATUS status;
    DEVICE_CONTEXT* context = DeviceGetContext(WdfIoQueueGetDevice(Queue));

    if (IoControlCode != IOCTL_AIMER_MOVE_RELATIVE) {
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }
    if (InputBufferLength != sizeof(*move) || OutputBufferLength != 0) {
        WdfRequestComplete(Request, STATUS_INFO_LENGTH_MISMATCH);
        return;
    }
    status = WdfRequestRetrieveInputBuffer(Request, sizeof(*move), (PVOID*)&move, NULL);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
        return;
    }
    if (move->dx < -AIMER_VHID_MAX_DELTA || move->dx > AIMER_VHID_MAX_DELTA ||
        move->dy < -AIMER_VHID_MAX_DELTA || move->dy > AIMER_VHID_MAX_DELTA) {
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }
    if (context->Vhf == NULL) {
        WdfRequestComplete(Request, STATUS_DEVICE_NOT_READY);
        return;
    }
    x = (USHORT)(SHORT)move->dx;
    y = (USHORT)(SHORT)move->dy;
    report[0] = 1;
    report[1] = (UCHAR)x;
    report[2] = (UCHAR)(x >> 8);
    report[3] = (UCHAR)y;
    report[4] = (UCHAR)(y >> 8);
    packet.reportBuffer = report;
    packet.reportBufferLen = sizeof(report);
    packet.reportId = 1;
    status = VhfReadReportSubmit(context->Vhf, &packet);
    WdfRequestComplete(Request, status); // NTSTATUS is propagated to GetLastError in user mode.
}
