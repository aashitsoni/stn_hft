
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

Author      Date                Comments
------      -----               ---------
mt          02/25/2014          inits rev
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/tcp.h>
#include <semaphore.h>
#include <stdarg.h>
#include <pthread.h>
#include "stn_errno.h"
#include "stn_hft_constants.h"
#include "stn_hft_fix_op_public.h"
#include "stn_hft_fix_op_private.h"
//#include "stn_list_macros.h"
#include "stn_hft_rng_buf_cmn.h"

#include "stn_numa_impl.h"

#include "stn_hft_pair_strategy_private.h"
#include "stn_hft_fix_decode.h"
#include "stn_sdk_version.h"

int __stn_hft_fix_skip_next_value(uint8_t* pkt,
                                        int visLen,
                                        uint8_t* pkt_End)
{
    /*grab the tag value*/
    int offset=0;

    /*loop through the next offset*/
    for(offset = 0; (pkt+offset) < pkt_End && visLen > 0 && *(pkt+offset)!= FIX_TLV_SEPERATOR ; offset++,visLen--);

    offset++;
    return (offset);/*increment by 1 as well as we start from 0 offset so the bytes parsed is always 1 more.*/
}

int __stn_hft_fix_get_next_tag(uint8_t* pkt,
                                    int visLen,
                                    uint8_t* pkt_end,
                                    unsigned int* puTag)
{
    /*grab the tag value*/
    int offset=0;
    unsigned int uTag = 0;
    uint8_t* puTagByte = (uint8_t*)&uTag;
    
    
    if(!puTag || visLen< sizeof(*puTag))
        return FIX_PARSING_ERROR;
        
    
    while (*(pkt+offset) != FIX_TAG_DELIMETER&& offset < visLen)
    {
        if(offset < sizeof(*puTag))
            {
            *puTagByte = *(pkt+offset);
            puTagByte++;
            }
        offset ++;
    }
    offset ++;
    *puTag = uTag;
    return (offset);
}


int __stn_hft_fix_get_next_value_as_char    (uint8_t* pkt,
                                            int visLen,
                                            uint8_t* pkt_end,
                                            uint8_t* puByte)
{
    int offset  = 0;
    
    if(!puByte)
        return 0;

    *puByte = *(pkt+offset++);

    while ( *(pkt+offset) != FIX_TLV_SEPERATOR)
    {
        offset ++;
        if ( pkt_end - pkt - offset <= 0)
            return FIX_PARSING_ERROR;
    }

    offset ++; /*skip the separator*/

    return (offset);
}


int __stn_hft_fix_get_next_value_as_string(uint8_t* pkt,
                                                    int  visLen,
                                                    uint8_t* pkt_end,
                                                    uint8_t* pszValue,
                                                    int* puSzValueCount)
{
    int offset=0;

    if(!pszValue || !puSzValueCount)
        return 0;

    while ( *(pkt+offset) != FIX_TLV_SEPERATOR )
    {
        /*store the value*/
        if(offset < *puSzValueCount -1 )
            *(pszValue + offset) = *(pkt+offset);

        offset ++;

        /*Packet boudary check, if crossed the packet boundary, return zero*/
        if (offset >= visLen)
            return FIX_PARSING_ERROR;
    }

    if(offset < *puSzValueCount)
    {
        *puSzValueCount = offset;
        pszValue[*puSzValueCount]=0;
    }
    else
    {
        pszValue[*puSzValueCount-1]=0;
    }

    offset ++;
    return (offset);
}

int __stn_hft_fix_get_next_value_as_unsigned_int(uint8_t* pkt,
                                                int visLen,
                                                uint8_t * pkt_end,
                                                unsigned int* puValue)
{
    unsigned int    offset=0;
    unsigned int    uValue=0,nValue1,nValue2;
    unsigned char   uByte = 0;

    while ( *(pkt+offset) != FIX_TLV_SEPERATOR )
    {
        uByte   = *(pkt+offset) - ASCII_CHAR;
        nValue1 = nValue2 = uValue;
        nValue1<<=3;
        nValue2<<=1;

        /*uValue    = uValue*FIX_TAG_MULTIPLIER + uByte;*/
        uValue  = nValue1+nValue2 + uByte;

        /*store the value*/
        offset ++;

        /*Packet boudary check, if crossed the packet boundary, return zero*/
        if ( pkt_end - pkt - offset <= 0)
            return FIX_PARSING_ERROR;
    }
    *puValue    = uValue;
    offset ++;
    return (offset);
}




/*
STN HFT Decoding function for the order op core socket : processes the incoming messages from server and then dispatches it to its call back from 
the message processing threads.
*/
int __stn_hft_decode_fix_msg(char* msg, int msg_len, STN_HFT_FIX_DECODED_MSG* fix_decoded_msg)
{

    int i = 0;
    int parsed_byte = 0;
    unsigned int uTag = 0;
    int szValueCount = 0;
   
    
    for (;i<msg_len ;)
        {

            parsed_byte = __stn_hft_fix_get_next_tag((unsigned char*)(msg+i),msg_len-i,(unsigned char*)(msg+msg_len),&uTag);
            if(parsed_byte == FIX_PARSING_ERROR)
                {
                return parsed_byte;
                }
            
            i+=parsed_byte;
            switch(uTag)
                {
                case TAG_FIX_MsgType:
                    parsed_byte =__stn_hft_fix_get_next_value_as_char((unsigned char*)(msg+i),msg_len-i,(unsigned char*)(msg+msg_len),(unsigned char*)(&fix_decoded_msg->msg_type));
                    break;
					
                case TAG_FIX_ClOrdId:
                    szValueCount = sizeof(fix_decoded_msg->ClOrdId);
                    parsed_byte = __stn_hft_fix_get_next_value_as_string((unsigned char*)(msg+i),msg_len-i,(unsigned char*)(msg+msg_len), (unsigned char*)(&fix_decoded_msg->ClOrdId[0]),&szValueCount);
                    break;
					
                case TAG_FIX_OrdId:
                    szValueCount = sizeof(fix_decoded_msg->OrdId);
                    parsed_byte = __stn_hft_fix_get_next_value_as_string((unsigned char*)(msg+i),msg_len-i,(unsigned char*)(msg+msg_len), (unsigned char*)(&fix_decoded_msg->OrdId[0]),&szValueCount);
                    break;
					
                case TAG_FIX_InstrumentCode:                    
                    szValueCount = sizeof(fix_decoded_msg->InstrumentCode);
                    parsed_byte = __stn_hft_fix_get_next_value_as_string((unsigned char*)(msg+i),msg_len-i,(unsigned char*)(msg+msg_len),(unsigned char*)(&fix_decoded_msg->InstrumentCode[0]),&szValueCount);
                    break;

				case TAG_FIX_Price:
                    break;

				case TAG_FIX_OrderQty:
                    parsed_byte = __stn_hft_fix_get_next_value_as_unsigned_int( (unsigned char*)(msg+i),msg_len-i,(unsigned char*)(msg+msg_len),(unsigned int*)(&fix_decoded_msg->OrdQty));
                    break;

				case TAG_FIX_OrdStatus:
                    parsed_byte =__stn_hft_fix_get_next_value_as_char((unsigned char*)(msg+i),msg_len-i,(unsigned char*)(msg+msg_len), (unsigned char*)(&fix_decoded_msg->OrdStatus));
                    break;

				case TAG_FIX_ExecType:
                    parsed_byte =__stn_hft_fix_get_next_value_as_char((unsigned char*)(msg+i),msg_len-i,(unsigned char*)(msg+msg_len), (unsigned char*)(&fix_decoded_msg->ExecType));
					break;
					
				case TAG_FIX_60_MsgCreationTime:
                    
                    szValueCount = sizeof(fix_decoded_msg->tag60CreationTime);
                    parsed_byte = __stn_hft_fix_get_next_value_as_string((unsigned char*)(msg+i),msg_len-i,(unsigned char*)(msg+msg_len), (unsigned char*)(&fix_decoded_msg->tag60CreationTime[0]),&szValueCount);
                    break;
					
                default:
                    parsed_byte = __stn_hft_fix_skip_next_value((unsigned char*)(msg+i), msg_len-i,(unsigned char*)(msg+msg_len));
					break;
					
                }

            if(parsed_byte == FIX_PARSING_ERROR)
                {
                return parsed_byte;
                }

            i+=parsed_byte;
        }

        
    return STN_ERRNO_SUCCESS;
}



// regular fix decode parsing.
void* stn_hft_fix_decode_message_thread(void* hdl)
{
    struct _stn_hft_FIX_op_channel_handle_private_s* fix_private_hdl = (struct _stn_hft_FIX_op_channel_handle_private_s*)hdl;
	STN_HFT_FIX_MSG_DECODE_CALLBACK *pMsgCallback = (STN_HFT_FIX_MSG_DECODE_CALLBACK *)fix_private_hdl->fix_decode_msg_callback;

    int err;
	unsigned int msg_len;
    unsigned char* msg = 0;
	
    int iReturn = 0;

	STN_HFT_FIX_DECODED_MSG fix_decoded_msg;

    // check if FIX recv channel is terminated, then we have nothing to do up here.
    while(fix_private_hdl->quit == 0)
        {
        err = stn_hft_FIX_op_channel_get_next_msg(fix_private_hdl,&msg,&msg_len);
        if(STN_ERRNO_SUCCESS == err)
            {
            
            memset(&fix_decoded_msg,0,sizeof(fix_decoded_msg));

            __stn_hft_decode_fix_msg((char*)msg,msg_len,&fix_decoded_msg);
			  
			if(pMsgCallback)
			  	iReturn = (*pMsgCallback)(&fix_private_hdl->FIX_op_channel_public_attribs,&fix_decoded_msg,(char*)msg,msg_len);

			if(iReturn != STN_ERRNO_SUCCESS)
			  	{
			  	// log this stage and break out.
			  	break;
			  	}
            }
        else if(STN_ERRNO_FAIL == err || STN_ERRNO_MALLOC_FAIL == err)
        	{
        	// log this state and break out
            break;
        	}
        
        }
	return 0;
}



// initiate a fix message processing threads.

int stn_hft_FIX_initiate_message_decode(void * pax_hft_FIX_op_channel_handle,STN_HFT_FIX_MSG_DECODE_CALLBACK pMsgCallBack)
{
    struct _stn_hft_FIX_op_channel_handle_private_s* fix_private_hdl = (struct _stn_hft_FIX_op_channel_handle_private_s*)pax_hft_FIX_op_channel_handle;

	if(fix_private_hdl->fix_hft_decode_thread_hdl == 0)
		{
		fix_private_hdl->fix_decode_msg_callback = pMsgCallBack;
		pthread_create(&fix_private_hdl->fix_hft_decode_thread_hdl,NULL,stn_hft_fix_decode_message_thread,fix_private_hdl);
		}

	return STN_ERRNO_SUCCESS;
}




int stn_hft_FIX_terminate_message_decode(void* pax_hft_FIX_op_channel_handle)
{
    struct _stn_hft_FIX_op_channel_handle_private_s* fix_private_hdl = (struct _stn_hft_FIX_op_channel_handle_private_s*)pax_hft_FIX_op_channel_handle;

	if(fix_private_hdl->fix_hft_decode_thread_hdl)
		pthread_cancel(fix_private_hdl->fix_hft_decode_thread_hdl);
	
	fix_private_hdl->fix_hft_decode_thread_hdl = 0;
	return STN_ERRNO_SUCCESS;

}



