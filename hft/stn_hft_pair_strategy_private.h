
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

Author      Date                Comments
------      -----               ---------
mt      10/01/2012          inits rev
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/


#ifndef _STN_HFT_PAIR_STRATEGY_PRIVATE_H
#define _STN_HFT_PAIR_STRATEGY_PRIVATE_H


#include "stn_hft_pair_strategy_public.h"

#define TRUE 1
#define FALSE 0
#define MAX_PAIR_STRATEGY       20

#define TAG_FIX_MsgType     0x3533  // 35 - MsgType
#define TAG_FIX_ClOrdId     0x3131  // 31- ClOrdId
#define TAG_FIX_OrdId       0x3733  //37 - OrdId
#define TAG_FIX_Symbol      0x3535  // 55 - symbol
#define TAG_FIX_Price       0x3434  // 44 - price
#define TAG_FIX_OrderQty    0x3833  // 38 - orderqty            
#define TAG_FIX_OrdStatus   0x3933  // 39 - order status
#define TAG_FIX_ExecType    0x353031 // 105 - execType
#define TAG_FIX_InstrumentCode 0x3834 // 48 - instrument code
#define TAG_FIX_60_MsgCreationTime 0x3036 // 60 - msg creation time

#define FIX_TLV_SEPERATOR   0x01
#define FIX_TAG_DELIMETER   0x3d
#define FIX_PARSING_ERROR   0xAFFF

#define FIX_MsgType_ExecutionReport 0x38
#define ASCII_CHAR      0x30
#define FIX_HFT_PAIR_MAX_SUPPORTED_FIELD 9


#define FLOOR_BROKER_INSTR '1'

#define BUY_SIDE '1'
#define SELL_SIDE '2'

#define MARKET_ORDER '1'
#define LIMIT_ORDER '2'


#define INSTRUMENT_CODE "CUSIP"
#define INSTRUMENT_TYPE "OPT"
#define ALTERNATE_SEC_ID "8"
#define MAT_YEAR_MTH "YYYYMM"
#define MAT_DAY 0x30



#define ORDSTATUS_NEW               '0'
#define ORDSTATUS_PARTIALLYFILLED   '1'
#define ORDSTATUS_FILLED            '2'
#define ORDSTATUS_DONE_FOR_DAY      '3'
#define ORDSTATUS_CANCELLED         '4'
#define ORDSTATUS_REPLACED          '5'
#define ORDSTATUS_PENDING_CANCEL    '6'
#define ORDSTATUS_STOPPED           '7'
#define ORDSTATUS_REJECTED          '8'
#define ORDSTATUS_SUSPENDED         '9'
#define ORDSTATUS_PENDING_NEW       'A'
#define ORDSTATUS_CALCULATED        'B'
#define ORDSTATUS_EXPIRED           'C'
#define ORDSTATUS_ACCEPT_FOR_BID    'D'
#define ORDSTATUS_PENDING_REPLACE   'E'



#define FIX_MSGTYPE_EXEC_REPORT     '8'

struct stn_hft_pair_decode_fix_msg_s
{
    uint8_t     ClOrdId[32];
    uint8_t     OrdId[32];
    uint8_t     InstrumentCode[64];
    double      Price;
    uint32_t    Ordqty;
    uint8_t     MsgType;
    uint8_t     OrdStatus;
    uint8_t     ExecType;
    uint8_t     tag60[64];
};



/*
    Define structure struct pax_hft_active_orders_s 
    Create a GLOBAL array of this structure. [MAX_ACTIVE_ORDERS]
    "Posting"  thread will search for OrigClorid and update this location
    Fields:
    -- pthread_t thread_handle
    -- ClorId
    -- OrigClorId -- if user has posted Modify and or Cancel.
    -- filled qty
    -- posting_thr_updates_count -- strategy_worker will have a local stack variable: last_update_count. 
                                 If last_updated_count < glbl_arry[i].posting_thr_updates_count then it will update stn_hft_pair_strategy_attrib_priavte_s.pending_qty
*/
struct stn_hft_pair_active_orders_s
{
    uint32_t    isValidOrder;
    uint8_t     ClOrdId[16];
    uint8_t     OrigClOrdId[16];
    double      price;
    double      qty;
    uint8_t     OrdId[32];
    uint8_t     tag60[64];// replace and cancel should follow this time
};


#define ORDER_A     0
#define ORDER_B     1
#define TOTAL_ORDER 2

struct stn_hft_pair_strategy_attrib_priavte_s
{
    struct stn_hft_pair_strategy_attrib_s pair_public;

    /*
    need to add: 
        > pending_qty (??) which one? A or B or both
        > strategy_change_flags.. price/ relation/ qty/ size
        > price_A
        > price_B
    */

    // management variables specific to us

    int                     enable;
    int                     quit;
    
    pthread_t               pair_run_thrd_handle;
    struct _stn_hft_FIX_op_channel_handle_clone_s* 
                            FIX_Clone_hdl;
    struct stn_hft_FIX_op_channel_public_attrib_s* fix_op_chnl; 

    struct stn_hft_pair_active_orders_s 
                            active_order[TOTAL_ORDER];
    
    uint32_t    posting_thr_updates_count;
    
    struct      stn_hft_pair_decode_fix_msg_s
                            decoded_msg;
    
    struct FIX_OE_variables_s 
                            OE_crumbs;

    int                     side_A;
    int                     side_B;
    

    double                  total_filled_qty_A;
    double                  total_filled_qty_B;
    
    double                  pending_qty_A;
    double                  pending_qty_B;

    
    unsigned int            price_reln_update_flag;
    unsigned int            net_qty_update_flag;
    unsigned int            ord_size_update_flag;
    unsigned int            price_update_from_market_flag;

    unsigned int            is_portfolio_done;
    int                     strat_counter;
    char                    strat_sstn[30];
    
    
};


#define PAIR_LOG_FILE_NAME "pax_hft_stats_%d_%d_%d.log"

struct stn_hft_pair_strategy_global_s
{
    char __clordid[8];
    FILE* fp_pair_log;
    int log_status_flag;
    pthread_spinlock_t  pair_file_lock;
    char _pairlog_file[256];
    struct stn_hft_FIX_op_channel_public_attrib_s* __fix_op_channel; 
    int _number_of_strategy_created;
    int _pair_sequence_count;
    int _pair_sstn_count;
    struct stn_hft_pair_strategy_attrib_priavte_s 
        __stn_pair_stategy_array[MAX_PAIR_STRATEGY];

};



#endif
