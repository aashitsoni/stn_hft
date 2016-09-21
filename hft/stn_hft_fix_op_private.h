
#ifndef PAX_HFT_FIX_OP_PRIVATE_H
#define PAX_HFT_FIX_OP_PRIVATE_H

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

#include "stn_errno.h"
#include "stn_hft_constants.h"
#include "stn_hft_fix_op_public.h"
#include "stn_hft_rng_buf_cmn.h"

#define TAG_8_LEN   10       // 8=FIX.4.2. begine string
#define TAG_9_LEN   7       // 9=0000. message length
#define TAG_35_LEN  5       // 35=x.  message type
#define TAG_52_LEN  21      // 52=99990011-22:33:55.  time stamp
#define TAG_INIT_LEN    4

#define LOGIN_MSG_TYPE          'A'
#define ORDER_NEW_MSG_TYPE      'D'
#define ORDER_CANCEL_MSG_TYPE   'F'
#define ORDER_REPLACE_MSG_TYPE  'G'
#define FIX_HEARTBEAT_MSG_TYPE 	'0'
#define FIX_MSG_OFFSET           128
#define FIX_MSG_SIZE             4096
#define FIX_BUFF_SIZE            FIX_MSG_SIZE-FIX_MSG_OFFSET

#define FIX_MSG_HDR_LEN (TAG_8_LEN + TAG_9_LEN + TAG_35_LEN + TAG_52_LEN)


struct _stn_hft_FIX_op_channel_handle_private_s {

	struct stn_hft_FIX_op_channel_public_attrib_s 
						FIX_op_channel_public_attribs;	// store the channel attribs provided by the calling app

   struct FIX_session_constants_s session_constants;

// socket binding strcutures...
	struct sockaddr_in local_addr;
	struct sockaddr_in svr_addr;
	int sd;
	int quit;

	pthread_t fix_op_recv_run_thr_hdl;
	sem_t start_FIX_op_recv_sema;

	unsigned char* channel_data_recv_bfr_hp; // Huge Page buffer.. to be used for producer/ consumer queue
	int total_bytes_recvd;

    uint32_t fix_msg_seq_num;

	char*	formattedMsg;

	unsigned long FIX_op_chnl_rng_buf_write_index;
	unsigned long FIX_op_chnl_rng_buf_read_index;

    unsigned char*  generic_msg_bfr_segment_hp;
    int generic_msg_bfr_segment_sz_total;
    int FIX_generic_msg_bfr_area_sz;
    int FIX_generic_msg_bfr_area_allowed_sz;
    int FIX_generic_msg_bfr_area_reserved_sz;

    int FIX_msg_hdr_fixed_length;

	FILE* fp_stats_log;
	struct timeval start_time,end_time; 
	long log_interval; // in secs..
	int file_header_logged; // indicates if the CSV file header has been logged
	unsigned long time_outs;

	pthread_t	fix_pair_strategy_master_thr_hdl;

};


struct _stn_hft_FIX_op_channel_handle_clone_s {

	
	struct _stn_hft_FIX_op_channel_handle_private_s* FIX_real_handle;
	unsigned long FIX_clone_chnl_rng_buf_read_index;
};

struct _stn_hft_FIX_op_channel_handle_clone_s* __stn_fix_create_clone_private_handle(void* FIX_chnl_handle);
int __stn_fix_destroy_clone_private_handle  (struct _stn_hft_FIX_op_channel_handle_clone_s* FIX_clone_handle);
int __stn_hft_FIX_clone_chnl_get_next_msg (struct _stn_hft_FIX_op_channel_handle_clone_s* FIX_clone_handle,
												unsigned char** msg, 
												unsigned int *msg_len);
int stn_hft_FIX_op_channel_send_order_cancel_without_orderid (void* pax_hft_FIX_op_channel_handle, struct FIX_OC_variables_s *p_FIX_op_order_cancel_crumbs);
int stn_hft_FIX_op_channel_send_order_replace_without_orderid (void* pax_hft_FIX_op_channel_handle, struct FIX_OR_variables_s *p_FIX_op_order_replace_crumbs);



#endif
