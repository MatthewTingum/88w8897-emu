#include "Mwifiex.h"
#include <Wdm.h>

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