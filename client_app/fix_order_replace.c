
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Network LLC

File Name  : client_app/fix_order_replace.c

Author      Date				Comments
------      -----				---------
mt			10/20/2016			Created
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
#include "stn_hft_fix_op_public.h"
#include "stn_hft_mkt_data_public.h"
#include "stn_hft_pair_strategy_public.h"

#include "fix_server_test.h"
#include "console_log.h"






// - - - - - - - - - - - - - - - -
// process fix order replace for the fix operation testing with exchange
int fix_op_test_process_order_replace(void* chnl_hdl,struct order_state_info* ord_info)
{
	uint8_t* msg;
	int msg_len;
 	int iMsg = 0;

	struct FIX_OR_variables_s FIX_OR_Constants =
	{
		.tag_34_msg_seq_num = seqNum+1,
		//.tag_22_alternative_SecurityID ="SECID",
		//.tag_11_client_order_id = "MCX02",
		.tag_21_floor_broker_instr = '1',
		//.tag_37_order_id ="MCXORDER00101",
		.tag_38_order_qty = 2,
		.tag_40_order_type = '2',
		//.tag_41_orig_clor_id = "MCX01",
		.tag_44_price =618075,
		 .tag_9366_strat_id=20,
		//.tag_48_instrument_code ="INST",
		//.tag_54_side ='1',
		//.tag_55_symbol = "GOOG",
		//.tag_167_instrument_type="INSTCODE",
		//.tag_200_mat_year_mth = "201203",
		//.tag_205_mat_day ='2',
	};

	FIX_OR_Constants.tag_34_msg_seq_num = seqNum+1;
	char rplNum[16] = "";
	bzero(rplNum, sizeof(rplNum));
				sprintf(rplNum, "MCXSX%06d", seqNum+1);
				
	strcpy(FIX_OR_Constants.tag_11_client_order_id, rplNum);
	strcpy(FIX_OR_Constants.tag_37_order_id, ord_info->tag37);
	strcpy(FIX_OR_Constants.tag_41_orig_clor_id, ord_info->tag41);
	console_log_write("%s:%d setting transTime=%s\n",__FILE__,__LINE__, ord_info->tag61);
	fflush(stdout);
	strcpy(FIX_OR_Constants.tag_60_message_creation_time, ord_info->tag61);
	console_log_write("%s:%d set transTime=%s\n", __FILE__,__LINE__,ord_info->tag61);
	fflush(stdout);
	stn_hft_FIX_op_channel_send_order_replace(chnl_hdl,&FIX_OR_Constants);
	console_log_write("%s:%d sent replace request\n",__FILE__,__LINE__);
	fflush(stdout); 
	// retrieve two message as echo server would be replying
	sleep(2);
	// assign it back from the replace order number
	strcpy(ord_info->tag41,rplNum);
	
	
	iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);
	if( STN_ERRNO_SUCCESS == iMsg)
		{
		console_log_write("%s:%d RPLD Recieved Message :%d %s\n",__FILE__,__LINE__,msg_len, msg);
		int transTimeFound = 0;
		int num = 0;
		for(num=0; num<msg_len; num++)
			{
			if(msg[num] == '6' && msg[num+1] == '0' && msg[num+2] == '=')
				{
				transTimeFound = 1;
				memcpy(ord_info->tag61, msg+num+3, 17);
				ord_info->tag61[17] = 0;
				break;
				}
		}
		if(transTimeFound)
				console_log_write("%s:%d transTime=%s\n", __FILE__,__LINE__,ord_info->tag61);
		else
				console_log_write("%s:%d transtime not found\n",__FILE__,__LINE__);
		}
	else if(STN_ERRNO_FAIL == iMsg)
		console_log_write("%s:%d closure of channel",__FILE__,__LINE__);

	fflush(stdout);
	return iMsg;	
}
