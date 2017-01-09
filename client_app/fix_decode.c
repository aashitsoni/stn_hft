
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Network LLC

file name : client_app/fix_decode.c

Author      Date				Comments
------      -----				---------
mt			01/02/2017			Created after the completion of login testing
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "stn_errno.h"
#include "console_log.h"
#include "stn_hft_fix_op_public.h"
#include "stn_hft_mkt_data_public.h"
#include "stn_hft_pair_strategy_public.h"
#include "fix_server_test.h"
#include "stn_hft_fix_decode.h"
#include "console_log.h"

extern STN_FIX_CLIENT_APP_STATE_E g_stn_fix_client_app_state_e;


int hft_client_decode_fix_message_callback(void* chnl_hdl,STN_HFT_FIX_DECODED_MSG* msg_struct,char* pkt, int msg_len)
{
	switch(msg_struct->msg_type)
		{
		case FIX_MSG_TYPE_TEST_REQUEST:
		case FIX_MSG_TYPE_HEARTBEAT:
			stn_hft_FIX_op_channel_send_hb(chnl_hdl);
			break;
		case FIX_MSG_TYPE_RESEND_REQUEST:
			break;
		case FIX_MSG_TYPE_REJECT:
			break;
		case FIX_MSG_TYPE_SEQUENCE_RESET:
			break;

		// Process Logout from client side as well to terminate the session.
		case FIX_MSG_TYPE_LOGOUT:
			break;
		case FIX_MSG_TYPE_EXECUTION_REPORT:
			break;
		case FIX_MSG_TYPE_ORDER_CANCEL_REJECT:
			break;

		case FIX_MSG_TYPE_TRADING_SESSION_STATUS:
			// process Trading Session Status
			g_stn_fix_client_app_state_e = STN_FIX_CLIENT_TRADING_STATE;
			
			break;
		case FIX_MSG_TYPE_BUSINESS_MESSAGE_REJECT:
			break;
		default:
			console_log_write("%s:%d fix_message_callback handler not supported for msg_type %c",__FILE__,__LINE__,msg_struct->msg_type);
			break;
		}

	return 0;
}
