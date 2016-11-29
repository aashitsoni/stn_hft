
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Network LLC

file name : client_app/fix_server_test.c

Author      Date				Comments
------      -----				---------
mt			10/05/2016			init rev
mt 			10/19/2016          client application changes for the login tests.

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/

#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "stn_errno.h"
#include "console_log.h"
#include "stn_hft_fix_op_public.h"
#include "stn_hft_mkt_data_public.h"
#include "stn_hft_pair_strategy_public.h"
#include "fix_server_test.h"
#include "console_log.h"


/*Global variables*/
int bShutdown = 0;
int seqNum=1;
int bAppRunning = 1;

HFT_CONFIG_T g_hft_config;
int g_logged_in;
pthread_t 	g_hb_thread = NULL;




__inline__ uint64_t rdtsc_lcl(void) {
    uint32_t lo, hi;
    __asm__ __volatile__ (
    "        xorl %%eax,%%eax \n"
    "        cpuid"      // serialize
    ::: "%rax", "%rbx", "%rcx", "%rdx");
    /* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t)hi << 32 | lo;
}


int hft_client_fix_heartbeat_thread (void* hdl)
{
	while(bAppRunning == 1)
		{

		stn_hft_FIX_op_channel_send_hb(hdl);

		sleep(30);//sleep for 30 seconds
		}

	
}

int print_stn_fix_client_header(int logged_in)
{
	printf("\n");
	if(logged_in)
		printf("    STN FIX HFT Connection (Logged into the FIX server)\n");
	else
		printf("    STN FIX HFT Connection (Not logged into the FIX server)\n");
		
	printf(" ----------------------------------------------\n");
	printf("*      FIX Gateway IP     : %s\n",g_hft_config.g_fix_gw);
	printf("*      FIX Gateway Port   : %s\n",g_hft_config.g_fix_port);
	printf("*      Client Interface   : %s\n",g_hft_config.g_interface);
	printf("\n");
	return 0;
}

int process_for_logged_out_screen(void *chnl_hdl)
{

	int choice = 0;
	print_stn_fix_client_header(0);
	
	printf("  [1] Login\n");
	printf(" [11] Configuration\n");
	printf(" [12] Exit\n");
	printf(" Enter Choice:");
	scanf("%d",&choice);
	
	if(choice == 1) // do the login
		{
		if(g_logged_in == 0)
			{
			g_logged_in = hft_client_fix_channel_process_login(chnl_hdl);
			//once logged in set the heartbeat thread
			if(g_logged_in == 1)
				{
					pthread_create(&g_hb_thread, NULL, hft_client_fix_heartbeat_thread,chnl_hdl);
				}
			}
		}
	else if(choice == 11) // manage configuration - here it is display configuration
		{
		hft_print_config();
		}
	else if(choice == 12) // exit out of the program
		{
		return -1;
		}
	return 1;
	
}



// process once we log in to the system.
int process_for_logged_in_screen(void *chnl_hdl)
{
	int choice = 0;
	print_stn_fix_client_header(1);
	
	printf("  [1] Logout\n");
	printf("  [2] New Order\n");
	printf("  [3] Change Order\n");
	printf("  [4] Cancel Order\n");
	printf("  [5] List Order\n");
	printf(" [11] Configuration\n");
	printf(" [12] Exit\n");
	printf(" Enter Choice:");
	scanf("%d",&choice);
	
	switch(choice)
		{
		case 1:
			//TODO : send logout message
			break;
		case 2:
			//TODO : Send New order message
			break;
		case 3:
			//TODO : Order Replace
			break;
			
		case 4:
			//TODO : Order Cancel
			break;
			
		case 5:
			//TODO : List Order
			break;
		
		case 11:
			// TODO : Configuration details
			break;
			
		case 12:
			// exit the application. make sure you logoff before you exit
			return -1;
			
			
		}
		
	return 1;
		
}

int hft_client_fix_process(uint8_t* hw_iface_ip, uint8_t* exchg_FIX_gway_ip, uint16_t exchg_port)
{
	struct stn_hft_FIX_op_channel_public_attrib_s FIX_op_chnl_attrb ;
	void 	*chnl_hdl;
	uint8_t	*msg;
	int 	msg_len;
    int 	overall_loop = 1;
	int 	iMsg = 0;
	struct order_state_info ord_info;
	
	int user_input = 0;

	

	// SETUP the session constants for this channel
	struct FIX_session_constants_s FIX_session_constants = 
	{
		.tag_98_encrpt_methd = 0,			
		.tag_141_Reset_seq_num = 'Y',
		.tag_204_cust_or_firm = 1,
		.tag_59_time_in_force = 0,			
		.tag_34_msg_seq_num=1,
	};
	
	strcpy(FIX_session_constants.tag_8_FIXString, "FIX.4.2");


	console_log_write("%s:%d configuration details ...\ninterface: %s\n",__FILE__,__LINE__,hw_iface_ip);
	console_log_write("FIX GW IP: %s\n",exchg_FIX_gway_ip);
	console_log_write("FIX port: %u\n",exchg_port);
	
	//Proficient specific values:
	bzero(FIX_session_constants.tag_91_encrpted_digest,sizeof(FIX_session_constants.tag_91_encrpted_digest));
	strcpy(FIX_session_constants.tag_91_encrpted_digest, g_hft_config.g_fix_tag_91);
	strcpy(FIX_session_constants.tag_96_raw_data, g_hft_config.g_fix_tag_96);
	strcpy(FIX_session_constants.tag_49_sender_comp_id, g_hft_config.g_fix_tag_49);
	// fix up the length field
	FIX_session_constants.tag_95_raw_data_length = strlen(FIX_session_constants.tag_96_raw_data);
	FIX_session_constants.tag_90_encrptd_digest_length = strlen(FIX_session_constants.tag_91_encrpted_digest);

	strcpy(FIX_session_constants.tag_56_target_comp_id, g_hft_config.g_fix_tag_56);
	strcpy(FIX_session_constants.tag_57_target_comp_sub_id , g_hft_config.g_fix_tag_57);
	strcpy(FIX_session_constants.mcx_tag_9227_terminal_info, g_hft_config.g_fix_tag_9227);
	strcpy(FIX_session_constants.tag_554_password,g_hft_config.g_fix_tag_554);
	strcpy(FIX_session_constants.tag_925_newpassword,g_hft_config.g_fix_tag_925);
	
	

	// SETUP the attribs for this channel
	strcpy(FIX_op_chnl_attrb.FIX_gway_addr,exchg_FIX_gway_ip);
	FIX_op_chnl_attrb.FIX_gway_port = exchg_port;
	strcpy(FIX_op_chnl_attrb.local_interface_ip,hw_iface_ip);
	FIX_op_chnl_attrb.recv_cpu_id = 0;
	FIX_op_chnl_attrb.log_stats_flag = 1; // make sure if you want to log the events
	FIX_op_chnl_attrb.fix_op_chnl_generic_bfrs_needed = 2;
    

	iMsg = stn_hft_FIX_op_channel_create (&FIX_op_chnl_attrb, 
        &FIX_session_constants, 
        seqNum++,
        &chnl_hdl);
	
	if (STN_ERRNO_SUCCESS != iMsg)
		{
		console_log_write("%s:%d stn_hft_FIX_op_channel_create failed\n",__FILE__,__LINE__);
		return 0;
		}
		
		
	iMsg = 	stn_hft_FIX_op_channel_start(chnl_hdl);
	if(STN_ERRNO_SUCCESS != iMsg)
		{
		console_log_write("%s:%d stn_hft_FIX_op_channel_start failed\n",__FILE__,__LINE__);
		return 0;
		}
		
	while(user_input != -1)
	{
		if(g_logged_in == 0)
			{
			user_input = process_for_logged_out_screen(chnl_hdl);
			}
		else
			{
			user_input = process_for_logged_in_screen(chnl_hdl);
			}
	}
		
	bAppRunning = 0; // close the application 
	bShutdown = 1; // set the shutdown value
	
	console_log_write("%s:%d deleting channel and cleaning up resources please wait...\n",__FILE__,__LINE__);
	stn_hft_FIX_op_channel_delete(chnl_hdl);
	if(g_logged_in)
		pthread_join(g_hb_thread);
}


// - - - - - - - - - - - - - - - - 

void sig_handler(int signum)
{
	if(signum == SIGINT)
		{
		printf("\nRecieve signal, terminating application");

		bShutdown = 1;
		sleep(5); // go hibernate for 5 seconds.
		
		}
	return;
}


// - - - - - - - - - - - - - - - - 
int main (int argc, char** argv)
{
	int ret =0, pme_type = -1, nic_series = -1;
	char eth_interface [256] = {0};
	int c, ops =-1;
	uint8_t exchg_mcast_ip [16] = {0}, hw_iface_ip[16] = {0}, exchg_FIX_gway_ip[16] = {0};;
	uint16_t exchg_fix_port = 0;
	uint16_t exchg_mcast_port = 0;
	int ans = 0;

	console_log_open();
	
	hft_read_config_file();
	
	ans = hft_print_config_info_ask_answer();
	
	if(ans != 1)
		return 0;
	
	printf("\nDo you want to provide initial sequence number?(default 0)");
	scanf("%d",&seqNum);


	console_log_write("Initial sequence Number:%d\n",seqNum);
	signal(SIGINT,sig_handler);

	exchg_fix_port = atoi(g_hft_config.g_fix_port);
	exchg_mcast_port = atoi(g_hft_config.g_multicast_port);
	
	hft_client_fix_process(g_hft_config.g_interface, g_hft_config.g_fix_gw, exchg_fix_port);	// -i 192.168.1.34  -g 192.168.1.230 -p 2002 -o 2
	
	console_log_close();
	
	return 0;


}



