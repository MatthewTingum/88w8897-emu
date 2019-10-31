/*++
Copyright (c) Microsoft Corporation

Module Name:

usbdevice.cpp

Abstract:


--*/

#include "Misc.h"
#include "Driver.h"
#include "Device.h"
#include "usbdevice.h"
#include "USBCom.h"
#include "ucx/1.4/ucxobjects.h"
#include "usbdevice.tmh"



#define UDECXMBIM_POOL_TAG 'UDEI'



// START ------------------ descriptor -------------------------------


DECLARE_CONST_UNICODE_STRING(g_ManufacturerStringEnUs, L"Marvell");
DECLARE_CONST_UNICODE_STRING(g_ProductStringEnUs, L"Bluetooth and Wireless LAN Composite Device");
DECLARE_CONST_UNICODE_STRING(g_SerialStringEnUs, L"6045BDDB2C7C");
DECLARE_CONST_UNICODE_STRING(g_InterfaceStringEnUs, L"Wireless LAN Interface");


const USHORT AMERICAN_ENGLISH = 0x409;

const UCHAR g_LanguageDescriptor[] = { 4,3,9,4 };

// PID 0x2044 descriptors
const UCHAR g_UsbDeviceDescriptor[18] =
{
	0x12,                            // Descriptor size
	USB_DEVICE_DESCRIPTOR_TYPE,      // Device descriptor type
	0x00, 0x02,                      // USB 2.0
	0xEF,                            // Device class (interface-class defined)
	0x02,                            // Device subclass
	0x01,                            // Device protocol
	0x40,                            // Maxpacket size for EP0
	MWIFIEX_DEVICE_VENDOR_ID,         // Vendor ID
	MWIFIEX_DEVICE_PROD_ID2,           // Product ID
	0x01,                            // LSB of firmware revision
	0x32,                            // MSB of firmware revision
	0x01,                            // Manufacture string index
	0x02,                            // Product string index
	0x03,                            // Serial number string index
	0x01                             // Number of configurations
};

const UCHAR g_UsbConfigDescriptorSet[] =
{
	/*
			===>Configuration Descriptor<===
		bLength:                           0x09
		bDescriptorType:                   0x02
		wTotalLength:                    0x00E5  -> Validated
		bNumInterfaces:                    0x03
		bConfigurationValue:               0x01
		iConfiguration:                    0x00
		bmAttributes:                      0xE0  -> Self Powered
		  -> Remote Wakeup
		MaxPower:                          0xFA = 500 mA
	*/
	0x9,                              // Descriptor Size
	USB_CONFIGURATION_DESCRIPTOR_TYPE, // Configuration Descriptor Type
	0xE5, 0x00,                        // Length of this descriptor and all sub descriptors
	0x03,                               // Number of interfaces	// 2 interfaces for the mwifiex card
	0x01,                              // Configuration number
	0x00,                              // Configuration string index
	0xE0,                              // Config characteristics - bus powered
	0xFA,                              // Max power consumption of device (in 2mA unit) : 500 mA


		/*
							  ===>IAD Descriptor<===
			bLength:                           0x08
			bDescriptorType:                   0x0B
			bFirstInterface:                   0x00
			bInterfaceCount:                   0x02
			bFunctionClass:                    0xE0  -> This is a Wireless RF Controller USB Device Interface Class with Bluetooth Programming Interface
			bFunctionSubClass:                 0x01
			bFunctionProtocol:                 0x01
			iFunction:                         0x00
		*/
		0x08,											// Descriptor size
		USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE,      // Interface Association Descriptor Type
		0x00,											// bFirstInterface
		0x02,											// bInterfaceCount
		0xE0,											// bFunctionClass
		0x01,											// bFunctionSubClass
		0x01,											// bFunctionProtocol
		0x00,											// iFunction

		/*
							  ===>Interface Descriptor<===
			bLength:                           0x09
			bDescriptorType:                   0x04
			bInterfaceNumber:                  0x00
			bAlternateSetting:                 0x00
			bNumEndpoints:                     0x03
			bInterfaceClass:                   0xE0  -> This is a Wireless RF Controller USB Device Interface Class with Bluetooth Programming Interface
			bInterfaceSubClass:                0x01
			bInterfaceProtocol:                0x01
			iInterface:                        0x00
		*/
		0x9,										// Descriptor size
		USB_INTERFACE_DESCRIPTOR_TYPE,				// Interface Association Descriptor Type
		0x00,                                       // bInterfaceNumber
		0x00,                                       // bAlternateSetting
		0x03,                                       // bNumEndpoints
		0xE0,										// bInterfaceClass
		0x01,										// bInterfaceSubClass
		0x01,										// bInterfaceProtocol
		0x00,										// iInterface

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x83  -> Direction: IN - EndpointID: 3
			bmAttributes:                      0x03  -> Interrupt Transfer Type
			wMaxPacketSize:                  0x0040 = 1 transactions per microframe, 0x40 max bytes
			bInterval:                         0x01
		*/
		0x07,                           // Descriptor size 
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // Descriptor type
		g_InterruptEndpointAddress83,	// Endpoint address and description
		USB_ENDPOINT_TYPE_INTERRUPT,	// bmAttributes - interrupt
		0x00, 0x04,                     // Max packet size = 512
		0x01,                           // Servicing interval for interrupt (1ms/1 frame)

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x84  -> Direction: IN - EndpointID: 4
			bmAttributes:                      0x02  -> Bulk Transfer Type
			wMaxPacketSize:                  0x0200 = 0x200 max bytes
			bInterval:                         0x00
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_BulkInEndpointAddress84,		// bEndpointAddress
		USB_ENDPOINT_TYPE_BULK,         // bmAttributes - bulk
		0x00, 0x02,                     // wMaxPacketSize
		0x00,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x04  -> Direction: OUT - EndpointID: 4
			bmAttributes:                      0x02  -> Bulk Transfer Type
			wMaxPacketSize:                  0x0200 = 0x200 max bytes
			bInterval:                         0xFF
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_BulkOutEndpointAddress4,		// bEndpointAddress
		USB_ENDPOINT_TYPE_BULK,         // bmAttributes - bulk
		0x00, 0x02,                     // wMaxPacketSize
		0xFF,							// bInterval

		/*
							  ===>Interface Descriptor<===
			bLength:                           0x09
			bDescriptorType:                   0x04
			bInterfaceNumber:                  0x01
			bAlternateSetting:                 0x00
			bNumEndpoints:                     0x02
			bInterfaceClass:                   0xE0  -> This is a Wireless RF Controller USB Device Interface Class with Bluetooth Programming Interface
			bInterfaceSubClass:                0x01
			bInterfaceProtocol:                0x01
			iInterface:                        0x00
		*/
		0x9,										// Descriptor size
		USB_INTERFACE_DESCRIPTOR_TYPE,				// Interface Association Descriptor Type
		0x01,                                       // bInterfaceNumber
		0x00,                                       // bAlternateSetting
		0x02,                                       // bNumEndpoints
		0xE0,										// bInterfaceClass
		0x01,										// bInterfaceSubClass
		0x01,										// bInterfaceProtocol
		0x00,										// iInterface

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x85  -> Direction: IN - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0000*!*ERROR:  Invalid maximum packet size, should be between 1 and 1024
			 = 1 transactions per microframe, 0x00 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousInEndpointAddress85,	// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x00, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x05  -> Direction: OUT - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0000*!*ERROR:  Invalid maximum packet size, should be between 1 and 1024
			 = 1 transactions per microframe, 0x00 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousOutEndpointAddress5,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x00, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Interface Descriptor<===
			bLength:                           0x09
			bDescriptorType:                   0x04
			bInterfaceNumber:                  0x01
			bAlternateSetting:                 0x01
			bNumEndpoints:                     0x02
			bInterfaceClass:                   0xE0  -> This is a Wireless RF Controller USB Device Interface Class with Bluetooth Programming Interface
			bInterfaceSubClass:                0x01
			bInterfaceProtocol:                0x01
			iInterface:                        0x00
		*/
		0x9,										// Descriptor size
		USB_INTERFACE_DESCRIPTOR_TYPE,				// Interface Association Descriptor Type
		0x01,                                       // bInterfaceNumber
		0x01,                                       // bAlternateSetting
		0x02,                                       // bNumEndpoints
		0xE0,										// bInterfaceClass
		0x01,										// bInterfaceSubClass
		0x01,										// bInterfaceProtocol
		0x00,										// iInterface

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x85  -> Direction: IN - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0009 = 1 transactions per microframe, 0x09 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousInEndpointAddress85,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x09, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x05  -> Direction: OUT - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0009 = 1 transactions per microframe, 0x09 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousOutEndpointAddress5,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x09, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Interface Descriptor<===
			bLength:                           0x09
			bDescriptorType:                   0x04
			bInterfaceNumber:                  0x01
			bAlternateSetting:                 0x02
			bNumEndpoints:                     0x02
			bInterfaceClass:                   0xE0  -> This is a Wireless RF Controller USB Device Interface Class with Bluetooth Programming Interface
			bInterfaceSubClass:                0x01
			bInterfaceProtocol:                0x01
			iInterface:                        0x00
		*/
		0x9,										// Descriptor size
		USB_INTERFACE_DESCRIPTOR_TYPE,				// Interface Association Descriptor Type
		0x01,                                       // bInterfaceNumber
		0x02,                                       // bAlternateSetting
		0x02,                                       // bNumEndpoints
		0xE0,										// bInterfaceClass
		0x01,										// bInterfaceSubClass
		0x01,										// bInterfaceProtocol
		0x00,										// iInterface

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x85  -> Direction: IN - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0011 = 1 transactions per microframe, 0x11 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousInEndpointAddress85,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x11, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x05  -> Direction: OUT - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0011 = 1 transactions per microframe, 0x11 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousOutEndpointAddress5,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x11, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Interface Descriptor<===
			bLength:                           0x09
			bDescriptorType:                   0x04
			bInterfaceNumber:                  0x01
			bAlternateSetting:                 0x03
			bNumEndpoints:                     0x02
			bInterfaceClass:                   0xE0  -> This is a Wireless RF Controller USB Device Interface Class with Bluetooth Programming Interface
			bInterfaceSubClass:                0x01
			bInterfaceProtocol:                0x01
			iInterface:                        0x00
		*/
		0x9,										// Descriptor size
		USB_INTERFACE_DESCRIPTOR_TYPE,				// Interface Association Descriptor Type
		0x01,                                       // bInterfaceNumber
		0x03,                                       // bAlternateSetting
		0x02,                                       // bNumEndpoints
		0xE0,										// bInterfaceClass
		0x01,										// bInterfaceSubClass
		0x01,										// bInterfaceProtocol
		0x00,										// iInterface

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x85  -> Direction: IN - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0019 = 1 transactions per microframe, 0x19 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousInEndpointAddress85,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x19, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x05  -> Direction: OUT - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0019 = 1 transactions per microframe, 0x19 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousOutEndpointAddress5,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x19, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Interface Descriptor<===
			bLength:                           0x09
			bDescriptorType:                   0x04
			bInterfaceNumber:                  0x01
			bAlternateSetting:                 0x04
			bNumEndpoints:                     0x02
			bInterfaceClass:                   0xE0  -> This is a Wireless RF Controller USB Device Interface Class with Bluetooth Programming Interface
			bInterfaceSubClass:                0x01
			bInterfaceProtocol:                0x01
			iInterface:                        0x00
		*/
		0x9,										// Descriptor size
		USB_INTERFACE_DESCRIPTOR_TYPE,				// Interface Association Descriptor Type
		0x01,                                       // bInterfaceNumber
		0x04,                                       // bAlternateSetting
		0x02,                                       // bNumEndpoints
		0xE0,										// bInterfaceClass
		0x01,										// bInterfaceSubClass
		0x01,										// bInterfaceProtocol
		0x00,										// iInterface

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x85  -> Direction: IN - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0021 = 1 transactions per microframe, 0x21 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousInEndpointAddress85,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x21, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x05  -> Direction: OUT - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0021 = 1 transactions per microframe, 0x21 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousOutEndpointAddress5,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x21, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Interface Descriptor<===
			bLength:                           0x09
			bDescriptorType:                   0x04
			bInterfaceNumber:                  0x01
			bAlternateSetting:                 0x05
			bNumEndpoints:                     0x02
			bInterfaceClass:                   0xE0  -> This is a Wireless RF Controller USB Device Interface Class with Bluetooth Programming Interface
			bInterfaceSubClass:                0x01
			bInterfaceProtocol:                0x01
			iInterface:                        0x00
		*/
		0x9,										// Descriptor size
		USB_INTERFACE_DESCRIPTOR_TYPE,				// Interface Association Descriptor Type
		0x01,                                       // bInterfaceNumber
		0x05,                                       // bAlternateSetting
		0x02,                                       // bNumEndpoints
		0xE0,										// bInterfaceClass
		0x01,										// bInterfaceSubClass
		0x01,										// bInterfaceProtocol
		0x00,										// iInterface

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x85  -> Direction: IN - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0031 = 1 transactions per microframe, 0x31 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousInEndpointAddress85,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x31, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x05  -> Direction: OUT - EndpointID: 5
			bmAttributes:                      0x01  -> Isochronous Transfer Type, Synchronization Type = No Synchronization, Usage Type = Data Endpoint
			wMaxPacketSize:                  0x0031 = 1 transactions per microframe, 0x31 max bytes
			bInterval:                         0x04
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_IsocronousOutEndpointAddress5,							// bEndpointAddress
		USB_ENDPOINT_TYPE_ISOCHRONOUS,  // bmAttributes - bulk
		0x31, 0x00,                      // wMaxPacketSize
		0x04,							// bInterval

		/*
							  ===>Interface Descriptor<===
			bLength:                           0x09
			bDescriptorType:                   0x04
			bInterfaceNumber:                  0x02
			bAlternateSetting:                 0x00
			bNumEndpoints:                     0x05
			bInterfaceClass:                   0xFF  -> Interface Class Unknown to USBView
			bInterfaceSubClass:                0xFF
			bInterfaceProtocol:                0xFF
			iInterface:                        0x05
				 English (United States)  "Wireless LAN Interface"
		*/
		0x9,										// Descriptor size
		USB_INTERFACE_DESCRIPTOR_TYPE,				// Interface Association Descriptor Type
		0x02,                                       // bInterfaceNumber
		0x00,                                       // bAlternateSetting
		0x05,                                       // bNumEndpoints
		0xFF,										// bInterfaceClass
		0xFF,										// bInterfaceSubClass
		0xFF,										// bInterfaceProtocol
		0x05,										// iInterface

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x01  -> Direction: OUT - EndpointID: 1
			bmAttributes:                      0x02  -> Bulk Transfer Type
			wMaxPacketSize:                  0x0200 = 0x200 max bytes
			bInterval:                         0x00
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_BulkOutEndpointAddress1,							// bEndpointAddress
		USB_ENDPOINT_TYPE_BULK,			// bmAttributes - bulk
		0x00, 0x02,                     // wMaxPacketSize
		0x00,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x81  -> Direction: IN - EndpointID: 1
			bmAttributes:                      0x02  -> Bulk Transfer Type
			wMaxPacketSize:                  0x0200 = 0x200 max bytes
			bInterval:                         0x00
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_BulkInEndpointAddress81,							// bEndpointAddress
		USB_ENDPOINT_TYPE_BULK,			// bmAttributes - bulk
		0x00, 0x02,                     // wMaxPacketSize
		0x00,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x02  -> Direction: OUT - EndpointID: 2
			bmAttributes:                      0x02  -> Bulk Transfer Type
			wMaxPacketSize:                  0x0200 = 0x200 max bytes
			bInterval:                         0x00
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_BulkOutEndpointAddress2,							// bEndpointAddress
		USB_ENDPOINT_TYPE_BULK,			// bmAttributes - bulk
		0x00, 0x02,                     // wMaxPacketSize
		0x00,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x82  -> Direction: IN - EndpointID: 2
			bmAttributes:                      0x02  -> Bulk Transfer Type
			wMaxPacketSize:                  0x0200 = 0x200 max bytes
			bInterval:                         0x00
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_BulkInEndpointAddress82,							// bEndpointAddress
		USB_ENDPOINT_TYPE_BULK,			// bmAttributes - bulk
		0x00, 0x02,                     // wMaxPacketSize
		0x00,							// bInterval

		/*
							  ===>Endpoint Descriptor<===
			bLength:                           0x07
			bDescriptorType:                   0x05
			bEndpointAddress:                  0x06  -> Direction: OUT - EndpointID: 6
			bmAttributes:                      0x02  -> Bulk Transfer Type
			wMaxPacketSize:                  0x0200 = 0x200 max bytes
			bInterval:                         0x00
		*/
		0x07,                           // Descriptor size
		USB_ENDPOINT_DESCRIPTOR_TYPE,   // bDescriptorType
		g_BulkOutEndpointAddress6,							// bEndpointAddress
		USB_ENDPOINT_TYPE_BULK,			// bmAttributes - bulk
		0x00, 0x02,                     // wMaxPacketSize
		0x00,							// bInterval
};


//
// Generic descriptor asserts
//
static
FORCEINLINE
VOID
UsbValidateConstants(
)
{
	DbgPrint("[MWIFIEX] UsbValidateConstants\n");
    //
    // C_ASSERT doesn't treat these expressions as constant, so use NT_ASSERT
    //
    NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bString[0] == AMERICAN_ENGLISH);
    //NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bString[1] == PRC_CHINESE);
    NT_ASSERT(((PUSB_DEVICE_DESCRIPTOR)g_UsbDeviceDescriptor)->iManufacturer == g_ManufacturerIndex);
    NT_ASSERT(((PUSB_DEVICE_DESCRIPTOR)g_UsbDeviceDescriptor)->iProduct == g_ProductIndex);

    NT_ASSERT(((PUSB_DEVICE_DESCRIPTOR)g_UsbDeviceDescriptor)->bLength ==
        sizeof(USB_DEVICE_DESCRIPTOR));
    NT_ASSERT(sizeof(g_UsbDeviceDescriptor) == sizeof(USB_DEVICE_DESCRIPTOR));
    NT_ASSERT(((PUSB_CONFIGURATION_DESCRIPTOR)g_UsbConfigDescriptorSet)->wTotalLength ==
        sizeof(g_UsbConfigDescriptorSet));
    NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bLength ==
        sizeof(g_LanguageDescriptor));

    NT_ASSERT(((PUSB_DEVICE_DESCRIPTOR)g_UsbDeviceDescriptor)->bDescriptorType ==
        USB_DEVICE_DESCRIPTOR_TYPE);
    NT_ASSERT(((PUSB_CONFIGURATION_DESCRIPTOR)g_UsbConfigDescriptorSet)->bDescriptorType ==
        USB_CONFIGURATION_DESCRIPTOR_TYPE);
    NT_ASSERT(((PUSB_STRING_DESCRIPTOR)g_LanguageDescriptor)->bDescriptorType ==
        USB_STRING_DESCRIPTOR_TYPE);
}


// END ------------------ descriptor -------------------------------





NTSTATUS
Usb_Initialize(
    _In_ WDFDEVICE WdfDevice
)
{
	DbgPrint("[MWIFIEX] Usb_Initialize\n");
    NTSTATUS                                status;
    PUDECX_USBCONTROLLER_CONTEXT            controllerContext;
    UDECX_USB_DEVICE_STATE_CHANGE_CALLBACKS   callbacks;



    //
    // Allocate per-controller private contexts used by other source code modules (I/O,
    // etc.)
    //


    controllerContext = GetUsbControllerContext(WdfDevice);

    UsbValidateConstants();

    controllerContext->ChildDeviceInit = UdecxUsbDeviceInitAllocate(WdfDevice);

    if (controllerContext->ChildDeviceInit == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(TRACE_DEVICE, "Failed to allocate UDECXUSBDEVICE_INIT %!STATUS!", status);
        goto exit;
    }

    //
    // State changed callbacks
    //
    UDECX_USB_DEVICE_CALLBACKS_INIT(&callbacks);

    callbacks.EvtUsbDeviceLinkPowerEntry = UsbDevice_EvtUsbDeviceLinkPowerEntry;
    callbacks.EvtUsbDeviceLinkPowerExit = UsbDevice_EvtUsbDeviceLinkPowerExit;
    callbacks.EvtUsbDeviceSetFunctionSuspendAndWake = UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake;

    UdecxUsbDeviceInitSetStateChangeCallbacks(controllerContext->ChildDeviceInit, &callbacks);

    //
    // Set required attributes.
    //
    UdecxUsbDeviceInitSetSpeed(controllerContext->ChildDeviceInit, UdecxUsbHighSpeed);

    UdecxUsbDeviceInitSetEndpointsType(controllerContext->ChildDeviceInit, UdecxEndpointTypeSimple);

    //
    // Device descriptor
    //
    status = UdecxUsbDeviceInitAddDescriptor(controllerContext->ChildDeviceInit,
        (PUCHAR)g_UsbDeviceDescriptor,
        sizeof(g_UsbDeviceDescriptor));

    if (!NT_SUCCESS(status)) {

        goto exit;
    }


    //
    // String descriptors
    //
    status = UdecxUsbDeviceInitAddDescriptorWithIndex(controllerContext->ChildDeviceInit,
        (PUCHAR)g_LanguageDescriptor,
        sizeof(g_LanguageDescriptor),
        0);

    if (!NT_SUCCESS(status)) {

        goto exit;
    }

    status = UdecxUsbDeviceInitAddStringDescriptor(controllerContext->ChildDeviceInit,
        &g_ManufacturerStringEnUs,
        g_ManufacturerIndex,
        AMERICAN_ENGLISH);

    if (!NT_SUCCESS(status)) {

        goto exit;
    }

    status = UdecxUsbDeviceInitAddStringDescriptor(controllerContext->ChildDeviceInit,
        &g_ProductStringEnUs,
        g_ProductIndex,
        AMERICAN_ENGLISH);

    if (!NT_SUCCESS(status)) {

        goto exit;
    }

	status = UdecxUsbDeviceInitAddStringDescriptor(controllerContext->ChildDeviceInit,
		&g_SerialStringEnUs,
		g_SerialIndex,
		AMERICAN_ENGLISH);

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	status = UdecxUsbDeviceInitAddStringDescriptor(controllerContext->ChildDeviceInit,
		&g_InterfaceStringEnUs,
		g_InterfaceIndex,
		AMERICAN_ENGLISH);

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

    //
    // Remaining init requires lower edge interaction.  Postpone to Usb_ReadDescriptorsAndPlugIn.
    //

exit:

    //
    // On failure in this function (or later but still before creating the UDECXUSBDEVICE),
    // UdecxUsbDeviceInit will be freed by Usb_Destroy.
    //

    return status;
}





NTSTATUS
Usb_ReadDescriptorsAndPlugIn(
    _In_ WDFDEVICE WdfControllerDevice
)
{
	DbgPrint("[MWIFIEX] Usb_ReadDescriptorsAndPlugIn\n");
    NTSTATUS                          status;
    PUSB_CONFIGURATION_DESCRIPTOR     pComputedConfigDescSet;
    WDF_OBJECT_ATTRIBUTES             attributes;
    PUDECX_USBCONTROLLER_CONTEXT      controllerContext;
    PUSB_CONTEXT                      deviceContext = NULL;
    UDECX_USB_DEVICE_PLUG_IN_OPTIONS  pluginOptions;

    controllerContext = GetUsbControllerContext(WdfControllerDevice);
    pComputedConfigDescSet = NULL;

    //
    // Compute configuration descriptor dynamically.
    //
    pComputedConfigDescSet = (PUSB_CONFIGURATION_DESCRIPTOR)
        ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(g_UsbConfigDescriptorSet), UDECXMBIM_POOL_TAG);

    if (pComputedConfigDescSet == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(TRACE_DEVICE, "Failed to allocate %d bytes for temporary config descriptor %!STATUS!",
            sizeof(g_UsbConfigDescriptorSet), status);
        goto exit;
    }

    RtlCopyMemory(pComputedConfigDescSet,
        g_UsbConfigDescriptorSet,
        sizeof(g_UsbConfigDescriptorSet));

    status = UdecxUsbDeviceInitAddDescriptor(controllerContext->ChildDeviceInit,
        (PUCHAR)pComputedConfigDescSet,
        sizeof(g_UsbConfigDescriptorSet));

    if (!NT_SUCCESS(status)) {

        goto exit;
    }


    //
    // Create emulated USB device
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, USB_CONTEXT);

    status = UdecxUsbDeviceCreate(&controllerContext->ChildDeviceInit,
        &attributes,
        &(controllerContext->ChildDevice) );

    if (!NT_SUCCESS(status)) {

        goto exit;
    }


    status = Io_AllocateContext(controllerContext->ChildDevice);
    if (!NT_SUCCESS(status)) {

        goto exit;
    }


    deviceContext = GetUsbDeviceContext(controllerContext->ChildDevice);

    // create link to parent
    deviceContext->ControllerDevice = WdfControllerDevice;


    LogInfo(TRACE_DEVICE, "USB device created, controller=%p, UsbDevice=%p",
        WdfControllerDevice, controllerContext->ChildDevice);

    deviceContext->IsAwake = TRUE;  // for some strange reason, it starts out awake!

	// We have different endpoints

	// Control Endpoint
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		USB_DEFAULT_ENDPOINT_ADDRESS,
		&(deviceContext->MWIFIEXControlEndpoint));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_BulkOutEndpointAddress1,
		&(deviceContext->MWIFIEXBulkOutEndpoint1));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_BulkOutEndpointAddress2,
		&(deviceContext->MWIFIEXBulkOutEndpoint2));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_BulkOutEndpointAddress4,
		&(deviceContext->MWIFIEXBulkOutEndpoint4));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_BulkOutEndpointAddress6,
		&(deviceContext->MWIFIEXBulkOutEndpoint6));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_BulkInEndpointAddress81,
		&(deviceContext->MWIFIEXBulkInEndpoint81));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_BulkInEndpointAddress82,
		&(deviceContext->MWIFIEXBulkInEndpoint82));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_BulkInEndpointAddress84,
		&(deviceContext->MWIFIEXBulkInEndpoint84));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_InterruptEndpointAddress83,
		&(deviceContext->MWIFIEXInterruptInEndpoint83));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_IsocronousOutEndpointAddress5,
		&(deviceContext->MWIFIEXIsoOutEndpoint5));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		g_IsocronousInEndpointAddress85,
		&(deviceContext->MWIFIEXIsoInEndpoint85));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

	// ENDPOINT
	status = UsbCreateEndpointObj(controllerContext->ChildDevice,
		USB_DEFAULT_ENDPOINT_ADDRESS,
		&(deviceContext->MWIFIEXControlEndpoint));

	if (!NT_SUCCESS(status)) {

		goto exit;
	}

    //
    // This begins USB communication and prevents us from modifying descriptors and simple endpoints.
    //
    UDECX_USB_DEVICE_PLUG_IN_OPTIONS_INIT(&pluginOptions);
    pluginOptions.Usb20PortNumber = 1;
    status = UdecxUsbDevicePlugIn(controllerContext->ChildDevice, &pluginOptions);

	// TODO_rc1
	// Pause here long enough for the mwlu97w8x64.sys driver to load
	// We need to get the address range of this driver
	// This will be problematic if we let the USB URB timeout
	// Preloading the driver is the best way to go if we could make that work be connecting the 2043 first
	// AFTER this happens is when we need to load in kAFL payload data


    LogInfo(TRACE_DEVICE, "Usb_ReadDescriptorsAndPlugIn ends successfully");

exit:

    //
    // Free temporary allocation always.
    //
    if (pComputedConfigDescSet != NULL) {

        ExFreePoolWithTag(pComputedConfigDescSet, UDECXMBIM_POOL_TAG);
        pComputedConfigDescSet = NULL;
    }

    return status;
}

NTSTATUS
Usb_Disconnect(
    _In_  WDFDEVICE WdfDevice
)
{
	DbgPrint("[MWIFIEX] Usb_Disconnect\n");
    NTSTATUS status;
    PUDECX_USBCONTROLLER_CONTEXT controllerCtx;
    IO_CONTEXT ioContextCopy;


    controllerCtx = GetUsbControllerContext(WdfDevice);

    Io_StopDeferredProcessing(controllerCtx->ChildDevice, &ioContextCopy);

    status = UdecxUsbDevicePlugOutAndDelete(controllerCtx->ChildDevice);
    // Not deleting the queues that belong to the controller, as this
    // happens only in the last disconnect.  But if we were to connect again,
    // we would need to do that as the queues would leak.

    if (!NT_SUCCESS(status)) {
        LogError(TRACE_DEVICE, "UdecxUsbDevicePlugOutAndDelete failed with %!STATUS!", status);
        goto exit;
    }

    Io_FreeEndpointQueues(&ioContextCopy);

    LogInfo(TRACE_DEVICE, "Usb_Disconnect ends successfully");

exit:

    return status;
}


VOID
Usb_Destroy(
    _In_ WDFDEVICE WdfDevice
)
{
	DbgPrint("[MWIFIEX] Usb_Destroy\n");
    PUDECX_USBCONTROLLER_CONTEXT pControllerContext;

    pControllerContext = GetUsbControllerContext(WdfDevice);

    //
    // Free device init in case we didn't successfully create the device.
    //
    if (pControllerContext != NULL && pControllerContext->ChildDeviceInit != NULL) {

        UdecxUsbDeviceInitFree(pControllerContext->ChildDeviceInit);
        pControllerContext->ChildDeviceInit = NULL;
    }
    LogError(TRACE_DEVICE, "Usb_Destroy ends successfully");

    return;
}

VOID
Usb_UdecxUsbEndpointEvtReset(
    _In_ UCXCONTROLLER ctrl,
    _In_ UCXENDPOINT ep,
    _In_ WDFREQUEST r
)
{
	DbgPrint("[MWIFIEX] Usb_UdecxUsbEndpointEvtReset\n");
    UNREFERENCED_PARAMETER(ctrl);
    UNREFERENCED_PARAMETER(ep);
    UNREFERENCED_PARAMETER(r);

    // TODO: endpoint reset. will require a different function prototype
}



NTSTATUS
UsbCreateEndpointObj(
    _In_   UDECXUSBDEVICE    WdfUsbChildDevice,
    _In_   UCHAR             epAddr,
    _Out_  UDECXUSBENDPOINT *pNewEpObjAddr
)
{
	DbgPrint("[MWIFIEX] UsbCreateEndpointObj\n");
    NTSTATUS                      status;
    PUSB_CONTEXT                  pUsbContext;
    WDFQUEUE                      epQueue;
    UDECX_USB_ENDPOINT_CALLBACKS  callbacks;
    PUDECXUSBENDPOINT_INIT        endpointInit;


    pUsbContext = GetUsbDeviceContext(WdfUsbChildDevice);
    endpointInit = NULL;

    status = Io_RetrieveEpQueue(WdfUsbChildDevice, epAddr, &epQueue);

    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    endpointInit = UdecxUsbSimpleEndpointInitAllocate(WdfUsbChildDevice);

    if (endpointInit == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        LogError(TRACE_DEVICE, "Failed to allocate endpoint init %!STATUS!", status);
        goto exit;
    }

    UdecxUsbEndpointInitSetEndpointAddress(endpointInit, epAddr);

    UDECX_USB_ENDPOINT_CALLBACKS_INIT(&callbacks, UsbEndpointReset);
    UdecxUsbEndpointInitSetCallbacks(endpointInit, &callbacks);

    status = UdecxUsbEndpointCreate(&endpointInit,
        WDF_NO_OBJECT_ATTRIBUTES,
        pNewEpObjAddr );

    if (!NT_SUCCESS(status)) {

        LogError(TRACE_DEVICE, "UdecxUsbEndpointCreate failed for endpoint %x, %!STATUS!", epAddr, status);
        goto exit;
    }

    UdecxUsbEndpointSetWdfIoQueue( *pNewEpObjAddr,  epQueue);

exit:

    if (endpointInit != NULL) {

        NT_ASSERT(!NT_SUCCESS(status));
        UdecxUsbEndpointInitFree(endpointInit);
        endpointInit = NULL;
    }

    return status;
}





VOID
UsbEndpointReset(
    _In_ UDECXUSBENDPOINT UdecxUsbEndpoint,
    _In_ WDFREQUEST     Request
)
{
	DbgPrint("[MWIFIEX] UsbEndpointReset\n");
    UNREFERENCED_PARAMETER(UdecxUsbEndpoint);
    UNREFERENCED_PARAMETER(Request);
}



VOID
UsbDevice_EvtUsbDeviceEndpointsConfigure(
    _In_ UDECXUSBDEVICE                    UdecxUsbDevice,
    _In_ WDFREQUEST                        Request,
    _In_ PUDECX_ENDPOINTS_CONFIGURE_PARAMS Params
)
{
	DbgPrint("[MWIFIEX] UsbDevice_EvtUsbDeviceEndpointsConfigure\n");
    UNREFERENCED_PARAMETER(UdecxUsbDevice);
    UNREFERENCED_PARAMETER(Params);

    WdfRequestComplete(Request, STATUS_SUCCESS);
}

NTSTATUS
UsbDevice_EvtUsbDeviceLinkPowerEntry(
    _In_ WDFDEVICE       UdecxWdfDevice,
    _In_ UDECXUSBDEVICE    UdecxUsbDevice )
{
	DbgPrint("[MWIFIEX] UsbDevice_EvtUsbDeviceLinkPowerEntry\n");
    PUSB_CONTEXT pUsbContext;
    UNREFERENCED_PARAMETER(UdecxWdfDevice);

    pUsbContext = GetUsbDeviceContext(UdecxUsbDevice);
    Io_DeviceWokeUp(UdecxUsbDevice);
    pUsbContext->IsAwake = TRUE;
    LogInfo(TRACE_DEVICE, "USB Device power ENTRY");

    return STATUS_SUCCESS;
}

NTSTATUS
UsbDevice_EvtUsbDeviceLinkPowerExit(
    _In_ WDFDEVICE UdecxWdfDevice,
    _In_ UDECXUSBDEVICE UdecxUsbDevice,
    _In_ UDECX_USB_DEVICE_WAKE_SETTING WakeSetting )
{
	DbgPrint("[MWIFIEX] UsbDevice_EvtUsbDeviceLinkPowerExit\n");
    PUSB_CONTEXT pUsbContext;
    UNREFERENCED_PARAMETER(UdecxWdfDevice);

    pUsbContext = GetUsbDeviceContext(UdecxUsbDevice);
    pUsbContext->IsAwake = FALSE;

    Io_DeviceSlept(UdecxUsbDevice);

    LogInfo(TRACE_DEVICE, "USB Device power EXIT [wdfDev=%p, usbDev=%p], WakeSetting=%x", UdecxWdfDevice, UdecxUsbDevice, WakeSetting);
    return STATUS_SUCCESS;
}

NTSTATUS
UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake(
    _In_ WDFDEVICE                        UdecxWdfDevice,
    _In_ UDECXUSBDEVICE                   UdecxUsbDevice,
    _In_ ULONG                            Interface,
    _In_ UDECX_USB_DEVICE_FUNCTION_POWER  FunctionPower
)
{
	DbgPrint("[MWIFIEX] UsbDevice_EvtUsbDeviceSetFunctionSuspendAndWake\n");
    UNREFERENCED_PARAMETER(UdecxWdfDevice);
    UNREFERENCED_PARAMETER(UdecxUsbDevice);
    UNREFERENCED_PARAMETER(Interface);
    UNREFERENCED_PARAMETER(FunctionPower);

    // this never gets printed!
    LogInfo(TRACE_DEVICE, "USB Device SuspendAwakeState=%x", FunctionPower );

    return STATUS_SUCCESS;
}





