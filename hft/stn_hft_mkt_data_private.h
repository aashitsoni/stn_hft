
#ifndef PAX_HFT_MKT_DATA_PRIVATE_H
#define PAX_HFT_MKT_DATA_PRIVATE_H

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

#include "stn_hft_constants.h"
#include "stn_hft_mkt_data_public.h"
#include "stn_hft_rng_buf_cmn.h"


struct mkt_msg_sz_dist_s {
	uint32_t low;
	uint32_t high;
};

#define STN_HFT_SOCKET_SELECT_WAIT_TIME_USEC		1

// stats globally accross all message types..
struct mkt_channel_stats_s {
	uint8_t messge_id;			// 0th location will always be for the entire market channel across all the message types..
	uint32_t num_packets;
	uint32_t num_messages_sum;
	uint32_t message_size_sum;
	uint32_t time_outs;
	uint32_t message_size_dist[6];		// <=64, 65-128, 129-184, 185-236, 236-384, >384
	uint32_t last_seen_seq_number;
	struct timeval time_last_seen;
};



struct _stn_hft_mkt_channel_handle_private_s {

	struct stn_hft_mkt_channel_public_s mkt_channel_public;	// store the channel attribs provided by the calling app

// socket binding strcutures...
	struct sockaddr_in local_addr;
	struct sockaddr_in svr_addr;
	struct ip_mreq mc_group;
	int sd;
	int quit;
	int channel_type;

	pthread_t mkt_data_run_thr_hdl;
	sem_t start_mkt_data_sema;

	unsigned char* channel_data_hp_buffer; // HUGE PAGE buffer.. to be use as a producer/ consumer queue

	//-- functions per channel type.. 
	int (*channel_login_fn)(struct _stn_hft_mkt_channel_handle_private_s*,uint8_t*,uint8_t*);
	int (*channel_logout_fn)(struct _stn_hft_mkt_channel_handle_private_s*);
	//-- message processing function per exchange type and channel feed type ----//
	int (*process_mkt_msg_fn)(struct _stn_hft_mkt_channel_handle_private_s*,uint8_t*, uint16_t);
	
	int total_bytes_recvd;

	MKT_MSG_CBFN decompress_cbfn;
	uint8_t* user_cbfn_handle;

	//--- logging related fields ------//
	int (*log_mkt_msg_stats_fn)(struct _stn_hft_mkt_channel_handle_private_s*, uint8_t*, uint16_t);
	struct mkt_channel_stats_s *chnnl_stats_blob;
	uint32_t chnnl_stats_blob_sz;
	FILE* fp_stats_log;
	struct timeval start_time,end_time; 
	long log_interval; // in secs..
	int file_header_logged; // indicates if the CSV file header has been logged
	unsigned long time_outs;
	unsigned long mkt_chnl_rng_buf_write_index;
	unsigned long mkt_chnl_rng_buf_read_index;
	
};


int __stn_hft_mkt_data_channel_init_common		(struct stn_hft_mkt_channel_public_s *mkt_chnl_public, 
												 struct _stn_hft_mkt_channel_handle_private_s **mkt_chnl_hdl_private);
int __stn_hft_log_stats							(struct _stn_hft_mkt_channel_handle_private_s *_mkt_chnl_hdl, struct mkt_channel_stats_s *chnnl_stats);



#endif
