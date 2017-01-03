
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Network LLC

File Name : client_app/fix_server_test.h

Author      Date				Comments
------      -----				---------
mt			10/20/2016			Created
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/
#ifndef _FIX_SERVER_TEST_H_
#define _FIX_SERVER_TEST_H_

struct order_state_info
{
	uint8_t tag37[64]; // exchange order number
	uint8_t tag41[64]; // origclordid
	uint8_t tag61[64]; // transact time
};


/*
#comments are written with this format
# anything beside # is considered comments
name=swifttrade
support=support@swifttradenetworks.com
engine=stn
version=1.0
client=proficient
interface=10.20.201.1
multicast_ip=239.235.255.21
multicast_port=50001
fix_gw=192.168.8.11
fix_port=21456
fix_tag_91=378DDF05FE43F488
fix_tag_96=13556,13555,152900129
fix_tag_49=STNHFT
fix_tag_56=MCXTRADE
fix_tag_57=ADMIN
fix_tag_9227=700001001001110
instrument_A=9123456
instrument_B=9324399

*/

typedef struct hft_config_s
{
	char g_name[64];
	char g_support[64];
	char g_engine[64];
	char g_version[64];
	char g_client[64];
	char g_interface[64];
	char g_multicast_ip[64];
	char g_multicast_port[64];
	char g_fix_gw[64];
	char g_fix_port[64];
	char g_fix_tag_91[64];
	char g_fix_tag_96[64];
	char g_fix_tag_49[64];
	char g_fix_tag_56[64];
	char g_fix_tag_57[64];
	char g_fix_tag_9227[64];
	char g_instrument_A[64];
	char g_instrument_B[64];
	char g_fix_tag_554[64]; // old password
	char g_fix_tag_925[64]; // new passowrd
	char g_fix_tag_108[32]; // heartbeat interval
}HFT_CONFIG_T,*HFT_CONFIG_P;



struct channel_trd_attribs_s

{
    struct stn_hft_FIX_op_channel_public_attrib_s* chl;
    void* chnl_hdl;
};


// exetrn variables
extern HFT_CONFIG_T g_hft_config;
extern int bShutdown;
extern int seqNum;
extern int bAppRunning;
extern int g_logged_in;
extern pthread_t 	g_hb_thread;

int hft_client_fix_channel_process_login(void* chnl_hdl);
int hft_read_config_file();
int hft_print_config_info_ask_answer();
int hft_print_config();

int fix_op_test_process_order_new(void* chnl_hdl,struct order_state_info* ord_info);
int fix_op_test_process_order_replace(void* chnl_hdl,struct order_state_info* ord_info);
int fix_op_test_process_order_cancel(void *chnl_hdl,struct order_state_info* ord_info);




#endif