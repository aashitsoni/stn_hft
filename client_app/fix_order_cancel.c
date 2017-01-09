
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Network LLC

File Name : client_app/fix_order_cancel.c


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





// need exchange id - 37, 41 and 60 to be pass along with the order
// process order cancel message for the fix operation testing
int fix_op_test_process_order_cancel(void *chnl_hdl,struct order_state_info* ord_info)
{
	
	uint8_t* msg;
	unsigned int msg_len;
	int iMsg = 0;

	struct FIX_OC_variables_s FIX_OC_Constants =
	{
		.tag_34_msg_seq_num = seqNum+2,
		.tag_9366_strat_id=20,
	};
	
	char cxlNum[16] = "";
	bzero(cxlNum, sizeof(cxlNum));
	sprintf(cxlNum, "MCXSX%06d", seqNum+2);
	strcpy((char*)FIX_OC_Constants.tag_11_client_order_id, cxlNum);
	strcpy((char*)FIX_OC_Constants.tag_37_order_id, (char*)ord_info->tag37);
	strcpy((char*)FIX_OC_Constants.tag_41_orig_clor_id, (char*)ord_info->tag41);
	strcpy((char*)FIX_OC_Constants.tag_60_message_creation_time, (char*)ord_info->tag61);
	
	stn_hft_FIX_op_channel_send_order_cancel(chnl_hdl,&FIX_OC_Constants); 
	console_log_write("%s:%d sent cxl req",__FILE__,__LINE__);
	fflush(stdout);
	sleep(2);
	// retrieve two message as echo server would be replying
	iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);

	if( STN_ERRNO_SUCCESS == iMsg)
		console_log_write("%s:%d CXLD Recieved Message :%d %s\n",__FILE__,__LINE__,msg_len, msg);
	else if(STN_ERRNO_FAIL == iMsg)
		console_log_write("%s:%d stn_hft_FIX_op_channel_get_next_msg (...) failed\n"__FILE__,__LINE__);

	fflush(stdout);
	return iMsg;
	
}
