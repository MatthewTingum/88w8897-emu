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
	UDECXUSBENDPOINT      MWIFIEXBulkOutEndpoint1;
	UDECXUSBENDPOINT      MWIFIEXBulkOutEndpoint2;
	UDECXUSBENDPOINT      MWIFIEXBulkOutEndpoint4;
	UDECXUSBENDPOINT      MWIFIEXBulkOutEndpoint6;
    UDECXUSBENDPOINT      MWIFIEXBulkInEndpoint81;
	UDECXUSBENDPOINT      MWIFIEXBulkInEndpoint82;
	UDECXUSBENDPOINT      MWIFIEXBulkInEndpoint84;
	UDECXUSBENDPOINT      MWIFIEXInterruptInEndpoint83;
	UDECXUSBENDPOINT      MWIFIEXIsoOutEndpoint5;
	UDECXUSBENDPOINT      MWIFIEXIsoInEndpoint85;

	UDECXUSBENDPOINT      MWIFIEXRxCmdEndpoint;

    BOOLEAN               IsAwake;
} USB_CONTEXT, *PUSB_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(USB_CONTEXT, GetUsbDeviceContext);










// ----- descriptor constants/strings/indexes
#define g_ManufacturerIndex   1
#define g_ProductIndex        2
#define g_SerialIndex		  3
#define g_InterfaceIndex	  5

#define g_BulkOutEndpointAddress1 1	//
#define g_BulkOutEndpointAddress2 2 //
//#define g_BulkOutEndpointAddress3 3
#define g_BulkOutEndpointAddress4 4	//
//#define g_BulkOutEndpointAddress5 5
#define g_BulkOutEndpointAddress6 6	//

#define g_BulkInEndpointAddress81    0x81	//
#define g_BulkInEndpointAddress82    0x82	//
//#define g_BulkInEndpointAddress83    0x83
#define g_BulkInEndpointAddress84    0x84	//

#define g_InterruptEndpointAddress83 0x83	//
//#define g_InterruptEndpointAddress85 0x85	
//#define g_InterruptEndpointAddress86 0x86

#define g_IsocronousOutEndpointAddress5 0x05	//

#define g_IsocronousInEndpointAddress85 0x85	//

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


