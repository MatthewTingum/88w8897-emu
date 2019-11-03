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

typedef struct _ENDPOINTQUEUE_CONTEXT {
    UDECXUSBDEVICE usbDeviceObj;
    WDFDEVICE      backChannelDevice;
} ENDPOINTQUEUE_CONTEXT, *PENDPOINTQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(ENDPOINTQUEUE_CONTEXT, GetEndpointQueueContext);

void hexdump(void* ptr, int buflen) {
	unsigned char* buf = (unsigned char*)ptr;
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
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "\n");
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
		// This has something! -- LOGINFO IT!!!
		//hexdump(transferBuffer, transferBufferLength);
		LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtControlUrb - transferBufferLength: %lu\n", transferBufferLength);
		//DbgPrint("[MWIFIEX] IoEvtControlUrb - transferBufferLength: %lu\n", transferBufferLength);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtControlUrb - transferBufferLength: %lu\n", transferBufferLength);

		HANDLE hFile;
		OBJECT_ATTRIBUTES objAttribs = { 0 };
		PCWSTR filePath = L"\\??\\C:\\transferBuffer.bin";
		UNICODE_STRING unicodeString;
		RtlInitUnicodeString(&unicodeString, filePath);
		InitializeObjectAttributes(&objAttribs, &unicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);
		IO_STATUS_BLOCK ioStatusBlock = { 0 };
		const int allocSize = 2048;
		LARGE_INTEGER largeInteger;
		largeInteger.QuadPart = allocSize;
		NtCreateFile(&hFile, FILE_GENERIC_WRITE, &objAttribs, &ioStatusBlock, 0,
			FILE_ATTRIBUTE_NORMAL, FILE_SHARE_WRITE, FILE_OVERWRITE_IF, FILE_RANDOM_ACCESS | FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
		NtWriteFile(hFile, NULL, NULL, NULL, &ioStatusBlock, transferBuffer, 2048, NULL, NULL);
		NtClose(hFile);

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
    }
exit:
    return;
}


static int _Test_rebound = 0;


static VOID
IoEvtBulkOutUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtBulkOutUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkOutUrb\n");
	LogInfo(TRACE_DEVICE, "[MWIFIEX] IoEvtBulkOutUrb");
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

	// Mark the last Bulk IN as having a 'valid' CRC (We don't care)
	//transferBuffer[0] = 0x00;
	memset(transferBuffer, 0x00, transferBufferLength);
	//WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, transferBufferLength);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Bulk Out - transferBufferLength: %lu\n", transferBufferLength);

	// END Firmware continued
	if (transferBufferLength == 20) {
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Firmware Download Complete. Begin Mode Switch...\n");
	}

exit:
    // writes never pended, always completed
    UdecxUrbSetBytesCompleted(Request, transferBufferLength);
    UdecxUrbCompleteWithNtStatus(Request, status);
    return;
}


static VOID
IoEvtBulkInUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtBulkInUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtBulkInUrb\n");
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

	if (transferBufferLength == 20) {
		//UdecxUsbDevicePlugOutAndDelete(pEpQContext->usbDeviceObj);
		//Usb_Disconnect();
	}

	if (transferBufferLength < 200) {
		hexdump(transferBuffer, transferBufferLength);
	}

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
IoEvtInterruptInUrb(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
	//DbgPrint("[MWIFIEX] IoEvtInterruptInUrb\n");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvtInterruptInUrb\n");
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
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] IoEvt bHasData\n");
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

    status = STATUS_SUCCESS;
    pIoContext = WdfDeviceGetIoContext(Device);
    pUsbContext = GetUsbDeviceContext(Device);

    wdfController = pUsbContext->ControllerDevice;

    switch (EpAddr)
    {

    case USB_DEFAULT_ENDPOINT_ADDRESS:
        pQueueRecord = &(pIoContext->ControlQueue);
        pIoCallback = IoEvtControlUrb;
        break;

	case g_BulkInEndpointAddress:
		pQueueRecord = &(pIoContext->BulkOutQueue);
		pIoCallback = IoEvtBulkOutUrb;
		break;

	case g_BulkOutEndpointAddress:
		pQueueRecord = &(pIoContext->BulkInQueue);
		pIoCallback = IoEvtBulkInUrb;
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

	if (pIoContext->InterruptUrbQueue != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->InterruptUrbQueue);
		WdfObjectDelete(pIoContext->InterruptUrbQueue);
	}

	if (pIoContext->BulkInQueue != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkInQueue);
		WdfObjectDelete(pIoContext->BulkInQueue);
	}

	if (pIoContext->BulkOutQueue != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkOutQueue);
		WdfObjectDelete(pIoContext->BulkOutQueue);
	}

	if (pIoContext->BulkOutQueue2 != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->BulkOutQueue2);
		WdfObjectDelete(pIoContext->BulkOutQueue2);
	}

	if (pIoContext->RxCmdQueue != NULL) {
		WdfIoQueuePurgeSynchronously(pIoContext->RxCmdQueue);
		WdfObjectDelete(pIoContext->RxCmdQueue);
	}

}



