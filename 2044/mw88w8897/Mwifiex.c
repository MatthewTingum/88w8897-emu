#include "Mwifiex.h"
#include <Wdm.h>

static int payloadSequence = 0;

void* GetPayload(char command0) {

	static char payload_a9_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xa9, 0x80, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00 };

	static char payload_03_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x03, 0x80, 0x47, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x20, 0x40,
							0x00, 0x00, 0x40, 0x00, 0x54, 0x35, 0x30, 0x31, 0x08, 0x24, 0x10, 0x00, 0x02, 0x00, 0x1a, 0x8f,
							0x0e, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa3, 0x3f,
							0x00, 0x00, 0x33, 0x00, 0xd3, 0x37, 0x22, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
							0x00, 0x00, 0x00, 0xb0, 0x79, 0xc1, 0x33, 0xfa, 0xff, 0xfa, 0xff };

	static char payload_d9_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xd9, 0x80, 0x0c, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x3c, 0x07 };

	static char payload_d4_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xd4, 0x80, 0x24, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
							0xe8, 0x03, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00,
							0xd0, 0x07, 0x00, 0x00, 0xe8, 0x03, 0x00, 0x00 };

	static char payload_19_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x19, 0x80, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x21,
							0xf7, 0xf3, 0xf3, 0x2f };

	static char payload_75_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x75, 0x80, 0x12, 0x00, 0x06, 0x00, 0x00, 0x00, 0x02, 0x00, 0x08, 0x00,
							0x07, 0x01, 0x02, 0x00, 0x1e, 0x00 };

	// skipping fb responses

	static char payload_d6_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xd6, 0x80, 0x3a, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							0x53, 0x01, 0x24, 0x00, 0x0f, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x01, 0x18, 0x04, 0x03, 0x00 };

	static char payload_1e_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x1e, 0x80, 0x0e, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							0x12, 0x08 };

	static char payload_83_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x83, 0x80, 0x18, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	static char payload_df_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xdf, 0x80, 0x0e, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
							0xff, 0xff };

	static char payload_28_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x28, 0x80, 0x0c, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x11, 0x10, 0x00, 0x00 };

	static char payload_e4_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xe4, 0x80, 0x12, 0x00, 0x10, 0x00, 0x00, 0x00, 0xff, 0x00, 0x01, 0x00,
							0x71, 0x01, 0x02, 0x00, 0x64, 0x00 };

	static char payload_19_1[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x19, 0x81, 0x0e, 0x00, 0x11, 0x00, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00,
							0x00, 0x00 };

	static char payload_28_1[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x28, 0x80, 0x0c, 0x00, 0x12, 0x10, 0x00, 0x00, 0x11, 0x10, 0x00, 0x00 };

	static char payload_b0_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xb0, 0x80, 0x14, 0x00, 0x13, 0x10, 0x00, 0x00, 0x00, 0x00, 0x2b, 0x01,
							0x06, 0x00, 0x54, 0x35, 0x30, 0x31, 0x08, 0x24 };

	static char payload_d6_1[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xd6, 0x80, 0x39, 0x00, 0x14, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							0x53, 0x01, 0x24, 0x00, 0x0f, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x01, 0x18, 0x04, 0x03 };

	static char payload_1e_1[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x1e, 0x80, 0x0e, 0x00, 0x15, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							0x12, 0x08 };

	static char payload_83_1[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x83, 0x80, 0x18, 0x00, 0x16, 0x20, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	static char payload_df_1[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xdf, 0x80, 0x0e, 0x00, 0x17, 0x20, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
							0xff, 0xff };

	static char payload_28_2[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x28, 0x80, 0x0c, 0x00, 0x18, 0x20, 0x00, 0x00, 0x11, 0x10, 0x00, 0x00 };

	static char payload_19_2[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x19, 0x81, 0x0e, 0x00, 0x19, 0x20, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00,
							0x00, 0x00 };

	static char payload_4d_0[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x4d, 0x80, 0x10, 0x00, 0x1a, 0x20, 0x00, 0x00, 0x00, 0x00, 0x54, 0x35,
							0x30, 0x31, 0x08, 0x24 };

	static char payload_d6_2[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xd6, 0x80, 0x38, 0x00, 0x1b, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							0x53, 0x01, 0x24, 0x00, 0x0f, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x01, 0x18, 0x04 };

	static char payload_1e_2[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x1e, 0x80, 0x0e, 0x00, 0x1c, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							0x12, 0x08 };

	static char payload_83_2[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x83, 0x80, 0x18, 0x00, 0x1d, 0x21, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	static char payload_df_2[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xdf, 0x80, 0x0e, 0x00, 0x1e, 0x21, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
							0xff, 0xff };

	static char payload_28_3[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x28, 0x80, 0x0c, 0x00, 0x1f, 0x21, 0x00, 0x00, 0x11, 0x10, 0x00, 0x00 };

	static char payload_19_3[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x19, 0x81, 0x0e, 0x00, 0x20, 0x21, 0x00, 0x00, 0x01, 0x00, 0x05, 0x00,
							0x00, 0x00 };

	static char payload_4d_1[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x4d, 0x80, 0x10, 0x00, 0x21, 0x21, 0x00, 0x00, 0x00, 0x00, 0x54, 0x35,
							0x30, 0x31, 0x08, 0x24 };

	static char payload_d6_3[] = { 0xce, 0xfa, 0x0d, 0xf0, 0xd6, 0x80, 0x38, 0x00, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							0x53, 0x01, 0x24, 0x00, 0x0f, 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x03, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00,
							0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x01, 0x18, 0x04 };

	static char payload_1e_3[] = { 0xce, 0xfa, 0x0d, 0xf0, 0x1e, 0x80, 0x0e, 0x00, 0x23, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
							0x12, 0x08 };

	switch (command0) {
		case 0xa9:
			if (payloadSequence == 0)
				return payload_a9_0;
			return NULL;
			break;
		case 0x03:
			if (payloadSequence == 0)
				return payload_03_0;
			return NULL;
			break;
		case 0xd9:
			if (payloadSequence == 0)
				return payload_d9_0;
			return NULL;
			break;
		case 0xd4:
			if (payloadSequence == 0)
				return payload_d4_0;
			return NULL;
			break;
		case 0x19:
			if (payloadSequence == 0)
				return payload_19_0;
			if (payloadSequence == 1)
				return payload_19_1;
			if (payloadSequence == 2)
				return payload_19_2;
			if (payloadSequence == 3)
				return payload_19_3;
			return NULL;
			break;
		case 0x75:
			if (payloadSequence == 0)
				return payload_75_0;
			return NULL;
			break;
		case 0xd6:
			if (payloadSequence == 0)
				return payload_d6_0;
			if (payloadSequence == 1)
				return payload_d6_1;
			if (payloadSequence == 2)
				return payload_d6_2;
			if (payloadSequence == 3)
				return payload_d6_3;
			return NULL;
			break;
		case 0x1e:
			if (payloadSequence == 0)
				return payload_1e_0;
			if (payloadSequence == 1)
				return payload_1e_1;
			if (payloadSequence == 2)
				return payload_1e_2;
			if (payloadSequence == 3) {
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[MWIFIEX] Response noted\n");
				return payload_1e_3;
			}
			return NULL;
			break;
		case 0x83:
			if (payloadSequence == 0)
				return payload_83_0;
			if (payloadSequence == 1)
				return payload_83_1;
			if (payloadSequence == 2)
				return payload_83_2;
			return NULL;
			break;
		case 0xdf:
			if (payloadSequence == 0)
				return payload_df_0;
			if (payloadSequence == 1) {
				payloadSequence++;
				return payload_df_1;
			}
			if (payloadSequence == 2) {
				payloadSequence++;
				return payload_df_2;
			}
			return NULL;
			break;
		case 0x28:
			if (payloadSequence == 0)
				return payload_28_0;
			if (payloadSequence == 1)
				return payload_28_1;
			if (payloadSequence == 2)
				return payload_28_2;
			if (payloadSequence == 3)
				return payload_28_3;
			return NULL;
			break;
		case 0xe4:
			if (payloadSequence == 0) {
				payloadSequence++;
				return payload_e4_0;
			}
			return NULL;
			break;
		case 0xb0:
			return payload_b0_0;
			break;
		case 0x4d:
			if (payloadSequence == 2) {
				return payload_4d_0;
			}
			if (payloadSequence == 3) {
				return payload_4d_1;
			}
			return NULL;
			break;
		default:
			return NULL;
	}
}

void PrintMwifiexCmd(void* command) {

	HostCmd* hostCmd = (HostCmd*)command;

	switch (hostCmd->command_type) {

	case MWIFIEX_USB_TYPE_CMD:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Command Type:\tMWIFIEX_USB_TYPE_CMD\n");

		switch (hostCmd->hostCmd) {

		case HostCmd_CMD_GET_HW_SPEC:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_GET_HW_SPEC\n");
			break;

		case HostCmd_CMD_802_11_SCAN:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_SCAN\n");
			break;

		case HostCmd_CMD_802_11_GET_LOG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_GET_LOG\n");
			break;

		case HostCmd_CMD_MAC_MULTICAST_ADR:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_MAC_MULTICAST_ADR\n");
			break;

		case HostCmd_CMD_802_11_EEPROM_ACCESS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_EEPROM_ACCESS\n");
			break;

		case HostCmd_CMD_802_11_ASSOCIATE:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_ASSOCIATE\n");
			break;

		case HostCmd_CMD_802_11_SNMP_MIB:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_SNMP_MIB\n");
			break;

		case HostCmd_CMD_MAC_REG_ACCESS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_MAC_REG_ACCESS\n");
			break;

		case HostCmd_CMD_BBP_REG_ACCESS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_BBP_REG_ACCESS\n");
			break;

		case HostCmd_CMD_RF_REG_ACCESS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_RF_REG_ACCESS\n");
			break;

		case HostCmd_CMD_PMIC_REG_ACCESS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_PMIC_REG_ACCESS\n");
			break;

		case HostCmd_CMD_RF_TX_PWR:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_RF_TX_PWR\n");
			break;

		case HostCmd_CMD_RF_ANTENNA:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_RF_ANTENNA\n");
			break;

		case HostCmd_CMD_802_11_DEAUTHENTICATE:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_DEAUTHENTICATE\n");
			break;

		case HostCmd_CMD_MAC_CONTROL:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_MAC_CONTROL\n");
			break;

		case HostCmd_CMD_802_11_AD_HOC_START:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_AD_HOC_START\n");
			break;

		case HostCmd_CMD_802_11_AD_HOC_JOIN:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_AD_HOC_JOIN\n");
			break;

		case HostCmd_CMD_802_11_AD_HOC_STOP:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_AD_HOC_STOP\n");
			break;

		case HostCmd_CMD_802_11_MAC_ADDRESS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_MAC_ADDRESS\n");
			break;

		case HostCmd_CMD_802_11D_DOMAIN_INFO:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11D_DOMAIN_INFO\n");
			break;

		case HostCmd_CMD_802_11_KEY_MATERIAL:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_KEY_MATERIAL\n");
			break;

		case HostCmd_CMD_802_11_BG_SCAN_CONFIG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_BG_SCAN_CONFIG\n");
			break;

		case HostCmd_CMD_802_11_BG_SCAN_QUERY:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_BG_SCAN_QUERY\n");
			break;

		case HostCmd_CMD_WMM_GET_STATUS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_WMM_GET_STATUS\n");
			break;

		case HostCmd_CMD_802_11_SUBSCRIBE_EVENT:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_SUBSCRIBE_EVENT\n");
			break;

		case HostCmd_CMD_802_11_TX_RATE_QUERY:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_TX_RATE_QUERY\n");
			break;

		case HostCmd_CMD_802_11_IBSS_COALESCING_STATUS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_IBSS_COALESCING_STATUS\n");
			break;

		case HostCmd_CMD_MEM_ACCESS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_MEM_ACCESS\n");
			break;

		case HostCmd_CMD_CFG_DATA:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_CFG_DATA\n");
			break;

		case HostCmd_CMD_VERSION_EXT:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_VERSION_EXT\n");
			break;

		case HostCmd_CMD_MEF_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_MEF_CFG\n");
			break;

		case HostCmd_CMD_RSSI_INFO:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_RSSI_INFO\n");
			break;

		case HostCmd_CMD_FUNC_INIT:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_FUNC_INIT\n");
			break;

		case HostCmd_CMD_FUNC_SHUTDOWN:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_FUNC_SHUTDOWN\n");
			break;

		case HOST_CMD_APCMD_SYS_RESET:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHOST_CMD_APCMD_SYS_RESET\n");
			break;

		case HostCmd_CMD_UAP_SYS_CONFIG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_UAP_SYS_CONFIG\n");
			break;

		case HostCmd_CMD_UAP_BSS_START:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_UAP_BSS_START\n");
			break;

		case HostCmd_CMD_UAP_BSS_STOP:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_UAP_BSS_STOP\n");
			break;

		case HOST_CMD_APCMD_STA_LIST:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHOST_CMD_APCMD_STA_LIST\n");
			break;

		case HostCmd_CMD_UAP_STA_DEAUTH:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_UAP_STA_DEAUTH\n");
			break;

		case HostCmd_CMD_11N_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_11N_CFG\n");
			break;

		case HostCmd_CMD_11N_ADDBA_REQ:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_11N_ADDBA_REQ\n");
			break;

		case HostCmd_CMD_11N_ADDBA_RSP:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_11N_ADDBA_RSP\n");
			break;

		case HostCmd_CMD_11N_DELBA:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_11N_DELBA\n");
			break;

		case HostCmd_CMD_RECONFIGURE_TX_BUFF:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_RECONFIGURE_TX_BUFF\n");
			break;

		case HostCmd_CMD_CHAN_REPORT_REQUEST:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_CHAN_REPORT_REQUEST\n");
			break;

		case HostCmd_CMD_AMSDU_AGGR_CTRL:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_AMSDU_AGGR_CTRL\n");
			break;

		case HostCmd_CMD_TXPWR_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_TXPWR_CFG\n");
			break;

		case HostCmd_CMD_TX_RATE_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_TX_RATE_CFG\n");
			break;

		case HostCmd_CMD_ROBUST_COEX:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_ROBUST_COEX\n");
			break;

		case HostCmd_CMD_802_11_PS_MODE_ENH:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_PS_MODE_ENH\n");
			break;

		case HostCmd_CMD_802_11_HS_CFG_ENH:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_HS_CFG_ENH\n");
			break;

		case HostCmd_CMD_P2P_MODE_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_P2P_MODE_CFG\n");
			break;

		case HostCmd_CMD_CAU_REG_ACCESS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_CAU_REG_ACCESS\n");
			break;

		case HostCmd_CMD_SET_BSS_MODE:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_SET_BSS_MODE\n");
			break;

		case HostCmd_CMD_PCIE_DESC_DETAILS:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_PCIE_DESC_DETAILS\n");
			break;

		case HostCmd_CMD_802_11_SCAN_EXT:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_802_11_SCAN_EXT\n");
			break;

		case HostCmd_CMD_COALESCE_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_COALESCE_CFG\n");
			break;

		case HostCmd_CMD_MGMT_FRAME_REG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_MGMT_FRAME_REG\n");
			break;

		case HostCmd_CMD_REMAIN_ON_CHAN:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_REMAIN_ON_CHAN\n");
			break;

		case HostCmd_CMD_GTK_REKEY_OFFLOAD_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_GTK_REKEY_OFFLOAD_CFG\n");
			break;

		case HostCmd_CMD_11AC_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_11AC_CFG\n");
			break;

		case HostCmd_CMD_HS_WAKEUP_REASON:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_HS_WAKEUP_REASON\n");
			break;

		case HostCmd_CMD_TDLS_CONFIG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_TDLS_CONFIG\n");
			break;

		case HostCmd_CMD_MC_POLICY:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_MC_POLICY\n");
			break;

		case HostCmd_CMD_TDLS_OPER:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_TDLS_OPER\n");
			break;

		case HostCmd_CMD_FW_DUMP_EVENT:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_FW_DUMP_EVENT\n");
			break;

		case HostCmd_CMD_SDIO_SP_RX_AGGR_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_SDIO_SP_RX_AGGR_CFG\n");
			break;

		case HostCmd_CMD_STA_CONFIGURE:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_STA_CONFIGURE\n");
			break;

		case HostCmd_CMD_CHAN_REGION_CFG:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_CHAN_REGION_CFG\n");
			break;

		case HostCmd_CMD_PACKET_AGGR_CTRL:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tHostCmd_CMD_PACKET_AGGR_CTRL\n");
			break;

		default:
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "HostCMD:\t\tUNKNOWN\n");
			break;

		}

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Size:\t\t\t%hu\n", hostCmd->size);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Seq Num:\t\t%hu\n", hostCmd->seq_num);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Result:\t\t%hu\n", hostCmd->result);

		break;

	case MWIFIEX_USB_TYPE_DATA:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Command Type:\tMWIFIEX_USB_TYPE_DATA\n");
		break;

	case MWIFIEX_USB_TYPE_EVENT:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Command Type:\tMWIFIEX_USB_TYPE_EVENT\n");
		break;

	default:
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Command Type:\tUNKNOWN - %d\n", hostCmd->hostCmd);
		break;

	}
}