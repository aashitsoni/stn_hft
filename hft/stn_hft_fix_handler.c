
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
#include "stn_list_macros.h"
#include "stn_hft_rng_buf_cmn.h"

#include "stn_numa_impl.h"

#include "stn_hft_pair_strategy_private.h"
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
__stn_hft_pair_decode_fix_msg : FIX protocol decoding function.
*/
int __stn_hft_pair_decode_fix_msg(uint8_t* msg,
                                        int msg_len, 
                                        struct stn_hft_pair_decode_fix_msg_s* fix_msg)
{
    int i = 0;
    int parsed_byte = 0;
    unsigned int uTag = 0;
    unsigned int szValueCount = 0;
    unsigned char supported_tag = FIX_HFT_PAIR_MAX_SUPPORTED_FIELD;

    WRITE_PAIR_LOG("Entering __stn_hft_pair_decode_fix_msg");
        
    
    for (;i<msg_len && supported_tag > 0;)
        {

            parsed_byte = __stn_hft_fix_get_next_tag(msg+i,msg_len-i,msg+msg_len,&uTag);
            if(parsed_byte == FIX_PARSING_ERROR)
                {
                WRITE_PAIR_LOG("Leaving __stn_hft_pair_decode_fix_msg FIX_PARSING_ERROR@tag");
                return parsed_byte;
                }
            
            i+=parsed_byte;
            switch(uTag)
                {
                case TAG_FIX_MsgType:
                    {
                    parsed_byte =__stn_hft_fix_get_next_value_as_char(msg+i,msg_len-i,msg+msg_len,&fix_msg->MsgType);
                    if(parsed_byte == FIX_PARSING_ERROR)
                        {
                        return FIX_PARSING_ERROR;
                        }

                    if(fix_msg->MsgType != FIX_MsgType_ExecutionReport)
                        {
                        return STN_ERRNO_SUCCESS;
                        }
                    supported_tag--;
                    }
                    break;
                case TAG_FIX_ClOrdId:
                    szValueCount = sizeof(fix_msg->ClOrdId);
                    parsed_byte = __stn_hft_fix_get_next_value_as_string(msg+i,
                        msg_len-i,msg+msg_len, &fix_msg->ClOrdId[0],&szValueCount);
                    supported_tag--;
                    break;
                case TAG_FIX_OrdId:
                    szValueCount = sizeof(fix_msg->OrdId);
                    parsed_byte = __stn_hft_fix_get_next_value_as_string(msg+i,msg_len-i,msg+msg_len,
                                                                    &fix_msg->OrdId[0],&szValueCount);
                    supported_tag--;
                    break;
                case TAG_FIX_InstrumentCode:                    
                    szValueCount = sizeof(fix_msg->InstrumentCode);
                    parsed_byte = __stn_hft_fix_get_next_value_as_string(msg+i,msg_len-i,msg+msg_len,
                                                                    &fix_msg->InstrumentCode[0],&szValueCount);
                    supported_tag--;
                    break;
                case TAG_FIX_Price:
                    supported_tag--;
                    break;
                case TAG_FIX_OrderQty:
                    parsed_byte = __stn_hft_fix_get_next_value_as_unsigned_int( msg+i,msg_len-i,msg+msg_len,
                        &fix_msg->Ordqty);
                    supported_tag--;
                    break;
                case TAG_FIX_OrdStatus:
                    parsed_byte =__stn_hft_fix_get_next_value_as_char(msg+i,msg_len-i,msg+msg_len,
                                                                    &fix_msg->OrdStatus);
                    if(parsed_byte == FIX_PARSING_ERROR)
                        {
                        return FIX_PARSING_ERROR;
                        }
                    supported_tag--;
                    break;
                case TAG_FIX_ExecType:
                    parsed_byte =__stn_hft_fix_get_next_value_as_char(msg+i,msg_len-i,msg+msg_len,
                                                                    &fix_msg->ExecType);
                    if(parsed_byte == FIX_PARSING_ERROR)
                        {
                        return FIX_PARSING_ERROR;
                        }
                    supported_tag--;
                    break;
                case TAG_FIX_60_MsgCreationTime:
                    
                    szValueCount = sizeof(fix_msg->tag60);
                    parsed_byte = __stn_hft_fix_get_next_value_as_string(msg+i,msg_len-i,msg+msg_len,
                                                                    &fix_msg->tag60[0],&szValueCount);
                    supported_tag--;
                    break;
                default:
                    parsed_byte = __stn_hft_fix_skip_next_value(msg+i, msg_len-i,msg+msg_len);
                }
            if(parsed_byte == FIX_PARSING_ERROR)
                {
                WRITE_PAIR_LOG("Leaving __stn_hft_pair_decode_fix_msg @after check");
                return parsed_byte;
                }

            i+=parsed_byte;
        }

    WRITE_PAIR_LOG("Leaving __stn_hft_pair_decode_fix_msg :%s",fix_msg->ClOrdId);
        
    return STN_ERRNO_SUCCESS;
}



