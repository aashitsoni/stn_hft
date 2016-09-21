/*

	Copyright 2012 : SwiftTrade Networks LLC, MA
	********************************************************************************
	Author : Mohan Thakur 
	Module : Strategy file for the Proficient project

	Change Logs
	********************************************************************************
	05/29/14 : created 

*/


#ifndef _STN_HFT_PROF_STRATEGY_PUBLIC_H_
#define _STN_HFT_PROF_STRATEGY_PUBLIC_H_

#include <stn_hft_fix_op_public.h>

typedef enum stn_hft_strat_prof_type_e
{
	STN_PROF_STRAT_PROF_TYPE_UNK,
	STN_PROF_STRAT_PROF_TYPE_B2S1,
	STN_PROF_STRAT_PROF_TYPE_B1S2
}STN_HFT_strat_PROF_TYPE;

typedef enum stn_hft_prof_computation_type_e
{
	STN_HFT_PROF_COMP_TYPE_UNK,
	STN_HFT_PROF_COMP_TYPE_TOUCHLINE,
	
}STN_HFT_PROF_COMPUTATION_TYPE;

typedef struct stn_hft_prof_strategy_attrib_s
{

	char 					instrument_leg1[64];
	char 					instrument_leg2[64];
	STN_HFT_PROF_TYPE 		type;
	double 					leg1_net_qty;
	double 					leg1_order_qty;
	double 					leg2_order_qty;
	int 					order_multiplier;
	double					target_reverse;
	double					target_forward;
	double					leg2_threshold;
	double					jump_counter;
	int 					leg1_retires; // set this to zero to place leg1 order into market
										  // 1 - 1 retries may be based on limit price and at interval defined
										  // 2 - 2 retries may be based on limit price computed and at interval

//{{ These are common fields and make it understandable to see if they are needed.

    char        			tag_200_mat_month[16];          // maturity Month
    char        			tag_48_instrument_code[16];     // instrument code
    char        			tag_167_instrument_type[16];    // instrument type
    char        			tag_1_account_name[16];
    int         			strat_id;
//Check field}}
}STN_HFT_PROF_STRATEGY_ATTRIB_T,*STN_HFT_PROF_STRATEGY_ATTRIB_P;

int stn_hft_prof_create_strategy(/*[IN]*/STN_HFT_PROF_STRATEGY_ATTRIB_P new_prof_strat_attrib,
								/*[IN]*/void* fix_chnl_handle,
								/*[IN:OUT]*/void** prof_handle);

int stn_hft_prof_update_strategy(/*[IN]*/STN_HFT_PROF_STRATEGY_ATTRIB_P update_prof_strat_attrib,
								 /*[IN]*/void* prof_handle,
								 /*[IN]*/int update_flag);
int stn_hft_prof_resume_strategy(/*[IN]*/void * prof_handle);
int stn_hft_prof_pause_strategy(/*[IN]*/void* prof_handle);
int stn_hft_prof_delete_strategy(/*[IN]*/void* prof_handle);

#endif

