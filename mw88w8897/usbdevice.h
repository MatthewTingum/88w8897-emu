/*++
Copyright (c) Microsoft Corporation

Module Name:

misc.h

Abstract:


--*/

#pragma once

#include <ntddk.h>
#include <wdf.h>
#include <usb.h>
#include <wdfusb.h>
#include <usbdlib.h>
#include <ude/1.0/UdeCx.h>
#include <initguid.h>
#include <usbioctl.h>

#include "trace.h"



// device context
typedef struct _USB_CONTEXT {
    WDFDEVICE             ControllerDevice;
    UDECXUSBENDPOINT      MWIFIEXControlEndpoint;
	UDECXUSBENDPOINT      MWIFIEXBulkOutEndpoint;
	UDECXUSBENDPOINT      MWIFIEXBulkOut2Endpoint;
    UDECXUSBENDPOINT      MWIFIEXBulkInEndpoint;
    UDECXUSBENDPOINT      MWIFIEXInterruptInEndpoint;
	UDECXUSBENDPOINT      MWIFIEXRxCmdEndpoint;
    BOOLEAN               IsAwake;
} USB_CONTEXT, *PUSB_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USB_CONTEXT, GetUsbDeviceContext);










// ----- descriptor constants/strings/indexes
#define g_ManufacturerIndex   1
#define g_ProductIndex        2
#define g_SerialIndex		  3
#define g_BulkOutEndpointAddress 2
#define g_BulkInEndpointAddress    0x84
#define g_InterruptEndpointAddress 0x86

// Device USB\VID_1209&PID_0887\2&540a18d&0&1 was configured.
// #define USB8XXX_VID	0x1286
// #define USB8997_PID_1		0x2052
// #define USB8997_PID_2	0x204e
// #define USB8797_PID_1		0x2043
// #define USB8797_PID_2	0x2044
// Emulating an 8997 (PID_1 is configuration)
#define MWIFIEX_DEVICE_VENDOR_ID  0x86, 0x12 // little endian
#define MWIFIEX_DEVICE_PROD_ID    0x43, 0x20 // little endian
#define MWIFIEX_DEVICE_PROD_ID2    0x44, 0x20 // little endian

extern const UCHAR g_UsbDeviceDescriptor[];
extern const UCHAR g_UsbConfigDescriptorSet[];

// ------------------------------------------------



EXTERN_C_START


NTSTATUS
Usb_Initialize(
	_In_ WDFDEVICE WdfControllerDevice
);


NTSTATUS
Usb_ReadDescriptorsAndPlugIn(
	_In_ WDFDEVICE WdfControllerDevice
);

NTSTATUS
Usb_Disconnect(
	_In_ WDFDEVICE WdfDevice
);

VOID
Usb_Destroy(
	_In_ WDFDEVICE WdfDevice
);

//
// Private functions
//
NTSTATUS
UsbCreateEndpointObj(
	_In_   UDECXUSBDEVICE    WdfUsbChildDevice,
    _In_   UCHAR             epAddr,
    _Out_  UDECXUSBENDPOINT *pNewEpObjAddr
);


EVT_UDECX_USB_DEVICE_ENDPOINTS_CONFIGURE              UsbDevice_EvtUsbDeviceEndpointsConfigure;
EVT_UDECX_USB_DEVICE_D0_ENTRY                         UsbDevice_EvtUsbDeviceLinkPowerEntry;
EVT_UDECX_USB_DEVICE_D0_EXIT                          UsbDevice_EvtUsbDeviceLinkPowerExit;
EVT_UDECX_USB_DEVICE_SET_FUNCTION_SUSPEND_AND_WAKE    UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake;
EVT_UDECX_USB_ENDPOINT_RESET                          UsbEndpointReset;


EXTERN_C_END


