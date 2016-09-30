/******************************************************************************************************************

 Author      Date               Comments
 ------      -----              ---------
   mt       09/24/2016          Seperated fix messgae functionality into new file from the op-core

*******************************************************************************************************************/

#ifndef _STN_HFT_FIX_ORDER_DB_H_
#define _STN_HFT_FIX_ORDER_DB_H_

#define FIX_ORDER_DB_HASHSIZE 1013


typedef struct _STN_HFT_FIX_ORDER_STRUCT_S
{
	uint8_t tag11_clordid[64];
	uint8_t tag38_ordqty[16];
	uint8_t tag44_price[32];
	uint8_t tag55_symbol[16];
	uint8_t tag48_instrumentcode[32];
	struct _STN_HFT_FIX_ORDER_STRUCT_S* next; // hash-list next element
}STN_HFT_FIX_ORDER_S,*STN_HFT_FIX_ORDER_P;

typedef struct _STN_HFT_FIX_ORDER_HASH_S
{
	STN_HFT_FIX_ORDER_P hash_list[FIX_ORDER_DB_HASHSIZE];

}STN_HFT_FIX_ORDER_HASH_T,*STN_HFT_FIX_ORDER_HASH_P;


int stn_hft_FIX_add_new_order_into_db();



#endif
