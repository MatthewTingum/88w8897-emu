/*++
Copyright (c) Microsoft Corporation

Module Name:

USBCom.c

Abstract:
    Implementation of functions defined in USBCom.h

--*/


#include <ntifs.h>
#include "Misc.h"
#include "Driver.h"
#include "Device.h"
#include "usbdevice.h"
#include "USBCom.h"
#include "BackChannel.h"
#include "ucx/1.4/ucxobjects.h"
#include "USBCom.tmh"
#include "Mwifiex.h"

typedef struct _ENDPOINTQUEUE_CONTEXT {
    UDECXUSBDEVICE usbDeviceObj;
    WDFDEVICE      backChannelDevice;
} ENDPOINTQUEUE_CONTEXT, *PENDPOINTQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ENDPOINTQUEUE_CONTEXT, GetEndpointQueueContext);

void hexdump(void *ptr, int buflen) {
	unsigned char *buf = (unsigned char*)ptr;
	int i, j;
	for (i = 0; i < buflen; i += 16) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%06x: ", i);
		for (j = 0; j < 16; j++)
			if (i + j < buflen)
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%02x ", buf[i + j]);
			else
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "   ");
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, " ");
		for (j = 0; j < 16; j++)
			if (i + j < buflen)
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL,                                                                                     "\n");
	}
}


NTSTATUS
Io_AllocateContext(
    _In_ UDECXUSBDEVICE Object
)
/*++

Routine Description:

Object context allocation helper

Arguments:

Object - WDF object upon which to allocate the new context

Return value:

NTSTATUS. Could fail on allocation failure or the same context type already exists on the object

--*/
{
	//DbgPrint("[MWIFIEX] Io_AllocateContext\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Io_AllocateContext\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] Io_AllocateContext");
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, IO_CONTEXT);

    status = WdfObjectAllocateContext(Object, &attributes, NULL);

    if (!NT_SUCCESS(status)) {

        LogError(TRACE_DEVICE, "Unable to allocate new context for WDF object %p", Object);
        goto exit;
    }

exit:

    return status;
}





/*
static VOID
IoEvtControlUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtControlUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtControlUrb\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtControlUrb");
    WDF_USB_CONTROL_SETUP_PACKET setupPacket;
    NTSTATUS status;
	PUCHAR transferBuffer;
	ULONG transferBufferLength, transferedLength = 0;
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    //NT_VERIFY(IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB);

    if (IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB)
    {
        // These are on the control pipe.
        // We don't do anything special with these requests,
        // but this is where we would add processing for vendor-specific commands.

		status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);

		if (!NT_SUCCESS(status)) {

			//
			// Could mean there is no buffer on the request
			//
			LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtControlUrb - UdecxUrbRetrieveBuffer failed: %!STATUS!", status);
			transferBuffer = NULL;
			transferBufferLength = 0;
			status = STATUS_SUCCESS;
		}
		transferedLength = transferBufferLength;

		hexdump(transferBuffer, transferBufferLength);
		LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtControlUrb - transferBufferLength: %lu\n", transferBufferLength);
		//DbgPrint("[MWIFIEX] IoEvtControlUrb - transferBufferLength: %lu\n", transferBufferLength);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtControlUrb - transferBufferLength: %lu\n", transferBufferLength);

		//HANDLE hFile;
		//OBJECT_ATTRIBUTES objAttribs = { 0 };
		//PCWSTR filePath = L"\\??\\C:\\transferBuffer.bin";
		//UNICODE_STRING unicodeString;
		//RtlInitUnicodeString(&unicodeString, filePath);
		//InitializeObjectAttributes(&objAttribs, &unicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);
		//IO_STATUS_BLOCK ioStatusBlock = { 0 };
		//const int allocSize = 2048;
		//LARGE_INTEGER largeInteger;
		//largeInteger.QuadPart = allocSize;
		//NtCreateFile(&hFile, FILE_GENERIC_WRITE, &objAttribs, &ioStatusBlock, 0, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE, FILE_OVERWRITE_IF, FILE_RANDOM_ACCESS | FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		//NtWriteFile(hFile, NULL, NULL, NULL, &ioStatusBlock, transferBuffer, 2048, NULL, NULL);
		//NtClose(hFile);

		// This seems to fail for us - maybe the transfer buffer has something interesting
        status = UdecxUrbRetrieveControlSetupPacket(Request, &setupPacket);

        if (!NT_SUCCESS(status))
        {
            LogError(TRACE_DEVICE, "WdfRequest %p is not a control URB? UdecxUrbRetrieveControlSetupPacket %!STATUS!",
                Request, status);
            UdecxUrbCompleteWithNtStatus(Request, status);
            goto exit;
        }


        LogInfo(TRACE_DEVICE, "v44 control CODE %x, [type=%x dir=%x recip=%x] req=%x [wv = %x wi = %x wlen = %x]",
            IoControlCode,
            (int)(setupPacket.Packet.bm.Request.Type),
            (int)(setupPacket.Packet.bm.Request.Dir),
            (int)(setupPacket.Packet.bm.Request.Recipient),
            (int)(setupPacket.Packet.bRequest),
            (int)(setupPacket.Packet.wValue.Value),
            (int)(setupPacket.Packet.wIndex.Value),
            (int)(setupPacket.Packet.wLength)
        );




        UdecxUrbCompleteWithNtStatus(Request, STATUS_SUCCESS);
    }
    else
    {
        LogError(TRACE_DEVICE, "control NO submit code is %x", IoControlCode);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtControlUrb - control NO submit code is %x\n", IoControlCode);
    }
exit:
    return;
}
*/

VOID
IoEvtControlUrb(
	_In_
	WDFQUEUE Queue,
	_In_
	WDFREQUEST Request,
	_In_
	size_t OutputBufferLength,
	_In_
	size_t InputBufferLength,
	_In_
	ULONG IoControlCode
)
/*++
Routine Description:
	Handle all applicable USB MBIM control requests.
Arguments:
	Queue - Queue on which the request arrived
	Request - Request to process
	OutputBufferLength - Ignored
	InputBufferLength - Ignored
	IoControlCode - IOCTL_INTERNAL_USB_SUBMIT_URB
Return Value:
	NTSTATUS
--*/
{
	WDF_USB_CONTROL_SETUP_PACKET setupPacket;
	NTSTATUS status;
	PUCHAR transferBuffer;
	ULONG transferBufferLength, transferedLength = 0;
	WDFDEVICE wdfDevice;
	//PUSB_CONTEXT usbContext;
	PIO_CONTEXT  ioContext;
	//    ULONG lengthRead = 0;

	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	NT_VERIFY(IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB);

	wdfDevice = WdfIoQueueGetDevice(Queue);
	//usbContext = WdfDeviceGetUsbContext(wdfDevice);
	ioContext = WdfDeviceGetIoContext(wdfDevice);

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);

	if (!NT_SUCCESS(status)) {

		//
		// Could mean there is no buffer on the request
		//
		transferBuffer = NULL;
		transferBufferLength = 0;
		status = STATUS_SUCCESS;
	}
	//Initlize transferedLength and update it later to real length if it was changed.
	transferedLength = transferBufferLength;
	status = UdecxUrbRetrieveControlSetupPacket(Request, &setupPacket);

	if (!NT_SUCCESS(status)) {

		LogError(TRACE_DEVICE, "WdfRequest %p is not a control URB? UdecxUrbRetrieveControlSetupPacket %!STATUS!",
			Request, status);
		UdecxUrbCompleteWithNtStatus(Request, status);
		goto exit;
	}

	if (setupPacket.Packet.bm.Request.Recipient != BmRequestToInterface ||
		setupPacket.Packet.bm.Request.Type != BmRequestClass)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Unrecognized control request 0x%02x 0x%02x wValue: 0x%04x wIndex: 0x%04x "
			"wLength: 0x%04x %!STATUS!\n", setupPacket.Packet.bm.Byte,
			setupPacket.Packet.bRequest, setupPacket.Packet.wValue.Value,
			setupPacket.Packet.wIndex.Value, setupPacket.Packet.wLength, status);
		LogError(TRACE_DEVICE, "Unrecognized control request 0x%02x 0x%02x wValue: 0x%04x wIndex: 0x%04x "
			"wLength: 0x%04x %!STATUS!", setupPacket.Packet.bm.Byte,
			setupPacket.Packet.bRequest, setupPacket.Packet.wValue.Value,
			setupPacket.Packet.wIndex.Value, setupPacket.Packet.wLength, status);
		goto exit;
	}
	else if (setupPacket.Packet.wIndex.Value != USB_DEFAULT_ENDPOINT_ADDRESS)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtControlUrb - 6\n");
		status = STATUS_INVALID_DEVICE_REQUEST;
		LogError(TRACE_DEVICE, "Unrecognized control request whose wIndex (0x%x) is not g_CommunicationInterfaceIndex (0x%x) "
			, setupPacket.Packet.wIndex.Value, USB_DEFAULT_ENDPOINT_ADDRESS);
		goto exit;
	}

	if (setupPacket.Packet.bm.Request.Dir == BmRequestDeviceToHost)
	{
		/*
		switch (setupPacket.Packet.bRequest)
		{
		case GET_NTB_PARAMETERS:
			status = GetNtbParameters(transferBuffer, transferBufferLength, &transferedLength);
			break;
		case GET_ENCAPSULATE_RESPONSE:
			status = GetEncapsulatedCommand(ioContext, setupPacket.Packet.wValue.Value, transferBuffer, transferBufferLength, &transferedLength);
			break;
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			LogError(TRACE_FLAG_Driver, "Unrecognized control request 0x%02x 0x%02x wValue: 0x%04x wIndex: 0x%04x "
				"wLength: 0x%04x %!STATUS!", setupPacket.Packet.bm.Byte,
				setupPacket.Packet.bRequest, setupPacket.Packet.wValue.Value,
				setupPacket.Packet.wIndex.Value, setupPacket.Packet.wLength, status);
			goto exit;
		}
		*/
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtControlUrb - 7\n");
		status = STATUS_SUCCESS;
		goto exit;
	}
	else
	{
		/*
		switch (setupPacket.Packet.bRequest)
		{
		case SEND_ENCAPSULATE_COMMAND:
			status = SendEncapsulatedCommand(ioContext, setupPacket.Packet.wValue.Value, transferBuffer, transferBufferLength);
			break;
		case RESET_FUNCTION:
			status = ResetAdapter();
			break;
		case SET_NTB_INPUT_SIZE:
			status = SetNtbInputSize(usbContext, transferBuffer, transferBufferLength);
			break;
		case SET_NBT_FORMAT:
			status = SetNtbFormat(usbContext, setupPacket.Packet.wValue.Value);
			break;
		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
			LogError(TRACE_FLAG_Driver, "Unrecognized control request 0x%02x 0x%02x wValue: 0x%04x wIndex: 0x%04x "
				"wLength: 0x%04x %!STATUS!", setupPacket.Packet.bm.Byte,
				setupPacket.Packet.bRequest, setupPacket.Packet.wValue.Value,
				setupPacket.Packet.wIndex.Value, setupPacket.Packet.wLength, status);
			goto exit;
		}
		*/
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtControlUrb - 8\n");
		status = STATUS_SUCCESS;
		goto exit;
	}

exit:
	//CompleteURB(Request, transferedLength, status);
	UdecxUrbCompleteWithNtStatus(Request, STATUS_SUCCESS);
	return;
}


static int _Test_rebound = 0;


static VOID
IoEvtBulkOutUrb1(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	UDECXUSBDEVICE device;
	WDFQUEUE* pQueueRecord = NULL;
	PIO_CONTEXT pIoContext;
	WDFREQUEST requestToComplete;
	PUCHAR transferBufferToComplete;
	ULONG transferBufferLengthToComplete = 0;

	//WDFREQUEST matchingRead;
	WDFDEVICE backchannel;
	PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
	PENDPOINTQUEUE_CONTEXT pEpQContext;
	NTSTATUS status = STATUS_SUCCESS;
	PUCHAR transferBuffer;
	ULONG transferBufferLength = 0;
	//SIZE_T completeBytes = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkOutUrb1\n");

	pEpQContext = GetEndpointQueueContext(Queue);
	backchannel = pEpQContext->backChannelDevice;
	pBackChannelContext = GetBackChannelContext(backchannel);

	device = pEpQContext->usbDeviceObj;
	pIoContext = WdfDeviceGetIoContext(device);
	pQueueRecord = &(pIoContext->BulkInQueue81);

	if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
	{
		LogError(TRACE_DEVICE, "WdfRequest BOUT %p Incorrect IOCTL %x, %!STATUS!",
			Request, IoControlCode, status);
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	if (!NT_SUCCESS(status))
	{
		LogError(TRACE_DEVICE, "WdfRequest BOUT %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}

	hexdump(transferBuffer, transferBufferLength);

	if (transferBufferLength >= 8) {
		PrintMwifiexCmd(transferBuffer);
	}

	// Get a Bulk IN request to complete
	/*
	status = WdfIoQueueRetrieveNextRequest(pBackChannelContext->missionRequest.ReadBufferQueue, &requestToComplete);
	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkOutUrb1 - Unable to find a pended request: %d\n", status);
		LogError(TRACE_DEVICE, "WdfRequest BOUT %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}
	*/
	requestToComplete = pBackChannelContext->pendedRequest;

	// Get that request's transfer buffer
	status = UdecxUrbRetrieveBuffer(requestToComplete, &transferBufferToComplete, &transferBufferLengthToComplete);
	if (!NT_SUCCESS(status))
	{
		LogError(TRACE_DEVICE, "WdfRequest BIN %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}

	// Copy the MwifiexCmd
	RtlCopyMemory(transferBufferToComplete, transferBuffer, transferBufferLength);
	// Direction
	transferBufferToComplete[5] = 0x80;

	// TODO_rc1: Feed in a payload from kAFL
	// The payload will be fed in via IOCTL at some earlier point
	// In a queue of sorts, the data will be popped off (or rather pointer moved) for each byte that is consumed by a command
	// We can use up to 2mb of data by default -- otherwise kAFL will have to be modified


	// Check if there is an available payload
	
	void* payload = GetPayload(transferBufferToComplete[4]);

	if (payload == NULL) {
		// Poor man's fuzzer
		LARGE_INTEGER p = KeQueryPerformanceCounter(NULL);
		ULONG seed = p.LowPart ^ (ULONG)p.HighPart;

		for (ULONG i = 12; i < transferBufferLength; i++) {
			transferBufferToComplete[i] = (UCHAR)RtlRandom(&seed);
		}
	}
	else {
		RtlCopyMemory(transferBufferToComplete, payload, transferBufferLength);
	}
	/*
	for (ULONG i = 12; i < transferBufferLength; i++) {
		if (i % 2 == 0) {
			transferBufferToComplete[i] = 0xDE;
		}
		else {
			transferBufferToComplete[i] = 0xAD;
		}
	}
	*/

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Randomized Response\n");
	hexdump(transferBufferToComplete, transferBufferLength);

	UdecxUrbSetBytesCompleted(Request, transferBufferLength);
	UdecxUrbCompleteWithNtStatus(Request, status);

	UdecxUrbSetBytesCompleted(requestToComplete, transferBufferLength);
	UdecxUrbCompleteWithNtStatus(requestToComplete, status);


exit:
	// writes never pended, always completed
	
	return;
}

static VOID
IoEvtBulkOutUrb2(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtBulkOutUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkOutUrb2\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkOutUrb");
	WDFDEVICE backchannel;
	PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
	PENDPOINTQUEUE_CONTEXT pEpQContext;
	NTSTATUS status = STATUS_SUCCESS;
	PUCHAR transferBuffer;
	ULONG transferBufferLength = 0;
	//SIZE_T completeBytes = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	pEpQContext = GetEndpointQueueContext(Queue);
	backchannel = pEpQContext->backChannelDevice;
	pBackChannelContext = GetBackChannelContext(backchannel);

	if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
	{
		LogError(TRACE_DEVICE, "WdfRequest BOUT %p Incorrect IOCTL %x, %!STATUS!",
			Request, IoControlCode, status);
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	if (!NT_SUCCESS(status))
	{
		LogError(TRACE_DEVICE, "WdfRequest BOUT %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkOutUrb - transferBufferLength: %lu\n", transferBufferLength);

	memset(transferBuffer, 0x00, transferBufferLength);
	//WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, transferBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Bulk Out - transferBufferLength: %lu\n", transferBufferLength);

exit:
	// writes never pended, always completed
	UdecxUrbSetBytesCompleted(Request, transferBufferLength);
	UdecxUrbCompleteWithNtStatus(Request, status);
	return;
}

static VOID
IoEvtBulkOutUrb4(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtBulkOutUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkOutUrb4\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkOutUrb");
	WDFDEVICE backchannel;
	PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
	PENDPOINTQUEUE_CONTEXT pEpQContext;
	NTSTATUS status = STATUS_SUCCESS;
	PUCHAR transferBuffer;
	ULONG transferBufferLength = 0;
	//SIZE_T completeBytes = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	pEpQContext = GetEndpointQueueContext(Queue);
	backchannel = pEpQContext->backChannelDevice;
	pBackChannelContext = GetBackChannelContext(backchannel);

	if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
	{
		LogError(TRACE_DEVICE, "WdfRequest BOUT %p Incorrect IOCTL %x, %!STATUS!",
			Request, IoControlCode, status);
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	if (!NT_SUCCESS(status))
	{
		LogError(TRACE_DEVICE, "WdfRequest BOUT %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkOutUrb - transferBufferLength: %lu\n", transferBufferLength);

	memset(transferBuffer, 0x00, transferBufferLength);
	//WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, transferBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Bulk Out - transferBufferLength: %lu\n", transferBufferLength);

exit:
	// writes never pended, always completed
	UdecxUrbSetBytesCompleted(Request, transferBufferLength);
	UdecxUrbCompleteWithNtStatus(Request, status);
	return;
}

static VOID
IoEvtBulkOutUrb6(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtBulkOutUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkOutUrb6\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkOutUrb");
	WDFDEVICE backchannel;
	PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
	PENDPOINTQUEUE_CONTEXT pEpQContext;
	NTSTATUS status = STATUS_SUCCESS;
	PUCHAR transferBuffer;
	ULONG transferBufferLength = 0;
	//SIZE_T completeBytes = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	pEpQContext = GetEndpointQueueContext(Queue);
	backchannel = pEpQContext->backChannelDevice;
	pBackChannelContext = GetBackChannelContext(backchannel);

	if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
	{
		LogError(TRACE_DEVICE, "WdfRequest BOUT %p Incorrect IOCTL %x, %!STATUS!",
			Request, IoControlCode, status);
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	if (!NT_SUCCESS(status))
	{
		LogError(TRACE_DEVICE, "WdfRequest BOUT %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkOutUrb - transferBufferLength: %lu\n", transferBufferLength);

	//memset(transferBuffer, 0x00, transferBufferLength);
	hexdump(transferBuffer, transferBufferLength);
	WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, transferBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Bulk Out - transferBufferLength: %lu\n", transferBufferLength);

exit:
	// writes never pended, always completed
	UdecxUrbSetBytesCompleted(Request, transferBufferLength);
	UdecxUrbCompleteWithNtStatus(Request, status);
	return;
}


static VOID
IoEvtBulkInUrb81(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFDEVICE backchannel;
	PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
	PENDPOINTQUEUE_CONTEXT pEpQContext;
	//BOOLEAN bReady = FALSE;
	PUCHAR transferBuffer;
	ULONG transferBufferLength;
	//SIZE_T completeBytes = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb81\n");

	pEpQContext = GetEndpointQueueContext(Queue);
	backchannel = pEpQContext->backChannelDevice;
	pBackChannelContext = GetBackChannelContext(backchannel);

	if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
	{
		LogError(TRACE_DEVICE, "WdfRequest BIN %p Incorrect IOCTL %x, %!STATUS!",
			Request, IoControlCode, status);
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	if (!NT_SUCCESS(status))
	{
		LogError(TRACE_DEVICE, "WdfRequest BIN %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}

	pBackChannelContext->pendedRequest = Request;

exit:
	return;
}

static VOID
IoEvtBulkInUrb82(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	NTSTATUS status = STATUS_SUCCESS;
	//WDFDEVICE backchannel;
	//PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
	PENDPOINTQUEUE_CONTEXT pEpQContext;
	//BOOLEAN bReady = FALSE;
	PUCHAR transferBuffer;
	ULONG transferBufferLength;
	//SIZE_T completeBytes = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb82 - 0\n");

	pEpQContext = GetEndpointQueueContext(Queue);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb82 - 0.5\n");
	//backchannel = pEpQContext->backChannelDevice;
	//pBackChannelContext = GetBackChannelContext(backchannel);

	if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb82 - Incorrect IOCTL\n");
		LogError(TRACE_DEVICE, "WdfRequest BIN %p Incorrect IOCTL %x, %!STATUS!",
			Request, IoControlCode, status);
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb82 - 1\n");
	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkInUrb82 - transferBufferLength: %lu\n", transferBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb82 - transferBufferLength: %lu\n", transferBufferLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb82 - 2\n");

	if (!NT_SUCCESS(status))
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb82 - Retrieve buffer error\n");
		LogError(TRACE_DEVICE, "WdfRequest BIN %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb82 - 3\n");
	//hexdump(transferBuffer, transferBufferLength);

	memset(transferBuffer, 0x00, transferBufferLength);

exit:
	return;
}

static VOID
IoEvtBulkInUrb84(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFDEVICE backchannel;
	PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
	PENDPOINTQUEUE_CONTEXT pEpQContext;
	//BOOLEAN bReady = FALSE;
	PUCHAR transferBuffer;
	ULONG transferBufferLength;
	//SIZE_T completeBytes = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb84\n");

	pEpQContext = GetEndpointQueueContext(Queue);
	backchannel = pEpQContext->backChannelDevice;
	pBackChannelContext = GetBackChannelContext(backchannel);

	if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
	{
		LogError(TRACE_DEVICE, "WdfRequest BIN %p Incorrect IOCTL %x, %!STATUS!",
			Request, IoControlCode, status);
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	if (!NT_SUCCESS(status))
	{
		LogError(TRACE_DEVICE, "WdfRequest BIN %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}

	hexdump(transferBuffer, transferBufferLength);

	memset(transferBuffer, 0x00, transferBufferLength);

exit:
	return;
}

static VOID
IoEvtIsoOutUrb5(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtBulkInUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtIsoOutUrb5\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkInUrb");
	NTSTATUS status = STATUS_SUCCESS;
	WDFDEVICE backchannel;
	PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
	PENDPOINTQUEUE_CONTEXT pEpQContext;
	//BOOLEAN bReady = FALSE;
	PUCHAR transferBuffer;
	ULONG transferBufferLength;
	//SIZE_T completeBytes = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	pEpQContext = GetEndpointQueueContext(Queue);
	backchannel = pEpQContext->backChannelDevice;
	pBackChannelContext = GetBackChannelContext(backchannel);

	if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
	{
		LogError(TRACE_DEVICE, "WdfRequest BIN %p Incorrect IOCTL %x, %!STATUS!",
			Request, IoControlCode, status);
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	if (!NT_SUCCESS(status))
	{
		//DbgPrint("[MWIFIEX] Bulk In - unable to retrieve buffer\n");
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Bulk In - unable to retrieve buffer\n");
		LogError(TRACE_DEVICE, "WdfRequest BIN %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}

	//DbgPrint("[MWIFIEX] Bulk In - transferBufferLength: %lu\n", transferBufferLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Bulk In - transferBufferLength: %lu\n", transferBufferLength);
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkInUrb transferBufferLength: %lu", transferBufferLength);
	//hexdump(transferBuffer, transferBufferLength);


	// We have no mission, so don't let this pend (STATUS_SUCCESS)
	UdecxUrbSetBytesCompleted(Request, (ULONG)transferBufferLength);
	UdecxUrbCompleteWithNtStatus(Request, STATUS_SUCCESS);
	LogInfo(TRACE_DEVICE, "Mission response %p completed with pre-existing data", Request);


exit:
	return;
}

static VOID
IoEvtIsoInUrb85(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtBulkInUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtIsoInUrb85\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkInUrb");
	NTSTATUS status = STATUS_SUCCESS;
	WDFDEVICE backchannel;
	PUDECX_BACKCHANNEL_CONTEXT pBackChannelContext;
	PENDPOINTQUEUE_CONTEXT pEpQContext;
	//BOOLEAN bReady = FALSE;
	PUCHAR transferBuffer;
	ULONG transferBufferLength;
	//SIZE_T completeBytes = 0;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	pEpQContext = GetEndpointQueueContext(Queue);
	backchannel = pEpQContext->backChannelDevice;
	pBackChannelContext = GetBackChannelContext(backchannel);

	if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)
	{
		LogError(TRACE_DEVICE, "WdfRequest BIN %p Incorrect IOCTL %x, %!STATUS!",
			Request, IoControlCode, status);
		status = STATUS_INVALID_PARAMETER;
		goto exit;
	}

	status = UdecxUrbRetrieveBuffer(Request, &transferBuffer, &transferBufferLength);
	if (!NT_SUCCESS(status))
	{
		//DbgPrint("[MWIFIEX] Bulk In - unable to retrieve buffer\n");
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Bulk In - unable to retrieve buffer\n");
		LogError(TRACE_DEVICE, "WdfRequest BIN %p unable to retrieve buffer %!STATUS!",
			Request, status);
		goto exit;
	}

	//DbgPrint("[MWIFIEX] Bulk In - transferBufferLength: %lu\n", transferBufferLength);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Bulk In - transferBufferLength: %lu\n", transferBufferLength);
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkInUrb transferBufferLength: %lu", transferBufferLength);
	//hexdump(transferBuffer, transferBufferLength);


	// We have no mission, so don't let this pend (STATUS_SUCCESS)
	UdecxUrbSetBytesCompleted(Request, (ULONG)transferBufferLength);
	UdecxUrbCompleteWithNtStatus(Request, STATUS_SUCCESS);
	LogInfo(TRACE_DEVICE, "Mission response %p completed with pre-existing data", Request);


exit:
	return;
}


static VOID
IoEvtCancelInterruptInUrb(
    IN WDFQUEUE Queue,
    IN WDFREQUEST  Request
)
{
	//DbgPrint("[MWIFIEX] IoEvtCancelInterruptInUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtCancelInterruptInUrb\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtCancelInterruptInUrb");
    UNREFERENCED_PARAMETER(Queue);
    LogInfo(TRACE_DEVICE, "Canceling request %p", Request);
    UdecxUrbCompleteWithNtStatus(Request, STATUS_CANCELLED);
}


static VOID
IoCompletePendingRequest(
    _In_ WDFREQUEST request,
    _In_ DEVICE_INTR_FLAGS LatestStatus)
{
	//DbgPrint("[MWIFIEX] IoEvtCancelInterruptInUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtCancelInterruptInUrb\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoCompletePendingRequest");
    NTSTATUS status = STATUS_SUCCESS;
    PUCHAR transferBuffer;
    ULONG transferBufferLength;

    status = UdecxUrbRetrieveBuffer(request, &transferBuffer, &transferBufferLength);
    if (!NT_SUCCESS(status))
    {
        LogError(TRACE_DEVICE, "WdfRequest  %p unable to retrieve buffer %!STATUS!",
            request, status);
        goto exit;
    }

    if (transferBufferLength != sizeof(LatestStatus))
    {
        LogError(TRACE_DEVICE, "Error: req %p Invalid interrupt buffer size, %d",
            request, transferBufferLength);
        status = STATUS_INVALID_BLOCK_LENGTH;
        goto exit;
    }

    memcpy(transferBuffer, &LatestStatus, sizeof(LatestStatus) );

    LogInfo(TRACE_DEVICE, "INTR completed req=%p, data=%x",
        request,
        LatestStatus
    );

    UdecxUrbSetBytesCompleted(request, transferBufferLength);

exit:
    UdecxUrbCompleteWithNtStatus(request, status);

    return;

}



NTSTATUS
Io_RaiseInterrupt(
    _In_ UDECXUSBDEVICE    Device,
    _In_ DEVICE_INTR_FLAGS LatestStatus )
{
	//DbgPrint("[MWIFIEX] Io_RaiseInterrupt\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Io_RaiseInterrupt\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] Io_RaiseInterrupt");
    PIO_CONTEXT pIoContext;
    WDFREQUEST request;
    NTSTATUS status = STATUS_SUCCESS;

    pIoContext = WdfDeviceGetIoContext(Device);

    status = WdfIoQueueRetrieveNextRequest( pIoContext->IntrDeferredQueue, &request);

    // no items in the queue?  it is safe to assume the device is sleeping
    if (!NT_SUCCESS(status))    {
        LogInfo(TRACE_DEVICE, "Save update and wake device as queue status was %!STATUS!", status);

        WdfSpinLockAcquire(pIoContext->IntrState.sync);
        pIoContext->IntrState.latestStatus = LatestStatus;
        if ((pIoContext->IntrState.numUnreadUpdates) < INTR_STATE_MAX_CACHED_UPDATES)
        {
            ++(pIoContext->IntrState.numUnreadUpdates);
        }
        WdfSpinLockRelease(pIoContext->IntrState.sync);

        UdecxUsbDeviceSignalWake(Device);
        status = STATUS_SUCCESS;
    } else {
        IoCompletePendingRequest(request, LatestStatus);
    }

    return status;
}



static VOID
IoEvtInterruptInUrb83(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtInterruptInUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtInterruptInUrb83\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtInterruptInUrb");
	//DbgPrint("[MWIFIEX] IoEvt I/O control code 0x%x\n", IoControlCode);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvt I/O control code 0x%x\n", IoControlCode);
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvt I/O control code 0x%x\n", IoControlCode);
    PIO_CONTEXT pIoContext;
    UDECXUSBDEVICE tgtDevice;
    NTSTATUS status = STATUS_SUCCESS;
    DEVICE_INTR_FLAGS LatestStatus = 0;
    PENDPOINTQUEUE_CONTEXT pEpQContext;

    BOOLEAN bHasData = FALSE;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    pEpQContext = GetEndpointQueueContext(Queue);
    tgtDevice = pEpQContext->usbDeviceObj;
    pIoContext = WdfDeviceGetIoContext(tgtDevice);

    if (IoControlCode != IOCTL_INTERNAL_USB_SUBMIT_URB)   {
		//DbgPrint("[MWIFIEX] IoEvt Invalid Interrupt/IN out IOCTL code %x\n", IoControlCode);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvt Invalid Interrupt/IN out IOCTL code %x\n", IoControlCode);
        LogError(TRACE_DEVICE, "Invalid Interrupt/IN out IOCTL code %x", IoControlCode);
        status = STATUS_ACCESS_DENIED;
        goto exit;
    }

    // gate cached data we may have and clear it
    WdfSpinLockAcquire(pIoContext->IntrState.sync);

    if( pIoContext->IntrState.numUnreadUpdates > 0)
    {
        bHasData = TRUE;
		//DbgPrint("[MWIFIEX] IoEvt bHasData\n");
		LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvt bHasData");
        LatestStatus = pIoContext->IntrState.latestStatus;
    }
    pIoContext->IntrState.latestStatus = 0;
    pIoContext->IntrState.numUnreadUpdates = 0;
    WdfSpinLockRelease(pIoContext->IntrState.sync);


    if (bHasData)  {
        IoCompletePendingRequest(Request, LatestStatus);
    } else {
        status = WdfRequestForwardToIoQueue(Request, pIoContext->IntrDeferredQueue);
        if (NT_SUCCESS(status)) {
            LogInfo(TRACE_DEVICE, "Request %p forwarded for later", Request);
        } else {
            LogError(TRACE_DEVICE, "ERROR: Unable to forward Request %p error %!STATUS!", Request, status);
            UdecxUrbCompleteWithNtStatus(Request, status);
        }

    }

exit:
    return;
}


static NTSTATUS
Io_CreateDeferredIntrQueue(
    _In_ WDFDEVICE   ControllerDevice,
    _In_ PIO_CONTEXT pIoContext )
{
	//DbgPrint("[MWIFIEX] Io_CreateDeferredIntrQueue\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Io_CreateDeferredIntrQueue\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] Io_CreateDeferredIntrQueue");
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    pIoContext->IntrState.latestStatus = 0;
    pIoContext->IntrState.numUnreadUpdates = 0;

    //
    // Register a manual I/O queue for handling Interrupt Message Read Requests.
    // This queue will be used for storing Requests that need to wait for an
    // interrupt to occur before they can be completed.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    // when a request gets canceled, this is where we want to do the completion
    queueConfig.EvtIoCanceledOnQueue = IoEvtCancelInterruptInUrb;

    //
    // We shouldn't have to power-manage this queue, as we will manually 
    // purge it and de-queue from it whenever we get power indications.
    //
    queueConfig.PowerManaged = WdfFalse;

    status = WdfIoQueueCreate(ControllerDevice,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &(pIoContext->IntrDeferredQueue)
    );

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE,
            "WdfIoQueueCreate failed 0x%x\n", status);
        goto Error;
    }


    status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES,
        &(pIoContext->IntrState.sync));
    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE,
            "WdfSpinLockCreate failed  %!STATUS!\n", status);
        goto Error;
    }

Error:
    return status;
}


NTSTATUS
Io_DeviceSlept(
    _In_ UDECXUSBDEVICE  Device
)
{
	//DbgPrint("[MWIFIEX] Io_DeviceSlept\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Io_DeviceSlept\n");
    PIO_CONTEXT pIoContext;
    pIoContext = WdfDeviceGetIoContext(Device);

    // thi will result in all current requests being canceled
    LogInfo(TRACE_DEVICE, "About to purge deferred request queue" );
    WdfIoQueuePurge(pIoContext->IntrDeferredQueue, NULL, NULL);

    return STATUS_SUCCESS;
}


NTSTATUS
Io_DeviceWokeUp(
    _In_ UDECXUSBDEVICE  Device
)
{
	//DbgPrint("[MWIFIEX] Io_DeviceWokeUp\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Io_DeviceWokeUp\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] Io_DeviceWokeUp");
    PIO_CONTEXT pIoContext;
    pIoContext = WdfDeviceGetIoContext(Device);

    // thi will result in all current requests being canceled
    LogInfo(TRACE_DEVICE, "About to re-start paused deferred queue");
    WdfIoQueueStart(pIoContext->IntrDeferredQueue);

    return STATUS_SUCCESS;
}

void EvtWdfIoQueueIoCanceledOnQueue(
	WDFQUEUE Queue,
	WDFREQUEST Request
)
{
	UNREFERENCED_PARAMETER(Queue);
	UNREFERENCED_PARAMETER(Request);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] EvtWdfIoQueueIoCanceledOnQueue\n");
}

NTSTATUS
Io_RetrieveEpQueue(
    _In_ UDECXUSBDEVICE  Device,
    _In_ UCHAR           EpAddr,
    _Out_ WDFQUEUE     * Queue
)
{
	//DbgPrint("[MWIFIEX] Io_RetrieveEpQueue\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Io_RetrieveEpQueue\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] Io_RetrieveEpQueue");
    NTSTATUS status;
    PIO_CONTEXT pIoContext;
    PUSB_CONTEXT pUsbContext;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFDEVICE wdfController;
    WDFQUEUE *pQueueRecord = NULL;
    WDF_OBJECT_ATTRIBUTES  attributes;
    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL pIoCallback = NULL;
	EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE* pEvtWdfIoQueueIoCanceledOnQueue = NULL;

    status = STATUS_SUCCESS;
    pIoContext = WdfDeviceGetIoContext(Device);
    pUsbContext = GetUsbDeviceContext(Device);

    wdfController = pUsbContext->ControllerDevice;

    switch (EpAddr)
    {
	// Interface 1: Default / CMD / Setup
    case USB_DEFAULT_ENDPOINT_ADDRESS:
        pQueueRecord = &(pIoContext->ControlQueue);
        pIoCallback = IoEvtControlUrb;
        break;

	case g_BulkOutEndpointAddress1:
		pQueueRecord = &(pIoContext->BulkOutQueue1);
		pIoCallback = IoEvtBulkOutUrb1;
		break;

    case g_BulkOutEndpointAddress2:
        pQueueRecord = &(pIoContext->BulkOutQueue2);
        pIoCallback = IoEvtBulkOutUrb2;
        break;

	case g_BulkOutEndpointAddress4:
		pQueueRecord = &(pIoContext->BulkOutQueue4);
		pIoCallback = IoEvtBulkOutUrb4;
		break;

	case g_BulkOutEndpointAddress6:
		pQueueRecord = &(pIoContext->BulkOutQueue6);
		pIoCallback = IoEvtBulkOutUrb6;
		break;

    case g_BulkInEndpointAddress81:
        pQueueRecord = &(pIoContext->BulkInQueue81);
        pIoCallback = IoEvtBulkInUrb81;
		pEvtWdfIoQueueIoCanceledOnQueue = EvtWdfIoQueueIoCanceledOnQueue;
        break;

	case g_BulkInEndpointAddress82:
		pQueueRecord = &(pIoContext->BulkInQueue82);
		pIoCallback = IoEvtBulkInUrb82;
		break;

	case g_BulkInEndpointAddress84:
		pQueueRecord = &(pIoContext->BulkInQueue84);
		pIoCallback = IoEvtBulkInUrb84;
		break;

	case g_InterruptEndpointAddress83:
		status = Io_CreateDeferredIntrQueue(wdfController, pIoContext);
		pQueueRecord = &(pIoContext->InterruptUrbQueue83);
		pIoCallback = IoEvtInterruptInUrb83;
		break;

	case g_IsocronousOutEndpointAddress5:
		pQueueRecord = &(pIoContext->IsoOutQueue5);
		pIoCallback = IoEvtIsoOutUrb5;
		break;

	case g_IsocronousInEndpointAddress85:
		pQueueRecord = &(pIoContext->IsoInQueue85);
		pIoCallback = IoEvtIsoInUrb85;
		break;

    default:
        LogError(TRACE_DEVICE, "Io_RetrieveEpQueue received unrecognized ep %x", EpAddr);
        status = STATUS_ILLEGAL_FUNCTION;
        goto exit;
    }


    *Queue = NULL;
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    if ( (*pQueueRecord)  == NULL) {
        PENDPOINTQUEUE_CONTEXT pEPQContext;
        WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

        //Sequential must specify this callback
        queueConfig.EvtIoInternalDeviceControl = pIoCallback;

		if (pEvtWdfIoQueueIoCanceledOnQueue != NULL) {
			queueConfig.EvtIoCanceledOnQueue = pEvtWdfIoQueueIoCanceledOnQueue;
		}
		
        WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, ENDPOINTQUEUE_CONTEXT);

        status = WdfIoQueueCreate(wdfController,
            &queueConfig,
            &attributes,
            pQueueRecord);

        pEPQContext = GetEndpointQueueContext(*pQueueRecord);
        pEPQContext->usbDeviceObj      = Device;
        pEPQContext->backChannelDevice = wdfController; // this is a dirty little secret, so we contain it.

        if (!NT_SUCCESS(status)) {

            LogError(TRACE_DEVICE, "WdfIoQueueCreate failed for queue of ep %x %!STATUS!", EpAddr, status);
            goto exit;
        }
    }

    *Queue = (*pQueueRecord);

exit:

    return status;
}



VOID
Io_StopDeferredProcessing(
    _In_ UDECXUSBDEVICE  Device,
    _Out_ PIO_CONTEXT   pIoContextCopy
)
{
	//DbgPrint("[MWIFIEX] Io_StopDeferredProcessing\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Io_StopDeferredProcessing\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] Io_StopDeferredProcessing");
    PIO_CONTEXT pIoContext = WdfDeviceGetIoContext(Device);

    pIoContext->bStopping = TRUE;
    // plus this queue will no longer accept incoming requests
	if (pIoContext->IntrDeferredQueue != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->IntrDeferredQueue);
	}

    (*pIoContextCopy) = (*pIoContext);
}


VOID
Io_FreeEndpointQueues(
    _In_ PIO_CONTEXT   pIoContext
)
{
	//DbgPrint("[MWIFIEX] Io_FreeEndpointQueues\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Io_FreeEndpointQueues\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] Io_FreeEndpointQueues");

	if (pIoContext->IntrDeferredQueue != NULL) {
		WdfObjectDelete(pIoContext->IntrDeferredQueue);
	}

	if (pIoContext->ControlQueue != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->ControlQueue);
		WdfObjectDelete(pIoContext->ControlQueue);
	}

	if (pIoContext->InterruptUrbQueue83 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->InterruptUrbQueue83);
		WdfObjectDelete(pIoContext->InterruptUrbQueue83);
	}

	if (pIoContext->BulkInQueue81 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkInQueue81);
		WdfObjectDelete(pIoContext->BulkInQueue81);
	}

	if (pIoContext->BulkInQueue82 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkInQueue82);
		WdfObjectDelete(pIoContext->BulkInQueue82);
	}

	if (pIoContext->BulkInQueue84 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkInQueue84);
		WdfObjectDelete(pIoContext->BulkInQueue84);
	}

	if (pIoContext->BulkOutQueue1 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkOutQueue1);
		WdfObjectDelete(pIoContext->BulkOutQueue1);
	}

	if (pIoContext->BulkOutQueue2 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkOutQueue2);
		WdfObjectDelete(pIoContext->BulkOutQueue2);
	}

	if (pIoContext->BulkOutQueue4 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkOutQueue4);
		WdfObjectDelete(pIoContext->BulkOutQueue4);
	}

	if (pIoContext->BulkOutQueue6 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkOutQueue6);
		WdfObjectDelete(pIoContext->BulkOutQueue6);
	}

	if (pIoContext->IsoOutQueue5 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->IsoOutQueue5);
		WdfObjectDelete(pIoContext->IsoOutQueue5);
	}

	if (pIoContext->IsoInQueue85 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->IsoInQueue85);
		WdfObjectDelete(pIoContext->IsoInQueue85);
	}

	if (pIoContext->RxCmdQueue != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->RxCmdQueue);
		WdfObjectDelete(pIoContext->RxCmdQueue);
	}

}


