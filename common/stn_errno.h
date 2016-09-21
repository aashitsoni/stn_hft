
#ifndef PAX_ERRNO_H
#define	PAX_ERRNO_H

/* - - -  -
POSIX error codes
http://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
 - - - - - - 
 */

//----------------
// GENERAL 
#define STN_ERRNO_SUCCESS							0x0000
#define STN_ERRNO_FAIL								0x0001
#define	STN_ERRNO_MALLOC_FAIL						0x0002
#define STN_ERRNO_NODATARECV						0x0003
#define STN_ERRNO_NODATASENT                        0x0004
#define STN_ERRNO_BUFFER_TOO_LONG                   0x0005
#define STN_ERRNO_PARAM_PROBLEM						0x0006

//----------------
// PAS DEVICE 
#define	PAX_ERRNO_DEVICE_ALREADY_STARTED			0x0010
#define	PAX_ERRNO_DEVICE_OPEN_FAIL					0x0011
#define	PAX_ERRNO_DEVICE_INVALID					0x0012
#define PAX_ERRNO_DEVICE_NIC_SERIAL_STRING_INVALID	0x0013
#define PAX_ERRNO_DEVICE_FAILED_TO_INITIALIZE		0x0014
#define PAX_ERRNO_DEVICE_HUGE_TLB_INIT_FAILED		0x0015
#define PAX_ERRNO_DEVICE_GPS_CALIBRATE_FAILED		0x0016
#define PAX_ERRNO_DEVICE_RX_START_FAILED			0x0017
#define PAX_ERRNO_DEVICE_FN_NOT_AVAIL_FOR_DEVICE	0x0018
#define PAX_ERRNO_DEVICE_IOCTL_INVALID				0x0019
#define PAX_ERRNO_DEVICE_PS_SETUP_FAILED			0x001A
#define PAX_ERRNO_DEVICE_DOESNT_NEED_DMA_RESERVE	0x001B


//----------------
// NUMA MEMORY and PS
#define	PAX_ERRNO_PS_NUMA_UPDATE_NOT_SUPPORTED		0x0020
#define	PAX_ERRNO_PS_NUMA_CPUID_INVALID				0x0021
#define	PAX_ERRNO_PS_MAP_FULL						0x0022
#define	PAX_ERRNO_PS_BFR_FREE_LIST_EMPTY			0x0023
#define	PAX_ERRNO_PS_MAP_FREE_LIST_EMPTY			0x0024
#define	PAX_ERRNO_PS_MAP_READY_TO_USE_LIST_EMPTY	0x0025
#define	PAX_ERRNO_PS_INCOMPLETE_BFR_SET_ALLOCATED	0x0026
#define	PAX_ERRNO_PS_INCOMPLETE_BFR_SET_SETUP		0x0027
#define	PAX_ERRNO_PS_INCOMPLETE_BFR_SET_RETRIEVED	0x0028
#define	PAX_ERRNO_PS_NEED_NEW_BUFFER				0x0029
#define	PAX_ERRNO_PS_NO_BUFFERS_RESERVED_FOR_CPU	0x0030
#define PAX_ERRNO_NUMA_HUGE_PAGE_ALLOC_FAIL			0x0031 

//---------
// HFT
#define	STN_ERRNO_HFT_INVALID_EXCHANGE_ID			0x0040
#define	STN_ERRNO_HFT_MKT_CHNL_INIT_FAIL			0x0041
#define	STN_ERRNO_HFT_LOGIN_SUCCESS					0x0042
#define	STN_ERRNO_HFT_LOGIN_FAIL					0x0043
#define	STN_ERRNO_HFT_MCAST_MEMBERSHIP_FAIL			0x0044
#define	STN_ERRNO_HFT_HW_SOCKET_BIND_FAIL			0x0045
#define	STN_ERRNO_HFT_SOCKET_CREATION_FAIL			0x0046
#define STN_ERRNO_HFT_FIX_PARSING_FAILED			0x0047
#define STN_ERRNO_NODATASEND						0x0048

// HFT PAIR
#define STN_ERRNO_HFT_PAIR_ORDER_NOT_FOUND			0x0060
#define STN_ERRNO_HFT_PAIR_UPDATE_FAILED			0x0061
#define STN_ERRNO_PAIR_PORTFOLIO_DONE				0x0062

//---------
/// TOE
#define	PAX_ERRNO_TOE_XXX							0x0080


//-------------------
/// Pattern Matching
#define	PAX_ERRNO_PME_TOO_MANY_PMES_FOR_HDL			0x0100
#define	PAX_ERRNO_PME_FREE_WKE_LIST_EMPTY			0x0101
#define	PAX_ERRNO_PME_TOO_MANY_PATTRNS_FOR_HDL		0x0102
#define	PAX_ERRNO_PME_PATTRN_DB_FULL				0x0103
#define	PAX_ERRNO_PME_SEARCH_ALGO_NOT_SUPPORTED		0x0104

//-------------
// Event Bus
#define PAX_ERRNO_EB_NO_FREE_CBF_SLOTS				0x0200
#define PAX_ERRNO_EB_CBF_ALREADY_REGISTERED			0x0201
#define	PAX_ERRNO_EB_EVENT_FREE_POOL_EMPTY			0x0202
#define	PAX_ERRNO_EB_RECORD_TOO_LONG				0x0203
#define	PAX_ERRNO_EB_EVENT_ID_TOO_BIG				0x0204
#define	PAX_ERRNO_EB_EVENT_ID_RESERVED				0x0205

//-------------
// Hash Table
#define PAX_ERRNO_HT_ITEM_ALREADY_EXISTS			0x0300
#define PAX_ERRNO_HT_ITEM_NOT_FOUND					0x0301

//-------------
// FAST Alloc pool
#define PAX_ERRNO_FAST_BLOCK_LIST_CREATE_FAILED		0x0400
#define PAX_ERRNO_FAST_BLOCK_LIST_GROW_FAILED		0x0401

//---------------

#define PAX_ERRNO_NAVL_OPEN_FAIL					0x0500
#define PAX_ERRNO_NAVL_INTG_FREE_WKE_LIST_EMPTY		0x0501


#endif