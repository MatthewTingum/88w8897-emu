/*
 * QEMU USB Net devices
 *
 * Copyright (c) 2006 Thomas Sailer
 * Copyright (c) 2008 Andrzej Zaborowski
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "hw/usb.h"
#include "migration/vmstate.h"
#include "desc.h"
#include "net/net.h"
#include "qemu/error-report.h"
#include "qemu/queue.h"
#include "qemu/config-file.h"
#include "sysemu/sysemu.h"
#include "qemu/iov.h"
#include "qemu/module.h"
#include "qemu/cutils.h"
#include "hw/mwifiex/fw.h"

/*#define TRAFFIC_DEBUG*/

/* PID 2 of the 8797,
 * Skip PID 1 (FW Update - 0x2043) for now
 */
#define USB_VENDOR_NUM        0x1286  /* MARVELL SEMICONDUCTOR, INC. */
// this is set to 0x2043 on FW Download and 0x2044 for Active
//#define USB_PRODUCT_NUM       0x2044  /* USB8797_PID_2 */
#define USB_PRODUCT_NUM       0x204E  /* USB8997_PID_2 */

enum usbstring_idx {
    STRING_MANUFACTURER		= 1,
    STRING_PRODUCT,
    STRING_SERIALNUMBER,
    STRING_UNUSED,
    STRING_WIRELESS_LAN_INTERFACE,
};

#define DEV_CONFIG_VALUE		1	/* CDC or a subset */
#define DEV_RNDIS_CONFIG_VALUE		2	/* RNDIS; optional */

#define USB_CDC_SUBCLASS_ACM		0x02
#define USB_CDC_SUBCLASS_ETHERNET	0x06

#define USB_CDC_PROTO_NONE		0
#define USB_CDC_ACM_PROTO_VENDOR	0xff

#define USB_CDC_HEADER_TYPE		0x00	/* header_desc */
#define USB_CDC_CALL_MANAGEMENT_TYPE	0x01	/* call_mgmt_descriptor */
#define USB_CDC_ACM_TYPE		0x02	/* acm_descriptor */
#define USB_CDC_UNION_TYPE		0x06	/* union_desc */
#define USB_CDC_ETHERNET_TYPE		0x0f	/* ether_desc */

#define USB_CDC_SEND_ENCAPSULATED_COMMAND	0x00
#define USB_CDC_GET_ENCAPSULATED_RESPONSE	0x01
#define USB_CDC_REQ_SET_LINE_CODING		0x20
#define USB_CDC_REQ_GET_LINE_CODING		0x21
#define USB_CDC_REQ_SET_CONTROL_LINE_STATE	0x22
#define USB_CDC_REQ_SEND_BREAK			0x23
#define USB_CDC_SET_ETHERNET_MULTICAST_FILTERS	0x40
#define USB_CDC_SET_ETHERNET_PM_PATTERN_FILTER	0x41
#define USB_CDC_GET_ETHERNET_PM_PATTERN_FILTER	0x42
#define USB_CDC_SET_ETHERNET_PACKET_FILTER	0x43
#define USB_CDC_GET_ETHERNET_STATISTIC		0x44

#define LOG2_STATUS_INTERVAL_MSEC	5    /* 1 << 5 == 32 msec */
#define STATUS_BYTECOUNT		16   /* 8 byte header + data */

#define ETH_FRAME_LEN			1514 /* Max. octets in frame sans FCS */

static const USBDescStrings usb_net_stringtable = {
    [STRING_MANUFACTURER]           = "Marvell",
    [STRING_PRODUCT]                = "Bluetooth and Wireless LAN Composite Device",
    [STRING_SERIALNUMBER]           = "6045BDDB2C7C",
    [STRING_UNUSED]                 = "Unused",
    [STRING_WIRELESS_LAN_INTERFACE] = "Wireless LAN Interface",
};

static const USBDescIface desc_iface_rndis[] = {
    {
        /* Zero Interface */
        .bInterfaceNumber              = 0,
        .bNumEndpoints                 = 3,
        .bInterfaceClass               = 0xE0,
        .bInterfaceSubClass            = 0x01,
        .bInterfaceProtocol            = 0x01,
        .iInterface                    = 0x00,
        .ndesc                         = 0,
        .descs = (USBDescOther[]) {
        },
        .eps = (USBDescEndpoint[]) {
            {
                .bEndpointAddress      = USB_DIR_IN | 0x03,
                .bmAttributes          = USB_ENDPOINT_XFER_INT,
                .wMaxPacketSize        = 0x0040,
                .bInterval             = 0x01,
            },
            {
                .bEndpointAddress      = USB_DIR_OUT | 0x04,
                .bmAttributes          = USB_ENDPOINT_XFER_BULK,
                .wMaxPacketSize        = 0x0200,
                .bInterval             = 0x00,
            },
            {
                .bEndpointAddress      = USB_DIR_IN | 0x04,
                .bmAttributes          = USB_ENDPOINT_XFER_BULK,
                .wMaxPacketSize        = 0x0200,
                .bInterval             = 0xFF,
            },
        }
    },{
        /* One Interface */
        .bInterfaceNumber              = 1,
        .bAlternateSetting             = 0,
        .bNumEndpoints                 = 2,
        .bInterfaceClass               = 0xE0,
        .bInterfaceSubClass            = 0x01,
        .bInterfaceProtocol            = 0x01,
        .iInterface                    = 0x00,
        .eps = (USBDescEndpoint[]) {
            {
                .bEndpointAddress      = USB_DIR_IN | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0000,
                .bInterval             = 0x04,
            },{
                .bEndpointAddress      = USB_DIR_OUT | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0000,
                .bInterval             = 0x04,
            }
        }
    },{
        /* RNDIS Data Interface */
        .bInterfaceNumber              = 1,
        .bAlternateSetting             = 1,
        .bNumEndpoints                 = 2,
        .bInterfaceClass               = 0xE0,
        .bInterfaceSubClass            = 0x01,
        .bInterfaceProtocol            = 0x01,
        .iInterface                    = 0x00,
        .eps = (USBDescEndpoint[]) {
            {
                .bEndpointAddress      = USB_DIR_IN | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0009,
                .bInterval             = 0x04,
            },{
                .bEndpointAddress      = USB_DIR_OUT | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0009,
                .bInterval             = 0x04,
            }
        }
    },{
        /* RNDIS Data Interface */
        .bInterfaceNumber              = 1,
        .bAlternateSetting             = 2,
        .bNumEndpoints                 = 2,
        .bInterfaceClass               = 0xE0,
        .bInterfaceSubClass            = 0x01,
        .bInterfaceProtocol            = 0x01,
        .iInterface                    = 0x00,
        .eps = (USBDescEndpoint[]) {
            {
                .bEndpointAddress      = USB_DIR_IN | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0011,
                .bInterval             = 0x04,
            },{
                .bEndpointAddress      = USB_DIR_OUT | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0011,
                .bInterval             = 0x04,
            }
        }
    },{
        /* RNDIS Data Interface */
        .bInterfaceNumber              = 1,
        .bAlternateSetting             = 3,
        .bNumEndpoints                 = 2,
        .bInterfaceClass               = 0xE0,
        .bInterfaceSubClass            = 0x01,
        .bInterfaceProtocol            = 0x01,
        .iInterface                    = 0x00,
        .eps = (USBDescEndpoint[]) {
            {
                .bEndpointAddress      = USB_DIR_IN | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0019,
                .bInterval             = 0x04,
            },{
                .bEndpointAddress      = USB_DIR_OUT | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0019,
                .bInterval             = 0x04,
            }
        }
    },{
        /* RNDIS Data Interface */
        .bInterfaceNumber              = 1,
        .bAlternateSetting             = 4,
        .bNumEndpoints                 = 2,
        .bInterfaceClass               = 0xE0,
        .bInterfaceSubClass            = 0x01,
        .bInterfaceProtocol            = 0x01,
        .iInterface                    = 0x00,
        .eps = (USBDescEndpoint[]) {
            {
                .bEndpointAddress      = USB_DIR_IN | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0021,
                .bInterval             = 0x04,
            },{
                .bEndpointAddress      = USB_DIR_OUT | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0021,
                .bInterval             = 0x04,
            }
        }
    },{
        /* RNDIS Data Interface */
        .bInterfaceNumber              = 1,
        .bAlternateSetting             = 5,
        .bNumEndpoints                 = 2,
        .bInterfaceClass               = 0xE0,
        .bInterfaceSubClass            = 0x01,
        .bInterfaceProtocol            = 0x01,
        .iInterface                    = 0x00,
        .eps = (USBDescEndpoint[]) {
            {
                .bEndpointAddress      = USB_DIR_IN | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0031,
                .bInterval             = 0x04,
            },{
                .bEndpointAddress      = USB_DIR_OUT | 0x05,
                .bmAttributes          = USB_ENDPOINT_XFER_ISOC,
                .wMaxPacketSize        = 0x0031,
                .bInterval             = 0x04,
            }
        }
    },{
        /* CAREFUL -- THE LAST ENDPOINT WAS CHANGED */
        .bInterfaceNumber              = 2,
        .bAlternateSetting             = 0,
        .bNumEndpoints                 = 5,
        .bInterfaceClass               = 0xFF,
        .bInterfaceSubClass            = 0xFF,
        .bInterfaceProtocol            = 0xFF,
        .iInterface                    = STRING_WIRELESS_LAN_INTERFACE,
        .eps = (USBDescEndpoint[]) {
            {
                .bEndpointAddress      = USB_DIR_OUT | 0x01,
                .bmAttributes          = USB_ENDPOINT_XFER_BULK,
                .wMaxPacketSize        = 0x0200,
                .bInterval             = 0x00,
            },{
                .bEndpointAddress      = USB_DIR_IN | 0x01,
                .bmAttributes          = USB_ENDPOINT_XFER_BULK,
                .wMaxPacketSize        = 0x0200,
                .bInterval             = 0x00,
            },{
                .bEndpointAddress      = USB_DIR_OUT | 0x02,
                .bmAttributes          = USB_ENDPOINT_XFER_BULK,
                .wMaxPacketSize        = 0x0200,
                .bInterval             = 0x00,
            },{
                .bEndpointAddress      = USB_DIR_IN | 0x02,
                .bmAttributes          = USB_ENDPOINT_XFER_BULK,
                .wMaxPacketSize        = 0x0200,
                .bInterval             = 0x00,
            },{
                .bEndpointAddress      = USB_DIR_OUT | 0x03,
                .bmAttributes          = USB_ENDPOINT_XFER_BULK,
                .wMaxPacketSize        = 0x0200,
                .bInterval             = 0x00,
            }
        }
    }
};

static const USBDescIfaceAssoc desc_iface_assoc_rndis[] = {
    {
        .bFirstInterface        = 0x00,
        .bInterfaceCount        = 0x02,
        .bFunctionClass         = 0xE0,
        .bFunctionSubClass      = 0x01,
        .bFunctionProtocol      = 0x01,
        .iFunction              = 0x00,
        .nif                    = ARRAY_SIZE(desc_iface_rndis),
        .ifs                    = desc_iface_rndis,
    }
};

static const USBDescDevice desc_device_net = {
    .bcdUSB                        = 0x0200,
    .bDeviceClass                  = 0xEF,  // USB_CLASS_MI
    .bDeviceSubClass               = 0x02,
    .bDeviceProtocol               = 0x01,
    .bMaxPacketSize0               = 0x40,
    .bNumConfigurations            = 1,
    .confs = (USBDescConfig[]) {
        {
            .bNumInterfaces        = 3,
            .bConfigurationValue   = 1,
            .iConfiguration        = 0,
            .bmAttributes          = USB_CFG_ATT_ONE | USB_CFG_ATT_SELFPOWER | USB_CFG_ATT_WAKEUP,
            .bMaxPower             = 0xFA,
            .nif_groups = ARRAY_SIZE(desc_iface_assoc_rndis),
            .if_groups = desc_iface_assoc_rndis,
            //.nif = ARRAY_SIZE(desc_iface_rndis),
            //.ifs = desc_iface_rndis,
        }
    },
};

static const USBDesc desc_net = {
    .id = {
        .idVendor          = USB_VENDOR_NUM,
        .idProduct         = USB_PRODUCT_NUM,
        .bcdDevice         = 0x3201,
        .iManufacturer     = STRING_MANUFACTURER,
        .iProduct          = STRING_PRODUCT,
        .iSerialNumber     = STRING_SERIALNUMBER,
    },
    .full = &desc_device_net,
    .str  = usb_net_stringtable,
};

/*
 * RNDIS Definitions - in theory not specific to USB.
 */
#define RNDIS_MAXIMUM_FRAME_SIZE	1518
#define RNDIS_MAX_TOTAL_SIZE		1558

/* Remote NDIS Versions */
#define RNDIS_MAJOR_VERSION		1
#define RNDIS_MINOR_VERSION		0

/* Status Values */
#define RNDIS_STATUS_SUCCESS		0x00000000U /* Success */
#define RNDIS_STATUS_FAILURE		0xc0000001U /* Unspecified error */
#define RNDIS_STATUS_INVALID_DATA	0xc0010015U /* Invalid data */
#define RNDIS_STATUS_NOT_SUPPORTED	0xc00000bbU /* Unsupported request */
#define RNDIS_STATUS_MEDIA_CONNECT	0x4001000bU /* Device connected */
#define RNDIS_STATUS_MEDIA_DISCONNECT	0x4001000cU /* Device disconnected */

/* Message Set for Connectionless (802.3) Devices */
enum {
    RNDIS_PACKET_MSG		= 1,
    RNDIS_INITIALIZE_MSG	= 2,	/* Initialize device */
    RNDIS_HALT_MSG		= 3,
    RNDIS_QUERY_MSG		= 4,
    RNDIS_SET_MSG		= 5,
    RNDIS_RESET_MSG		= 6,
    RNDIS_INDICATE_STATUS_MSG	= 7,
    RNDIS_KEEPALIVE_MSG		= 8,
};

/* Message completion */
enum {
    RNDIS_INITIALIZE_CMPLT	= 0x80000002U,
    RNDIS_QUERY_CMPLT		= 0x80000004U,
    RNDIS_SET_CMPLT		= 0x80000005U,
    RNDIS_RESET_CMPLT		= 0x80000006U,
    RNDIS_KEEPALIVE_CMPLT	= 0x80000008U,
};

/* Device Flags */
enum {
    RNDIS_DF_CONNECTIONLESS	= 1,
    RNDIS_DF_CONNECTIONORIENTED	= 2,
};

#define RNDIS_MEDIUM_802_3		0x00000000U

/* from drivers/net/sk98lin/h/skgepnmi.h */
#define OID_PNP_CAPABILITIES		0xfd010100
#define OID_PNP_SET_POWER		0xfd010101
#define OID_PNP_QUERY_POWER		0xfd010102
#define OID_PNP_ADD_WAKE_UP_PATTERN	0xfd010103
#define OID_PNP_REMOVE_WAKE_UP_PATTERN	0xfd010104
#define OID_PNP_ENABLE_WAKE_UP		0xfd010106

typedef uint32_t le32;

typedef struct rndis_init_msg_type {
    le32 MessageType;
    le32 MessageLength;
    le32 RequestID;
    le32 MajorVersion;
    le32 MinorVersion;
    le32 MaxTransferSize;
} rndis_init_msg_type;

typedef struct rndis_init_cmplt_type {
    le32 MessageType;
    le32 MessageLength;
    le32 RequestID;
    le32 Status;
    le32 MajorVersion;
    le32 MinorVersion;
    le32 DeviceFlags;
    le32 Medium;
    le32 MaxPacketsPerTransfer;
    le32 MaxTransferSize;
    le32 PacketAlignmentFactor;
    le32 AFListOffset;
    le32 AFListSize;
} rndis_init_cmplt_type;

typedef struct rndis_halt_msg_type {
    le32 MessageType;
    le32 MessageLength;
    le32 RequestID;
} rndis_halt_msg_type;

typedef struct rndis_query_msg_type {
    le32 MessageType;
    le32 MessageLength;
    le32 RequestID;
    le32 OID;
    le32 InformationBufferLength;
    le32 InformationBufferOffset;
    le32 DeviceVcHandle;
} rndis_query_msg_type;

typedef struct rndis_query_cmplt_type {
    le32 MessageType;
    le32 MessageLength;
    le32 RequestID;
    le32 Status;
    le32 InformationBufferLength;
    le32 InformationBufferOffset;
} rndis_query_cmplt_type;

typedef struct rndis_set_msg_type {
    le32 MessageType;
    le32 MessageLength;
    le32 RequestID;
    le32 OID;
    le32 InformationBufferLength;
    le32 InformationBufferOffset;
    le32 DeviceVcHandle;
} rndis_set_msg_type;

typedef struct rndis_set_cmplt_type {
    le32 MessageType;
    le32 MessageLength;
    le32 RequestID;
    le32 Status;
} rndis_set_cmplt_type;

typedef struct rndis_reset_msg_type {
    le32 MessageType;
    le32 MessageLength;
    le32 Reserved;
} rndis_reset_msg_type;

typedef struct rndis_reset_cmplt_type {
    le32 MessageType;
    le32 MessageLength;
    le32 Status;
    le32 AddressingReset;
} rndis_reset_cmplt_type;

typedef struct rndis_indicate_status_msg_type {
    le32 MessageType;
    le32 MessageLength;
    le32 Status;
    le32 StatusBufferLength;
    le32 StatusBufferOffset;
} rndis_indicate_status_msg_type;

typedef struct rndis_keepalive_msg_type {
    le32 MessageType;
    le32 MessageLength;
    le32 RequestID;
} rndis_keepalive_msg_type;

typedef struct rndis_keepalive_cmplt_type {
    le32 MessageType;
    le32 MessageLength;
    le32 RequestID;
    le32 Status;
} rndis_keepalive_cmplt_type;

struct rndis_packet_msg_type {
    le32 MessageType;
    le32 MessageLength;
    le32 DataOffset;
    le32 DataLength;
    le32 OOBDataOffset;
    le32 OOBDataLength;
    le32 NumOOBDataElements;
    le32 PerPacketInfoOffset;
    le32 PerPacketInfoLength;
    le32 VcHandle;
    le32 Reserved;
};

struct rndis_config_parameter {
    le32 ParameterNameOffset;
    le32 ParameterNameLength;
    le32 ParameterType;
    le32 ParameterValueOffset;
    le32 ParameterValueLength;
};

/* implementation specific */
enum rndis_state
{
    RNDIS_UNINITIALIZED,
    RNDIS_INITIALIZED,
    RNDIS_DATA_INITIALIZED,
};

/* from ndis.h */
enum ndis_oid {
    /* Required Object IDs (OIDs) */
    OID_GEN_SUPPORTED_LIST		= 0x00010101,
    OID_GEN_HARDWARE_STATUS		= 0x00010102,
    OID_GEN_MEDIA_SUPPORTED		= 0x00010103,
    OID_GEN_MEDIA_IN_USE		= 0x00010104,
    OID_GEN_MAXIMUM_LOOKAHEAD		= 0x00010105,
    OID_GEN_MAXIMUM_FRAME_SIZE		= 0x00010106,
    OID_GEN_LINK_SPEED			= 0x00010107,
    OID_GEN_TRANSMIT_BUFFER_SPACE	= 0x00010108,
    OID_GEN_RECEIVE_BUFFER_SPACE	= 0x00010109,
    OID_GEN_TRANSMIT_BLOCK_SIZE		= 0x0001010a,
    OID_GEN_RECEIVE_BLOCK_SIZE		= 0x0001010b,
    OID_GEN_VENDOR_ID			= 0x0001010c,
    OID_GEN_VENDOR_DESCRIPTION		= 0x0001010d,
    OID_GEN_CURRENT_PACKET_FILTER	= 0x0001010e,
    OID_GEN_CURRENT_LOOKAHEAD		= 0x0001010f,
    OID_GEN_DRIVER_VERSION		= 0x00010110,
    OID_GEN_MAXIMUM_TOTAL_SIZE		= 0x00010111,
    OID_GEN_PROTOCOL_OPTIONS		= 0x00010112,
    OID_GEN_MAC_OPTIONS			= 0x00010113,
    OID_GEN_MEDIA_CONNECT_STATUS	= 0x00010114,
    OID_GEN_MAXIMUM_SEND_PACKETS	= 0x00010115,
    OID_GEN_VENDOR_DRIVER_VERSION	= 0x00010116,
    OID_GEN_SUPPORTED_GUIDS		= 0x00010117,
    OID_GEN_NETWORK_LAYER_ADDRESSES	= 0x00010118,
    OID_GEN_TRANSPORT_HEADER_OFFSET	= 0x00010119,
    OID_GEN_MACHINE_NAME		= 0x0001021a,
    OID_GEN_RNDIS_CONFIG_PARAMETER	= 0x0001021b,
    OID_GEN_VLAN_ID			= 0x0001021c,

    /* Optional OIDs */
    OID_GEN_MEDIA_CAPABILITIES		= 0x00010201,
    OID_GEN_PHYSICAL_MEDIUM		= 0x00010202,

    /* Required statistics OIDs */
    OID_GEN_XMIT_OK			= 0x00020101,
    OID_GEN_RCV_OK			= 0x00020102,
    OID_GEN_XMIT_ERROR			= 0x00020103,
    OID_GEN_RCV_ERROR			= 0x00020104,
    OID_GEN_RCV_NO_BUFFER		= 0x00020105,

    /* Optional statistics OIDs */
    OID_GEN_DIRECTED_BYTES_XMIT		= 0x00020201,
    OID_GEN_DIRECTED_FRAMES_XMIT	= 0x00020202,
    OID_GEN_MULTICAST_BYTES_XMIT	= 0x00020203,
    OID_GEN_MULTICAST_FRAMES_XMIT	= 0x00020204,
    OID_GEN_BROADCAST_BYTES_XMIT	= 0x00020205,
    OID_GEN_BROADCAST_FRAMES_XMIT	= 0x00020206,
    OID_GEN_DIRECTED_BYTES_RCV		= 0x00020207,
    OID_GEN_DIRECTED_FRAMES_RCV		= 0x00020208,
    OID_GEN_MULTICAST_BYTES_RCV		= 0x00020209,
    OID_GEN_MULTICAST_FRAMES_RCV	= 0x0002020a,
    OID_GEN_BROADCAST_BYTES_RCV		= 0x0002020b,
    OID_GEN_BROADCAST_FRAMES_RCV	= 0x0002020c,
    OID_GEN_RCV_CRC_ERROR		= 0x0002020d,
    OID_GEN_TRANSMIT_QUEUE_LENGTH	= 0x0002020e,
    OID_GEN_GET_TIME_CAPS		= 0x0002020f,
    OID_GEN_GET_NETCARD_TIME		= 0x00020210,
    OID_GEN_NETCARD_LOAD		= 0x00020211,
    OID_GEN_DEVICE_PROFILE		= 0x00020212,
    OID_GEN_INIT_TIME_MS		= 0x00020213,
    OID_GEN_RESET_COUNTS		= 0x00020214,
    OID_GEN_MEDIA_SENSE_COUNTS		= 0x00020215,
    OID_GEN_FRIENDLY_NAME		= 0x00020216,
    OID_GEN_MINIPORT_INFO		= 0x00020217,
    OID_GEN_RESET_VERIFY_PARAMETERS	= 0x00020218,

    /* IEEE 802.3 (Ethernet) OIDs */
    OID_802_3_PERMANENT_ADDRESS		= 0x01010101,
    OID_802_3_CURRENT_ADDRESS		= 0x01010102,
    OID_802_3_MULTICAST_LIST		= 0x01010103,
    OID_802_3_MAXIMUM_LIST_SIZE		= 0x01010104,
    OID_802_3_MAC_OPTIONS		= 0x01010105,
    OID_802_3_RCV_ERROR_ALIGNMENT	= 0x01020101,
    OID_802_3_XMIT_ONE_COLLISION	= 0x01020102,
    OID_802_3_XMIT_MORE_COLLISIONS	= 0x01020103,
    OID_802_3_XMIT_DEFERRED		= 0x01020201,
    OID_802_3_XMIT_MAX_COLLISIONS	= 0x01020202,
    OID_802_3_RCV_OVERRUN		= 0x01020203,
    OID_802_3_XMIT_UNDERRUN		= 0x01020204,
    OID_802_3_XMIT_HEARTBEAT_FAILURE	= 0x01020205,
    OID_802_3_XMIT_TIMES_CRS_LOST	= 0x01020206,
    OID_802_3_XMIT_LATE_COLLISIONS	= 0x01020207,
};

static const uint32_t oid_supported_list[] =
{
    /* the general stuff */
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_PHYSICAL_MEDIUM,

    /* the statistical stuff */
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,

    /* IEEE 802.3 */
    /* the general stuff */
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAC_OPTIONS,
    OID_802_3_MAXIMUM_LIST_SIZE,

    /* the statistical stuff */
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
};

#define NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA	(1 << 0)
#define NDIS_MAC_OPTION_RECEIVE_SERIALIZED	(1 << 1)
#define NDIS_MAC_OPTION_TRANSFERS_NOT_PEND	(1 << 2)
#define NDIS_MAC_OPTION_NO_LOOPBACK		(1 << 3)
#define NDIS_MAC_OPTION_FULL_DUPLEX		(1 << 4)
#define NDIS_MAC_OPTION_EOTX_INDICATION		(1 << 5)
#define NDIS_MAC_OPTION_8021P_PRIORITY		(1 << 6)

struct rndis_response {
    QTAILQ_ENTRY(rndis_response) entries;
    uint32_t length;
    uint8_t buf[0];
};

typedef struct USBNetState {
    USBDevice dev;

    enum rndis_state rndis_state;
    uint32_t medium;
    uint32_t speed;
    uint32_t media_state;
    uint16_t filter;
    uint32_t vendorid;

    unsigned int out_ptr;
    uint8_t out_buf[0x4000];

    unsigned int in_ptr, in_len;
    uint8_t in_buf[0x4000];

    USBEndpoint *intr;

    char usbstring_mac[13];
    NICState *nic;
    NICConf conf;
    QTAILQ_HEAD(, rndis_response) rndis_resp;

    USBPacket *pending_packet;
    uint8_t resp_buf[0x4000]; // probably could be using in/out_buf
    uint32_t resp_len;

#define CFIFO_LEN_MASK	255
#define DFIFO_LEN_MASK	4095
    struct usb_hci_in_fifo_s {
        uint8_t data[(DFIFO_LEN_MASK + 1) * 2];
        struct {
            uint8_t *data;
            int len;
        } fifo[CFIFO_LEN_MASK + 1];
        int dstart, dlen, dsize, start, len;
    } evt, acl, sco;

    struct usb_hci_out_fifo_s {
        uint8_t data[4096];
	int len;
    } outcmd, outacl, outsco;

    USBPacket *packet;

} USBNetState;

#define TYPE_USB_NET "usb-net-marvell"
#define USB_NET(obj) OBJECT_CHECK(USBNetState, (obj), TYPE_USB_NET)

static int is_rndis(USBNetState *s)
{
    return s->dev.config ?
            s->dev.config->bConfigurationValue == DEV_RNDIS_CONFIG_VALUE : 0;
}

static int ndis_query(USBNetState *s, uint32_t oid,
                      uint8_t *inbuf, unsigned int inlen, uint8_t *outbuf,
                      size_t outlen)
{
    unsigned int i;

    switch (oid) {
    /* general oids (table 4-1) */
    /* mandatory */
    case OID_GEN_SUPPORTED_LIST:
        for (i = 0; i < ARRAY_SIZE(oid_supported_list); i++) {
            stl_le_p(outbuf + (i * sizeof(le32)), oid_supported_list[i]);
        }
        return sizeof(oid_supported_list);

    /* mandatory */
    case OID_GEN_HARDWARE_STATUS:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_MEDIA_SUPPORTED:
        stl_le_p(outbuf, s->medium);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_MEDIA_IN_USE:
        stl_le_p(outbuf, s->medium);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_MAXIMUM_FRAME_SIZE:
        stl_le_p(outbuf, ETH_FRAME_LEN);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_LINK_SPEED:
        stl_le_p(outbuf, s->speed);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
        stl_le_p(outbuf, ETH_FRAME_LEN);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_RECEIVE_BLOCK_SIZE:
        stl_le_p(outbuf, ETH_FRAME_LEN);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_VENDOR_ID:
        stl_le_p(outbuf, s->vendorid);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_VENDOR_DESCRIPTION:
        pstrcpy((char *)outbuf, outlen, "QEMU USB RNDIS Net");
        return strlen((char *)outbuf) + 1;

    case OID_GEN_VENDOR_DRIVER_VERSION:
        stl_le_p(outbuf, 1);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_CURRENT_PACKET_FILTER:
        stl_le_p(outbuf, s->filter);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
        stl_le_p(outbuf, RNDIS_MAX_TOTAL_SIZE);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_MEDIA_CONNECT_STATUS:
        stl_le_p(outbuf, s->media_state);
        return sizeof(le32);

    case OID_GEN_PHYSICAL_MEDIUM:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    case OID_GEN_MAC_OPTIONS:
        stl_le_p(outbuf, NDIS_MAC_OPTION_RECEIVE_SERIALIZED |
                 NDIS_MAC_OPTION_FULL_DUPLEX);
        return sizeof(le32);

    /* statistics OIDs (table 4-2) */
    /* mandatory */
    case OID_GEN_XMIT_OK:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_RCV_OK:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_XMIT_ERROR:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_RCV_ERROR:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    /* mandatory */
    case OID_GEN_RCV_NO_BUFFER:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    /* ieee802.3 OIDs (table 4-3) */
    /* mandatory */
    case OID_802_3_PERMANENT_ADDRESS:
        memcpy(outbuf, s->conf.macaddr.a, 6);
        return 6;

    /* mandatory */
    case OID_802_3_CURRENT_ADDRESS:
        memcpy(outbuf, s->conf.macaddr.a, 6);
        return 6;

    /* mandatory */
    case OID_802_3_MULTICAST_LIST:
        stl_le_p(outbuf, 0xe0000000);
        return sizeof(le32);

    /* mandatory */
    case OID_802_3_MAXIMUM_LIST_SIZE:
        stl_le_p(outbuf, 1);
        return sizeof(le32);

    case OID_802_3_MAC_OPTIONS:
        return 0;

    /* ieee802.3 statistics OIDs (table 4-4) */
    /* mandatory */
    case OID_802_3_RCV_ERROR_ALIGNMENT:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    /* mandatory */
    case OID_802_3_XMIT_ONE_COLLISION:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    /* mandatory */
    case OID_802_3_XMIT_MORE_COLLISIONS:
        stl_le_p(outbuf, 0);
        return sizeof(le32);

    default:
        fprintf(stderr, "usbnet: unknown OID 0x%08x\n", oid);
        return 0;
    }
    return -1;
}

static int ndis_set(USBNetState *s, uint32_t oid,
                uint8_t *inbuf, unsigned int inlen)
{
    switch (oid) {
    case OID_GEN_CURRENT_PACKET_FILTER:
        s->filter = ldl_le_p(inbuf);
        if (s->filter) {
            s->rndis_state = RNDIS_DATA_INITIALIZED;
        } else {
            s->rndis_state = RNDIS_INITIALIZED;
        }
        return 0;

    case OID_802_3_MULTICAST_LIST:
        return 0;
    }
    return -1;
}

static int rndis_get_response(USBNetState *s, uint8_t *buf)
{
    int ret = 0;
    struct rndis_response *r = s->rndis_resp.tqh_first;

    if (!r)
        return ret;

    QTAILQ_REMOVE(&s->rndis_resp, r, entries);
    ret = r->length;
    memcpy(buf, r->buf, r->length);
    g_free(r);

    return ret;
}

static void *rndis_queue_response(USBNetState *s, unsigned int length)
{
    struct rndis_response *r =
            g_malloc0(sizeof(struct rndis_response) + length);

    if (QTAILQ_EMPTY(&s->rndis_resp)) {
        usb_wakeup(s->intr, 0);
    }

    QTAILQ_INSERT_TAIL(&s->rndis_resp, r, entries);
    r->length = length;

    return &r->buf[0];
}

static void rndis_clear_responsequeue(USBNetState *s)
{
    struct rndis_response *r;

    while ((r = s->rndis_resp.tqh_first)) {
        QTAILQ_REMOVE(&s->rndis_resp, r, entries);
        g_free(r);
    }
}

static int rndis_init_response(USBNetState *s, rndis_init_msg_type *buf)
{
    rndis_init_cmplt_type *resp =
            rndis_queue_response(s, sizeof(rndis_init_cmplt_type));

    if (!resp)
        return USB_RET_STALL;

    resp->MessageType = cpu_to_le32(RNDIS_INITIALIZE_CMPLT);
    resp->MessageLength = cpu_to_le32(sizeof(rndis_init_cmplt_type));
    resp->RequestID = buf->RequestID; /* Still LE in msg buffer */
    resp->Status = cpu_to_le32(RNDIS_STATUS_SUCCESS);
    resp->MajorVersion = cpu_to_le32(RNDIS_MAJOR_VERSION);
    resp->MinorVersion = cpu_to_le32(RNDIS_MINOR_VERSION);
    resp->DeviceFlags = cpu_to_le32(RNDIS_DF_CONNECTIONLESS);
    resp->Medium = cpu_to_le32(RNDIS_MEDIUM_802_3);
    resp->MaxPacketsPerTransfer = cpu_to_le32(1);
    resp->MaxTransferSize = cpu_to_le32(ETH_FRAME_LEN +
                    sizeof(struct rndis_packet_msg_type) + 22);
    resp->PacketAlignmentFactor = cpu_to_le32(0);
    resp->AFListOffset = cpu_to_le32(0);
    resp->AFListSize = cpu_to_le32(0);
    return 0;
}

static int rndis_query_response(USBNetState *s,
                rndis_query_msg_type *buf, unsigned int length)
{
    rndis_query_cmplt_type *resp;
    /* oid_supported_list is the largest data reply */
    uint8_t infobuf[sizeof(oid_supported_list)];
    uint32_t bufoffs, buflen;
    int infobuflen;
    unsigned int resplen;

    bufoffs = le32_to_cpu(buf->InformationBufferOffset) + 8;
    buflen = le32_to_cpu(buf->InformationBufferLength);
    if (buflen > length || bufoffs >= length || bufoffs + buflen > length) {
        return USB_RET_STALL;
    }

    infobuflen = ndis_query(s, le32_to_cpu(buf->OID),
                            bufoffs + (uint8_t *) buf, buflen, infobuf,
                            sizeof(infobuf));
    resplen = sizeof(rndis_query_cmplt_type) +
            ((infobuflen < 0) ? 0 : infobuflen);
    resp = rndis_queue_response(s, resplen);
    if (!resp)
        return USB_RET_STALL;

    resp->MessageType = cpu_to_le32(RNDIS_QUERY_CMPLT);
    resp->RequestID = buf->RequestID; /* Still LE in msg buffer */
    resp->MessageLength = cpu_to_le32(resplen);

    if (infobuflen < 0) {
        /* OID not supported */
        resp->Status = cpu_to_le32(RNDIS_STATUS_NOT_SUPPORTED);
        resp->InformationBufferLength = cpu_to_le32(0);
        resp->InformationBufferOffset = cpu_to_le32(0);
        return 0;
    }

    resp->Status = cpu_to_le32(RNDIS_STATUS_SUCCESS);
    resp->InformationBufferOffset =
            cpu_to_le32(infobuflen ? sizeof(rndis_query_cmplt_type) - 8 : 0);
    resp->InformationBufferLength = cpu_to_le32(infobuflen);
    memcpy(resp + 1, infobuf, infobuflen);

    return 0;
}

static int rndis_set_response(USBNetState *s,
                rndis_set_msg_type *buf, unsigned int length)
{
    rndis_set_cmplt_type *resp =
            rndis_queue_response(s, sizeof(rndis_set_cmplt_type));
    uint32_t bufoffs, buflen;
    int ret;

    if (!resp)
        return USB_RET_STALL;

    bufoffs = le32_to_cpu(buf->InformationBufferOffset) + 8;
    buflen = le32_to_cpu(buf->InformationBufferLength);
    if (buflen > length || bufoffs >= length || bufoffs + buflen > length) {
        return USB_RET_STALL;
    }

    ret = ndis_set(s, le32_to_cpu(buf->OID),
                    bufoffs + (uint8_t *) buf, buflen);
    resp->MessageType = cpu_to_le32(RNDIS_SET_CMPLT);
    resp->RequestID = buf->RequestID; /* Still LE in msg buffer */
    resp->MessageLength = cpu_to_le32(sizeof(rndis_set_cmplt_type));
    if (ret < 0) {
        /* OID not supported */
        resp->Status = cpu_to_le32(RNDIS_STATUS_NOT_SUPPORTED);
        return 0;
    }
    resp->Status = cpu_to_le32(RNDIS_STATUS_SUCCESS);

    return 0;
}

static int rndis_reset_response(USBNetState *s, rndis_reset_msg_type *buf)
{
    rndis_reset_cmplt_type *resp =
            rndis_queue_response(s, sizeof(rndis_reset_cmplt_type));

    if (!resp)
        return USB_RET_STALL;

    resp->MessageType = cpu_to_le32(RNDIS_RESET_CMPLT);
    resp->MessageLength = cpu_to_le32(sizeof(rndis_reset_cmplt_type));
    resp->Status = cpu_to_le32(RNDIS_STATUS_SUCCESS);
    resp->AddressingReset = cpu_to_le32(1); /* reset information */

    return 0;
}

static int rndis_keepalive_response(USBNetState *s,
                rndis_keepalive_msg_type *buf)
{
    rndis_keepalive_cmplt_type *resp =
            rndis_queue_response(s, sizeof(rndis_keepalive_cmplt_type));

    if (!resp)
        return USB_RET_STALL;

    resp->MessageType = cpu_to_le32(RNDIS_KEEPALIVE_CMPLT);
    resp->MessageLength = cpu_to_le32(sizeof(rndis_keepalive_cmplt_type));
    resp->RequestID = buf->RequestID; /* Still LE in msg buffer */
    resp->Status = cpu_to_le32(RNDIS_STATUS_SUCCESS);

    return 0;
}

/* Prepare to receive the next packet */
static void usb_net_reset_in_buf(USBNetState *s)
{
    s->in_ptr = s->in_len = 0;
    qemu_flush_queued_packets(qemu_get_queue(s->nic));
}

static int rndis_parse(USBNetState *s, uint8_t *data, int length)
{
    uint32_t msg_type = ldl_le_p(data);

    switch (msg_type) {
    case RNDIS_INITIALIZE_MSG:
        s->rndis_state = RNDIS_INITIALIZED;
        return rndis_init_response(s, (rndis_init_msg_type *) data);

    case RNDIS_HALT_MSG:
        s->rndis_state = RNDIS_UNINITIALIZED;
        return 0;

    case RNDIS_QUERY_MSG:
        return rndis_query_response(s, (rndis_query_msg_type *) data, length);

    case RNDIS_SET_MSG:
        return rndis_set_response(s, (rndis_set_msg_type *) data, length);

    case RNDIS_RESET_MSG:
        rndis_clear_responsequeue(s);
        s->out_ptr = 0;
        usb_net_reset_in_buf(s);
        return rndis_reset_response(s, (rndis_reset_msg_type *) data);

    case RNDIS_KEEPALIVE_MSG:
        /* For USB: host does this every 5 seconds */
        return rndis_keepalive_response(s, (rndis_keepalive_msg_type *) data);
    }

    return USB_RET_STALL;
}

static void usb_net_handle_reset(USBDevice *dev)
{
    fprintf(stderr, "usb_net_handle_reset\n");
}

static void dump_hex(const void* data, size_t size) {
	char ascii[17];
	size_t i, j;
	ascii[16] = '\0';
	for (i = 0; i < size; ++i) {
		fprintf(stderr, "%02X ", ((unsigned char*)data)[i]);
		if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
			ascii[i % 16] = ((unsigned char*)data)[i];
		} else {
			ascii[i % 16] = '.';
		}
		if ((i+1) % 8 == 0 || i+1 == size) {
			fprintf(stderr, " ");
			if ((i+1) % 16 == 0) {
				fprintf(stderr, "|  %s \n", ascii);
			} else if (i+1 == size) {
				ascii[(i+1) % 16] = '\0';
				if ((i+1) % 16 <= 8) {
					fprintf(stderr, " ");
				}
				for (j = (i+1) % 16; j < 16; ++j) {
					fprintf(stderr, "   ");
				}
				fprintf(stderr, "|  %s \n", ascii);
			}
		}
	}
}

static void usb_net_handle_control(USBDevice *dev, USBPacket *p,
               int request, int value, int index, int length, uint8_t *data)
{
    // This is broken, take not of how the serial number is wrong and an ocassional 'control not handled'
    fprintf(stderr, "usb_net_handle_control\n");
    fprintf(stderr, "usbnet: control transaction: "
                        "request 0x%x value 0x%x index 0x%x length 0x%x\n",
                        request, value, index, length);
    dump_hex(data, length);

    USBNetState *s = (USBNetState *) dev;
    int ret;

    ret = usb_desc_handle_control(dev, p, request, value, index, length, data);
    if (ret >= 0) {
        return;
    } else {
        /* Check if we can respond to this with a data out
        fprintf(stderr, "Responding to host control\n");
        s->resp_buf[5] = s->resp_buf[5] | 0x80;
        usb_packet_copy(p, s->resp_buf, s->resp_len);
        s->resp_len = 0;
        return;
        */
        //dump_hex(data, length);
        //memcpy(s->resp_buf, data, length);
        //s->resp_len = length;
        // see the fail condition -- we might be getting HCI commands
        fprintf(stderr, "control not handled: %d\n", ret);
        if ((data[0] == 0x03) && (data[1] == 0x0c) & (data[2] == 0x00)){
            data[0] = 0x0e;
            data[1] = 0x04;
            data[2] = 0x01;
            data[3] = 0x03;
            data[4] = 0x0c;
            data[5] = 0x00;
            p->actual_length = 6;
            ret = 0;
            return;
        } else {
            p->status = USB_RET_STALL;
        }
        return;
    }

    switch(request) {
    case ClassInterfaceOutRequest | USB_CDC_SEND_ENCAPSULATED_COMMAND:
        if (!is_rndis(s) || value || index != 0) {
            goto fail;
        }
#ifdef TRAFFIC_DEBUG
        {
            unsigned int i;
            fprintf(stderr, "SEND_ENCAPSULATED_COMMAND:");
            for (i = 0; i < length; i++) {
                if (!(i & 15))
                    fprintf(stderr, "\n%04x:", i);
                fprintf(stderr, " %02x", data[i]);
            }
            fprintf(stderr, "\n\n");
        }
#endif
        ret = rndis_parse(s, data, length);
        if (ret < 0) {
            p->status = ret;
        }
        break;

    case ClassInterfaceRequest | USB_CDC_GET_ENCAPSULATED_RESPONSE:
        if (!is_rndis(s) || value || index != 0) {
            goto fail;
        }
        p->actual_length = rndis_get_response(s, data);
        if (p->actual_length == 0) {
            data[0] = 0;
            p->actual_length = 1;
        }
#ifdef TRAFFIC_DEBUG
        {
            unsigned int i;
            fprintf(stderr, "GET_ENCAPSULATED_RESPONSE:");
            for (i = 0; i < p->actual_length; i++) {
                if (!(i & 15))
                    fprintf(stderr, "\n%04x:", i);
                fprintf(stderr, " %02x", data[i]);
            }
            fprintf(stderr, "\n\n");
        }
#endif
        break;

    default:
    fail:
        // request 0x2000 buffer "03 0c 00" - might be HCI Reset Command
        // https://bluekitchen-gmbh.com/usb-protocol-analyzer-for-bluetooth-communication-logging/
        fprintf(stderr, "usbnet: failed control transaction: "
                        "request 0x%x value 0x%x index 0x%x length 0x%x\n",
                        request, value, index, length);
        p->status = USB_RET_STALL;
        break;
    }

}




static void usb_net_handle_statusin(USBNetState *s, USBPacket *p)
{
    //dump_hex(s->in_buf, 128);
    fprintf(stderr, "DEBUG: statusin - resp_len: %d\n", s->resp_len);

    if (s->resp_len > 0){

        struct host_cmd_ds_command *msg = (struct host_cmd_ds_command *) &s->resp_buf[4];

        // Why does this have to be outside the switch?
        struct host_cmd_ds_get_hw_spec *hw_spec = (struct host_cmd_ds_get_hw_spec *) &msg->params.hw_spec;
        struct host_cmd_ds_txbuf_cfg *tx_buf = (struct host_cmd_ds_txbuf_cfg *) &msg->params.tx_buf;
        // Unused vars
        //struct host_cmd_ds_802_11_ps_mode_enh *psmode_enh = (struct host_cmd_ds_802_11_ps_mode_enh *) &msg->params.psmode_enh;
        //struct host_cmd_ds_tx_rate_cfg *tx_rate_cfg = (struct host_cmd_ds_tx_rate_cfg *) &msg->params.tx_rate_cfg;
        struct host_cmd_ds_tx_rate_query *tx_rate = (struct host_cmd_ds_tx_rate_query *) &msg->params.tx_rate;
        struct host_cmd_ds_rf_tx_pwr *txp = (struct host_cmd_ds_rf_tx_pwr *) &msg->params.txp;
        struct host_cmd_ds_802_11_ibss_status *ibss_coalescing = (struct host_cmd_ds_802_11_ibss_status *) &msg->params.ibss_coalescing;
        struct host_cmd_ds_amsdu_aggr_ctrl *amsdu_aggr_ctrl = (struct host_cmd_ds_amsdu_aggr_ctrl *) &msg->params.amsdu_aggr_ctrl;
        struct host_cmd_ds_mac_control *mac_ctrl = (struct host_cmd_ds_mac_control *) &msg->params.mac_ctrl;
        //struct host_cmd_ds_802_11_snmp_mib *smib = (struct host_cmd_ds_802_11_snmp_mib *) &msg->params.smib;
        struct host_cmd_ds_11n_cfg *htcfg = (struct host_cmd_ds_11n_cfg *) &msg->params.htcfg;
        struct host_cmd_ds_802_11_key_material *key_material = (struct host_cmd_ds_802_11_key_material *) &msg->params.key_material;

        switch(msg->command){
        case HostCmd_CMD_FUNC_INIT:
            fprintf(stderr, "HostCmd: HostCmd_CMD_FUNC_INIT\n");
            // simple echo will do
            break;
        case HostCmd_CMD_GET_HW_SPEC:
            fprintf(stderr, "HostCmd: HostCmd_CMD_GET_HW_SPEC\n");
            //struct host_cmd_ds_get_hw_spec *hw_spec = (struct host_cmd_ds_get_hw_spec *) &msg->params.hw_spec;
            hw_spec->hw_if_version = 0x0200;
            hw_spec->version = 0x2040;
            hw_spec->reserved = 0;
            hw_spec->num_of_mcast_adr = 0x4000;
            hw_spec->permanent_addr[0] = 0x54;
            hw_spec->permanent_addr[1] = 0x35;
            hw_spec->permanent_addr[2] = 0x30;
            hw_spec->permanent_addr[3] = 0x31;
            hw_spec->permanent_addr[4] = 0x08;
            hw_spec->permanent_addr[5] = 0x24;
            hw_spec->region_code = 0x1000;
            hw_spec->number_of_antenna = 0x0200;
            hw_spec->fw_release_number = 0x1a8f0e42;
            hw_spec->reserved_1 = 0;
            hw_spec->reserved_2 = 0;
            hw_spec->reserved_3 = 0;
            hw_spec->fw_cap_info = 0xa3340000;
            hw_spec->dev_mcs_support = 0x33;
            hw_spec->mp_end_port = 0x00d3;
            hw_spec->mgmt_buf_count = 0x3722;
            hw_spec->reserved_5 = 0x00000e00;
            hw_spec->reserved_6 = 0;
            hw_spec->dot_11ac_dev_cap = 0x01000000;
            hw_spec->dot_11ac_mcs_support = 0xb079c133;
            //hw_spec->tlvs = 0xfafffaff; // idk dawg

            // What happens if we pull a little sneaky here?
            /*
            s->resp_buf[4] = 0x5e;
            key_material->action = 0x0100;
            key_material->key_param_set.key_len = 0xffff;
            memset(hw_spec, 0xff, sizeof(struct host_cmd_ds_802_11_key_material));
            memset(key_material->key_param_set.key, 0xff, 54);
            fprintf(stderr, "Clean byte check: %x\n", key_material->key_param_set.key[49]);
            fprintf(stderr, "Dirty byte check: %x\n", key_material->key_param_set.key[50]);
            */
            break;
        case HostCmd_CMD_RECONFIGURE_TX_BUFF:
            fprintf(stderr, "HostCmd: HostCmd_CMD_RECONFIGURE_TX_BUFF\n");
            // Xbone sends 4 less bytes (presumably cutting off mp_end_port -- which is sdio only, and reserved3)
            tx_buf->action = 0x0100;
            tx_buf->buff_size = 0x3C07;
            tx_buf->mp_end_port = 0x00;
            tx_buf->reserved3 = 0x00;
            break;
        case HostCmd_CMD_802_11_PS_MODE_ENH:
            fprintf(stderr, "HostCmd: HostCmd_CMD_802_11_PS_MODE_ENH\tTODO!!!\n");
            //psmode_enh->action = 0xff00;
            // I am confuse
            break;
        case HostCmd_CMD_TX_RATE_CFG:
            fprintf(stderr, "HostCmd: HostCmd_CMD_TX_RATE_CFG\tTODO!!!\n");
            // tlv buffer on the other side of the struct or something -- that's why it's rly big
            break;
        case HostCmd_CMD_802_11_TX_RATE_QUERY:
            fprintf(stderr, "HostCmd: HostCmd_CMD_802_11_TX_RATE_QUERY\tUNVERIFIED\n");
            // VHT, BW80, SGI (I think)
            tx_rate->tx_rate = 0x1A;
            // Emulating an AC card
            tx_rate->ht_info = 0x00;
            break;
        case HostCmd_CMD_RF_TX_PWR:
            fprintf(stderr, "HostCmd: HostCmd_CMD_RF_TX_PWR\n");
            txp->action = 0x0000;
            txp->cur_level = 0x0000;
            txp->max_power = 0x12;
            txp->min_power = 0x08;
            break;
        case HostCmd_CMD_802_11_IBSS_COALESCING_STATUS:
            fprintf(stderr, "HostCmd: HostCmd_CMD_802_11_IBSS_COALESCING_STATUS\n");
            ibss_coalescing->action = 0x0100;
            ibss_coalescing->enable = 0x0100;
            ibss_coalescing->bssid[0] = 0x12;
            ibss_coalescing->bssid[1] = 0x34;
            ibss_coalescing->bssid[2] = 0x56;
            ibss_coalescing->bssid[3] = 0x78;
            ibss_coalescing->bssid[4] = 0x9a;
            ibss_coalescing->bssid[5] = 0xbc;
            ibss_coalescing->beacon_interval = 0x0000;
            ibss_coalescing->atim_window = 0x0000;
            ibss_coalescing->use_g_rate_protect = 0x0000;
            break;
        case HostCmd_CMD_AMSDU_AGGR_CTRL:
            fprintf(stderr, "HostCmd: HostCmd_CMD_AMSDU_AGGR_CTRL\n");
            amsdu_aggr_ctrl->action = 0x0100;
            amsdu_aggr_ctrl->enable = 0x0100;
            amsdu_aggr_ctrl->curr_buf_size = 0xffff;
            break;
        case HostCmd_CMD_MAC_CONTROL:
            fprintf(stderr, "HostCmd: HostCmd_CMD_MAC_CONTROL\n");
            mac_ctrl->action = 0x11100000;
            break;
        case HostCmd_CMD_802_11_SNMP_MIB:
            fprintf(stderr, "HostCmd: HostCmd_CMD_802_11_SNMP_MIB\tTODO\n");
            //smib->query_type = 0x0000;
            //smib->oid = 0x0000;
            //smib->buf_size = 0x0000;
            //smib->value[0] = 0x00;
            
            s->resp_buf[4] = 0x5e;
            key_material->action = 0x0100;
            key_material->key_param_set.key_len = 0x4000;
            //memset(key_material, 0xff, sizeof(struct host_cmd_ds_802_11_snmp_mib));
            memset(key_material->key_param_set.key, 0x00, 64);
            fprintf(stderr, "Clean byte check: %x\n", key_material->key_param_set.key[48]);
            fprintf(stderr, "Clean byte check: %x\n", key_material->key_param_set.key[49]);
            fprintf(stderr, "Dirty byte check: %x\n", key_material->key_param_set.key[50]);
            fprintf(stderr, "Dirty byte check: %x\n", key_material->key_param_set.key[51]);
            break;
        case HostCmd_CMD_11N_CFG:
            fprintf(stderr, "HostCmd: HostCmd_CMD_11N_CFG\n");
            htcfg->action = 0x0100;
            htcfg->ht_tx_cap = 0x8301;
            htcfg->ht_tx_info = 0x0000;
            htcfg->misc_config = 0x0000;
            break;
        default:
            fprintf(stderr, "HostCmd: NOT YET IMPLEMENTED\tTODO!!!\n");
            fprintf(stderr, "HostCmd: %d\n", msg->command);
            dump_hex(msg, s->resp_len);
            break;
        }

        fprintf(stderr, "Responding to hostcmd\n");
        s->resp_buf[5] = s->resp_buf[5] | 0x80;
        usb_packet_copy(p, s->resp_buf, s->resp_len);
        s->resp_len = 0;
        //fprintf(stderr, "DEBUG: Set resp_len to 0 - resp_len: %d\n", s->resp_len);

    } else {
        fprintf(stderr, "statusin stalled\n");
        p->actual_length = 0;
        //p->status = USB_RET_STALL;
        //p->status = USB_RET_ASYNC;
    }
    /*
    le32 buf[2];

    if (p->iov.size < 8) {
        p->status = USB_RET_STALL;
        return;
    }

    buf[0] = cpu_to_le32(1);
    buf[1] = cpu_to_le32(0);
    usb_packet_copy(p, buf, 8);
    if (!s->rndis_resp.tqh_first) {
        p->status = USB_RET_NAK;
    }
    */
#ifdef TRAFFIC_DEBUG
    fprintf(stderr, "usbnet: interrupt poll len %zu return %d",
            p->iov.size, p->status);
    iov_hexdump(p->iov.iov, p->iov.niov, stderr, "usbnet", p->status);
#endif
}

static void usb_net_handle_datain(USBNetState *s, USBPacket *p)
{
    int len;

    if (s->in_ptr > s->in_len) {
        usb_net_reset_in_buf(s);
        p->status = USB_RET_NAK;
        return;
    }
    if (!s->in_len) {
        p->status = USB_RET_NAK;
        return;
    }
    len = s->in_len - s->in_ptr;
    if (len > p->iov.size) {
        len = p->iov.size;
    }
    usb_packet_copy(p, &s->in_buf[s->in_ptr], len);
    s->in_ptr += len;
    if (s->in_ptr >= s->in_len &&
                    (is_rndis(s) || (s->in_len & (64 - 1)) || !len)) {
        // no short packet necessary
        usb_net_reset_in_buf(s);
    }

#ifdef TRAFFIC_DEBUG
    fprintf(stderr, "usbnet: data in len %zu return %d", p->iov.size, len);
    iov_hexdump(p->iov.iov, p->iov.niov, stderr, "usbnet", len);
#endif
}


static void usb_net_handle_dataout(USBNetState *s, USBPacket *p)
{
    int sz = sizeof(s->out_buf) - s->out_ptr;
    //fprintf(stderr, "DEBUG: sz: %d\n", sz);

    /*
    struct rndis_packet_msg_type *msg =
            (struct rndis_packet_msg_type *) s->out_buf;
    */
    //struct host_cmd_ds_command *msg = (struct host_cmd_ds_command *) s->out_buf;
    //uint32_t len;

#ifdef TRAFFIC_DEBUG
    fprintf(stderr, "usbnet: data out len %zu\n", p->iov.size);
    iov_hexdump(p->iov.iov, p->iov.niov, stderr, "usbnet", p->iov.size);
#endif

    fprintf(stderr, "usbnet: data out len %zu - sz: %d\n", p->iov.size, sz);
    if (sz > p->iov.size) {
        //fprintf(stderr, "DEBUG: Set sz to iov.size - sz: %d - iov: %ld\n", sz, p->iov.size);
        sz = p->iov.size;

        if (sz == 0){
            // Yeah -- this is probably not the solution
            return;
        }
    }
    usb_packet_copy(p, &s->out_buf[s->out_ptr], sz);
    //s->out_ptr += sz;
    // It appears that we have a stack corruption -- everything breaks when we include the fprintf guards below
    //fprintf(stderr, "DEBUG: Preparing to hex_dump...\n");
    //dump_hex(s->out_buf, sz);
    //fprintf(stderr, "DEBUG: Finished hex_dump\n");
    //s->out_buf[5] = 0x80;
    //dump_hex(s->out_buf, sz);
    p->actual_length = p->iov.size;

    // copy this command to the resp buffer
    // Looks like we're getting jacked because of zero size buffers following legitimate buffers down this 
    memcpy(s->resp_buf, s->out_buf, sz);

    s->resp_len = sz;
    //fprintf(stderr, "DEBUG: Set resp_len to sz - sz: %d - resp_len: %d\n", sz, s->resp_len);

    struct host_cmd_ds_command *msg = (struct host_cmd_ds_command *) &s->out_buf[4];
    fprintf(stderr, "Queued HostCMD: command: %x - size: %d - seq_num: %d - result: %d\n", msg->command, msg->size, msg->seq_num, msg->result);

    if (s->packet != NULL){
        fprintf(stderr, "Debug: Completing packet...\n");
        USBPacket *dp = s->packet;
        s->packet = NULL;
        //dp->actual_length = dp->iov.size;

        // Dirty -- this just verifies that it works
        char* idk = (char*)calloc(1, 12);
        idk[0] = 0xce;
        idk[1] = 0xfa;
        idk[2] = 0x0d;
        idk[3] = 0xf0;
        memcpy(idk + 4, msg, 8);
        idk[5] = 0x80;  // complete

        usb_packet_copy(dp, idk, 12);
        dump_hex(idk, 8);
        dp->status = USB_RET_SUCCESS;
        usb_packet_complete(&s->dev, dp);
    }

    return;
    /*
    if (!is_rndis(s)) {
        if (p->iov.size < 64) {
            qemu_send_packet(qemu_get_queue(s->nic), s->out_buf, s->out_ptr);
            s->out_ptr = 0;
        }
        return;
    }
    len = le32_to_cpu(msg->MessageLength);
    if (s->out_ptr < 8 || s->out_ptr < len) {
        return;
    }
    if (le32_to_cpu(msg->MessageType) == RNDIS_PACKET_MSG) {
        uint32_t offs = 8 + le32_to_cpu(msg->DataOffset);
        uint32_t size = le32_to_cpu(msg->DataLength);
        if (offs < len && size < len && offs + size <= len) {
            qemu_send_packet(qemu_get_queue(s->nic), s->out_buf + offs, size);
        }
    }
    */
    // TODO: make an isHostCMD function
    //uint16_t size = le16_to_cpu(msg->size);
    // https://www.linux-kvm.org/images/c/c5/Kvm-forum-2013-High-Performance-IO-for-VMs.pdf
    // This is where we read and write to tap backend
    //qemu_send_packet(qemu_get_queue(s->nic), s->out_buf, size);
    //s->out_ptr -= size;
    //memmove(s->out_buf, &s->out_buf[size], s->out_ptr);
}

static inline int usb_bt_fifo_dequeue(struct usb_hci_in_fifo_s *fifo,
                USBPacket *p)
{
    fprintf(stderr, "usb_bt_fifo_dequeue\n");

    int len;

    if (likely(!fifo->len))
        return USB_RET_STALL;

    len = MIN(p->iov.size, fifo->fifo[fifo->start].len);
    usb_packet_copy(p, fifo->fifo[fifo->start].data, len);
    if (len == p->iov.size) {
        fifo->fifo[fifo->start].len -= len;
        fifo->fifo[fifo->start].data += len;
    } else {
        fifo->start ++;
        fifo->start &= CFIFO_LEN_MASK;
        fifo->len --;
    }

    fifo->dstart += len;
    fifo->dlen -= len;
    if (fifo->dstart >= fifo->dsize) {
        fifo->dstart = 0;
        fifo->dsize = DFIFO_LEN_MASK + 1;
    }

    return len;
}

static inline void usb_bt_fifo_out_enqueue(struct USBNetState *s,
                struct usb_hci_out_fifo_s *fifo,
                USBPacket *p)
{
    fprintf(stderr, "usb_bt_fifo_out_enqueue\n");

    usb_packet_copy(p, fifo->data + fifo->len, p->iov.size);
    fifo->len += p->iov.size;

    /* Do not check this case yet -- at least the first packet must be queued
    if (complete(fifo->data, fifo->len)) {
        send(s->hci, fifo->data, fifo->len);
        fifo->len = 0;
    }
    */

    /* TODO: do we need to loop? */
}



static void usb_net_handle_data(USBDevice *dev, USBPacket *p)
{
    USBNetState *s = (USBNetState *) dev;

    //USBPacket *op = s->p;
    //USBEndpoint *oep = op->ep;

    fprintf(stderr, "usbnet: Transaction: pid 0x%x ep 0x%x len 0x%zx\n", p->pid, p->ep->nr, p->iov.size);
    //dump_hex(s->resp_buf, p->iov.size);

    switch(p->pid) {
    // 0x69 - TX
    case USB_TOKEN_IN:
        switch (p->ep->nr) {
        case MWIFIEX_USB_EP_CMD_EVENT:
            //usb_net_handle_statusin(s, p);
            //usb_net_handle_datain(s, p);
            //p->actual_length = 0;
            if (s->packet == NULL){
                fprintf(stderr, "DEBUG: Deferring Bulk In packet\n");
                s->packet = p;
                p->status = USB_RET_ASYNC;
            }
            /*
            p->state = USB_PACKET_QUEUED;
            QTAILQ_INSERT_TAIL(&p->ep->queue, p, queue);
            p->status = USB_RET_ASYNC;
            */
            //usb_bt_fifo_dequeue(&s->evt, p);
            //usb_packet_set_state(p, USB_PACKET_QUEUED);
            //QTAILQ_INSERT_TAIL(&p->ep->queue, p, queue);
            //p->status = USB_RET_ASYNC;
            break;

        case 234:
            // stop compiler from complaining
            usb_net_handle_statusin(s, p);
            usb_net_handle_datain(s, p);
            break;

        case MWIFIEX_USB_EP_DATA:
            //goto fail;
            //fprintf(stderr, "USB BULK IN - EP2\n");
            //usb_net_handle_datain(s, p);
            //p->state = USB_PACKET_QUEUED;
            //QTAILQ_INSERT_TAIL(&p->ep->queue, p, queue);
            p->status = USB_RET_ASYNC;

            //usb_net_handle_statusin(s, p);
            fprintf(stderr, "Unhandled MWIFIEX_USB_EP_DATA\n");
            break;

        default:
            p->status = USB_RET_ASYNC;
            //if (s->resp_len > 0){
            //    usb_net_handle_statusin(s, p);
            //} else {
            //usb_net_handle_datain(s, p);
            //}
            //goto fail;
        }
        break;

    // 0xe1 - RX
    case USB_TOKEN_OUT:
        switch (p->ep->nr) {
        case MWIFIEX_USB_EP_CMD_EVENT:
            usb_net_handle_dataout(s, p);
            //usb_bt_fifo_out_enqueue(s, &s->outacl, p);
            //usb_packet_complete_one(oep->dev, op);
            break;
        case MWIFIEX_USB_EP_DATA:
            //fprintf(stderr, "USB BULK OUT - EP2\n");
            // Adding this line seems to have broken this -- although it might just be hit or miss
            //usb_net_handle_dataout(s, p);
            //p->state = USB_PACKET_QUEUED;
            //QTAILQ_INSERT_TAIL(&p->ep->queue, p, queue);
            p->status = USB_RET_ASYNC;
            break;
        case MWIFIEX_USB_EP_DATA_CH2:
            //fprintf(stderr, "Unhandled MWIFIEX_USB_EP_DATA_CH2\n");
            //usb_net_handle_dataout(s, p);
            //p->state = USB_PACKET_QUEUED;
            //QTAILQ_INSERT_TAIL(&p->ep->queue, p, queue);
            p->status = USB_RET_ASYNC;
            break;
        default:
            p->status = USB_RET_ASYNC;
            //p->status = USB_RET_STALL;
            //goto fail;
        }
        break;

/* I don't know the word
    default:
    fail:
        p->status = USB_RET_STALL;
        break;
*/
    }

    if (p->status == USB_RET_STALL) {
        fprintf(stderr, "usbnet: failed data transaction: "
                        "pid 0x%x ep 0x%x len 0x%zx\n",
                        p->pid, p->ep->nr, p->iov.size);
    }
}

static ssize_t usbnet_receive(NetClientState *nc, const uint8_t *buf, size_t size)
{
    USBNetState *s = qemu_get_nic_opaque(nc);
    uint8_t *in_buf = s->in_buf;
    size_t total_size = size;

    if (!s->dev.config) {
        return -1;
    }

    if (is_rndis(s)) {
        if (s->rndis_state != RNDIS_DATA_INITIALIZED) {
            return -1;
        }
        total_size += sizeof(struct rndis_packet_msg_type);
    }
    if (total_size > sizeof(s->in_buf)) {
        return -1;
    }

    /* Only accept packet if input buffer is empty */
    if (s->in_len > 0) {
        return 0;
    }

    if (is_rndis(s)) {
        struct rndis_packet_msg_type *msg;

        msg = (struct rndis_packet_msg_type *)in_buf;
        memset(msg, 0, sizeof(struct rndis_packet_msg_type));
        msg->MessageType = cpu_to_le32(RNDIS_PACKET_MSG);
        msg->MessageLength = cpu_to_le32(size + sizeof(*msg));
        msg->DataOffset = cpu_to_le32(sizeof(*msg) - 8);
        msg->DataLength = cpu_to_le32(size);
        /* msg->OOBDataOffset;
         * msg->OOBDataLength;
         * msg->NumOOBDataElements;
         * msg->PerPacketInfoOffset;
         * msg->PerPacketInfoLength;
         * msg->VcHandle;
         * msg->Reserved;
         */
        in_buf += sizeof(*msg);
    }

    memcpy(in_buf, buf, size);
    s->in_len = total_size;
    s->in_ptr = 0;
    return size;
}

static void usbnet_cleanup(NetClientState *nc)
{
    USBNetState *s = qemu_get_nic_opaque(nc);

    s->nic = NULL;
}

static void usb_net_unrealize(USBDevice *dev, Error **errp)
{
    USBNetState *s = (USBNetState *) dev;

    /* TODO: remove the nd_table[] entry */
    rndis_clear_responsequeue(s);
    qemu_del_nic(s->nic);
}

static NetClientInfo net_usbnet_info = {
    .type = NET_CLIENT_DRIVER_NIC,
    .size = sizeof(NICState),
    .receive = usbnet_receive,
    .cleanup = usbnet_cleanup,
};

static void usb_net_realize(USBDevice *dev, Error **errp)
{
    USBNetState *s = USB_NET(dev);

    usb_desc_create_serial(dev);
    usb_desc_init(dev);

    s->rndis_state = RNDIS_UNINITIALIZED;
    QTAILQ_INIT(&s->rndis_resp);

    s->medium = 0;	/* NDIS_MEDIUM_802_3 */
    s->speed = 1000000; /* 100MBps, in 100Bps units */
    s->media_state = 0;	/* NDIS_MEDIA_STATE_CONNECTED */;
    s->filter = 0;
    s->vendorid = 0x1234;
    s->intr = usb_ep_get(dev, USB_TOKEN_IN, 1);

    qemu_macaddr_default_if_unset(&s->conf.macaddr);
    s->nic = qemu_new_nic(&net_usbnet_info, &s->conf,
                          object_get_typename(OBJECT(s)), s->dev.qdev.id, s);
    qemu_format_nic_info_str(qemu_get_queue(s->nic), s->conf.macaddr.a);
    snprintf(s->usbstring_mac, sizeof(s->usbstring_mac),
             "%02x%02x%02x%02x%02x%02x",
             0x40,
             s->conf.macaddr.a[1],
             s->conf.macaddr.a[2],
             s->conf.macaddr.a[3],
             s->conf.macaddr.a[4],
             s->conf.macaddr.a[5]);
    usb_desc_set_string(dev, STRING_UNUSED, s->usbstring_mac);
}

static void usb_net_instance_init(Object *obj)
{
    USBDevice *dev = USB_DEVICE(obj);
    USBNetState *s = USB_NET(dev);

    device_add_bootindex_property(obj, &s->conf.bootindex,
                                  "bootindex", "/ethernet-phy@0",
                                  &dev->qdev, NULL);
}

static const VMStateDescription vmstate_usb_net = {
    .name = "usb-net-marvell",
    .unmigratable = 1,
};

static Property net_properties[] = {
    DEFINE_NIC_PROPERTIES(USBNetState, conf),
    DEFINE_PROP_END_OF_LIST(),
};

static void usb_net_class_initfn(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    USBDeviceClass *uc = USB_DEVICE_CLASS(klass);

    uc->realize        = usb_net_realize;
    uc->product_desc   = "QEMU Wireless USB Network Interface";
    uc->usb_desc       = &desc_net;
    uc->handle_reset   = usb_net_handle_reset;
    uc->handle_control = usb_net_handle_control;
    uc->handle_data    = usb_net_handle_data;
    uc->unrealize      = usb_net_unrealize;
    set_bit(DEVICE_CATEGORY_NETWORK, dc->categories);
    dc->fw_name = "network";
    dc->vmsd = &vmstate_usb_net;
    dc->props = net_properties;
}

static const TypeInfo net_info = {
    .name          = TYPE_USB_NET,
    .parent        = TYPE_USB_DEVICE,
    .instance_size = sizeof(USBNetState),
    .class_init    = usb_net_class_initfn,
    .instance_init = usb_net_instance_init,
};

static void usb_net_register_types(void)
{
    type_register_static(&net_info);
}

type_init(usb_net_register_types)
