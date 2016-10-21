
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014 : SwiftTrade Networks LLC, MA


file name : public/hft/stn_hft_fix_op_public.h

Author      Date                Comments
------      -----               ---------
mt          01/11/2012          init rev
mi          03/27/2012          MCX FIX order processing
vi          04/04/2012          OE fixes for hardware alignment issue
mt		    01/21/2015		    fixes for the nse password tags into session
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

#ifndef STN_HFT_FIX_ORDER_PROCESSING_PUBLIC_H
#define STN_HFT_FIX_ORDER_PROCESSING_PUBLIC_H


struct FIX_session_constants_s {
    uint8_t tag_8_FIXString [9];

    // LOGIN related session crumbs
    uint32_t tag_34_msg_seq_num; // message sequence number
    uint8_t tag_91_encrpted_digest [128];       //=02B4E55C82083A3114A084D6BF984DF69DF845D19484761B |  -- Actual Encrypted Stream
    uint16_t tag_90_encrptd_digest_length;  // -- Encrptd Stream Length
    uint8_t tag_96_raw_data[128];           //=13641,13640 |  -- Raw Data
    uint16_t tag_95_raw_data_length;        //=11 – RawDataLength| 
    uint8_t tag_98_encrpt_methd;        //=0 | -- Encryption Method
    uint8_t tag_141_Reset_seq_num;      //=N |  -- ResetSeqNumFlag, if both sides should reset or not
    uint8_t tag_554_password[16];	// NSE custom tag - 554 for passing password
    // OP related session crumbs
    uint8_t tag_49_sender_comp_id[64];
    uint8_t tag_50_sender_comp_sub_id[64];  // check w/ ..
    uint8_t tag_56_target_comp_id[64];
    uint8_t tag_57_target_comp_sub_id[64];
    uint8_t mcx_tag_9227_terminal_info[64];
    uint16_t tag_204_cust_or_firm;
    uint32_t tag_59_time_in_force;          // check w/ ..
	uint8_t tag_925_newpassword[16];
};


// - - - - - - - - - - - - - - - - - - - - - - - - - 
// --  ORDER ENTRY 
// vars to be given from Algo Engine to Hardware
// - - - - - - - - - - - - - - - - - - - - - - - - - 
struct FIX_OE_variables_s {
    uint32_t tag_34_msg_seq_num;
    uint32_t tag_21_floor_broker_instr;
    uint8_t tag_60_message_creation_time[64];
    uint8_t tag_11_client_order_id[64];
    uint16_t tag_38_order_qty;
    uint8_t tag_40_order_type;
    uint32_t tag_44_price;
    uint8_t tag_48_instrument_code[64];
    uint8_t tag_54_side;
    uint8_t tag_55_symbol[16];
    uint8_t tag_167_instrument_type [8];
    uint8_t tag_200_mat_year_mth[8];
    uint8_t tag_205_mat_day;
    uint8_t tag_1_account[16]; // Set it null if not in use
    uint32_t tag_9366_strat_id;
    uint8_t tag_9367_sstn[20];// yymmddNNnnnnnnnnn

};

// - - - - - - - - - - - - - - - - - - - - - - - - - 
// --  ORDER REPLACE 
// vars to be given from Algo Engine to Hardware
// - - - - - - - - - - - - - - - - - - - - - - - - - 
struct FIX_OR_variables_s {
    uint32_t tag_34_msg_seq_num;
    uint8_t tag_60_message_creation_time[64];
    uint8_t tag_22_alternative_SecurityID[16];
    uint8_t tag_11_client_order_id[64];
    uint8_t tag_21_floor_broker_instr;
    uint8_t tag_37_order_id [64];   //
    uint16_t tag_38_order_qty;
    uint8_t tag_40_order_type;
    uint8_t tag_41_orig_clor_id [64];   //
    uint32_t tag_44_price;
    uint8_t tag_48_instrument_code[64];
    uint8_t tag_54_side;
    uint8_t tag_55_symbol[16];
    uint8_t tag_167_instrument_type [8];
    uint8_t tag_200_mat_year_mth[8];
    uint8_t tag_205_mat_day;
    uint32_t tag_9366_strat_id;// NN 
    uint8_t tag_9367_sstn[20];// yymmddNNnnnnnnnnn
};

// - - - - - - - - - - - - - - - - - - - - - - - - - 
// --  ORDER CANCEL
// vars to be given from Algo Engine to Hardware
// - - - - - - - - - - - - - - - - - - - - - - - - - 
struct FIX_OC_variables_s {
    uint32_t tag_34_msg_seq_num;
    uint8_t tag_21_floor_broker_instr;
    uint8_t tag_11_client_order_id[64];
    uint8_t tag_37_order_id [64];   //
    uint8_t tag_41_orig_clor_id [64];   //
    uint8_t tag_48_instrument_code[64];
    uint8_t tag_54_side;
    uint8_t tag_55_symbol[16];
    uint8_t tag_167_instrument_type [8];
    uint8_t tag_200_mat_year_mth[8];
    uint8_t tag_205_mat_day;
    uint8_t tag_60_message_creation_time[64];
    uint32_t tag_9366_strat_id;
    uint8_t tag_9367_sstn[20];
};


typedef struct stn_hft_FIX_op_genric_msg_bfr_descr_s
    {
    uint8_t*    p_msg_buff;    
    uint16_t    msg_sz;    
    uint64_t    buffer_id;
    }stn_hft_FIX_op_genric_msg_bfr_descr_t,
    *stn_hft_FIX_op_genric_msg_bfr_descr_p;

// - - - - - - - - - - - - - - - - - - - - - - - - - 
// FIX channel management routines
// - - - - - - - - - - - - - - - - - - - - - - - - - 
#define FIX_OP_CHNL_MAX_THRDS  16

struct stn_hft_FIX_op_channel_public_attrib_s {
    uint8_t     FIX_gway_addr[16];          //IPv4 addressing only
    uint16_t    FIX_gway_port;              //TCP port of the FIX G'way data stream source
    uint8_t     local_interface_ip [16];    //IP of the hardware port on which this FIX channel should bind. ipv4 addressing only
    uint8_t     recv_cpu_id;                //Canonical # - CPU ID on which the driver/hardware should send the MSI receive for the data for this channel
    
    uint32_t    fix_op_chnl_generic_bfrs_needed; 
    stn_hft_FIX_op_genric_msg_bfr_descr_t
                stn_generic_msg_bfr_arr[FIX_OP_CHNL_MAX_THRDS];

    /* Fields to enable stats logging for channel. for DEBUGGING ONLY..*/
    //NOTE: Enabling this will be a performance degradation.. this is  I/O
    uint32_t    log_stats_flag;
    uint32_t    log_interval; // in secs

    uint32_t    msg_drop_count;// message drop counter
    
};



/*apis for fix channels */
int stn_hft_FIX_op_channel_create   (struct stn_hft_FIX_op_channel_public_attrib_s* FIX_op_channel_attribs, 
                                     struct FIX_session_constants_s *FIX_session_constants,
                                     uint32_t session_seq_nmbr_seed,
                                     void** pax_hft_FIX_op_channel_handle);
int stn_hft_FIX_op_channel_start    (void* pax_hft_FIX_op_channel_handle);
int stn_hft_FIX_op_channel_login    (void* pax_hft_FIX_op_channel_handle);
int stn_hft_FIX_op_channel_send_order_new       (void* pax_hft_FIX_op_channel_handle, struct FIX_OE_variables_s *p_FIX_op_order_new_crumbs);
int stn_hft_FIX_op_channel_send_order_replace   (void* pax_hft_FIX_op_channel_handle, struct FIX_OR_variables_s *p_FIX_op_order_replace_crumbs);
int stn_hft_FIX_op_channel_send_order_cancel    (void* pax_hft_FIX_op_channel_handle, struct FIX_OC_variables_s *p_FIX_op_order_cancel_crumbs);
int stn_hft_FIX_op_channel_get_next_msg(void* pax_hft_FIX_op_channel_handle,unsigned char** msg, unsigned int *msg_len);
int stn_hft_FIX_op_channel_delete   (void* pax_hft_FIX_op_channel_handle);
int stn_hft_FIX_op_send_generic_msg(void* pax_hft_FIX_op_channel_handle, unsigned char msg_type,unsigned char* msg_buf,int sz_msg_buf, char* time_str);
int stn_hft_FIX_op_channel_send_hb(void* stn_hft_FIX_op_channel_handle);

#endif



