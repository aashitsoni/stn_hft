
#include <stdio.h>
//#include <curses.h>
//#include <term.h>

#include <sys/mman.h>
#include <numa.h>


#include "stn_errno.h"
#include "stn_hft_mkt_data_private.h"
//#include "stn_hft_nse_mkt_data.h"
#include "stn_numa_impl.h"
#include "console_log.h"




//----- fwd decls..

int __stn_hft_mkt_data_channel_thr_run (void* hdl);

//----------------------

struct mkt_msg_sz_dist_s msg_size_bdry [6]= { {1,64},
											 {65,128},
											 {129,184},
											 {185,236}, 
											 {236,384}, 
											 {384,65536}
										  };

//----------------------

int __stn_hft_mkt_data_channel_init_common(struct stn_hft_mkt_channel_public_s *mkt_chnl_public, 
										   struct _stn_hft_mkt_channel_handle_private_s **mkt_chnl_hdl_private)
{
	*mkt_chnl_hdl_private = (struct _stn_hft_mkt_channel_handle_private_s *)calloc(1,sizeof(struct _stn_hft_mkt_channel_handle_private_s));

	if (0 == *mkt_chnl_hdl_private)
		return STN_ERRNO_MALLOC_FAIL;

	switch(mkt_chnl_public->exchange_id)
	{
	case stn_hft_enum_exchange_id_nse_e:
		// allocate message processing fn..
		if (STN_HFT_MKT_CHANNEL_TYPE_TCP == mkt_chnl_public->channel_type) {
			(*mkt_chnl_hdl_private)->process_mkt_msg_fn = 0;
			(*mkt_chnl_hdl_private)->channel_login_fn = 0;
			(*mkt_chnl_hdl_private)->channel_logout_fn= 0;
		} else if (STN_HFT_MKT_CHANNEL_TYPE_MUDP == mkt_chnl_public->channel_type) {
			(*mkt_chnl_hdl_private)->process_mkt_msg_fn = 0;
		}
		// if calling app has enabled logging 
		if (mkt_chnl_public->log_stats_flag) {
			(*mkt_chnl_hdl_private)->chnnl_stats_blob = calloc (1, STN_HFT_MAX_MKT_DATA_MESSAGE_TYPES * sizeof(struct mkt_channel_stats_s));	
			if (STN_HFT_MKT_CHANNEL_TYPE_TCP == mkt_chnl_public->channel_type)
				(*mkt_chnl_hdl_private)->log_mkt_msg_stats_fn = 0;
			else if (STN_HFT_MKT_CHANNEL_TYPE_MUDP == mkt_chnl_public->channel_type)
				(*mkt_chnl_hdl_private)->log_mkt_msg_stats_fn = 0;

			(*mkt_chnl_hdl_private)->chnnl_stats_blob_sz = STN_HFT_MAX_MKT_DATA_MESSAGE_TYPES * sizeof(struct mkt_channel_stats_s);
		}
		break;
	case stn_hft_enum_exchange_id_mcx_e:
		break;
	default: 
		free((*mkt_chnl_hdl_private));
		return STN_ERRNO_HFT_INVALID_EXCHANGE_ID;
		break;
	}

	// bitwise copy the channel info into the HANDLE
	(*mkt_chnl_hdl_private)->mkt_channel_public = *mkt_chnl_public; // no pointers in source, so BIT WISE copy of all the fields..

	// setup stats logging
	if (mkt_chnl_public->log_stats_flag)
		(*mkt_chnl_hdl_private)->fp_stats_log = fopen("mkt_stats.log","w");

	return STN_ERRNO_SUCCESS;
}


int stn_hft_mkt_data_channel_login (void* pax_hft_hdl, uint8_t* user_name, uint8_t* passwd)
{
	struct _stn_hft_mkt_channel_handle_private_s *_mkt_chnl_hdl_private = pax_hft_hdl;
	int ret = _mkt_chnl_hdl_private->channel_login_fn(_mkt_chnl_hdl_private,user_name,passwd);
	return ret;
}

int stn_hft_mkt_data_channel_logout(void* pax_hft_hdl)
{
	struct _stn_hft_mkt_channel_handle_private_s *_mkt_chnl_hdl_private = pax_hft_hdl;
	int ret = _mkt_chnl_hdl_private->channel_logout_fn(_mkt_chnl_hdl_private);
	return ret;
}


int stn_hft_mkt_data_channel_tcp_create (struct stn_hft_mkt_channel_public_s* mkt_chnl_public, 
										 struct stn_mkt_channel_sym_filters_s* sym_filter_arr, 
									     uint16_t num_sym_filters, 
										 MKT_MSG_CBFN user_provided_cbfn,
										 void** opaque_hdl)
{
	struct _stn_hft_mkt_channel_handle_private_s *_mkt_chnl_hdl_private =0;
	char llm_flag = 1;

	if (STN_ERRNO_SUCCESS != __stn_hft_mkt_data_channel_init_common(mkt_chnl_public,&_mkt_chnl_hdl_private))
		return STN_ERRNO_HFT_MKT_CHNL_INIT_FAIL;

	_mkt_chnl_hdl_private->decompress_cbfn = user_provided_cbfn;
	_mkt_chnl_hdl_private->channel_type = mkt_chnl_public->channel_type;

	// setup the return opaque handle
	*opaque_hdl = _mkt_chnl_hdl_private ;

	/* Create a datagram socket on which to receive. */
	_mkt_chnl_hdl_private->sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(_mkt_chnl_hdl_private->sd < 0)
	{
		perror("Opening TCP socket error");
		return -1;
	}
	else
		console_log_write("%s:%d Opening TCP socket....OK.\n",__FILE__,__LINE__);

	/* 
	 Enable tcp_low_latency
	 For our NIC's we do not need this, since we will PIO and do a BUSY POLL anyway for the recv buffers
	 llm_flag = 1;
	 ret = setsockopt( _mkt_chnl_hdl_private->sd, IPPROTO_TCP, tcp_low_latency, (char *)&flag, sizeof(flag) );
	*/

	memset((char *) &(_mkt_chnl_hdl_private->svr_addr), 0, sizeof(_mkt_chnl_hdl_private->svr_addr));
	_mkt_chnl_hdl_private->svr_addr.sin_family = AF_INET;
	_mkt_chnl_hdl_private->svr_addr.sin_port = htons(mkt_chnl_public->mkt_data_port);
	_mkt_chnl_hdl_private->svr_addr.sin_addr.s_addr = inet_addr(mkt_chnl_public->mkt_data_addr);;

	if (connect(_mkt_chnl_hdl_private->sd, (struct sockaddr*)&(_mkt_chnl_hdl_private->svr_addr), sizeof(_mkt_chnl_hdl_private->svr_addr)) < 0)
	{
		perror("TCP connect error");
		return -1;
	}
	return STN_ERRNO_SUCCESS;
}


int stn_hft_mkt_data_channel_mcast_create (struct stn_hft_mkt_channel_public_s* mkt_chnl_public, 
										   struct stn_mkt_channel_sym_filters_s* sym_filter_arr, 
										   uint16_t num_sym_filters,
										   MKT_MSG_CBFN user_provided_cbfn,
										   uint8_t* user_cbfn_handle,
										   void** opaque_hdl)
{
	struct _stn_hft_mkt_channel_handle_private_s *_mkt_chnl_hdl_private =0;
	int node_id = -1;

	if (STN_ERRNO_SUCCESS != __stn_hft_mkt_data_channel_init_common(mkt_chnl_public,&_mkt_chnl_hdl_private))
		return STN_ERRNO_HFT_MKT_CHNL_INIT_FAIL;

	_mkt_chnl_hdl_private->channel_type = mkt_chnl_public->channel_type;
	_mkt_chnl_hdl_private->decompress_cbfn = user_provided_cbfn;
	_mkt_chnl_hdl_private->mkt_chnl_rng_buf_write_index = 0;
	_mkt_chnl_hdl_private->mkt_chnl_rng_buf_read_index = 0;

	// setup the return opaque handle
	*opaque_hdl = _mkt_chnl_hdl_private ;

	/* Create a datagram socket on which to receive. */
	_mkt_chnl_hdl_private->sd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(_mkt_chnl_hdl_private->sd < 0)
	{
		perror("Opening datagram socket error");
		return STN_ERRNO_HFT_SOCKET_CREATION_FAIL;
	}
	else
		console_log_write("%s:%d Opening datagram socket....OK.\n",__FILE__,__LINE__);

	/* Enable SO_REUSEADDR to allow multiple instances of this application 
	   to receive copies of the multicast datagrams. 
	  */
#if 0
	if (mkt_chnl_public->reuse)
	{
		int reuse = 1;
		if(setsockopt(_mkt_chnl_hdl_private->sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
		{
			perror("Setting SO_REUSEADDR error");
			close(_mkt_chnl_hdl_private->sd);
			return -1;
		}
		else
			console_log_write("%s:%d Setting SO_REUSEADDR...OK.\n",__FILE__,__LINE__);
	}
#endif

	/* Bind to the proper port number with the IP address 
	   specified as INADDR_ANY. 
	*/
	memset((char *) &(_mkt_chnl_hdl_private->local_addr), 0, sizeof(_mkt_chnl_hdl_private->local_addr));
	_mkt_chnl_hdl_private->local_addr.sin_family = AF_INET;
	_mkt_chnl_hdl_private->local_addr.sin_port = htons(mkt_chnl_public->mkt_data_port);
	_mkt_chnl_hdl_private->local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(_mkt_chnl_hdl_private->sd, (struct sockaddr*)&(_mkt_chnl_hdl_private->local_addr), sizeof(_mkt_chnl_hdl_private->local_addr)))
	{
		perror("Binding datagram socket error");
		close(_mkt_chnl_hdl_private->sd);
		return STN_ERRNO_HFT_HW_SOCKET_BIND_FAIL;
	}
	else
		console_log_write("%s:%d Binding datagram socket...OK.\n",__FILE__,__LINE__);
	 
	/* 
	   JOIN the MULTICAST group specified by mkt_chnl_public->mkt_data_addr on the local interface with IP 
	   specified by mkt_chnl_public->local_interface_ip interface. 
	   NOTE that this IP_ADD_MEMBERSHIP option must  called for each local interface over 
	   which the multicast datagrams are to be received. 
	*/
	//_mkt_chnl_hdl_private->mc_group.imr_interface.s_addr = htonl(INADDR_ANY);//inet_addr(mkt_chnl->local_interface_ip);
	_mkt_chnl_hdl_private->mc_group.imr_interface.s_addr = inet_addr(mkt_chnl_public->local_interface_ip);
	_mkt_chnl_hdl_private->mc_group.imr_multiaddr.s_addr = inet_addr(mkt_chnl_public->mkt_data_addr);
	
	if(setsockopt(_mkt_chnl_hdl_private->sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&(_mkt_chnl_hdl_private->mc_group), sizeof(_mkt_chnl_hdl_private->mc_group)) < 0)
	{
		console_log_write("%s:%d Join ERROR. Multicast group: %s, Local Interface: %s.\n",
					__FILE__,__LINE__,mkt_chnl_public->mkt_data_addr, mkt_chnl_public->local_interface_ip);
		perror("Exiting...\n");
		close(_mkt_chnl_hdl_private->sd);
		return STN_ERRNO_HFT_MCAST_MEMBERSHIP_FAIL;
	}
	else
		console_log_write("%s:%d ADD_MEMBERSHIP OK. Multicast group: %s, Local Interface: %s.\n",
		__FILE__,__LINE__, mkt_chnl_public->mkt_data_addr, mkt_chnl_public->local_interface_ip);

	/*DO:
		0. Identify the CPU node for the incoming CPU id.
		1. Allocate a 128 MB queue buffer from the HUGE PAGE for that CPU NODE 
		Each index location will be of size = struct __hft_channel_rng_buf_entry_s. i.e. about 1528 bytes..
		assign this memory to (*mkt_chnl_hdl_private)->channel_data_hp_buffer;
	*/
	node_id = __stn_hft_get_numa_node(_mkt_chnl_hdl_private->mkt_channel_public.recv_cpu_id);
	if(-1 == node_id)
		{
		console_log_write("%s:%d Error: Invalid NUMA Node, Exit\n",__FILE__,__LINE__);
		return PAX_ERRNO_PS_NUMA_CPUID_INVALID;
		}
	_mkt_chnl_hdl_private->channel_data_hp_buffer = __stn_numa_node_allocate_memory (node_id, STB_HFT_CHNL_HP_BUFFER_SIZE_MAX, NUMA_PRIVATE);

	if(0 == _mkt_chnl_hdl_private->channel_data_hp_buffer)
		{
		console_log_write("%s:%d Error: Huge page allocation failed, Exit\n",__FILE__,__LINE__);
		return PAX_ERRNO_PS_MAP_FULL;
		}

	// Initialize a SEMAPHORE for the mkt data LL recv thread
	sem_init(&(_mkt_chnl_hdl_private->start_mkt_data_sema),0,0);

	// now create the LL recv thread
	pthread_create(&(_mkt_chnl_hdl_private->mkt_data_run_thr_hdl),
					NULL,
					__stn_hft_mkt_data_channel_thr_run,
					(void*)_mkt_chnl_hdl_private);

	return STN_ERRNO_SUCCESS;
}


int stn_hft_mkt_data_channel_start (void* hdl)
{
	struct _stn_hft_mkt_channel_handle_private_s *_mkt_chnl_hdl = (struct _stn_hft_mkt_channel_handle_private_s *)hdl;
	
	console_log_write ("%s:%d starting hardware mkt channel. posting h/w signal..\n",__FILE__,__LINE__);

	sem_post(&(_mkt_chnl_hdl->start_mkt_data_sema));
}


/*EXPORT this to return the next packet out of the buffer*/
int stn_hft_mkt_data_channel_get_next_msg(void* hdl,unsigned char** pkt, unsigned int *data_len)
{
	struct _stn_hft_mkt_channel_handle_private_s *_mkt_chnl_hdl = (struct _stn_hft_mkt_channel_handle_private_s *)hdl;

	if(0 == _mkt_chnl_hdl->channel_data_hp_buffer)
		return STN_ERRNO_MALLOC_FAIL;
	
	if(1 == _mkt_chnl_hdl->quit)
		return STN_ERRNO_FAIL;

	if(_mkt_chnl_hdl->mkt_chnl_rng_buf_read_index < _mkt_chnl_hdl->mkt_chnl_rng_buf_write_index)
		{
		struct __hft_channel_rng_buf_entry_s* rng_buf_entry_to_read = 0;
		struct __hft_channel_rng_buf_entry_s* rng_buf_entry_first = (struct __hft_channel_rng_buf_entry_s*) &_mkt_chnl_hdl->channel_data_hp_buffer[0];
		unsigned long _read_idx = _mkt_chnl_hdl->mkt_chnl_rng_buf_read_index % STN_HFT_CHNL_HP_BUFFER_CNT_MAX;
			
		rng_buf_entry_to_read = rng_buf_entry_first + _read_idx;
			
		*pkt = (unsigned char*)&rng_buf_entry_to_read->data[0];
		*data_len = rng_buf_entry_to_read->data_length;

		// ring buffer increment atomic : 
		__sync_add_and_fetch(&_mkt_chnl_hdl->mkt_chnl_rng_buf_read_index,1);

		return STN_ERRNO_SUCCESS;
		}
	*data_len = 0;
	return STN_ERRNO_NODATARECV;
}



int __stn_hft_mkt_data_channel_thr_run (void* hdl)
{
	struct _stn_hft_mkt_channel_handle_private_s *_mkt_chnl_hdl = (struct _stn_hft_mkt_channel_handle_private_s *)hdl;
	fd_set read_set;
	struct timeval wait_time,start_time,end_time; 
	struct sockaddr_in source_address;	// Get a local address to capture the source of the messages... with m'cast the source would be any upstream m'cast DR
	int msg_bf_sz = STN_HFT_CHNL_MSG_SIZE_MAX;
	int msg_bytes_read = 0, rval =0;
	int source_addr_sz = sizeof(source_address);
	unsigned long time_outs =0;

	unsigned char msg_buff[STN_HFT_CHNL_MSG_SIZE_MAX];

	struct __hft_channel_rng_buf_entry_s *rng_buf_entry_first = 0;
	struct __hft_channel_rng_buf_entry_s *rng_buf_entry_to_write = 0;
	unsigned long _write_idx = 0;


	
	// set up dropped message count for this thread and its market channel handler
	_mkt_chnl_hdl->mkt_channel_public.msg_drop_count = 0;

	//set the tread affinit
	if(-1 == __stn_hft_set_thread_affinity(_mkt_chnl_hdl->mkt_channel_public.recv_cpu_id))
		{
		console_log_write("%s:%d Error: Thread affinity failed.\n",__FILE__,__LINE__);
		return 0;
		}


	rng_buf_entry_first = (struct __hft_channel_rng_buf_entry_s*) &_mkt_chnl_hdl->channel_data_hp_buffer[0];

	memset((char *) &(source_address), 0, sizeof(source_address));
	
	// setup the timer for logging.. OPTIONAL..
	gettimeofday(&_mkt_chnl_hdl->start_time, NULL);

	sem_wait(&(_mkt_chnl_hdl->start_mkt_data_sema));

	if (1 == _mkt_chnl_hdl->quit)
		return 0;

	console_log_write ("%s:%d h'ware mkt channel data get running...\n",__FILE__,__LINE__);

	FD_ZERO(&read_set);
	for (;;) 
		{
		if (_mkt_chnl_hdl->quit)
			break;

		FD_SET(_mkt_chnl_hdl->sd, &read_set );
		
		/*
			- SETUP the wait time for the poll on the socket 
			- This should be changed in the BUSY WAIT driver implementations
		*/
		wait_time.tv_sec  = 0;   
		wait_time.tv_usec = STN_HFT_SOCKET_SELECT_WAIT_TIME_USEC;

#ifdef _OLD_IMPLEMENT	
		rval = select(_mkt_chnl_hdl->sd, &read_set , NULL, NULL, &wait_time);
	        
		if ( (rval > 0) && (FD_ISSET(_mkt_chnl_hdl->sd, &read_set)) ) 
#endif
			{

			// check if the buffer is enough to hold the message. If the reader is lagging behind start dropping message.
			if(_mkt_chnl_hdl->mkt_chnl_rng_buf_write_index - _mkt_chnl_hdl->mkt_chnl_rng_buf_read_index < STN_HFT_CHNL_HP_BUFFER_CNT_MAX )
				{
				// Compute the ring modulo from continous running number.
				_write_idx = _mkt_chnl_hdl->mkt_chnl_rng_buf_write_index % STN_HFT_CHNL_HP_BUFFER_CNT_MAX;
				rng_buf_entry_to_write = rng_buf_entry_first + _write_idx;
				
				// CHANGE the msg_bf to CURRENT WRITE index in the HUGE PAGE buffer. i.e. struct __hft_channel_rng_buf_entry_s->data
				// CHANGE msg_bf_sz to sizeof(struct __hft_channel_rng_buf_entry_s->data)
				if ( (rng_buf_entry_to_write->data_length = recvfrom(_mkt_chnl_hdl->sd, &rng_buf_entry_to_write->data[0], msg_bf_sz, 0, 
						(struct sockaddr*)&(source_address),&source_addr_sz)) < 0 ) 
					{
					perror ("mk_chnl receive-from last packet error...\n");	    
					} 
				else 
					{

					_mkt_chnl_hdl->total_bytes_recvd += rng_buf_entry_to_write->data_length;
					if (_mkt_chnl_hdl->mkt_channel_public.log_stats_flag)
						{
						// log only if the log flag is ON... else just process the message...
						_mkt_chnl_hdl->log_mkt_msg_stats_fn(_mkt_chnl_hdl,rng_buf_entry_to_write->data,rng_buf_entry_to_write->data_length);
						//bzero(msg_bf,msg_bf_sz);
						}
					
					// ring buffer  increment by handled thro local variable
					//__sync_add_and_fetch(&_mkt_chnl_hdl->mkt_chnl_rng_buf_write_index,1);
					++_mkt_chnl_hdl->mkt_chnl_rng_buf_write_index;

					}
				}
			else		// drop the packet
				{
				// recieve and drop the packet
				++_mkt_chnl_hdl->mkt_channel_public.msg_drop_count; // increment the drop count
				recvfrom(_mkt_chnl_hdl->sd, &msg_buff, msg_bf_sz, 0, 
						(struct sockaddr*)&(source_address),&source_addr_sz);
				}

	    	} 
#ifdef _OLD_IMPLEMENT			
	    else if ( rval < 0 ) 
		   	{
			perror("ERROR: fd_set hw_slct error.");
			break;
	    	} 
        else if ( rval == 0 ) 
		   	{
			_mkt_chnl_hdl->time_outs++;
			if (_mkt_chnl_hdl->mkt_channel_public.log_stats_flag)
				{
				gettimeofday(&_mkt_chnl_hdl->end_time, NULL);
				if ((_mkt_chnl_hdl->end_time.tv_sec - _mkt_chnl_hdl->start_time.tv_sec) > _mkt_chnl_hdl->mkt_channel_public.log_interval)
					_mkt_chnl_hdl->log_mkt_msg_stats_fn(_mkt_chnl_hdl,rng_buf_entry_to_write->data,rng_buf_entry_to_write->data_length);
				}
		   	}
#endif		
		}	// end of for(;;) Endless loop for recieving packets from driver

		console_log_write("%s:%d Marketdata Channel : %s:%u, processed :%u, dropped %u messages\n",
						__FILE__,__LINE__,
						_mkt_chnl_hdl->mkt_channel_public.mkt_data_addr,
						_mkt_chnl_hdl->mkt_channel_public.mkt_data_port,
						_mkt_chnl_hdl->mkt_chnl_rng_buf_write_index,
						_mkt_chnl_hdl->mkt_channel_public.msg_drop_count);
		
						
}

int stn_hft_mkt_data_channel_delete (void* hdl)
{
	struct _stn_hft_mkt_channel_handle_private_s *_mkt_chnl_hdl = (struct _stn_hft_mkt_channel_handle_private_s *)hdl;

	_mkt_chnl_hdl->quit =1; //set the flag to quit..

	sleep (3); // sleep for 3 secs before proceeding with releasing memories etc..

	console_log_write ("%s:%d deleting mkt data channel..\n",__FILE__,__LINE__);

	if (!_mkt_chnl_hdl->total_bytes_recvd)
		console_log_write ("%s:%d No data received thus far in hardware for mcast channel %s/%d: on local IP %s\n",
		__FILE__,__LINE__,_mkt_chnl_hdl->mkt_channel_public.mkt_data_addr, _mkt_chnl_hdl->mkt_channel_public.mkt_data_port, _mkt_chnl_hdl->mkt_channel_public.local_interface_ip ); 

	if (STN_HFT_MKT_CHANNEL_TYPE_MUDP == _mkt_chnl_hdl->channel_type)
	{
		if(setsockopt(_mkt_chnl_hdl->sd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *)&(_mkt_chnl_hdl->mc_group), sizeof(_mkt_chnl_hdl->mc_group)) < 0)
		{
			console_log_write("%s:%d M'CAST Group Leave ERROR. Multicast group: %s, Local Interface: %s.\n",
									__FILE__,__LINE__,
									_mkt_chnl_hdl->mkt_channel_public.mkt_data_addr, 
									_mkt_chnl_hdl->mkt_channel_public.local_interface_ip);
			perror("Exiting...\n");
			close(_mkt_chnl_hdl->sd);
			return -1;
		}else 
			console_log_write("%s:%d DROP_MEMBERSHIP OK. Leaving Multicast group: %s, Local Interface: %s.\n",
									__FILE__,__LINE__,
									_mkt_chnl_hdl->mkt_channel_public.mkt_data_addr, 
									_mkt_chnl_hdl->mkt_channel_public.local_interface_ip);
	}

	// Unmap the Huge Page buffer allocated for this channel
	__stn_numa_node_release_memory (_mkt_chnl_hdl->channel_data_hp_buffer, STB_HFT_CHNL_HP_BUFFER_SIZE_MAX);
	_mkt_chnl_hdl->channel_data_hp_buffer = 0;

	// clean up
	close(_mkt_chnl_hdl->sd);
	free (_mkt_chnl_hdl->chnnl_stats_blob);
	free (_mkt_chnl_hdl);
}



