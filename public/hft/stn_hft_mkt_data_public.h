/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Networks LLC, MA

Author      Date				Comments
------      -----				---------
mt			01/04/2012			init rev
mt			01/07/2012			mcast data recv from hardware sample with callback
mt			01/21/2012			mcast data recv from hardware sample with Huge Page buffer
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

#ifndef STN_HFT_MKT_DATA_PUBLIC_H
#define STN_HFT_MKT_DATA_PUBLIC_H

#include <stdint.h>

//// PUBLIC ////

#define STN_HFT_SYM_FILTER_TYPE_SYMBOL					0x01
#define STN_HFT_SYM_FILTER_TYPE_TOKEN_SCRIP_CODE		0x02


/* Structure for supplying the symbol to be filtered in HARDWARE
   Assuming that the hardware is handling the entire message processing
 */
struct stn_mkt_channel_sym_filters_s {
	uint8_t sym_filter_union_type;		/*[IN] */
	union {								/*[IN] */	
		uint32_t token_or_scrip_code;		
		uint8_t  symbol[10];
	}u;
	uint8_t sym_len;
	uint16_t sym_filter_id;				/* [OUT] This is returned by the PAX subsystem for every symbol that has been provided for filtering */
};

// PUBLIC
typedef enum stn_hft_exchange_id_s_e {
	stn_hft_enum_exchange_id_nse_e,
	stn_hft_enum_exchange_id_mcx_e
}stn_hft_exchange_id_e;


#define STN_HFT_MKT_CHANNEL_TYPE_TCP					0x01
#define STN_HFT_MKT_CHANNEL_TYPE_MUDP					0x02


// PUBLIC
struct stn_hft_mkt_channel_public_s {
	uint8_t mkt_data_addr[16];			//Depending on the exchange type, this will be a Multicast or Unicast address. IPv4 addressing only
	uint16_t mkt_data_port;				//This could be the UDP or TCP port of the market data stream source
	uint8_t channel_type;				// One of the type, see #define above..
	uint8_t local_interface_ip [16];	//IP of the hardware port on which this market channel should bind. ipv4 addressing only

	stn_hft_exchange_id_e 
			exchange_id;				//One of the enum exchange_id_e's defined above

	struct stn_mkt_channel_sym_filters_s 
				symbol_filters[16];		// MAX 16 symbol filters per channel. Hardware will support 16 sym's only at this time

	uint8_t recv_cpu_id;				// Canonical # - CPU ID on which the driver/hardware should send the MSI receive for the data for this channel
	
	int msg_drop_count;// message drop counter

	/* Fields to enable stats logging for channel. for DEBUGGING ONLY..*/
	//NOTE: Enabling this will be a performance degradation.. this is  I/O
	int log_stats_flag;
	int log_interval; // in secs
};

// callback function to be provided by CALLER APPLICATION for decompress routines..
/* 
INPUT parameters:
	Handle for user routine, ptr to buffer as seen by hardware, length of full buffer as see by hardware 
*/
typedef int (*MKT_MSG_CBFN)(void* user_mkt_msg_handle, uint8_t* mkt_data_bfr, uint16_t mkt_data_bfr_len, uint32_t reserved); 

/// ---- MULTICAST mkt data accessors --
int stn_hft_mkt_data_channel_mcast_create	(struct stn_hft_mkt_channel_public_s*, 
											 struct stn_mkt_channel_sym_filters_s*, 
											 uint16_t num_sym_filters, 
											 MKT_MSG_CBFN user_provided_cbfn,
											 uint8_t* user_cbfn_handle, 
											void** pax_hft_mkt_data_channel_handle);

//---- 
int stn_hft_mkt_data_channel_start			(void* pax_hft_channel_handle);
int stn_hft_mkt_data_channel_delete			(void* pax_hft_channel_handle);

//--- 
int stn_hft_mkt_data_channel_get_next_msg	(void* hdl,unsigned char** msg, unsigned int *msg_len);


/// ---- TCP mkt data accessors --
int stn_hft_mkt_data_channel_tcp_create		(struct stn_hft_mkt_channel_public_s*, 
											 struct stn_mkt_channel_sym_filters_s*, 
											 uint16_t num_sym_filters, 
											 MKT_MSG_CBFN user_provided_cbfn,
											 void**pax_hft_channel_handle);
int stn_hft_mkt_data_channel_tcp_login		(void*pax_hft_channel_handle, uint8_t* user_name, uint8_t* passwd);
int stn_hft_mkt_data_channel_tcp_logout		(void*pax_hft_channel_handle);



#endif
