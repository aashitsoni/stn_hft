
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
#include "console_log.h"



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



int hft_client_decode_fix_message(void* hdl, char* msg, int msg_len)
{

}
