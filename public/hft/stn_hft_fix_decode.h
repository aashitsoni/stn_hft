
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014 : SwiftTrade Networks LLC, MA


file name : public/hft/stn_hft_fix_decode.h

Author      Date                Comments
------      -----               ---------
mt          01/03/2017          init rev
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/
#ifndef _STN_HFT_FIX_DECODE_H_
#define _STN_HFT_FIX_DECODE_H_

enum _stn_fix_msg_type_e
{
	// Session messages
 	FIX_MSG_TYPE_HEARTBEAT							= '0',
 	FIX_MSG_TYPE_TEST_REQUEST						= '1',
 	FIX_MSG_TYPE_RESEND_REQUEST						= '2',
 	FIX_MSG_TYPE_REJECT								= '3',
 	FIX_MSG_TYPE_SEQUENCE_RESET						= '4',
 	FIX_MSG_TYPE_LOGOUT								= '5',
	FIX_MSG_TYPE_LOGON								= 'A',

	// application messages
 	FIX_MSG_TYPE_EXECUTION_REPORT					= '8',
	FIX_MSG_TYPE_ORDER_CANCEL_REJECT				= '9',
	FIX_MSG_TYPE_ORDER_SINGLE						= 'D',
	FIX_MSG_TYPE_ORDER_CANCEL						= 'F',
	FIX_MSG_TYPE_ORDER_REPLACE						= 'G',

 	FIX_MSG_TYPE_TRADING_SESSION_STATUS_REQUEST 	= 'g',
 	FIX_MSG_TYPE_TRADING_SESSION_STATUS				= 'h',
	FIX_MSG_TYPE_BUSINESS_MESSAGE_REJECT			= 'j',
 	FIX_MSG_TYPE_NEWS								= 'B',
 	FIX_MSG_TYPE_ORDER_STATUS_REQUEST				= 'H',

	// let this be the last one 	
 	FIX_MSG_TYPE_NONE								= 0
}STN_FIX_MSG_TYPE_E;



typedef struct _STN_HFT_FIX_DECODED_MSG_T
{
	char 			msg_type;
	char 			ClOrdId[64];
	char 			OrigClOrdId[64];
	char 			OrdId[64];
	char 			InstrumentCode[32];
	unsigned int 	OrdQty;
	char 			OrdStatus;
	char			ExecType;
	char			tag60CreationTime[32];

}STN_HFT_FIX_DECODED_MSG;


typedef int (*STN_HFT_FIX_MSG_DECODE_CALLBACK)(void* fix_hdl,STN_HFT_FIX_DECODED_MSG* msg,char* pkt, int msg_len);

int stn_hft_FIX_initiate_message_decode(void * pax_hft_FIX_op_channel_handle,STN_HFT_FIX_MSG_DECODE_CALLBACK pMsgCallBack);
int stn_hft_FIX_terminate_message_decode(void* pax_hft_FIX_op_channel_handle);

#endif
