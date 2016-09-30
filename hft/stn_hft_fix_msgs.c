/******************************************************************************************************************

 Author      Date               Comments
 ------      -----              ---------
   mt       09/24/2016          Seperated fix messgae functionality into new file from the op-core

*******************************************************************************************************************/
#include <sys/mman.h>
#include <numa.h>

#include "stn_hft_fix_op_private.h"
#include "stn_numa_impl.h"
#include "stn_sdk_version.h"
#include <time.h>
#include "stn_hft_fix_msgs.h"
#include "Stn_errno.h"

//- - - - - - - - - - - - 

int stn_hft_FIX_op_send_generic_msg(void* pax_hft_FIX_op_channel_handle,unsigned char msg_type, unsigned char* fix_msg, int msg_size, char* time_str);


int g_hb_count = 0;

#define CALC_MSG_LENGTH(msg_length) msg_length = (msg_length>999)? (msg_length - 17):(msg_length - 16);



/*
format current time for logging as well as to be used inside the format message for the FIX api
*/
char* FormatCurrTime(char* szTime)
{
    time_t curr_time;
    struct tm* gm_time;
    
    
    time(&curr_time);
    gm_time = localtime(&curr_time);
    sprintf(szTime,"%d%02d%02d-%02d:%02d:%02d",gm_time->tm_year+1900,
                        gm_time->tm_mon+1,
                        gm_time->tm_mday,
                        gm_time->tm_hour,
                        gm_time->tm_min,
                        gm_time->tm_sec);

    return szTime;
}





/*get fix message sequence length*/
int get_fix_msg_seq_len(int sequenceno)
{
    if(sequenceno < 9999)
        return 4;
    else if(sequenceno < 99999)
        return 5;
    else if(sequenceno < 999999)
        return 6;
    else if(sequenceno < 9999999)
        return 7;
    else if(sequenceno < 99999999)
        return 8;
    else if(sequenceno < 999999999)
        return 9;
    else                        // TODO: if it can really go beyond 10
        return 10;
    
}


/*
stn_hft_FIX_message_compute_checksum - Function computes FIX 4.2 related checksum value and pass it back into the checksum field

input parameters: 
			char* buf : a pointer to the string buffer for which the checksum should be computed.
			long buflen : a length of the string buffer this is very important to limit the computation.
			char* checksum : a pointer to the string where the checksum should be computed and copied back.
return values: 
		   char* : a pointer to the checksum string buffer.
*/
char *stn_hft_FIX_message_compute_checksum( char *buf, long bufLen, char* checkSum )
{
    long idx;
    unsigned int cks;
    for( idx = 0L, cks = 0; idx < bufLen; cks += (unsigned int)buf[ idx++ ] );
    sprintf( checkSum, "%03d", (unsigned int)( cks % 256 ) );
    return checkSum;
}





/*
stn_hft_FIX_op_channel_format_msg - a function that formats FIX message for final output by computing the checksum and encoding checksum into 
									the final message.

input parameters:
			char* fix_msg : a pointer to the FIX message buffer.
			int msg_length : a length of the message buffer
			buf_size : actual size of the message buffer - to make sure it does not stomp over other memory.

return values:
		   int : total number of message length.

*/
int stn_hft_FIX_op_channel_format_msg(char* fix_msg,int msg_length,size_t buf_size,char* tmpBuff)
{
    char checksum[10] ;
	char* str = NULL;

    CALC_MSG_LENGTH(msg_length);
    // FIX the 1 less message length issue, but still we need to fix up 
    // computation problem may be.
    msg_length = snprintf(tmpBuff,buf_size,fix_msg,msg_length+1);
    str = stn_hft_FIX_message_compute_checksum(tmpBuff,msg_length,checksum);
    msg_length = snprintf(fix_msg,buf_size,FIX_4_2_CheckSumTemplate,
                                            tmpBuff,            // message to send
                                            checksum,0x01);     // pad checksum
    return msg_length;
    
}



/*
send generic message to the outgoing FIX channel

[in] pax_hft_FIX_op_channel_handle : a handle of the channel, basically it is pointer to the fix-op-channel-private member, it has most of the 
                                                    initialized variable to consume in sending the FIX message

[in] msg_type : a character specifying the message type to send, this will be constructed into the FIX out going message

[in] fix_msg : fix message part to send out . generally FIX header is constructed by the API, so this would be any part remaining of the fix header
                    and trailer

[in] msg_size : size of the message part. this will indicate the size of the message to be used in the FIX out going message
*/
int stn_hft_FIX_op_send_generic_msg(void* pax_hft_FIX_op_channel_handle,unsigned char msg_type, unsigned char* fix_msg, int msg_size, char* time_str)
{
    uint8_t* msg = fix_msg;
    struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)pax_hft_FIX_op_channel_handle;
    char checksum[10],time_str_lcl[20] = "\0";// this shall not be more than 20 bytes
    uint16_t len_msg_header = 0,fix_msg_length = 0,total_fix_msg_len = 0;
    uint16_t buff_len = 0;
    unsigned long rt;


    // check the message length passed to the function so that we dont run over the buffer 
    if(msg_size > FIX_op_hdl_private->FIX_generic_msg_bfr_area_allowed_sz)
        return STN_ERRNO_FAIL;
       
    // Increment sequence number
    __sync_add_and_fetch(&FIX_op_hdl_private->fix_msg_seq_num,1);
    

    if (0 == time_str) 
		{
        FormatCurrTime(time_str_lcl);
        time_str = &time_str_lcl[0];
        }


    // this is length to offset in the buffer to copy the header.
    len_msg_header = FIX_op_hdl_private->FIX_msg_hdr_fixed_length + 
                        get_fix_msg_seq_len(FIX_op_hdl_private->fix_msg_seq_num);

    // this is lenght to encode in to the FIX message
    fix_msg_length = len_msg_header - TAG_8_LEN - TAG_9_LEN + msg_size ;
//                        + 1;  // to adjust the 0x01 that we will encode on the null byte

    // change the message buffer to the reserver aread where we have bytes we can use to format the message
    msg = fix_msg - len_msg_header; // 1 byte extra to accomodate null character, which will be overwritten by 0x01 later on.

    buff_len = sprintf(msg, FIX_4_2_Generic_Msg_Template,
                        0x01,
                        fix_msg_length,0x01,
                        msg_type,0x01,
                        FIX_op_hdl_private->fix_msg_seq_num,0x01,
                        FIX_op_hdl_private->session_constants.tag_49_sender_comp_id,0x01,
                        time_str,0x01,
                        FIX_op_hdl_private->session_constants.tag_56_target_comp_id);

    msg[buff_len]=0x01;  // set this one up to get rid of the null character posted by previous sprintf

    

    total_fix_msg_len = len_msg_header + msg_size;//account for the 0x01 added up

    stn_hft_FIX_message_compute_checksum(msg,total_fix_msg_len,checksum);
	
    total_fix_msg_len +=  sprintf((char*)&fix_msg[msg_size],"10=%s%c", checksum,0x01);     // pad checksum

    WRITE_LOG(FIX_op_hdl_private,time_str,msg,0);
	
	rt = rdtsc();
    printf("\n*Send out: %llu*\n",rt);
	
	if(send(FIX_op_hdl_private->sd, msg, total_fix_msg_len, 0) == -1)
        {
        printf("\nError Sending message out:s\n",msg);
        fflush(stdout);
        return STN_ERRNO_NODATASENT;
        }
    return STN_ERRNO_SUCCESS;
}



//- - - - - - - - - - - - - 
/*
	prototype : stn_hft_FIX_op_channel_login
	Description : FIX login function : it is an API that is used to log into the FIX exchange server. Mainly it 
			  sends FIX logon messages to the remote server.
			  
	input : pax_hft_FIX_op_channel_handle
	output : int : message length it has sent.
*/
int stn_hft_FIX_op_channel_login (void* pax_hft_FIX_op_channel_handle)
{
     uint8_t login_msg [FIX_MSG_SIZE] = {0};
     uint16_t msg_len = 0;


     struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)pax_hft_FIX_op_channel_handle;
     FIX_op_hdl_private->session_constants.tag_90_encrptd_digest_length= strlen(FIX_op_hdl_private->session_constants.tag_91_encrpted_digest);
     
	if(FIX_op_hdl_private->session_constants.tag_90_encrptd_digest_length >0)
		{
     msg_len = snprintf (&login_msg[FIX_MSG_OFFSET],FIX_BUFF_SIZE,
                           FIX_4_2_Login_Template, 
                         0x01,
                         FIX_op_hdl_private->session_constants.tag_90_encrptd_digest_length,0x01,
                         FIX_op_hdl_private->session_constants.tag_91_encrpted_digest,0x01,
                         FIX_op_hdl_private->session_constants.tag_95_raw_data_length,0x01,
                         FIX_op_hdl_private->session_constants.tag_96_raw_data,0x01,
                         0x01,
                         0x01,
                         0x01,
                         FIX_op_hdl_private->session_constants.tag_554_password,0x01
                 );
		}
	else
		{
		msg_len = snprintf (&login_msg[FIX_MSG_OFFSET],FIX_BUFF_SIZE,
							   FIX_4_2_Login_Template_without_Tag90, 
							 0x01,
							 FIX_op_hdl_private->session_constants.tag_95_raw_data_length,0x01,
							 FIX_op_hdl_private->session_constants.tag_96_raw_data,0x01,
							 0x01,
							 0x01,
							 0x01,
							 FIX_op_hdl_private->session_constants.tag_554_password,0x01
					 );
		}

/*    msg_length = stn_hft_FIX_op_channel_format_msg(login_msg,
                                    msg_length,
                                    sizeof(login_msg),
                                    FIX_op_hdl_private->formattedMsg);


    WRITE_LOG(FIX_op_hdl_private,time_str,login_msg,0);
    return send(FIX_op_hdl_private->sd, login_msg, msg_length, 0 );

    */
    return stn_hft_FIX_op_send_generic_msg(pax_hft_FIX_op_channel_handle,
                                            LOGIN_MSG_TYPE,
                                            &login_msg[FIX_MSG_OFFSET],
                                            msg_len, 0);

    
}





//- - - - - - - - - - - - - 
/*
	prototype: stn_hft_FIX_op_channel_send_order_new
	description: FIX API to send new order single from the client side to connected exchange server.
*/
int stn_hft_FIX_op_channel_send_order_new (void* pax_hft_FIX_op_channel_handle, struct FIX_OE_variables_s *p_FIX_op_new_order_crumbs)
{
    int8_t oe_msg [FIX_MSG_SIZE];
    int8_t  custom_msg[100];
  //  uint16_t lcl_var_tag_9_message_length = 0, lcl_var_var_tag10_message_crc =0, msg_length =0;
    uint16_t msg_len = 0; 

    struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)pax_hft_FIX_op_channel_handle;
    char time_str[100];

//    uint64_t ticks_start =0, ticks_to_fmt =0, ticks_to_snd =0, ticks_to_log =0; 

    //ticks_start = rdtsc();
    
    // include terminal info with the strat id
    if(p_FIX_op_new_order_crumbs->tag_9366_strat_id != 0)
        {
        snprintf(custom_msg,100,
                            "9227=%s%c"
                            "9366=%02d%c"
                            "9367=%s%c",
                            &FIX_op_hdl_private->session_constants.mcx_tag_9227_terminal_info[0], 0x01,
                            p_FIX_op_new_order_crumbs->tag_9366_strat_id,0x01,
                            p_FIX_op_new_order_crumbs->tag_9367_sstn,0x01);
        }
    else
        {
        snprintf(custom_msg,100,
                            "9227=%s%c",
                            &FIX_op_hdl_private->session_constants.mcx_tag_9227_terminal_info[0], 0x01);
        }


    FormatCurrTime(time_str);
    if(strlen(p_FIX_op_new_order_crumbs->tag_1_account) == 0 )
        {

        msg_len = snprintf (&oe_msg[FIX_MSG_OFFSET],
                            FIX_BUFF_SIZE,
                            FIX_4_2_Order_Entry_Template,
                         p_FIX_op_new_order_crumbs->tag_21_floor_broker_instr,0x01,
                         p_FIX_op_new_order_crumbs->tag_11_client_order_id, 0x01,
                         p_FIX_op_new_order_crumbs->tag_38_order_qty, 0x01,
                         p_FIX_op_new_order_crumbs->tag_40_order_type, 0x01,
                         p_FIX_op_new_order_crumbs->tag_44_price, 0x01,
                         &p_FIX_op_new_order_crumbs->tag_48_instrument_code[0], 0x01,
                         p_FIX_op_new_order_crumbs->tag_54_side, 0x01,  
                         0x01, // for tag_59 - time in force.. always zero
                         time_str,0x01,  // comes in from user..
                         0x01,// tag 204 , as the account id is zero.
                         custom_msg);
        }
    else
        {
        msg_len = snprintf (&oe_msg[FIX_MSG_OFFSET],
                            FIX_BUFF_SIZE,
                            FIX_4_2_Order_Entry_Template_with_Account,
                            p_FIX_op_new_order_crumbs->tag_1_account,0x01,
                         p_FIX_op_new_order_crumbs->tag_21_floor_broker_instr,0x01,
                         p_FIX_op_new_order_crumbs->tag_11_client_order_id, 0x01,
                         p_FIX_op_new_order_crumbs->tag_38_order_qty, 0x01,
                         p_FIX_op_new_order_crumbs->tag_40_order_type, 0x01,
                         p_FIX_op_new_order_crumbs->tag_44_price, 0x01,
                         &p_FIX_op_new_order_crumbs->tag_48_instrument_code[0], 0x01,
                         p_FIX_op_new_order_crumbs->tag_54_side, 0x01,  
                         0x01, // for tag_59 - time in force.. always zero
                         time_str,0x01,  // comes in from user..
                         0x01, // for tag 204, it shall be one as account id has something
                         custom_msg );
        }


    return stn_hft_FIX_op_send_generic_msg(pax_hft_FIX_op_channel_handle,
                                            ORDER_NEW_MSG_TYPE,
                                            &oe_msg[FIX_MSG_OFFSET],
                                            msg_len, time_str);

        
}

//- - - - - - - - - - - - - 

int stn_hft_FIX_op_channel_send_order_replace (void* pax_hft_FIX_op_channel_handle, struct FIX_OR_variables_s *p_FIX_op_order_replace_crumbs)
{
    uint8_t or_msg [FIX_MSG_SIZE];
    uint16_t msg_len = 0;
    struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)pax_hft_FIX_op_channel_handle;
    char time_str[100];
    char *pszStr = time_str;
    char custom_msg[100];
//    uint64_t time_b = 0, time_e =0 , delta =0, time_i = 0;    
//    int iReturn = 0;


    //time_b = rdtsc();

  
    FormatCurrTime(time_str);

    if(strlen(p_FIX_op_order_replace_crumbs->tag_60_message_creation_time)>0)
        pszStr = p_FIX_op_order_replace_crumbs->tag_60_message_creation_time;

    // include terminal info with the strat id
    if(p_FIX_op_order_replace_crumbs->tag_9366_strat_id != 0)
        {
        snprintf(custom_msg,100,
                            "60=%s%c"
                            "9366=%02d%c"
                            "9367=%s%c",
                            pszStr, 0x01,
                            p_FIX_op_order_replace_crumbs->tag_9366_strat_id,0x01,
                            p_FIX_op_order_replace_crumbs->tag_9367_sstn,0x01);
        }
    else
        {
        snprintf(custom_msg,100,
                            "60=%s%c",
                            pszStr, 0x01);
        }

        

    msg_len = snprintf (&or_msg[FIX_MSG_OFFSET],FIX_BUFF_SIZE,
                                 FIX_4_2_Order_Replace_Template,
                                 p_FIX_op_order_replace_crumbs->tag_11_client_order_id, 0x01,
                                 p_FIX_op_order_replace_crumbs->tag_21_floor_broker_instr, 0x01,
                                 p_FIX_op_order_replace_crumbs->tag_37_order_id,0x01,
                                 p_FIX_op_order_replace_crumbs->tag_38_order_qty,0x01,
                                 p_FIX_op_order_replace_crumbs->tag_40_order_type,0x01,
                                 p_FIX_op_order_replace_crumbs->tag_41_orig_clor_id,0x01,
                                 p_FIX_op_order_replace_crumbs->tag_44_price,0x01,
                                 0x01, // for tag_59 - TIME IN FORCE.. always zero
                                 custom_msg);

    return stn_hft_FIX_op_send_generic_msg(pax_hft_FIX_op_channel_handle,
                                            ORDER_REPLACE_MSG_TYPE,
                                            &or_msg[FIX_MSG_OFFSET],
                                            msg_len, time_str);

}



/*Sending order cancel message*/
int stn_hft_FIX_op_channel_send_order_cancel (void* pax_hft_FIX_op_channel_handle, struct FIX_OC_variables_s *p_FIX_op_order_cancel_crumbs)
{
     uint8_t oc_msg [FIX_MSG_SIZE];
     uint16_t msg_len = 0;
     struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)pax_hft_FIX_op_channel_handle;
     char time_str[100];
     char *pszStr = time_str;
     char custom_msg[100];
     
     FormatCurrTime(time_str);
     
 
     if(strlen(p_FIX_op_order_cancel_crumbs->tag_60_message_creation_time)>0)
         pszStr = p_FIX_op_order_cancel_crumbs->tag_60_message_creation_time;

     // include terminal info with the strat id
     if(p_FIX_op_order_cancel_crumbs->tag_9366_strat_id != 0)
         {
         snprintf(custom_msg,100,
                             "60=%s%c"
                             "9366=%02d%c"
                             "9367=%s%c",
                             pszStr, 0x01,
                             p_FIX_op_order_cancel_crumbs->tag_9366_strat_id,0x01,
                             p_FIX_op_order_cancel_crumbs->tag_9367_sstn,0x01);
         }
     else
         {
         snprintf(custom_msg,100,
                             "60=%s%c",
                             pszStr, 0x01);
         }


     msg_len = snprintf (&oc_msg[FIX_MSG_OFFSET],FIX_BUFF_SIZE,
                         FIX_4_2_Order_Cancel_Template,
                         p_FIX_op_order_cancel_crumbs->tag_11_client_order_id, 0x01,
                         p_FIX_op_order_cancel_crumbs->tag_37_order_id, 0x01,
                         p_FIX_op_order_cancel_crumbs->tag_41_orig_clor_id, 0x01,
                         custom_msg);

    return stn_hft_FIX_op_send_generic_msg(pax_hft_FIX_op_channel_handle,
                                            ORDER_CANCEL_MSG_TYPE,
                                            &oc_msg[FIX_MSG_OFFSET],
                                            msg_len, time_str);

    
}







int stn_hft_FIX_op_channel_send_hb(void* stn_hft_FIX_op_channel_handle)
{
	char hb_msg[FIX_MSG_SIZE];
	int msg_len;

	char time_str[100];
	char *pszStr = time_str;
	
	FormatCurrTime(time_str);

	msg_len = snprintf(&hb_msg[FIX_MSG_OFFSET],FIX_BUFF_SIZE,"112=FIXTEST%u",++g_hb_count);

	
	return stn_hft_FIX_op_send_generic_msg(stn_hft_FIX_op_channel_handle,FIX_HEARTBEAT_MSG_TYPE,&hb_msg[FIX_MSG_OFFSET],msg_len,time_str);

}


