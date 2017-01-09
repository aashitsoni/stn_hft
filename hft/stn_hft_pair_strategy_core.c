
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014


Author      Date                Comments
------      -----               ---------
mt      10/01/2012          inits rev
mt      11/27/2013          revisions for the pack implementation
mt      01/09/2014          changes accordingly to i-rage strategy discussion
mt      03/12/2014          fixing the tag 37 caching issue with mess posting on the strategy.
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
#include "stn_hft_rng_buf_cmn.h"

#include "stn_numa_impl.h"

#include "stn_hft_pair_strategy_private.h"
#include "stn_sdk_version.h"
#include "console_log.h"



struct stn_hft_pair_strategy_global_s _pair_globals;


#define QUIT_RUN 0
#define QUIT_COMPLETE 1
#define QUIT_KILL 2



int  WRITE_PAIR_LOG(char* log_string,...)
{
    
if(_pair_globals.fp_pair_log && _pair_globals.log_status_flag)
    {
    va_list ap;
    time_t curr_time;
    struct tm* gm_time;// it would be a local time
    char format_str[300],szTime[100];
    char str[256];
    
    va_start(ap,log_string);
    time(&curr_time);
    gm_time = localtime(&curr_time);
    sprintf(szTime,"%d%02d%02d-%02d:%02d:%02d",gm_time->tm_year+1900,
                        gm_time->tm_mon+1,
                        gm_time->tm_mday,
                        gm_time->tm_hour,
                        gm_time->tm_min,
                        gm_time->tm_sec);
    vsnprintf(str,256,log_string,ap);
    sprintf(format_str,"\n%s: %s",szTime,str);
    pthread_spin_lock(&_pair_globals.pair_file_lock);   
    fputs(format_str,_pair_globals.fp_pair_log);
    pthread_spin_unlock(&_pair_globals.pair_file_lock);
    }
	return STN_ERRNO_SUCCESS;
}

time_t* __stn_pair_get_time(time_t* curr_time)
{
    time(curr_time);
	return curr_time;

}

void __stn_pair_get_tag60_creation_time(char* psz_tag60_creation_time)
{

    struct tm* ptm;
    time_t pt;
    __stn_pair_get_time(&pt);
    ptm = gmtime(&pt);


    sprintf((char*)psz_tag60_creation_time,"%4u%02u%02u-%02u:%02u:%02u",
                                     ptm->tm_year+1900,
                                     ptm->tm_mon+1,
                                     ptm->tm_mday,
                                     ptm->tm_hour,
                                     ptm->tm_min,
                                     ptm->tm_sec);
    return;

}

__inline__ uint64_t rdtsc_pair(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ (
    "        xorl %%eax,%%eax \n"
    "        cpuid"      // serialize
    ::: "%rax", "%rbx", "%rcx", "%rdx");
    /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t)hi << 32 | lo;
}


/**/
int __stn_pair_get_next_sstn_count()
{
    __sync_add_and_fetch(&_pair_globals._pair_sstn_count,1);
    return _pair_globals._pair_sstn_count;
}

int __stn_pair_get_next_clordid(uint8_t* clorid)
{
    time_t curr_time;
    struct tm* ptm;
//    struct _stn_hft_FIX_op_channel_handle_private_s* pvt = (struct _stn_hft_FIX_op_channel_handle_private_s*)_pair_globals.__fix_op_channel;
    __stn_pair_get_time(&curr_time);
    ptm = localtime(&curr_time);

    // Increment sequence number
    __sync_add_and_fetch(&_pair_globals._pair_sequence_count,1);
    

    sprintf((char*)clorid,"%s%u%u%u%u%u",_pair_globals.__clordid,
                                    _pair_globals._pair_sequence_count,
                                     ptm->tm_mday,
                                     ptm->tm_hour,
                                     ptm->tm_min,
                                     ptm->tm_sec);
//  sprintf(clorid,"HFT_TEST");
    return STN_ERRNO_SUCCESS;

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
    int szValueCount = 0;
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




void* __stn_hft_pair_strategy_thr_run (void* hdl);


int stn_hft_init_strat_engines()
{

/*  _pair_globals.active_orders_array = calloc(1, sizeof(struct stn_hft_pair_active_orders_s) * MAX_ACTIVE_ORDERS);

    pax_fmem_create_fast_alloc_block_list (&_pair_globals.pax_hft_active_orders_array_hdr,  
    _pair_globals.active_orders_array,
    sizeof(struct pax_hft_active_orders_s),     
    MAX_ACTIVE_ORDERS   
    );
*/
return STN_ERRNO_SUCCESS;
}

int stn_hft_create_pair_strategy(/*[IN]*/stn_hft_pair_strategy_attrib* new_pair, 
                                    /*[IN]*/void*   fix_chnl_handle,
                                    /*[IN|OUT]*/void** pair_handle)
{

    int i = 0;
    int yy = 0;

    time_t currtime;
    struct tm* gmtime;// it would be a local time
    time(&currtime);
    gmtime = localtime(&currtime);

    struct stn_hft_pair_strategy_attrib_priavte_s* pair_strategy_private=NULL;
    
    WRITE_PAIR_LOG("Entering stn_hft_create_pair_strategy");
    
    if(new_pair == NULL || pair_handle == NULL)
        {
        WRITE_PAIR_LOG("Leaving stn_hft_create_pair_strategy STN_ERRNO_PARAM_PROBLEM");
        return STN_ERRNO_PARAM_PROBLEM;
        }


    if(_pair_globals._number_of_strategy_created >= MAX_PAIR_STRATEGY)
        {
        WRITE_PAIR_LOG("Leaving stn_hft_create_pair_strategy STN_ERRNO_FAIL");
        return STN_ERRNO_FAIL;
        }

    i = _pair_globals._number_of_strategy_created;

    pair_strategy_private = &_pair_globals.__stn_pair_stategy_array[i];
    

    memcpy(&pair_strategy_private->pair_public,new_pair,sizeof(stn_hft_pair_strategy_attrib));

    pair_strategy_private->quit = QUIT_RUN; // let it run
    pair_strategy_private->enable = 1; // it is enabled

    // initialize some of the variable which are used in execution for the strat thread
    pair_strategy_private->total_filled_qty_A = 0;
    pair_strategy_private->total_filled_qty_B = 0;
    pair_strategy_private->pending_qty_A = new_pair->net_quantity;
    pair_strategy_private->pending_qty_B = (new_pair->net_quantity*new_pair->qty_A)/new_pair->qty_B;

    if(pair_strategy_private->pair_public.direction == BUY_A_SELL_B)
        {
        pair_strategy_private->side_A = BUY_SIDE;
        pair_strategy_private->side_B = SELL_SIDE;
        }
    else
        {
        pair_strategy_private->side_A = SELL_SIDE;
        pair_strategy_private->side_B = BUY_SIDE;
        }

    if(new_pair->strat_id)
        {
        pair_strategy_private->strat_counter = 1;
        yy = gmtime->tm_year-100;
        snprintf(pair_strategy_private->strat_sstn,30,"%02d%02d%02d%02d",
            yy,
            gmtime->tm_mon+1,
            gmtime->tm_mday,
            new_pair->strat_id);
         }

    _pair_globals._number_of_strategy_created ++;

    //put back the handle of the pair
    *pair_handle = &pair_strategy_private->pair_public;
    
    
    // create stategy thread
    pthread_create(&(pair_strategy_private->pair_run_thrd_handle),
                    NULL,
                    __stn_hft_pair_strategy_thr_run,
                    (void*)pair_strategy_private);
    

    WRITE_PAIR_LOG("Leaving stn_hft_create_pair_strategy STN_ERRNO_SUCCESS");
    return STN_ERRNO_SUCCESS;
    
    
}

/*Search active order by the clordid to process and handle the message to the respective pair handle*/
struct stn_hft_pair_strategy_attrib_priavte_s* stn_hft_search_active_order_array_by_clorid(struct stn_hft_pair_decode_fix_msg_s *msg)
{
    int idx =0;
    WRITE_PAIR_LOG("Entering stn_hft_search_active_order_array_by_clorid");
    struct stn_hft_pair_strategy_attrib_priavte_s* pair=
                        (struct stn_hft_pair_strategy_attrib_priavte_s *)&_pair_globals.__stn_pair_stategy_array[0];
    
    for (idx =0; idx < MAX_PAIR_STRATEGY; idx++ )
    {
        if ((0 == strcmp((char*)pair->active_order[ORDER_A].ClOrdId, (char*)msg->ClOrdId)))
            {
            WRITE_PAIR_LOG("Leaving stn_hft_search_active_order_array_by_clorid found match A:%c,%s,%s,",
                        msg->OrdStatus,
                        msg->ClOrdId,
                        msg->OrdId);
            return pair;
            }
        else if (0 == strcmp((char*)pair->active_order[ORDER_B].ClOrdId, (char*)msg->ClOrdId))
            {
            WRITE_PAIR_LOG("Leaving stn_hft_search_active_order_array_by_clorid found match B:%c,%s,%s",
                        msg->OrdStatus,
                        msg->ClOrdId,
                        msg->OrdId);
                
            return pair;
            }
        pair++;
    }
    WRITE_PAIR_LOG("Leaving stn_hft_search_active_order_array_by_clorid, nothing found :%s",msg->ClOrdId);
    return STN_ERRNO_SUCCESS;
}

struct stn_hft_pair_strategy_attrib_priavte_s* stn_hft_search_active_order_array_by_strat_handle(struct stn_hft_pair_strategy_attrib_priavte_s *pair_hdl)
{
    int idx =0;
    struct stn_hft_pair_strategy_attrib_priavte_s *pair = (struct stn_hft_pair_strategy_attrib_priavte_s *)&_pair_globals.__stn_pair_stategy_array[0];
    for (idx =0; idx < MAX_PAIR_STRATEGY; idx++ )
    {
        if (pair == pair_hdl)
            return pair;
        pair++;
    }
    return STN_ERRNO_SUCCESS;
}

/*
NOTE: It seems like we can only handle conditions with a strategy having only 1 order outstanding per strategy handle. 
      There can be multiple strategies of the exact same type running simultaneously. i.e. multiple handles 
      This means that MAX_ACTIVE_ORDERS cannot be >'er than MAX_PAIR_STRATEGY

Situations to consider:
1. what happens if a Modify order with just PRICE change comes in, at the same time a ER from exchange arrives that completes the order
    > what is the quantity that we should send the price MODIFY? will the exchange reject this MODIFY since the qty does not match with 
      the leaves qty in the exchange books
2. Same as above but Modify order with just QTY change comes in from user
3. Same as above but Modify order with just a,b RELATION change comes in from user
*/

int stn_hft_update_price_of_B    (/*[IN*/double price_B,
                                        /*[IN]*/void* pair_handle)
{

    unsigned long long rt;

    /*
    First search in the global _active_orders [...]to see if the pair_handle.thread_handle is present.
        IF NOT FOUND 
            > Get a free entry from _active_orders [...]
                    struct pax_hft_active_orders_s *new_order = pax_fmem_fast_alloc(&_pair_globals.pax_hft_active_orders_array_hdr);
            > Compute A= Bx+y etc and set all relevant fields
            > For a new make sure to OrigClorId same as ClorID
            > Set flags
        ELSE  
        > Update pair_handle->x,y  and set price change flag..
   Worker thread when it spins around.. will check flags/ values etc . If changed, it will either send a NEW ORDER or a MODIFY ORDER
    */

    struct stn_hft_pair_strategy_attrib_priavte_s* pair = 
                        (struct stn_hft_pair_strategy_attrib_priavte_s* )pair_handle;

    /*make sure we really have change in the value. If not, pelase ignore this API upate*/
    WRITE_PAIR_LOG("Entering hft_update_price_of_B");
    // TODO: dump the timestamp

    rt = rdtsc_pair();
    console_log_write("%s:%d stn_hft_update_price_of_B:(%s,%s)- %llu*\n",
														__FILE__,__LINE__,
														pair->pair_public.InstrumentCode_A,
                                                        pair->pair_public.InstrumentCode_B,
                                                        rt);
                                                        
    fflush(stdout);
                                                      

    if(pair->pair_public.Price_B != price_B)
        {
        pair->pair_public.Price_B = price_B;
        pair->price_update_from_market_flag = TRUE;
        WRITE_PAIR_LOG("Recieved update on Price B:%u",price_B);
        }

    WRITE_PAIR_LOG("Leaving hft_update_price_of_B");
    return STN_ERRNO_SUCCESS;
}



/*updating price strategy*/
int stn_hft_update_pair_strategy (/*[IN]*/stn_hft_pair_strategy_attrib* update_pair,
                                     /*[IN]*/void* pair_handle,
                                     /*[IN]*/int update_flag)

{
    struct stn_hft_pair_strategy_attrib_priavte_s* pair = 
        (struct stn_hft_pair_strategy_attrib_priavte_s*)pair_handle;

    WRITE_PAIR_LOG("Entering stn_hft_update_pair_strategy");

    if(update_flag & PRICE_RELATION_CHANGE && 
        ((pair->pair_public.x != update_pair->x) || (pair->pair_public.y != update_pair->y)))
        {
        pair->pair_public.x = update_pair->x;
        pair->pair_public.y = update_pair->y;
        pair->price_reln_update_flag = TRUE;
        WRITE_PAIR_LOG("Recieved update on X:%u,Y:%u",update_pair->x,update_pair->y);
        
        }
    else if(update_flag & NET_QTY_CHANGE &&
        (pair->pair_public.net_quantity!= update_pair->net_quantity))
        {
        pair->pair_public.net_quantity = update_pair->net_quantity;
        pair->net_qty_update_flag = TRUE;
        WRITE_PAIR_LOG("Recieved update on net qty:%u",update_pair->net_quantity);
        }
    else if(update_flag & ORD_SIZE_CHANGE &&
        (pair->pair_public.order_size != update_pair->order_size))
        {
        pair->pair_public.order_size = update_pair->order_size;
        pair->ord_size_update_flag = TRUE;
        WRITE_PAIR_LOG("Recieved update on order size:%u",update_pair->order_size);
        }

    WRITE_PAIR_LOG("Leaving stn_hft_update_pair_strategy");

    return STN_ERRNO_HFT_PAIR_UPDATE_FAILED;
    
        
}


int stn_hft_resume_pair_execution    (void* pair_handle)
{
    struct stn_hft_pair_strategy_attrib_priavte_s* pair = 
        (struct stn_hft_pair_strategy_attrib_priavte_s*)pair_handle;

    WRITE_PAIR_LOG("Entering stn_hft_resume_pair_execution");

    if(pair->enable == 0)
        {
        pair->enable = 1;
        WRITE_PAIR_LOG("Resume execution of thread");
        }
    
    WRITE_PAIR_LOG("Leaving stn_hft_resume_pair_execution");
    return STN_ERRNO_SUCCESS;
}


int stn_hft_pause_pair_execution (void* pair_handle)
{
    struct stn_hft_pair_strategy_attrib_priavte_s* pair = 
        (struct stn_hft_pair_strategy_attrib_priavte_s*)pair_handle;

    WRITE_PAIR_LOG("Entering stn_hft_pause_pair_execution");

    if(pair->enable == 1)
        {
        pair->enable = 0;
        WRITE_PAIR_LOG("Pause Execution");
        }
    
    WRITE_PAIR_LOG("Leaving stn_hft_pause_pair_execution");
    return STN_ERRNO_SUCCESS;
}

int stn_hft_delete_pair_strategy (void* pair_handle)
{
    // remove the entry from the global _active_orders ARRAY as well
    //      > fast_free the active_order_entry

    // remove entry from global pair_strategy_array


    struct stn_hft_pair_strategy_attrib_priavte_s* pair = 
            (struct stn_hft_pair_strategy_attrib_priavte_s*) pair_handle;

    WRITE_PAIR_LOG("Entering stn_hft_delete_pair_strategy");

    pair->quit = QUIT_COMPLETE;
    
    WRITE_PAIR_LOG("Leaving stn_hft_delete_pair_strategy");
    return STN_ERRNO_SUCCESS;
    
}


/*
// find out clordid and respective thread id. and update the specific thread.

*/
int __stn_hft_pair_post_fix_msg_to_serving_pair_thrd(struct stn_hft_pair_decode_fix_msg_s* fix_msg)
{

    /*
        FIRST stn_hft_search_active_order_array_by_clorid
        IF FOUND 
            > UPDATE various fields and set flags
    */
    struct stn_hft_pair_strategy_attrib_priavte_s *pair = NULL;
    
    WRITE_PAIR_LOG("Entering __stn_hft_pair_post_fix_msg_to_serving_pair_thrd:%s,%s",fix_msg->ClOrdId,fix_msg->OrdId);
    pair = stn_hft_search_active_order_array_by_clorid(fix_msg);
    if(pair)
        {
        // hand off the message parsing to him.
        memcpy(&pair->decoded_msg,fix_msg,sizeof(pair->decoded_msg));
        pair->posting_thr_updates_count++;
        WRITE_PAIR_LOG("Leaving__pax_hft_pair_post_fix_msg_to_serving_pair_thrd, %s",pair->decoded_msg.OrdId);
        }
    else
        {
        WRITE_PAIR_LOG("Leaving __stn_hft_pair_post_fix_msg_to_serving_pair_thrd, Not found");
        return STN_ERRNO_HFT_PAIR_ORDER_NOT_FOUND;
        }
    
    
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_post_fix_msg_to_serving_pair_thrd");
    return STN_ERRNO_SUCCESS;
}



int __stn_hft_pair_post_kill_msg_to_serving_pair_thrd(struct stn_hft_pair_decode_fix_msg_s* fix_msg)
{
   struct stn_hft_pair_strategy_attrib_priavte_s *pair = NULL;
    
    WRITE_PAIR_LOG("Entering __stn_hft_pair_post_kill_msg_to_serving_pair_thrd");
    pair = stn_hft_search_active_order_array_by_clorid(fix_msg);
    if(pair)
        {
        pair->quit = QUIT_KILL;
        WRITE_PAIR_LOG("__stn_hft_pair_post_kill_msg_to_serving_pair_thrd, found ...");
        }
    else
        {
        WRITE_PAIR_LOG("__stn_hft_pair_post_kill_msg_to_serving_pair_thrd, Not found");
        return STN_ERRNO_HFT_PAIR_ORDER_NOT_FOUND;
        }
    
    
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_post_kill_msg_to_serving_pair_thrd");
    return STN_ERRNO_SUCCESS;}



/* write all the FIX message processing logic here.*/
int process_fix_msg(struct stn_hft_pair_decode_fix_msg_s* fix_msg)
{
   if(fix_msg->MsgType == FIX_MSGTYPE_EXEC_REPORT)
        return 1;
   return 0;
}


/*
master pair thread which does decoding of the fix packets
*/
void* __stn_hft_pair_strategy_master_thread_run(void* hdl)
{

    struct _stn_hft_FIX_op_channel_handle_private_s* fix_hdl = (struct _stn_hft_FIX_op_channel_handle_private_s*)hdl;



    struct _stn_hft_FIX_op_channel_handle_clone_s* FIX_Chnl_Clone_hdl = NULL;
    int err;
	unsigned int msg_len;
    unsigned char* msg = 0;
    struct stn_hft_pair_decode_fix_msg_s fix_decoded_msg;

    _pair_globals.__fix_op_channel = &fix_hdl->FIX_op_channel_public_attribs;
    strcpy(_pair_globals.__clordid,"P");
    
    FIX_Chnl_Clone_hdl = __stn_fix_create_clone_private_handle(hdl);
    if(FIX_Chnl_Clone_hdl == NULL)
        return NULL;

    // create a log file

    _pair_globals.log_status_flag = fix_hdl->FIX_op_channel_public_attribs.log_stats_flag;
    
    if(_pair_globals.log_status_flag == 1)
        {
        time_t curr_time;
        struct tm* local_time;
        
        time(&curr_time);
        local_time  = localtime(&curr_time);
        sprintf(_pair_globals._pairlog_file,PAIR_LOG_FILE_NAME,local_time->tm_mon+1,local_time->tm_mday,local_time->tm_year-100);
        
        _pair_globals.fp_pair_log = fopen(_pair_globals._pairlog_file,"w+");
        if(NULL == _pair_globals.fp_pair_log)
            {
            console_log_write("%s:%d Error opening pair log file\n",__FILE__,__LINE__);
            fflush(stdout);
            _pair_globals.fp_pair_log = 0;
            }
        pthread_spin_init(&_pair_globals.pair_file_lock, PTHREAD_PROCESS_PRIVATE);
        }

    WRITE_PAIR_LOG("Starting __stn_hft_pair_strategy_master_thread_run");

    
    if(0 == msg)
        WRITE_PAIR_LOG("__stn_hft_pair_strategy_master_thread_run, msg allocation failed");
    
    // check if FIX recv channel is terminated, then we have nothing to do up here.
    
    while(fix_hdl->quit == 0)
        {
        err = __stn_hft_FIX_clone_chnl_get_next_msg(FIX_Chnl_Clone_hdl,&msg,&msg_len);
        if(STN_ERRNO_SUCCESS == err)
            {

            if(_pair_globals._number_of_strategy_created > 0)
                {
                WRITE_PAIR_LOG("Recieved Message : %s",msg);
                console_log_write("%s:%d Received FIX Message : %s\n",__FILE__,__LINE__,msg);
                
                // push it to decoding FIX message and inform the various PAIR threads to pick it up
                __stn_hft_pair_decode_fix_msg(msg,msg_len,&fix_decoded_msg);
                if(1 == process_fix_msg(&fix_decoded_msg))
                    __stn_hft_pair_post_fix_msg_to_serving_pair_thrd(&fix_decoded_msg);

                }
            }
        else if(STN_ERRNO_FAIL == err)
            break;
        
        }

    WRITE_PAIR_LOG("Coming out of  __stn_hft_pair_strategy_master_thread_run");
    if(_pair_globals.log_status_flag == 1)
        fclose(_pair_globals.fp_pair_log);

    __stn_fix_destroy_clone_private_handle(FIX_Chnl_Clone_hdl);
	return NULL;
    
}


int __stn_hft_pair_send_new_order(struct stn_hft_pair_strategy_attrib_priavte_s * pair,
                                            struct FIX_OE_variables_s* OE_crumbs)
{
    int iRet = 0;
//    unsigned long long rt = 0;
    // FIX send new order message

    WRITE_PAIR_LOG("Entering __stn_hft_pair_send_new_order");
    OE_crumbs->tag_21_floor_broker_instr = FLOOR_BROKER_INSTR;
    OE_crumbs->tag_40_order_type = LIMIT_ORDER;
    // TODO: performance minimization 
    strcpy((char*)OE_crumbs->tag_1_account, (char*)pair->pair_public.tag_1_account_name);
    strcpy((char*)OE_crumbs->tag_200_mat_year_mth, (char*)pair->pair_public.tag_200_mat_month);
//    strcpy(OE_crumbs->tag_48_instrument_code,OE_crumbs->tag_48_instrument_code);
    strcpy((char*)OE_crumbs->tag_167_instrument_type, (char*)pair->pair_public.tag_167_instrument_type);
    __stn_pair_get_next_clordid(&OE_crumbs->tag_11_client_order_id[0]);
    __stn_pair_get_tag60_creation_time((char*)OE_crumbs->tag_60_message_creation_time);
    
    

    OE_crumbs->tag_9366_strat_id = pair->pair_public.strat_id;
    pair->strat_counter++;
    snprintf((char*)OE_crumbs->tag_9367_sstn,20,"%s%09d",pair->strat_sstn,__stn_pair_get_next_sstn_count());
    

    iRet =  stn_hft_FIX_op_channel_send_order_new(pair->fix_op_chnl,OE_crumbs);

#if 0
    rt = rdtsc_pair();
    console_log_write("\n*New Order:(%s,%s)- %llu*\n",pair->pair_public.InstrumentCode_A,
                                                        pair->pair_public.InstrumentCode_B,
                                                        rt);
    fflush(stdout);
#endif
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_send_new_order:%s",OE_crumbs->tag_11_client_order_id);
    return iRet;

}
int __stn_hft_pair_send_order_cancel(struct stn_hft_pair_strategy_attrib_priavte_s * pair,
                                        struct FIX_OC_variables_s* OC_crumbs)
{
    int iRet = 0;
//    unsigned long long rt = 0;

    WRITE_PAIR_LOG("Entering __stn_hft_pair_send_order_cancel");
    strcpy((char*)OC_crumbs->tag_167_instrument_type, (char*)pair->pair_public.tag_167_instrument_type);
    strcpy((char*)OC_crumbs->tag_200_mat_year_mth , (char*)pair->pair_public.tag_200_mat_month);
    OC_crumbs->tag_205_mat_day = MAT_DAY;
    OC_crumbs->tag_21_floor_broker_instr = FLOOR_BROKER_INSTR;
//    strcpy(OC_crumbs->tag_48_instrument_code, OC_crumbs->tag_48_instrument_code);
    __stn_pair_get_next_clordid(&OC_crumbs->tag_11_client_order_id[0]);

    OC_crumbs->tag_9366_strat_id = pair->pair_public.strat_id;
    pair->strat_counter++;
    snprintf((char*)OC_crumbs->tag_9367_sstn,20,"%s%09d",pair->strat_sstn,__stn_pair_get_next_sstn_count());
    
    iRet = stn_hft_FIX_op_channel_send_order_cancel(pair->fix_op_chnl,OC_crumbs);
#if 0
    rt = rdtsc_pair();
    console_log_write("%s:%d Order Cancel:(%s,%s)- %llu*\n",__FILE__,__LINE__,
														pair->pair_public.InstrumentCode_A,
                                                        pair->pair_public.InstrumentCode_B,
                                                        rt);
    fflush(stdout);
#endif
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_send_order_cancel:%s",OC_crumbs->tag_11_client_order_id);
    return iRet;
}


int __stn_hft_pair_send_order_replace(struct stn_hft_pair_strategy_attrib_priavte_s * pair,
                                        struct FIX_OR_variables_s* OR_crumbs)
{
    int iRet = 0;
//    unsigned long long rt = 0;
    // send and order replace 
    WRITE_PAIR_LOG("Entering __stn_hft_pair_send_order_replace");
    strcpy((char*)OR_crumbs->tag_167_instrument_type , pair->pair_public.tag_167_instrument_type);
    strcpy((char*)OR_crumbs->tag_200_mat_year_mth , pair->pair_public.tag_200_mat_month);
    OR_crumbs->tag_205_mat_day = MAT_DAY;
    OR_crumbs->tag_21_floor_broker_instr = FLOOR_BROKER_INSTR;
    strcpy((char*)OR_crumbs->tag_22_alternative_SecurityID , ALTERNATE_SEC_ID);
    OR_crumbs->tag_40_order_type = LIMIT_ORDER;

    OR_crumbs->tag_9366_strat_id = pair->pair_public.strat_id;
    pair->strat_counter++;
    snprintf((char*)OR_crumbs->tag_9367_sstn,20,"%s%09d",pair->strat_sstn,__stn_pair_get_next_sstn_count());

 //   strcpy(OR_crumbs->tag_48_instrument_code, OR_crumbs->tag_48_instrument_code);
    __stn_pair_get_next_clordid(&OR_crumbs->tag_11_client_order_id[0]);
    iRet =  stn_hft_FIX_op_channel_send_order_replace(pair->fix_op_chnl,OR_crumbs);
#if 0
    rt = rdtsc_pair();
    console_log_write("%s:%d Order Replace:(%s,%s)- %llu*\n",__FILE__,__LINE__,pair->pair_public.InstrumentCode_A,
                                            pair->pair_public.InstrumentCode_B,
                                            rt);
    fflush(stdout);
#endif    
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_send_order_replace:%s ",OR_crumbs->tag_11_client_order_id);
	return iRet;
}



/* 
int __stn_hft_pair_execute_price_change(struct stn_hft_pair_strategy_attrib_priavte_s * pair)

handles case no 2 and 5 

     2) Pair Execution Strategy

     2.1) pax recieves  market date for Instrument B(which it forwards to eye-rage decoder, and receives the 
        decoded information back from eye-rage decoding application)
     2.2) pax checks quantity executed (say 'e') for the instrument A -> if it is below 'p' then it 
        check if any order is pending in the market.

     2.3.1) if any order is pending in the market, 
     2.3.1.1)pax checks the price of the pending order
     2.3.1.2) if the price of the pending order is different tfrom the price determined using A=Bx+y, then 
            send a replace order to replace the price of the pending order to new price.
            this new order shoud be of size max(q,p-e) -> i.e. if we have a partially filled order in the queue,
            when we have to replace the order, then we set the new order size to the usual order size.

    2.3.2) If no order are pending in the market ( another branch of possibilities from 2.2)
    2.3.2.1) Send order of size 'q' ( or 'p' - 'e' if it is smaller than 'q') at price A=Bx+y
    
    2.2.1) if quantity executed 'e' is equal to ( or greater than) the limit 'p', then do nothing.

    2.4) If any Order is pending in instrument B ( for completing second leg of pair), then that might require 
        update of quote in B ( see 4.2) as per hedging logic - more on this later.
*/
    
int __stn_hft_pair_execute_price_change(struct stn_hft_pair_strategy_attrib_priavte_s * pair)
{

    struct stn_hft_pair_strategy_attrib_s* pair_g = &pair->pair_public;
    int iReturn = STN_ERRNO_SUCCESS;

    WRITE_PAIR_LOG("Entering __stn_hft_pair_execute_price_change");

    /*
        2.3.2) If no orders are pending in the market ( another branch of possibilities from 2.2
        2.3.2.1) Send order of size 'q' ( 'p'-'e'  if it is smaller than 'q' ) at price A = Bx + y
    */
    if(pair->active_order[ORDER_A].isValidOrder == FALSE)
        {
        struct FIX_OE_variables_s OE_crumbs;
        
        // send the order of size 'q' ( or 'p' -'e' if it is smaller than 'q') at price A=Bx+y
        
        uint32_t    order_qty = (pair->pair_public.order_size > pair->pending_qty_A)?
                                pair->pending_qty_A:pair->pair_public.order_size;
        

        // check if new order qty with relation p-e
        
        double price_A = pair_g->Price_B * pair_g->x + pair_g->y;

        // send the new order message based on price_A and New_order_qty
        OE_crumbs.tag_38_order_qty = order_qty;
        OE_crumbs.tag_44_price = price_A;
        OE_crumbs.tag_54_side = pair->side_A;
        
        // TODO: optimize for the performance number : for the latency minimization
        strcpy((char*)OE_crumbs.tag_48_instrument_code,(char*)pair->pair_public.InstrumentCode_A);

//      iReturn =  stn_hft_FIX_op_channel_send_order_new (pair,&OE_crumbs);
        iReturn =  __stn_hft_pair_send_new_order(pair,&OE_crumbs);
    
        if(STN_ERRNO_SUCCESS == iReturn)
            {
            pair->active_order[ORDER_A].isValidOrder = TRUE;
            memcpy(pair->active_order[ORDER_A].ClOrdId,OE_crumbs.tag_11_client_order_id,
                        sizeof(pair->active_order[ORDER_A].ClOrdId));
            memcpy(pair->active_order[ORDER_A].OrigClOrdId,OE_crumbs.tag_11_client_order_id,
                        sizeof(pair->active_order[ORDER_A].ClOrdId));
            pair->active_order[ORDER_A].price = OE_crumbs.tag_44_price;
            pair->active_order[ORDER_A].qty = OE_crumbs.tag_38_order_qty;
            WRITE_PAIR_LOG(" __stn_hft_pair_execute_price_change:2 New Order");
            }
        else
            {
            pair->active_order[ORDER_A].isValidOrder = FALSE;
            WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_price_change:3 New Order:%s ",OE_crumbs.tag_11_client_order_id);
            return iReturn;
            // log the error.
            }
        }
    else
        {

        /*
        2.2) pax checks quantity executed  say 'e' for instrument A -> if it is below p, then it check if any order is pending in 
            market.
        2.3.1) if any order is pending in the market,
        2.3.1.1) pax checks the price of the pending order
        2.3.1.2) if the price of the pending order is different from the price determined using A=Bx+y, then send a order replace
                to replace the price of the pending order to new price. 
                this order replace should be of size max (q,p-e) -> i.e. if we have a partially filled order in the queue, when we have to 
                replace the order, then we set the new order size to the usual orde size.
        */
        if ( pair->total_filled_qty_A < pair_g->net_quantity)
            {
            double price_A = pair_g->Price_B * pair_g->x + pair_g->y;
            
            WRITE_PAIR_LOG("_pax_hft_pair_execute_price_change:4 price_A:%f ",price_A);
            if(pair->active_order[ORDER_A].qty < pair_g->net_quantity && 
                price_A != pair->active_order[ORDER_A].price &&
                pair->active_order[ORDER_A].isValidOrder == TRUE )
                    {
                    
                    struct FIX_OR_variables_s OR_crumbs;
                    uint32_t    order_qty = (pair_g->order_size > pair->pending_qty_A)?
                                        pair->pending_qty_A:pair->pair_public.order_size;
                    // send and order replace 
                    OR_crumbs.tag_38_order_qty = order_qty;
                    strcpy((char*)OR_crumbs.tag_41_orig_clor_id,(char*)pair->active_order[ORDER_A].ClOrdId);
                    OR_crumbs.tag_44_price = price_A;
                    strcpy((char*)OR_crumbs.tag_48_instrument_code,(char*)pair_g->InstrumentCode_A);
                    OR_crumbs.tag_54_side = pair->side_A;
                    strcpy((char*)OR_crumbs.tag_37_order_id,(char*)pair->active_order[ORDER_A].OrdId);
                    strcpy((char*)OR_crumbs.tag_60_message_creation_time,(char*)pair->active_order[ORDER_A].tag60);
                    iReturn  = __stn_hft_pair_send_order_replace(pair,&OR_crumbs);
                    
                    
                    if(STN_ERRNO_SUCCESS == iReturn)
                        {
                        memcpy(pair->active_order[ORDER_A].ClOrdId,OR_crumbs.tag_11_client_order_id,
                                    sizeof(pair->active_order[ORDER_A].ClOrdId));
                        memcpy(pair->active_order[ORDER_A].OrigClOrdId,OR_crumbs.tag_41_orig_clor_id,
                                    sizeof(pair->active_order[ORDER_A].ClOrdId));
                        pair->active_order[ORDER_A].price = OR_crumbs.tag_44_price;
                        pair->active_order[ORDER_A].qty = OR_crumbs.tag_38_order_qty;
                        WRITE_PAIR_LOG(" __stn_hft_pair_execute_price_change:5 Replace Order");
                        }
                    else
                        {
                        pair->active_order[ORDER_A].isValidOrder = FALSE;
                        WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_price_change:6 Replace Order:%s ",OR_crumbs.tag_11_client_order_id);
                        return iReturn ;
                        // log the error.
                        }
                    }
            }
        else 
            {
            //2.2.1 ) if quantity executed 'e' is equal to ( or greater than) the limit 'p', then do nothing.

            pair->is_portfolio_done = TRUE;

            WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_price_change: 7 Portfolio Done:%u,%u",pair->total_filled_qty_A, pair_g->net_quantity);
            return STN_ERRNO_PAIR_PORTFOLIO_DONE;
        
            // just mark if it is required that portfolio is done.
            
            }
        }
    WRITE_PAIR_LOG(" executing:8 Section 2.4");
    
    /*
    2.4) If any order is pending in instrument B( for completing second leg of pair), then that mgiht require updation of quote in B.
    see 4.2 as per hedging logic = more on this later ?? ask eye rage
    */

    // TODO: second leg of hedging for the Instrument B
        {
        double  net_qty_B = (pair->total_filled_qty_A*pair_g->qty_A)/pair_g->qty_B;
        int32_t ord_qty = (int32_t)(net_qty_B - pair->total_filled_qty_B);

        WRITE_PAIR_LOG("__stn_hft_pair_execute_price_change:9 (%f, %f,:%d)", net_qty_B,pair->total_filled_qty_B,ord_qty);
        
        if( ord_qty > 0) // if 1
            {
            
            // we have pending order, or new order to spit out still.
            WRITE_PAIR_LOG("__stn_hft_pair_execute_price_change:10 (%d)",ord_qty);
            if(pair->active_order[ORDER_B].isValidOrder == TRUE) // if 2
                {

                struct FIX_OR_variables_s OR_crumbs;
                // send and order replace 
                WRITE_PAIR_LOG("__stn_hft_pair_execute_price_change:11 ");
                OR_crumbs.tag_38_order_qty = ord_qty;
                strcpy((char*)OR_crumbs.tag_41_orig_clor_id,(char*)pair->active_order[ORDER_B].ClOrdId);
                OR_crumbs.tag_44_price = pair_g->Price_B;
                strcpy((char*)OR_crumbs.tag_48_instrument_code,(char*)pair_g->InstrumentCode_B);
                OR_crumbs.tag_54_side = pair->side_B;
                strcpy((char*)OR_crumbs.tag_37_order_id,(char*)pair->active_order[ORDER_B].OrdId);
                strcpy((char*)OR_crumbs.tag_60_message_creation_time,(char*)pair->active_order[ORDER_B].tag60);
                
                iReturn = __stn_hft_pair_send_order_replace(pair,&OR_crumbs);
                
                if(STN_ERRNO_SUCCESS == iReturn) // if 3
                    {
                    memcpy(pair->active_order[ORDER_B].ClOrdId,OR_crumbs.tag_11_client_order_id,
                                sizeof(pair->active_order[ORDER_B].ClOrdId));
                    memcpy(pair->active_order[ORDER_B].OrigClOrdId,OR_crumbs.tag_41_orig_clor_id,
                                sizeof(pair->active_order[ORDER_B].ClOrdId));
                    pair->active_order[ORDER_B].price = OR_crumbs.tag_44_price;
                    pair->active_order[ORDER_B].qty = OR_crumbs.tag_38_order_qty;
                    WRITE_PAIR_LOG("__stn_hft_pair_execute_price_change:12 Replace Order");
                    }
                else
                    {
                    pair->active_order[ORDER_B].isValidOrder = FALSE;
                    WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_price_change:13 Replace Order:%s ",OR_crumbs.tag_11_client_order_id);
                    return iReturn;
                    // log the error.
                    }
                
                }
            else // we dont have any pending orders meaning we need to send a new order about B
                {
                struct FIX_OE_variables_s OE_crumbs;
                // send and order replace 
                WRITE_PAIR_LOG("__stn_hft_pair_execute_price_change:14");
                OE_crumbs.tag_38_order_qty = net_qty_B; // send for the whole net_qty_B
                OE_crumbs.tag_44_price = pair_g->Price_B;
                strcpy((char*)OE_crumbs.tag_48_instrument_code,(char*)pair_g->InstrumentCode_B);
                OE_crumbs.tag_54_side = pair->side_B;
                iReturn = __stn_hft_pair_send_new_order(pair,&OE_crumbs);
                
                if(STN_ERRNO_SUCCESS == iReturn) // if 4
                    {
                    WRITE_PAIR_LOG("__stn_hft_pair_execute_price_change:15");
                    
                    memcpy(pair->active_order[ORDER_B].ClOrdId,OE_crumbs.tag_11_client_order_id,
                                sizeof(pair->active_order[ORDER_B].ClOrdId));
                    memcpy(pair->active_order[ORDER_B].OrigClOrdId,OE_crumbs.tag_11_client_order_id,
                                sizeof(pair->active_order[ORDER_B].ClOrdId));
                    pair->active_order[ORDER_B].price = OE_crumbs.tag_44_price;
                    pair->active_order[ORDER_B].qty = OE_crumbs.tag_38_order_qty;
                    WRITE_PAIR_LOG("__stn_hft_pair_execute_price_change:16 New Order");
                    }
                else
                    {
                    WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_price_change:17 New Order:%s ",OE_crumbs.tag_11_client_order_id);
                    return iReturn;
                    // log the error.
                    }
                }
            }
        else 
            {
            // there isn't much to do
            
//          pair->is_portfolio_done = TRUE;
            WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_price_change: 18 Do nothing %d ",ord_qty);
//          return PAX_ERRNO_PAIR_PORTFOLIO_DONE;
            
            }
        }
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_price_change:19");

return STN_ERRNO_SUCCESS;
}



/*
3) pax receives execution report from exchange for instrument A ( could be either full fill OR partial fill).
3.1) pax checks net quantity executed for instrument A - say 'e'
3.2) pax calculates quantity to be executed for B, i.e. say e*n/m using quantity relationship mA=nB
3.3) pax compares quantity executed for B say 'b'
3.4) if 'b' is different from e*n/m, then it sends a new order for balancing quantity ( eye rage has to
    confirm if this is going to be a market order or limit order which might require update)
*/
int __stn_hft_pair_process_ER_for_instrument_A(struct stn_hft_pair_decode_fix_msg_s* fix_msg,
    struct stn_hft_pair_strategy_attrib_priavte_s* pair)
{
    struct stn_hft_pair_strategy_attrib_s* pair_g = &pair->pair_public;
    int iReturn;
    WRITE_PAIR_LOG("Entering __stn_hft_pair_process_ER_for_instrument_A ");

    if (fix_msg->OrdStatus == ORDSTATUS_FILLED ||
        fix_msg->OrdStatus == ORDSTATUS_PARTIALLYFILLED)
        {
        uint32_t qty_tobe_executed_for_B = 0;
        pair->pending_qty_A -= fix_msg->Ordqty;
        pair->total_filled_qty_A +=fix_msg->Ordqty;
    
        qty_tobe_executed_for_B = (fix_msg->Ordqty * pair_g->qty_A)/pair_g->qty_B;

        if(pair->total_filled_qty_B < qty_tobe_executed_for_B)
            {
            uint32_t order_qty = qty_tobe_executed_for_B - pair->total_filled_qty_B;
            struct FIX_OE_variables_s OE_crumbs;
            // send and order replace 
            OE_crumbs.tag_38_order_qty = order_qty; // send for the whole net_qty_B
            OE_crumbs.tag_44_price = pair_g->Price_B;
            strcpy((char*)OE_crumbs.tag_48_instrument_code,(char*)pair_g->InstrumentCode_B);
            OE_crumbs.tag_54_side = pair->side_B;
            iReturn = __stn_hft_pair_send_new_order(pair,&OE_crumbs);
            
            if(STN_ERRNO_SUCCESS == iReturn)
                {
                pair->active_order[ORDER_B].isValidOrder = TRUE;
                memcpy(pair->active_order[ORDER_B].ClOrdId,OE_crumbs.tag_11_client_order_id,
                            sizeof(pair->active_order[ORDER_B].ClOrdId));
                memcpy(pair->active_order[ORDER_B].OrigClOrdId,OE_crumbs.tag_11_client_order_id,
                            sizeof(pair->active_order[ORDER_B].ClOrdId));
                pair->active_order[ORDER_B].price = OE_crumbs.tag_44_price;
                pair->active_order[ORDER_B].qty = OE_crumbs.tag_38_order_qty;
                WRITE_PAIR_LOG("__stn_hft_pair_process_ER_for_instrument_A New Order B");
                }
            else
                {
                pair->active_order[ORDER_B].isValidOrder = FALSE;
                WRITE_PAIR_LOG("Leaving __stn_hft_pair_process_ER_for_instrument_A New Order B:%s ",OE_crumbs.tag_11_client_order_id);
                return iReturn;
                }
            }
        else if (qty_tobe_executed_for_B < pair->total_filled_qty_B)
            {
            //log error: if somethings wrong here.
            WRITE_PAIR_LOG("__stn_hft_pair_process_ER_for_instrument_A Somethings wrong");
            }
        }

        WRITE_PAIR_LOG("Leaving __stn_hft_pair_process_ER_for_instrument_A ");
		return STN_ERRNO_SUCCESS;
}




/* 
    4) Pax receieves execution report from exchange for instrument B
    4.1)  check the quantity that should have been executed for given amount of total executions in A
        if we have traded more quanitity in B than what we should have, then it means there is 
        some error -> set status flag for this portfolio to false ( see concept of flag in section 6)

    4.2) check execution report -> is the filled quantity same as the order size send in step 3.4
        if it did not get executed fully and the order type for the second leg was limit orders, 
        then we will have to keep updating our quote in instrument B for the unfilled amount 
        ( will have to get confirmation for the hedging logic by eye-rage)
*/
int __stn_hft_pair_process_ER_for_instrument_B(struct stn_hft_pair_decode_fix_msg_s* fix_msg,
        struct stn_hft_pair_strategy_attrib_priavte_s* pair)
{

    int iReturn;
    struct stn_hft_pair_strategy_attrib_s* pair_g = &pair->pair_public;
    int bProcessA = 0;
    
    WRITE_PAIR_LOG("Entering __stn_hft_pair_process_ER_for_instrument_B ");

    if (fix_msg->OrdStatus == ORDSTATUS_FILLED ||
        fix_msg->OrdStatus == ORDSTATUS_PARTIALLYFILLED)
        {

        uint32_t    qty_tobe_executed_B = (pair->total_filled_qty_A * pair_g->qty_A)/pair_g->qty_B;


        pair->pending_qty_B -= fix_msg->Ordqty;
        pair->total_filled_qty_B +=fix_msg->Ordqty;
    
        if(pair->total_filled_qty_B > qty_tobe_executed_B)
                
            {
            // TODO: some error condition, which has happend.
            // portfolio status to be set to false, and report this as error condition
            WRITE_PAIR_LOG("Leaving __stn_hft_pair_process_ER_for_instrument_B, filled > tobe_executed ");
            return STN_ERRNO_FAIL;
            
            }

        if(pair->active_order[ORDER_B].qty !=  fix_msg->Ordqty)
            {
            // TODO: section 4.2. why we should keep updating the partial fill. 
            WRITE_PAIR_LOG("__stn_hft_pair_process_ER_for_instrument_B , Why update partial Fills");
            }
        else if(pair->active_order[ORDER_B].qty == fix_msg->Ordqty)
            {
            WRITE_PAIR_LOG("__stn_hft_pair_process_ER_for_instrument_B , filled:%d",fix_msg->Ordqty);
            bProcessA = 1;
            
            }

    }

    // check if we need to process the A's order now
    if(bProcessA == 1)
        {
        // keep checking if we get the order size fullfilled for the instrument A
          if(pair->total_filled_qty_A < pair_g->net_quantity)
              {
              if(fix_msg->OrdStatus == ORDSTATUS_FILLED)
                  {
                  struct FIX_OE_variables_s OE_crumbs;
                  uint32_t order_qty = ((pair_g->net_quantity - pair->total_filled_qty_A)> pair_g->order_size)
                      ? pair_g->order_size:(pair_g->net_quantity - pair->total_filled_qty_A);
                  double price_A = pair_g->Price_B*pair_g->x + pair_g->y;
                  // send and order replace 
                  OE_crumbs.tag_38_order_qty = order_qty; // send for the whole net_qty_B
                  OE_crumbs.tag_44_price = price_A;
                  strcpy((char*)OE_crumbs.tag_48_instrument_code,(char*)pair_g->InstrumentCode_A);
                  OE_crumbs.tag_54_side = pair->side_A;
                  iReturn = __stn_hft_pair_send_new_order(pair,&OE_crumbs);
                  
                  if(STN_ERRNO_SUCCESS == iReturn)
                      {
                      memcpy(pair->active_order[ORDER_A].ClOrdId,OE_crumbs.tag_11_client_order_id,
                                  sizeof(pair->active_order[ORDER_A].ClOrdId));
                      memcpy(pair->active_order[ORDER_A].OrigClOrdId,OE_crumbs.tag_11_client_order_id,
                                  sizeof(pair->active_order[ORDER_A].ClOrdId));
                      pair->active_order[ORDER_A].price = OE_crumbs.tag_44_price;
                      pair->active_order[ORDER_A].qty = OE_crumbs.tag_38_order_qty;
                      WRITE_PAIR_LOG("__stn_hft_pair_process_ER_for_instrument_B New Order A");
                      }
                  else
                      {
                      pair->active_order[ORDER_A].isValidOrder = FALSE;
                      WRITE_PAIR_LOG("Leaving __stn_hft_pair_process_ER_for_instrument_B New Order A:%s ",OE_crumbs.tag_11_client_order_id);
                      return iReturn;
                      // log the error.
                      }
                  }
              }
        
        }
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_process_ER_for_instrument_B");
	return STN_ERRNO_SUCCESS;
}


/*
    5.1) Pricing relationship A = Bx + y is modified by trader to A =  Bx' + y'
    5.1.1) Pax updates FPGA data structures to reflect the changes in these parameter
    5.1.2) pricing of the pending order for A in the market is compared to newly derived price using the
        new pricing relationshiop -> if they are different then send new order at new calculated price.
*/
int __stn_hft_pair_execute_price_reln_change(struct stn_hft_pair_strategy_attrib_priavte_s * pair)
{
    
    struct stn_hft_pair_strategy_attrib_s* pair_g = &pair->pair_public;
    int iReturn = STN_ERRNO_SUCCESS;

    WRITE_PAIR_LOG("Entering __stn_hft_pair_execute_price_reln_change");

    double price_A = pair_g->Price_B*pair_g->x + pair_g->y;

    if(pair->active_order[ORDER_A].isValidOrder == TRUE &&
        price_A != pair->active_order[ORDER_A].price)
        {
        struct FIX_OR_variables_s OR_crumbs;
        OR_crumbs.tag_38_order_qty = pair_g->order_size;
        strcpy((char*)OR_crumbs.tag_48_instrument_code,(char*)pair_g->InstrumentCode_A);
        OR_crumbs.tag_44_price = price_A;
        OR_crumbs.tag_54_side = pair->side_A;
        strcpy((char*)OR_crumbs.tag_41_orig_clor_id,(char*)pair->active_order[ORDER_A].ClOrdId);
        strcpy((char*)OR_crumbs.tag_37_order_id,(char*)pair->active_order[ORDER_A].OrdId);
        strcpy((char*)OR_crumbs.tag_60_message_creation_time,(char*)pair->active_order[ORDER_A].tag60);
        
        
        iReturn = __stn_hft_pair_send_order_replace (pair,&OR_crumbs);
        
        if(STN_ERRNO_SUCCESS == iReturn)
            {
            memcpy(pair->active_order[ORDER_A].ClOrdId,OR_crumbs.tag_11_client_order_id,
                        sizeof(pair->active_order[ORDER_A].ClOrdId));
            memcpy(pair->active_order[ORDER_A].OrigClOrdId,OR_crumbs.tag_41_orig_clor_id,
                        sizeof(pair->active_order[ORDER_A].OrigClOrdId));
            pair->active_order[ORDER_A].price = OR_crumbs.tag_44_price;
            pair->active_order[ORDER_A].qty = OR_crumbs.tag_38_order_qty;
            WRITE_PAIR_LOG("__stn_hft_pair_execute_price_reln_change, Order Replace A");
            }
        else
            {
            WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_price_reln_change OR,%s",OR_crumbs.tag_11_client_order_id);
            return iReturn;
            // log the error.
            }
        }
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_price_reln_change");
	return STN_ERRNO_SUCCESS;
}




/*

    5.2) Net Quantity Changed from p to p'
    5.2.1) pax updates FPGA data structures to reflect the change in these parameters. the next time
        any market data arrives for B ( step 2.1) in step 2.2 it will compare e against p' instead of p -> therefore
        in this way, it is possible for the trader to restart trading for the portfolio by increasing p to a value greater than e
    5.2.2) if p' is [greater : correction less] than e , then cancel all pending order in the market.
    5.2.3) if p'  - e < q, and size of pending order in market is q, then replace the size of pending order to p' - e.
*/
int __stn_hft_pair_execute_net_qty_change(struct stn_hft_pair_strategy_attrib_priavte_s * pair)
{

    struct stn_hft_pair_strategy_attrib_s* pair_g = &pair->pair_public;
    int iReturn = STN_ERRNO_SUCCESS;

    WRITE_PAIR_LOG("Entering __stn_hft_pair_execute_net_qty_change");
    if( pair_g->net_quantity < pair->total_filled_qty_A)
        {
        if(pair->active_order[ORDER_A].isValidOrder == TRUE)
            {
            struct FIX_OC_variables_s OC_crumbs;
            strcpy((char*)OC_crumbs.tag_41_orig_clor_id,(char*)pair->active_order[ORDER_A].ClOrdId);
            strcpy((char*)OC_crumbs.tag_48_instrument_code,(char*)pair_g->InstrumentCode_A);
            OC_crumbs.tag_54_side = pair->side_A;
            strcpy((char*)OC_crumbs.tag_37_order_id,(char*)pair->active_order[ORDER_A].OrdId);
            strcpy((char*)OC_crumbs.tag_60_message_creation_time,(char*)pair->active_order[ORDER_A].tag60);

            iReturn =   __stn_hft_pair_send_order_cancel(pair,&OC_crumbs);
            if(STN_ERRNO_SUCCESS == iReturn)
                {
                pair->active_order[ORDER_A].isValidOrder = FALSE;
                
                memset(pair->active_order[ORDER_A].ClOrdId,0x00,sizeof(pair->active_order[ORDER_A].ClOrdId));
                memset(pair->active_order[ORDER_A].OrigClOrdId,0x00,sizeof(pair->active_order[ORDER_A].OrigClOrdId));
                pair->active_order[ORDER_A].price = 0;
                pair->active_order[ORDER_A].qty = 0;
                WRITE_PAIR_LOG("__stn_hft_pair_execute_net_qty_change, Order Cancel A");
                }
            else
                {
                WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_net_qty_change, Order Cancel %s",OC_crumbs.tag_11_client_order_id);
                return iReturn;
                // log the error.
                }
                
            }
        }
    else if(pair_g->net_quantity - pair->total_filled_qty_A > pair_g->order_size)
        {
        uint32_t ord_qty = pair_g->net_quantity - pair->total_filled_qty_A;
        if(pair->active_order[ORDER_A].isValidOrder == TRUE &&
            pair->active_order[ORDER_A].qty == pair_g->order_size)
            {
            struct FIX_OR_variables_s OR_crumbs;
            double price_A = pair_g->Price_B*pair_g->x + pair_g->y;
            OR_crumbs.tag_38_order_qty = ord_qty;
            strcpy((char*)OR_crumbs.tag_48_instrument_code,(char*)pair_g->InstrumentCode_A);
            OR_crumbs.tag_44_price = price_A;
            OR_crumbs.tag_54_side = pair->side_A;
            strcpy((char*)OR_crumbs.tag_41_orig_clor_id,(char*)pair->active_order[ORDER_A].ClOrdId);
            strcpy((char*)OR_crumbs.tag_37_order_id,(char*)pair->active_order[ORDER_A].OrdId);
            strcpy((char*)OR_crumbs.tag_60_message_creation_time,(char*)pair->active_order[ORDER_A].tag60);
            
            
            iReturn = __stn_hft_pair_send_order_replace (pair,&OR_crumbs);
            
            if(STN_ERRNO_SUCCESS == iReturn)
                {
                memcpy(pair->active_order[ORDER_A].ClOrdId,OR_crumbs.tag_11_client_order_id,
                            sizeof(pair->active_order[ORDER_A].ClOrdId));
                memcpy(pair->active_order[ORDER_A].OrigClOrdId,OR_crumbs.tag_41_orig_clor_id,
                            sizeof(pair->active_order[ORDER_A].OrigClOrdId));
                pair->active_order[ORDER_A].price = OR_crumbs.tag_44_price;
                pair->active_order[ORDER_A].qty = OR_crumbs.tag_38_order_qty;
                WRITE_PAIR_LOG("__stn_hft_pair_execute_net_qty_change, Order Replace A");
                }
            else
                {
                WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_net_qty_change, Order replace %s",OR_crumbs.tag_11_client_order_id);
                return iReturn;
                // log the error.
                }
            }
        
        }
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_net_qty_change");
	return STN_ERRNO_SUCCESS;
        
}


/*
    5.3 : Order Size is changed from q to q'
    5.3.1) Existing order for instrument A is replace by a new order for size q' (if p-e > q')
*/
int __stn_hft_pair_execute_ord_size_change(struct stn_hft_pair_strategy_attrib_priavte_s * pair)
{
    struct stn_hft_pair_strategy_attrib_s* pair_g = &pair->pair_public;
    int iReturn = STN_ERRNO_SUCCESS;

    WRITE_PAIR_LOG("Entering __stn_hft_pair_execute_ord_size_change");

    if(pair->active_order[ORDER_A].isValidOrder == TRUE)
        {
        struct FIX_OR_variables_s OR_crumbs;

        if( pair_g->net_quantity - pair->total_filled_qty_A > pair_g->order_size)
            {
            double price_A = pair_g->Price_B*pair_g->x + pair_g->y;
            OR_crumbs.tag_38_order_qty = pair_g->order_size;
            strcpy((char*)OR_crumbs.tag_48_instrument_code,(char*)pair_g->InstrumentCode_A);
            OR_crumbs.tag_44_price = price_A;
            OR_crumbs.tag_54_side = pair->side_A;
            strcpy((char*)OR_crumbs.tag_41_orig_clor_id,(char*)pair->active_order[ORDER_A].ClOrdId);
            strcpy((char*)OR_crumbs.tag_37_order_id,(char*)pair->active_order[ORDER_A].OrdId);
            strcpy((char*)OR_crumbs.tag_60_message_creation_time,(char*)pair->active_order[ORDER_A].tag60);
            
            
            iReturn = __stn_hft_pair_send_order_replace (pair,&OR_crumbs);
            
            if(STN_ERRNO_SUCCESS == iReturn)
                {
                memcpy(pair->active_order[ORDER_A].ClOrdId,OR_crumbs.tag_11_client_order_id,
                            sizeof(pair->active_order[ORDER_A].ClOrdId));
                memcpy(pair->active_order[ORDER_A].OrigClOrdId,OR_crumbs.tag_41_orig_clor_id,
                            sizeof(pair->active_order[ORDER_A].OrigClOrdId));
                pair->active_order[ORDER_A].price = OR_crumbs.tag_44_price;
                pair->active_order[ORDER_A].qty = OR_crumbs.tag_38_order_qty;
                WRITE_PAIR_LOG("__stn_hft_pair_execute_ord_size_change, Order Replace A");
                }
            else
                {
                WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_ord_size_change, Order Replace %s",OR_crumbs.tag_11_client_order_id);
                return iReturn;
                // log the error.
                }
            }
        }
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_execute_ord_size_change");
	return STN_ERRNO_SUCCESS;
}

/*
6.3 cancel all pending orders for instruments A & B, due to stop execution command from UI
*/
int __stn_hft_pair_cancel_all_active_orders(struct stn_hft_pair_strategy_attrib_priavte_s * pair)
{

    int iReturn = 0;
    struct FIX_OC_variables_s OC_crumbs;
    WRITE_PAIR_LOG("Entering __stn_hft_pair_cancel_all_active_orders");
    if(pair->active_order[ORDER_A].isValidOrder == TRUE)
        {
        strcpy((char*)OC_crumbs.tag_41_orig_clor_id,(char*)pair->active_order[ORDER_A].ClOrdId);
        strcpy((char*)OC_crumbs.tag_48_instrument_code,(char*)pair->pair_public.InstrumentCode_A);
        OC_crumbs.tag_54_side = pair->side_A;
        strcpy((char*)OC_crumbs.tag_37_order_id,(char*)pair->active_order[ORDER_A].OrdId);
        strcpy((char*)OC_crumbs.tag_60_message_creation_time,(char*)pair->active_order[ORDER_A].tag60);
        
        
        iReturn =   __stn_hft_pair_send_order_cancel(pair,&OC_crumbs);
        if(STN_ERRNO_SUCCESS == iReturn)
            {
            pair->active_order[ORDER_A].isValidOrder = FALSE;
            
            memset(pair->active_order[ORDER_A].ClOrdId,0x00,sizeof(pair->active_order[ORDER_A].ClOrdId));
            memset(pair->active_order[ORDER_A].OrigClOrdId,0x00,sizeof(pair->active_order[ORDER_A].OrigClOrdId));
            pair->active_order[ORDER_A].price = 0;
            pair->active_order[ORDER_A].qty = 0;
            WRITE_PAIR_LOG("__stn_hft_pair_execute_ord_size_change, Order Cancel A");
            }
        else
            {
            WRITE_PAIR_LOG("Leaving __stn_hft_pair_cancel_all_active_orders,Order Cancel %s", OC_crumbs.tag_11_client_order_id);
            return iReturn;
            // log the error.
            }
        }
        
    if(pair->active_order[ORDER_B].isValidOrder == TRUE)
        {
        strcpy((char*)OC_crumbs.tag_41_orig_clor_id,(char*)pair->active_order[ORDER_B].ClOrdId);
        strcpy((char*)OC_crumbs.tag_48_instrument_code,(char*)pair->pair_public.InstrumentCode_B);
        OC_crumbs.tag_54_side = pair->side_B;
        strcpy((char*)OC_crumbs.tag_37_order_id,(char*)pair->active_order[ORDER_A].OrdId);
        strcpy((char*)OC_crumbs.tag_60_message_creation_time,(char*)pair->active_order[ORDER_A].tag60);
        
                
        iReturn =   __stn_hft_pair_send_order_cancel(pair,&OC_crumbs);
        if(STN_ERRNO_SUCCESS == iReturn)
            {
            pair->active_order[ORDER_B].isValidOrder = FALSE;
            
            memset(pair->active_order[ORDER_B].ClOrdId,0x00,sizeof(pair->active_order[ORDER_B].ClOrdId));
            memset(pair->active_order[ORDER_B].OrigClOrdId,0x00,sizeof(pair->active_order[ORDER_B].OrigClOrdId));
            pair->active_order[ORDER_B].price = 0;
            pair->active_order[ORDER_B].qty = 0;
            WRITE_PAIR_LOG("__stn_hft_pair_execute_ord_size_change, Order Cancel B");
            }
        else
            {
            WRITE_PAIR_LOG("Leaving __stn_hft_pair_cancel_all_active_orders,Order Cancel %s", OC_crumbs.tag_11_client_order_id);
            return iReturn;
            // log the error.
            }
        }

    WRITE_PAIR_LOG("Leaving __stn_hft_pair_cancel_all_active_orders");
	return STN_ERRNO_SUCCESS;
}


/*
process ER that are received from exchange. It is really crucial how do we handle this.

*/
int  __stn_hft_pair_process_ER(struct stn_hft_pair_decode_fix_msg_s*fix_msg,struct stn_hft_pair_strategy_attrib_priavte_s *pair_strategy_private)
{


    WRITE_PAIR_LOG("Entering __stn_hft_pair_process_ER:%s,%c",fix_msg->OrdId,fix_msg->OrdStatus);

    if(strcmp((char*)fix_msg->ClOrdId, (char*)pair_strategy_private->active_order[ORDER_A].ClOrdId) == 0)
        {
        strcpy((char*)pair_strategy_private->active_order[ORDER_A].OrdId,(char*)fix_msg->OrdId);
        strcpy((char*)pair_strategy_private->active_order[ORDER_A].tag60,(char*)fix_msg->tag60);
        WRITE_PAIR_LOG("Information:ER Acks, Order_A :%s,39=%c",fix_msg->OrdId,fix_msg->OrdStatus);
        }
    else if(strcmp((char*)fix_msg->ClOrdId , (char*)pair_strategy_private->active_order[ORDER_B].ClOrdId) == 0)
        {
        strcpy((char*)pair_strategy_private->active_order[ORDER_B].OrdId,(char*)fix_msg->OrdId);
        strcpy((char*)pair_strategy_private->active_order[ORDER_B].tag60,(char*)fix_msg->tag60);
        WRITE_PAIR_LOG("Information:ER Acks, Order_B :%s,39=%c",fix_msg->OrdId,fix_msg->OrdStatus);
        }
    else
        {
        WRITE_PAIR_LOG("Error: Processing ER-Acks :%s,39=%c",fix_msg->OrdId,fix_msg->OrdStatus);
        }
    
    switch(fix_msg->OrdStatus)
        {
        case ORDSTATUS_FILLED:
        case ORDSTATUS_PARTIALLYFILLED:
            {
            WRITE_PAIR_LOG("Success: Processing ER-fills :%s,39=%c",fix_msg->OrdId,fix_msg->OrdStatus);
            
            if(strcmp((char*)fix_msg->ClOrdId, (char*)pair_strategy_private->active_order[ORDER_A].ClOrdId) == 0)
                {
                __stn_hft_pair_process_ER_for_instrument_A(fix_msg,pair_strategy_private);
                }
            else if(strcmp((char*)fix_msg->ClOrdId , (char*)pair_strategy_private->active_order[ORDER_B].ClOrdId) == 0)
                {
                __stn_hft_pair_process_ER_for_instrument_B(fix_msg,pair_strategy_private);
                }
            else
                {
                WRITE_PAIR_LOG("Error: Processing ER-fills :%s,39=%c",fix_msg->OrdId,fix_msg->OrdStatus);
                }
            }
            break;
        case ORDSTATUS_DONE_FOR_DAY:
        case ORDSTATUS_REJECTED:
        case ORDSTATUS_CANCELLED:
        case ORDSTATUS_STOPPED:
        case ORDSTATUS_EXPIRED:
            {
            pair_strategy_private->quit = QUIT_KILL;
            WRITE_PAIR_LOG("Alert:Order Rejectction,Kill,:%s,39=%c",fix_msg->OrdId,fix_msg->OrdStatus);
            }
            break;
        default:
            WRITE_PAIR_LOG("Information: Done Nothing ER-Acks :%s,39=%c",fix_msg->OrdId,fix_msg->OrdStatus);
            break;
        }

        WRITE_PAIR_LOG("Leaving __stn_hft_pair_process_ER");
        return STN_ERRNO_SUCCESS;

}

/*overall strategy thread logic */
void* __stn_hft_pair_strategy_thr_run (void* hdl)
{

    struct stn_hft_pair_strategy_attrib_priavte_s *pair_strategy_private = 
                                            (struct stn_hft_pair_strategy_attrib_priavte_s*)hdl;

    int thread_decoding_count = 0;

    struct stn_hft_pair_strategy_attrib_s* pair_g = &pair_strategy_private->pair_public;
    struct stn_hft_pair_decode_fix_msg_s* fix_msg = &pair_strategy_private->decoded_msg;
    WRITE_PAIR_LOG("Entering __stn_hft_pair_strategy_thr_run,(%s,%s)", pair_g->InstrumentCode_B,pair_g->InstrumentCode_A);
    
    pair_strategy_private->fix_op_chnl = _pair_globals.__fix_op_channel;

    __stn_pair_get_next_sstn_count(); // just bump up once

    // first initiate the order, based on stategy portfolio logic, then go back to the decoding stage.
    

    while(pair_strategy_private->quit == QUIT_RUN)
        {
            if(pair_strategy_private->enable == 0)
                {
                if(pair_strategy_private->active_order[ORDER_A].isValidOrder == TRUE ||
                    pair_strategy_private->active_order[ORDER_B].isValidOrder == TRUE)
                    __stn_hft_pair_cancel_all_active_orders(pair_strategy_private);
                
                pair_strategy_private->price_reln_update_flag = FALSE;
                pair_strategy_private->price_update_from_market_flag = FALSE;
                pair_strategy_private->ord_size_update_flag = FALSE;
                pair_strategy_private->net_qty_update_flag = FALSE;
                thread_decoding_count = pair_strategy_private->posting_thr_updates_count;               
                
                }
            else
                {
                if(pair_strategy_private->price_reln_update_flag == TRUE)
                    {

                    // reset the price relation flag
                    pair_strategy_private->price_reln_update_flag = FALSE;

                    // do the price relationship changes
                   if( STN_ERRNO_SUCCESS != __stn_hft_pair_execute_price_reln_change(pair_strategy_private))
                        {
                        // log error and move on
                        }
                            
                    }
                else if(pair_strategy_private->price_update_from_market_flag == TRUE)
                    {
#if 0                    
                    unsigned long long rt = rdtsc_pair();
                    console_log_write("%s:%d thread post: %llu\n",__FILE__,__LINE__,rt);
                    fflush(stdout);
#endif                    
                    // do the price relationship changes

                    // reset the price relation flag
                    pair_strategy_private->price_update_from_market_flag = FALSE;

                    if( STN_ERRNO_SUCCESS != __stn_hft_pair_execute_price_change(pair_strategy_private))
                        {
                        // log error and move on
                        }
                    }
                else if(pair_strategy_private->ord_size_update_flag == TRUE)
                    {
                    // do the price relationship changes
                    if( STN_ERRNO_SUCCESS != __stn_hft_pair_execute_ord_size_change(pair_strategy_private))
                        {
                        // log error and move on
                        }
                    // do the order size changes
                    pair_strategy_private->ord_size_update_flag = FALSE;
                    }
                else if(pair_strategy_private->net_qty_update_flag == TRUE)
                    {
                    // do the price relationship changes
                    if( STN_ERRNO_SUCCESS != __stn_hft_pair_execute_net_qty_change(pair_strategy_private))
                        {
                        // log error and move on
                        }
                    pair_strategy_private->net_qty_update_flag = FALSE;
                    }
                else if(pair_strategy_private->posting_thr_updates_count > thread_decoding_count)
                    {
                    thread_decoding_count++;

                    __stn_hft_pair_process_ER(fix_msg,pair_strategy_private);
            
                    }
                }
        }
    // if the thread is ended with proper complete status then clean up the order pending. Otherwise if it is killed.
    // nothing should be done.
    if(pair_strategy_private->quit ==  QUIT_COMPLETE)
        __stn_hft_pair_cancel_all_active_orders(pair_strategy_private);
    
    
    WRITE_PAIR_LOG("Leaving __stn_hft_pair_strategy_thr_run,(%s,%s)", pair_g->InstrumentCode_B,pair_g->InstrumentCode_A);
    return 0;
}

