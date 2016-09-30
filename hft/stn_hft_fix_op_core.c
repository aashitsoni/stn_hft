/******************************************************************************************************************

 Author      Date                Comments
 ------      -----               ---------
   mt       01/11/2012          init rev
   mt       01/19/2012          removed tag 22 from order replace
   mt 	    02/04/2014		updates for the removal of tag 91 and tag 90 based on the lenght supplied.

*******************************************************************************************************************/
#include <sys/mman.h>
#include <numa.h>

#include "stn_hft_fix_op_private.h"
#include "stn_numa_impl.h"
#include "stn_sdk_version.h"
#include <time.h>





//http://fixprotocol.org/FIXimate3.0/en/FIX.4.2/fields_sorted_by_tagnum.html


//----------------------------------------------------------------------------------------------------------------------
// fwd dcecls..
int __stn_hft_fix_op_channel_thr_run (void* hdl);
int __stn_hft_pair_strategy_master_thread_run(void*);

void stn_hft_FIX_op_channel_setup_generic_msg_buffer(struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private,
    struct stn_hft_FIX_op_channel_public_attrib_s* FIX_op_channel_attribs);



//----------------------------------------------------------------------------------------------------------------------
__inline__ uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ (
    "        xorl %%eax,%%eax \n"
    "        cpuid"      // serialize
    ::: "%rax", "%rbx", "%rcx", "%rdx");
    /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t)hi << 32 | lo;
}



//----------------------------------------------------------------------------------------------------------------------
int stn_hft_FIX_op_channel_create   (struct stn_hft_FIX_op_channel_public_attrib_s* FIX_op_channel_attribs, 
                                     struct FIX_session_constants_s *FIX_session_constants,
                                     uint32_t session_seq_nmbr_seed,
                                     void** pax_hft_FIX_op_channel_handle)
{
    struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private =0;
    struct sockaddr_in local_addr;
    int node_id = -1,ret;
    int option = 1;
    char statlogFile[256];
    time_t curr_time;
    struct tm* local_time;
    
    printf("PAX HFT SDK version: %s\n", STN_SDK_VERSION_STRING);
    fflush(stdout);

    FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s*)calloc(1,sizeof(struct _stn_hft_FIX_op_channel_handle_private_s));
    memset(FIX_op_hdl_private,0,sizeof(struct _stn_hft_FIX_op_channel_handle_private_s));
    
    FIX_op_hdl_private->FIX_op_channel_public_attribs = *FIX_op_channel_attribs;
    FIX_op_hdl_private->session_constants = *FIX_session_constants;
    FIX_op_hdl_private->fix_msg_seq_num = session_seq_nmbr_seed;

    if(FIX_op_channel_attribs->log_stats_flag == 1)
        {
        time(&curr_time);
        local_time  = localtime(&curr_time);
        sprintf(statlogFile,LOG_FILE_NAME,local_time->tm_mon+1,local_time->tm_mday,local_time->tm_year-100);
        }
    
    // setup the return opaque handle
    *pax_hft_FIX_op_channel_handle = FIX_op_hdl_private ;

    /* Create a datagram socket on which to receive. */
    FIX_op_hdl_private->sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(FIX_op_hdl_private->sd < 0)
    {
        perror("Opening TCP socket error\n");
        return STN_ERRNO_FAIL;
    }
    else
        printf("FIX g'way socket....OK.\n");

    /* - - - - 
       Look at this link: http://codingrelic.geekhold.com/2009/10/code-snippet-sobindtodevice.html
       This could be used to send out a particular interface? alternately use socket-> bind()
       - - - - 
    */
    memset(&local_addr,0,sizeof( struct sockaddr_in));

    
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(0);  // Random port
    
    local_addr.sin_addr.s_addr = inet_addr(FIX_op_hdl_private->FIX_op_channel_public_attribs.local_interface_ip);

    ret = bind(FIX_op_hdl_private->sd,(struct sockaddr*)&local_addr,sizeof(struct sockaddr));
    
    if(-1 == ret )
        {
        perror("\nError binding socket");
        return STN_ERRNO_FAIL;
        }
    
    printf("FIX g'way socket binding ....OK.\n");
    fflush(stdout);
    /* 
     Enable tcp_low_latency
     For our NIC's we do not need this, since we will PIO and do a BUSY POLL anyway for the recv buffers
     llm_flag = 1;
     ret = setsockopt( FIX_op_hdl_private->sd, IPPROTO_TCP, tcp_low_latency, (char *)&flag, sizeof(flag) );
    */

    memset((char *) &(FIX_op_hdl_private->svr_addr), 0, sizeof(FIX_op_hdl_private->svr_addr));
    FIX_op_hdl_private->svr_addr.sin_family = AF_INET;
    FIX_op_hdl_private->svr_addr.sin_port = htons(FIX_op_hdl_private->FIX_op_channel_public_attribs.FIX_gway_port);
    FIX_op_hdl_private->svr_addr.sin_addr.s_addr = inet_addr(FIX_op_hdl_private->FIX_op_channel_public_attribs.FIX_gway_addr);;

    if (connect(FIX_op_hdl_private->sd, (struct sockaddr*)&(FIX_op_hdl_private->svr_addr), sizeof(FIX_op_hdl_private->svr_addr)) < 0)
    {
        perror("\nFIX g'way connect error");
        return STN_ERRNO_FAIL;
    }

    printf("FIX g'way socket g'way connect....OK.\n");
    fflush(stdout);

    if(setsockopt( FIX_op_hdl_private->sd, IPPROTO_TCP,TCP_NODELAY,&option,sizeof(option))<0)
        {
        perror("\nSO_NODELAY could not be set");
        }
    printf("FIX g'way h'ware sockopt....OK.\n");
    fflush(stdout);
    /*DO:
        0. Identify the CPU node for the incoming CPU id.
        1. Allocate a 128 MB queue buffer from the HUGE PAGE for that CPU NODE 
        Each index location will be of size = struct __hft_channel_rng_buf_entry_s. i.e. about 1528 bytes..
        assign this memory to (*mkt_chnl_hdl_private)->channel_data_recv_bfr_hp;
    */
    node_id = __stn_hft_get_numa_node(FIX_op_hdl_private->FIX_op_channel_public_attribs.recv_cpu_id);
    if(-1 == node_id)
        {
        printf("\nError: Invalid NUMA Node, Exit.");
        return PAX_ERRNO_PS_NUMA_CPUID_INVALID;
        }
    FIX_op_hdl_private->channel_data_recv_bfr_hp = __stn_numa_node_allocate_memory (node_id, STB_HFT_CHNL_HP_BUFFER_SIZE_MAX, NUMA_PRIVATE);

    if(0 == FIX_op_hdl_private->channel_data_recv_bfr_hp)
        {
        printf("\nError: Huge page allocation failed, Exit.");
        return PAX_ERRNO_NUMA_HUGE_PAGE_ALLOC_FAIL;
        }
    
    printf("FIX NUMA mmap....OK.\n");
    fflush(stdout);

    FIX_op_hdl_private->generic_msg_bfr_segment_sz_total = 2*1024*1024;// TWOMB
    FIX_op_hdl_private->generic_msg_bfr_segment_hp = __stn_numa_node_allocate_memory (node_id, FIX_op_hdl_private->generic_msg_bfr_segment_sz_total, NUMA_PRIVATE);

    if(0 == FIX_op_hdl_private->generic_msg_bfr_segment_hp)
        {
        printf("\nError: Format Msg Huge page allocation failed for generic message, Exit.");
        return PAX_ERRNO_NUMA_HUGE_PAGE_ALLOC_FAIL;
        }

    // set up the generic msg infrastructure
    stn_hft_FIX_op_channel_setup_generic_msg_buffer(FIX_op_hdl_private,FIX_op_channel_attribs);
    
    printf("FIX generic msg h'ware buffers setup....OK.\n");
    fflush(stdout);

    FIX_op_hdl_private->formattedMsg = (char*)malloc(2048);
    if(0 == FIX_op_hdl_private->formattedMsg)
        {
        printf("\nError: error allocation local heap vars");
        return STN_ERRNO_FAIL;
        }

    if(FIX_op_channel_attribs->log_stats_flag == 1)
        {

        FIX_op_hdl_private->fp_stats_log = fopen(statlogFile,"a+");
        if(NULL == FIX_op_hdl_private->fp_stats_log)
            {
            printf("\nError: Could not open log file : %s",statlogFile);
            FIX_op_hdl_private->fp_stats_log = 0;
            }
        }
        

    // Initialize a SEMAPHORE for the mkt data LL recv thread
    sem_init(&(FIX_op_hdl_private->start_FIX_op_recv_sema),0,0);

    printf("FIX h'ware sem init....OK.\n");
    fflush(stdout);

    // now create the LL recv thread
    pthread_create(&(FIX_op_hdl_private->fix_op_recv_run_thr_hdl),
                    NULL,
                    __stn_hft_fix_op_channel_thr_run,
                    (void*)FIX_op_hdl_private);

    printf("\nFIX OP channel create done..\n");

    // now create the Master thread for the pair strategy
    pthread_create(&(FIX_op_hdl_private->fix_pair_strategy_master_thr_hdl),
                    NULL,
                    __stn_hft_pair_strategy_master_thread_run,
                    (void*)FIX_op_hdl_private);
    fflush(stdout);

    return STN_ERRNO_SUCCESS;
}







//----------------------------------------------------------------------------------------------------------------------
int stn_hft_FIX_op_channel_start    (void* pax_hft_FIX_op_channel_handle)
{
    struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)pax_hft_FIX_op_channel_handle;
    
    printf ("Starting CN: hardware FIX Order Processing channel. Posting h/w signal..\n");

    sem_post(&(FIX_op_hdl_private->start_FIX_op_recv_sema));
}







//----------------------------------------------------------------------------------------------------------------------
int __stn_hft_fix_op_channel_thr_run (void* hdl)
{
    struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)hdl;
    fd_set read_set;
    struct timeval wait_time,start_time,end_time; 
    struct sockaddr_in source_address;  // Get a local address to capture the source of the messages... with m'cast the source would be any upstream m'cast DR
    int msg_bf_sz = STN_HFT_CHNL_MSG_SIZE_MAX;
    int msg_bytes_read = 0, rval =0;
    int source_addr_sz = sizeof(source_address);
    unsigned long time_outs =0;

    unsigned char msg_buff[STN_HFT_CHNL_MSG_SIZE_MAX];

    struct __hft_channel_rng_buf_entry_s *rng_buf_entry_first = 0;
    struct __hft_channel_rng_buf_entry_s *rng_buf_entry_to_write= 0;
    unsigned long _write_idx = 0;


        // set up dropped message count for this thread and its market channel handler
    FIX_op_hdl_private->FIX_op_channel_public_attribs.msg_drop_count = 0;

    //set the tread affinit
    if(-1 == __stn_hft_set_thread_affinity(FIX_op_hdl_private->FIX_op_channel_public_attribs.recv_cpu_id))
        {
        printf("Error: Thread affinity failed.\n");
        return 0;
        }

    rng_buf_entry_first = (struct __hft_channel_rng_buf_entry_s*) &FIX_op_hdl_private->channel_data_recv_bfr_hp[0];

    memset((char *) &(source_address), 0, sizeof(source_address));
    
    // setup the timer for logging.. OPTIONAL..
    gettimeofday(&FIX_op_hdl_private->start_time, NULL);

    sem_wait(&(FIX_op_hdl_private->start_FIX_op_recv_sema));


    if (1 == FIX_op_hdl_private->quit)
        return 0;

    printf ("CN h'ware FIX OP channel data recv running...\n");
    fflush(stdout);

    for (;;) 
        {
        if (FIX_op_hdl_private->quit)
            break;


        // check if the buffer is enough to hold the message. If the reader is lagging behind start dropping message.
        if(FIX_op_hdl_private->FIX_op_chnl_rng_buf_write_index - FIX_op_hdl_private->FIX_op_chnl_rng_buf_read_index < STN_HFT_CHNL_HP_BUFFER_CNT_MAX )
            {
            // Compute the ring modulo from continous running number.
            _write_idx = FIX_op_hdl_private->FIX_op_chnl_rng_buf_write_index % STN_HFT_CHNL_HP_BUFFER_CNT_MAX;
            rng_buf_entry_to_write = rng_buf_entry_first + _write_idx;
            
            // CHANGE the msg_bf to CURRENT WRITE index in the HUGE PAGE buffer. i.e. struct __hft_channel_rng_buf_entry_s->data
            // CHANGE msg_bf_sz to sizeof(struct __hft_channel_rng_buf_entry_s->data)
            rng_buf_entry_to_write->data_length = recv(FIX_op_hdl_private->sd, 
                                                         &rng_buf_entry_to_write->data[0], 
                                                         msg_bf_sz,0);
            
            if ( -1 == rng_buf_entry_to_write->data_length)
                {
                perror ("\nfix_op_channel,recv error");     
                break;
                } 
            else if (0 == rng_buf_entry_to_write->data_length)
                {
                printf("\nfix_op_channel,Shutdown recv from remote side");
                break;
                }
            else 
                {                
                FIX_op_hdl_private->total_bytes_recvd += rng_buf_entry_to_write->data_length;
                if (FIX_op_hdl_private->FIX_op_channel_public_attribs.log_stats_flag)
                    {
                    // log only if the log flag is ON... else just process the message...
                    //FIX_op_hdl_private->log_mkt_msg_stats_fn(FIX_op_hdl_private,rng_buf_entry_to_write->data,rng_buf_entry_to_write->data_length);
                    //bzero(msg_bf,msg_bf_sz);
                    }
                
                // ring buffer  increment by handled thro local variable
                //__sync_add_and_fetch(&FIX_op_hdl_private->FIX_op_chnl_rng_buf_write_index,1);
                ++FIX_op_hdl_private->FIX_op_chnl_rng_buf_write_index;

                }
            }
        else        // drop the packet
            {
            // recieve and drop the packet
            ++FIX_op_hdl_private->FIX_op_channel_public_attribs.msg_drop_count; // increment the drop count
            recv(FIX_op_hdl_private->sd, &msg_buff, msg_bf_sz,0);
                    
            }
        
        } 

    FIX_op_hdl_private->quit = 1;

        
                        
}







//----------------------------------------------------------------------------------------------------------------------
void stn_hft_FIX_op_channel_setup_generic_msg_buffer(struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private,
    struct stn_hft_FIX_op_channel_public_attrib_s* FIX_op_channel_attribs)
{

    uint8_t loop;
    uint8_t *pNextBuffer = 0;


    // initialize the generic message buffer and hand it off to client
    // allowed per thread size
    FIX_op_hdl_private->FIX_generic_msg_bfr_area_sz = 4*1024;  // give 4 kb to each thread
    // reserver size
    FIX_op_hdl_private->FIX_generic_msg_bfr_area_reserved_sz = 128; // 128 bytes reserver for the formatting FIX header

    // allowed size for the thread message 
    FIX_op_hdl_private->FIX_generic_msg_bfr_area_allowed_sz = FIX_op_hdl_private->FIX_generic_msg_bfr_area_sz - 
                                                              FIX_op_hdl_private->FIX_generic_msg_bfr_area_reserved_sz;
    
    

    if(FIX_op_channel_attribs->fix_op_chnl_generic_bfrs_needed > FIX_OP_CHNL_MAX_THRDS)
       FIX_op_channel_attribs->fix_op_chnl_generic_bfrs_needed = FIX_OP_CHNL_MAX_THRDS;


    pNextBuffer =  FIX_op_hdl_private->generic_msg_bfr_segment_hp;
    
    for(loop = 0; loop < FIX_op_hdl_private->FIX_op_channel_public_attribs.fix_op_chnl_generic_bfrs_needed;loop++)
        {
        

        FIX_op_channel_attribs->stn_generic_msg_bfr_arr[loop].msg_sz = FIX_op_hdl_private->FIX_generic_msg_bfr_area_allowed_sz;

        FIX_op_channel_attribs->stn_generic_msg_bfr_arr[loop].p_msg_buff = 
                                        pNextBuffer + FIX_op_hdl_private->FIX_generic_msg_bfr_area_reserved_sz;

        FIX_op_channel_attribs->stn_generic_msg_bfr_arr[loop].buffer_id = (uint64_t)
                                FIX_op_channel_attribs->stn_generic_msg_bfr_arr[loop].p_msg_buff;

        pNextBuffer += FIX_op_hdl_private->FIX_generic_msg_bfr_area_sz; // lets get to 
        
        }

    FIX_op_hdl_private->FIX_msg_hdr_fixed_length = FIX_MSG_HDR_LEN + 
                                TAG_INIT_LEN + // 4 lenght for  sender id sign 
                                TAG_INIT_LEN + // 4 lenght byte for target id signature
                                TAG_INIT_LEN + // 4 length bytes for sequence number signature
                                strlen(FIX_op_hdl_private->session_constants.tag_49_sender_comp_id)+
                                strlen(FIX_op_hdl_private->session_constants.tag_56_target_comp_id);

}








//---------------------------------------------------------------------------------------------------------------------- 
int stn_hft_FIX_op_channel_get_next_msg(void* pax_hft_FIX_op_channel_handle,unsigned char** msg, unsigned int *msg_len)
{
    struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)pax_hft_FIX_op_channel_handle;

    if(0 == FIX_op_hdl_private->channel_data_recv_bfr_hp)
        return STN_ERRNO_MALLOC_FAIL;

    if(FIX_op_hdl_private->quit)
        return STN_ERRNO_FAIL;
    
    if(FIX_op_hdl_private->FIX_op_chnl_rng_buf_read_index < FIX_op_hdl_private->FIX_op_chnl_rng_buf_write_index)
        {
        struct __hft_channel_rng_buf_entry_s* rng_buf_entry_to_read = 0;
        struct __hft_channel_rng_buf_entry_s* rng_buf_entry_first = (struct __hft_channel_rng_buf_entry_s*) &FIX_op_hdl_private->channel_data_recv_bfr_hp[0];
        unsigned long _read_idx = FIX_op_hdl_private->FIX_op_chnl_rng_buf_read_index % STN_HFT_CHNL_HP_BUFFER_CNT_MAX;
            
        rng_buf_entry_to_read = rng_buf_entry_first + _read_idx;
            
        *msg = (unsigned char*)&rng_buf_entry_to_read->data[0];
        *msg_len = rng_buf_entry_to_read->data_length;

        // ring buffer increment atomic : 
        __sync_add_and_fetch(&FIX_op_hdl_private->FIX_op_chnl_rng_buf_read_index,1);

        return STN_ERRNO_SUCCESS;
        }
    *msg_len = 0;
    return STN_ERRNO_NODATARECV;
}






//----------------------------------------------------------------------------------------------------------------------
int stn_hft_FIX_op_channel_delete   (void* pax_hft_FIX_op_channel_handle)
{
    struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)pax_hft_FIX_op_channel_handle;

    FIX_op_hdl_private ->quit =1; //set the flag to quit..

    sleep (3); // sleep for 3 secs before proceeding with releasing memories etc..

    printf ("\nDeleting FIX OP channel..\n");
    fflush(stdout);

    /*clear the format msg buffer*/
    if(FIX_op_hdl_private->formattedMsg)
        free(FIX_op_hdl_private->formattedMsg);
    
    if(FIX_op_hdl_private->fp_stats_log && 
        FIX_op_hdl_private->FIX_op_channel_public_attribs.log_stats_flag == 1)
        fclose(FIX_op_hdl_private->fp_stats_log);
    
    

    if (!FIX_op_hdl_private->total_bytes_recvd)
        printf ("\nOops! No data received thus far in hardware for FIX channel %s/%d: on local IP %s\n",
                                                                    FIX_op_hdl_private->FIX_op_channel_public_attribs.FIX_gway_addr, 
                                                                    FIX_op_hdl_private->FIX_op_channel_public_attribs.FIX_gway_port, 
                                                                    FIX_op_hdl_private->FIX_op_channel_public_attribs.local_interface_ip ); 
    else
        printf("\nFIX OP Channel Delete Stats: %s:%u:%s, processed :%u, dropped %u messages\n",
                        FIX_op_hdl_private->FIX_op_channel_public_attribs.FIX_gway_addr,
                        FIX_op_hdl_private->FIX_op_channel_public_attribs.FIX_gway_port,
                        FIX_op_hdl_private->FIX_op_channel_public_attribs.local_interface_ip,
                        (FIX_op_hdl_private->FIX_op_chnl_rng_buf_write_index +1),
                        FIX_op_hdl_private->FIX_op_channel_public_attribs.msg_drop_count);
        
    fflush(stdout);

    // Unmap the Huge Page buffer allocated for this channel    
    __stn_numa_node_release_memory (FIX_op_hdl_private->channel_data_recv_bfr_hp, STB_HFT_CHNL_HP_BUFFER_SIZE_MAX);
    FIX_op_hdl_private->channel_data_recv_bfr_hp = 0;

    // clean up
    close(FIX_op_hdl_private->sd);
    free (FIX_op_hdl_private);
}



// cloning PAX handle

//----------------------------------------------------------------------------------------------------------------------
struct _stn_hft_FIX_op_channel_handle_clone_s* __stn_fix_create_clone_private_handle(void* FIX_chnl_handle)
{

    struct _stn_hft_FIX_op_channel_handle_clone_s* clone = NULL;

    clone = malloc(sizeof(struct _stn_hft_FIX_op_channel_handle_clone_s));
    if(0 == clone)
        return NULL;

    memset(clone,0,sizeof(struct _stn_hft_FIX_op_channel_handle_clone_s));

    clone->FIX_clone_chnl_rng_buf_read_index = 0;
    clone->FIX_real_handle = (struct _stn_hft_FIX_op_channel_handle_private_s*)FIX_chnl_handle;
    return clone;
}


//----------------------------------------------------------------------------------------------------------------------
int __stn_fix_destroy_clone_private_handle  (struct _stn_hft_FIX_op_channel_handle_clone_s* FIX_clone_handle)
{
    if(FIX_clone_handle = 0)
        free(FIX_clone_handle);
    return STN_ERRNO_SUCCESS;

}





/*
	prototype : __stn_hft_FIX_clone_chnl_get_next_msg
	Description: it has clone handle for the FIX socket connection to the exchange. which is used to retrieve the socket incoming
				 message.
				 

*/
int __stn_hft_FIX_clone_chnl_get_next_msg (struct _stn_hft_FIX_op_channel_handle_clone_s* FIX_clone_handle,
                                                unsigned char** msg, 
                                                unsigned int *msg_len)
{

    struct _stn_hft_FIX_op_channel_handle_private_s *FIX_op_hdl_private = (struct _stn_hft_FIX_op_channel_handle_private_s *)
                                                            FIX_clone_handle->FIX_real_handle;

    if(0 == FIX_op_hdl_private->channel_data_recv_bfr_hp)
        return STN_ERRNO_MALLOC_FAIL;


    if(FIX_op_hdl_private->quit)
        return STN_ERRNO_FAIL;

    if(FIX_clone_handle->FIX_clone_chnl_rng_buf_read_index < FIX_op_hdl_private->FIX_op_chnl_rng_buf_write_index)
        {
        struct __hft_channel_rng_buf_entry_s* rng_buf_entry_to_read = 0;
        struct __hft_channel_rng_buf_entry_s* rng_buf_entry_first = (struct __hft_channel_rng_buf_entry_s*) &FIX_op_hdl_private->channel_data_recv_bfr_hp[0];
        unsigned long _read_idx = FIX_clone_handle->FIX_clone_chnl_rng_buf_read_index % STN_HFT_CHNL_HP_BUFFER_CNT_MAX;
            
        rng_buf_entry_to_read = rng_buf_entry_first + _read_idx;
            
        *msg = (unsigned char*)&rng_buf_entry_to_read->data[0];
        *msg_len = rng_buf_entry_to_read->data_length;

        // ring buffer increment atomic : 
        __sync_add_and_fetch(&FIX_clone_handle->FIX_clone_chnl_rng_buf_read_index,1);

        return STN_ERRNO_SUCCESS;
        }
    *msg_len = 0;
    return STN_ERRNO_NODATARECV;
}




