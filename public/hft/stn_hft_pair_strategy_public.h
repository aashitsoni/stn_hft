/*

	Copyright 2012 : SwiftTrade Networks LLC, MA
	********************************************************************************
	Author : Mohan Thakur 
	Module : Pair Strategy

	Change Logs
	********************************************************************************
	05/29/14 : created 

*/

#ifndef _STN_HFT_PAIR_STRATEGY_PUBLIC_H_
#define _STN_HFT_PAIR_STRATEGY_PUBLIC_H_

#include <stn_hft_fix_op_public.h>


#define BUY_A_SELL_B    0x01
#define SELL_A_BUY_B    0x02

#define PRICE_RELATION_CHANGE   0x01
#define NET_QTY_CHANGE      0x02
#define ORD_SIZE_CHANGE         0x04

struct stn_hft_pair_strategy_attrib_s
{
    double      x,
                y;// represents A=Bx+y;
    char        InstrumentCode_A[64],
                InstrumentCode_B[64];
    double      Price_B;
    int         qty_A,
                qty_B;// to represent quantity relationship
    int         order_size;
    int         net_quantity;
    int         direction;
    int         lot_A,
                lot_B;
    int         tick_A,
                tick_B;// NOTE: --> what is tick? do we need tick A? Since A will always be defined by Bx+y 
    char        risk_mgmt[8];
    
    char        tag_200_mat_month[16];          // maturity Month
    char        tag_48_instrument_code[16];     // instrument code
    char        tag_167_instrument_type[16];    // instrument type
    char        tag_1_account_name[16];
    int         strat_id;
    
    
    
};

typedef struct stn_hft_pair_strategy_attrib_s stn_hft_pair_strategy_attrib;


/* apis pair trading functionality*/
int stn_hft_create_pair_strategy(/*[IN]*/stn_hft_pair_strategy_attrib* new_pair, 
                                    /*[IN]*/void*   fix_chnl_handle,
                                    /*[IN|OUT]*/void** pair_handle);

int stn_hft_update_price_of_B    (/*[IN*/double price_B,
                                        /*[IN]*/void* pair_handle);

int stn_hft_update_pair_strategy (/*[IN]*/stn_hft_pair_strategy_attrib* update_pair,
                                     /*[IN]*/void* pair_handle,
                                     /*[IN]*/int update_flag);

int stn_hft_resume_pair_execution    (void* pair_handle);
int stn_hft_pause_pair_execution (void* pair_handle);
int stn_hft_delete_pair_strategy (void* pair_handle);
#endif


