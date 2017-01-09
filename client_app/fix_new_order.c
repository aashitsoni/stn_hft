
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Network LLC

File Name : client_app/fix_new_order.c

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
// process fix order new  for the fix operation testing with exchange
int fix_op_test_process_order_new(void* chnl_hdl,struct order_state_info* ord_info)
{
	uint8_t* msg;
	unsigned int msg_len;
    char ordNum[16] = ""; 
	int iMsg = STN_ERRNO_SUCCESS;

	struct FIX_OE_variables_s FIX_OE_Constants =
	{
		 .tag_34_msg_seq_num=seqNum,
		 .tag_21_floor_broker_instr='1',
		 .tag_38_order_qty = 1,
		 .tag_40_order_type= '2',
		 .tag_44_price=618075,
		 .tag_48_instrument_code="201979",
		 .tag_54_side='2',
		 .tag_9366_strat_id=20,
	};
	
	// send generic message
	bzero(ord_info->tag37, 64);
	bzero(ord_info->tag61, 64);

	sprintf(ordNum, "MCXSX%06d", seqNum);
	strcpy((char*)FIX_OE_Constants.tag_11_client_order_id, ordNum);
	stn_hft_FIX_op_channel_send_order_new(chnl_hdl,&FIX_OE_Constants);

	strcpy((char*)ord_info->tag41,ordNum);
	
// retrieve two message as echo server would be replying
	sleep(3);
	
	while((iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len)) == STN_ERRNO_NODATARECV);
	//		iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);

	if( STN_ERRNO_SUCCESS == iMsg)
		{
		int ordIdFound=0, transTimeFound=0;
		int num=0;
		console_log_write("%s:%d Recieved ACK Message :%d %s\n",__FILE__,__LINE__,msg_len, msg);
		for( num=0; num < msg_len; num++)
		{
			if(msg[num] == '3' && msg[num+1] == '7' && msg[num+2] == '=' )
			{
				ordIdFound = 1;
				memcpy(ord_info->tag37, msg+num+3, 15);
				break;
			}
		}
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
		if(ordIdFound)
			console_log_write("%s:%d exchId = %s\n", __FILE__,__LINE__,ord_info->tag37);
		else
			console_log_write("%s:%d Order Identifier not found\n",__FILE__,__LINE__);

		if(transTimeFound)
			console_log_write("%s:%d transTime=%s\n",__FILE__,__LINE__, ord_info->tag61);
		else
			console_log_write("%s:%d tag61 not found\n",__FILE__,__LINE__);

		}
	else if(STN_ERRNO_FAIL == iMsg)
		console_log_write("%s:%d stn_hft_FIX_op_channel_get_next_msg (...) failed\n",__FILE__,__LINE__);
	else 
		console_log_write("%s:%d received some imsg val=%d\n",__FILE__,__LINE__, iMsg);

	fflush(stdout);
	return iMsg;
}
