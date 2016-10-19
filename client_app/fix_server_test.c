
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Network LLC

Author      Date				Comments
------      -----				---------
mt			10/05/2016			init rev

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
#include "stn_hft_fix_op_public.h"
#include "stn_hft_mkt_data_public.h"
#include "stn_hft_pair_strategy_public.h"

int bShutdown = 0;
int seqNum=1;
int bAppRunning = 1;



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


struct order_state_info
{
	uint8_t tag37[64]; // exchange order number
	uint8_t tag41[64]; // origclordid
	uint8_t tag61[64]; // transact time
};


/*
#comments are written with this format
# anything beside # is considered comments
name=swifttrade
support=support@swifttradenetworks.com
engine=stn
version=1.0
client=proficient
interface=10.20.201.1
multicast_ip=239.235.255.21
multicast_port=50001
fix_gw=192.168.8.11
fix_port=21456
fix_tag_91=378DDF05FE43F488
fix_tag_96=13556,13555,152900129
fix_tag_49=STNHFT
fix_tag_56=MCXTRADE
fix_tag_57=ADMIN
fix_tag_9227=700001001001110
instrument_A=9123456
instrument_B=9324399

*/

typedef struct hft_config_s
{
	char g_name[64];
	char g_support[64];
	char g_engine[64];
	char g_version[64];
	char g_client[64];
	char g_interface[64];
	char g_multicast_ip[64];
	char g_multicast_port[64];
	char g_fix_gw[64];
	char g_fix_port[64];
	char g_fix_tag_91[64];
	char g_fix_tag_96[64];
	char g_fix_tag_49[64];
	char g_fix_tag_56[64];
	char g_fix_tag_57[64];
	char g_fix_tag_9227[64];
	char g_instrument_A[64];
	char g_instrument_B[64];
	char g_fix_tag_554[64]; // old password
	char g_fix_tag_925[64]; // new passowrd
}HFT_CONFIG_T,*HFT_CONFIG_P;

HFT_CONFIG_T g_hft_config;


int hft_read_config_file()
{
	int iRet;
	FILE* config_file = 0;
	unsigned char txt[1028];
	unsigned char *pRetTxt;
	unsigned char *pszValue = 0;
	unsigned char *pszText = NULL;
	
	config_file = fopen("hft_config.cfg","r");
	if(NULL == config_file)
		{
		perror("config file open error");
		return -1;
		}

	while(1)
		{
		pszText = 0;

		if(NULL == fgets(txt,1028,config_file))
			break;

		if(txt[0] == '#') // ignore the comments
			continue;
		
		pszValue = strchr(txt,'=');

		if(pszValue == NULL)
			continue;

		pszValue ++; // increment for the =
		 
		if(strncmp(txt,"interface",9) == 0)
			pszText = g_hft_config.g_interface;
		else if(strncmp(txt,"multicast_ip",12) == 0)
			pszText = g_hft_config.g_multicast_ip;
		else if(strncmp(txt,"multicast_port",14) == 0)
			pszText = g_hft_config.g_multicast_port;
		else if(strncmp(txt,"fix_gw",6) == 0)
			pszText = g_hft_config.g_fix_gw;
		else if(strncmp(txt,"fix_port",8) == 0)
			pszText = g_hft_config.g_fix_port;
		else if (strncmp(txt,"fix_tag_91",10) == 0)
			pszText = g_hft_config.g_fix_tag_91;
		else if (strncmp(txt,"fix_tag_96",10) == 0)
			pszText = g_hft_config.g_fix_tag_96;
		else if (strncmp(txt,"fix_tag_56",10) == 0)
			pszText = g_hft_config.g_fix_tag_56;
		else if (strncmp(txt,"fix_tag_57",10) == 0)
			pszText = g_hft_config.g_fix_tag_57;
		else if (strncmp(txt,"fix_tag_9227",12) == 0)
			pszText = g_hft_config.g_fix_tag_9227;
		else if (strncmp(txt,"fix_tag_49",10) == 0)
			pszText = g_hft_config.g_fix_tag_49;
		else if (strncmp(txt,"instrument_A",12) == 0)
			pszText = g_hft_config.g_instrument_A;
		else if (strncmp(txt,"instrument_B",12) == 0)
			pszText = g_hft_config.g_instrument_B;
		else if (strncmp(txt,"password",8) == 0)
			pszText = g_hft_config.g_fix_tag_554;
		else if (strncmp(txt,"new_password",12) == 0)
			pszText = g_hft_config.g_fix_tag_925;
		else 
			pszText = NULL;
		
		if(pszText != NULL)
			{
			int len = 0;
			strcpy(pszText,pszValue);
			len = strlen(pszText);
			pszText[len-1] = 0; // set the null for last \n character
			}
		
		}
	
}


int hft_print_config_info_ask_answer()
{
	char ch[10],ch1[10];
	int ans = 0;

	printf("\nconfiguration details..\n");
	printf("name:     %s\n",g_hft_config.g_name);
	printf("support:  %s\n",g_hft_config.g_support);
	printf("engine:   %s\n",g_hft_config.g_engine);
	printf("version:  %s\n",g_hft_config.g_version);
	printf("client:   %s\n",g_hft_config.g_client);
	printf("interface: %s\n",g_hft_config.g_interface);
	if(strlen(g_hft_config.g_multicast_ip)>0)
	{
		printf("multicast ip:   %s\n",g_hft_config.g_multicast_ip);
		printf("multicast port: %s\n",g_hft_config.g_multicast_port);
	}
	
	if(strlen(g_hft_config.g_fix_gw)> 0)
	{
		printf("fix gateway:    %s\n",g_hft_config.g_fix_gw);
		printf("fix port:       %s\n\n",g_hft_config.g_fix_port);
		printf("fix_tag_91:      %s\n",g_hft_config.g_fix_tag_91);
		printf("fix_tag_96:      %s\n",g_hft_config.g_fix_tag_96);
		printf("fix_tag_49:      %s\n",g_hft_config.g_fix_tag_49);
		printf("fix_tag_56:      %s\n",g_hft_config.g_fix_tag_56);
		printf("fix_tag_57:      %s\n",g_hft_config.g_fix_tag_57);
		printf("fix_tag_9227:    %s\n",g_hft_config.g_fix_tag_9227);
		printf("instrument_A:    %s\n",g_hft_config.g_instrument_A);
		printf("instrument_B:    %s\n",g_hft_config.g_instrument_B);
		
		printf("password:        %s\n",g_hft_config.g_fix_tag_554);
		printf("new password:    %s\n",g_hft_config.g_fix_tag_925);
	}
	
	printf("Do you want to continue test?(Y/N)");
	fflush(stdin);
	scanf("%s",ch);
	
	if(ch[0] == 'y' || ch[0] == 'Y')
	{
		if(strlen(g_hft_config.g_fix_tag_925)>0)
		{
			printf("\nThe configuration has new password, this will send login request with new password for password change\n");
			printf("Do you want to send change password login request as well..?(Y/N)");
			fflush(stdin);
			scanf("%s",ch1);
			if(ch1[0] == 'Y' || ch1[0] == 'y')
			{
				ans = 1;
			}
		}
		else
			ans = 1;
	}
	
	return ans;

}


//- - - - - - - - - - - - - - - - - - - 
int hft_mcx_fix_oe_test(void *chnl_hdl)
{
	struct FIX_OE_variables_s test_order = 
	{
		.tag_34_msg_seq_num =1,
		.tag_21_floor_broker_instr = 23,
		.tag_38_order_qty = 12,
		.tag_40_order_type = 34,
		.tag_44_price = 56,
		.tag_54_side = 2,
		.tag_205_mat_day= 30
	};
	
	strcpy(test_order.tag_11_client_order_id,"ABR4540085CVB");
	strcpy(test_order.tag_48_instrument_code,"TYE56");
	strcpy(test_order.tag_55_symbol, "ZINF");
	strcpy(test_order.tag_167_instrument_type, "UIWE");
	strcpy(test_order.tag_200_mat_year_mth, "201204");

}


struct channel_trd_attribs_s

{
    struct stn_hft_FIX_op_channel_public_attrib_s* chl;
    void* chnl_hdl;
};


int fix_channel_thread (void* thread_arg)
{
	struct channel_trd_attribs_s* thrd = (struct channel_trd_attribs_s*)thread_arg;
    int buf_len, j;
    

	char gMsg[1024]={ 0x39,0x39,0x32,0x3d,0x54,0x46,0x46,0x01,//sender comp id
	0x39,0x36,0x32,0x3d,0x57,0x46,0x47,0x01,//target comp id
					0x00};

	struct stn_hft_FIX_op_channel_public_attrib_s* FIX_op_chnl_attrb =
		thrd->chl ;


	// send generic message
	strcpy(FIX_op_chnl_attrb->stn_generic_msg_bfr_arr[1].p_msg_buff,gMsg);
    
    buf_len = strlen(FIX_op_chnl_attrb->stn_generic_msg_bfr_arr[1].p_msg_buff);
    
	for (j =0; j < 1000; j++)
    {
	if ( STN_ERRNO_NODATASEND == stn_hft_FIX_op_send_generic_msg(thrd->chnl_hdl,0x30,
		FIX_op_chnl_attrb->stn_generic_msg_bfr_arr[1].p_msg_buff,
		buf_len, 0))
		printf("\n failed while sending");
       }
    


}


int hft_mcx_fix_wait_for_dnld_msg(void* chnl_hdl)
{
	uint8_t* msg;
	int msg_len;
	int iMsg = STN_ERRNO_FAIL;
	printf("waiting for DNLDCOMPLETE..\n");
	fflush(stdout);
	
	while(iMsg)
		{
		iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);

		if( STN_ERRNO_SUCCESS == iMsg)
			{
			printf("\nmain.hft Recieved Message :%d %s\n",msg_len, msg);
			if(strstr(msg, "DNLDCOMPLETE"))
				{
				printf("DNLDCOMPLETE Message Recieved..\n");
				break;
				}
			}
		else if(STN_ERRNO_FAIL == iMsg)
			{
			printf("\nFailure: Channel closure..");
			break;
			}
		sleep(1);
		}
	fflush(stdout);
return 0;
}

// - - - - - - - - - - - - - - - -
// process fix login request for the fix operation testing with exchange
int hft_mcx_fix_channel_process_login(void* chnl_hdl)
{
	int loggedIn = 0;
	uint8_t* msg;
	int msg_len;
	int iMsg = 0;

	if(STN_ERRNO_SUCCESS !=	stn_hft_FIX_op_channel_login (chnl_hdl))
		return loggedIn;

	while(!loggedIn)
	{
		while((iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len)) == STN_ERRNO_NODATARECV);
		if( STN_ERRNO_SUCCESS == iMsg)
		{
			loggedIn = 1;
		}
		else if(STN_ERRNO_FAIL == iMsg)
		{
			loggedIn = 0;
			printf("\nChannel closure..");
			break;
		}
		sleep(1);
	}
	fflush(stdout);
	return loggedIn;
}




// - - - - - - - - - - - - - - - -
// process fix order new  for the fix operation testing with exchange
int fix_op_test_process_order_new(void* chnl_hdl,struct order_state_info* ord_info)
{
	uint8_t* msg;
	int msg_len;
    char ordNum[16] = ""; 
	int iMsg = STN_ERRNO_SUCCESS;
	int j=0;

	struct FIX_OE_variables_s FIX_OE_Constants =
	{
		 .tag_34_msg_seq_num=seqNum,
		 .tag_21_floor_broker_instr='1',
		 .tag_38_order_qty = 1,
		 .tag_40_order_type= '2',
		 .tag_44_price=618075,
		 .tag_48_instrument_code="201979",
		 .tag_54_side='2',
		 .tag_9366_strat_id=20,
	};
	
	// send generic message
	bzero(ord_info->tag37, 64);
	bzero(ord_info->tag61, 64);

	sprintf(ordNum, "MCXSX%06d", seqNum);
	strcpy(FIX_OE_Constants.tag_11_client_order_id, ordNum);
	stn_hft_FIX_op_channel_send_order_new(chnl_hdl,&FIX_OE_Constants);

	strcpy(ord_info->tag41,ordNum);
	
// retrieve two message as echo server would be replying
	sleep(3);
	
	while((iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len)) == STN_ERRNO_NODATARECV);
	//		iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);

	if( STN_ERRNO_SUCCESS == iMsg)
		{
		int ordIdFound=0, transTimeFound=0;
		int num=0;
		printf("\nmain.hft Recieved ACK Message :%d %s\n",msg_len, msg);
		for( num=0; num < msg_len; num++)
		{
			if(msg[num] == '3' && msg[num+1] == '7' && msg[num+2] == '=' )
			{
				ordIdFound = 1;
				memcpy(ord_info->tag37, msg+num+3, 15);
				break;
			}
		}
		for(num=0; num<msg_len; num++)
		{
			if(msg[num] == '6' && msg[num+1] == '0' && msg[num+2] == '=')
			{
				transTimeFound = 1;
				memcpy(ord_info->tag61, msg+num+3, 17);
				ord_info->tag61[17] = 0;
				break;
			}
		}
		if(ordIdFound)
			printf("exchId = %s\n", ord_info->tag37);
		else
			printf("Order Identifier not found\n");

		if(transTimeFound)
			printf("transTime=%s\n", ord_info->tag61);
		else
			printf("tag61 not found\n");

		}
	else if(STN_ERRNO_FAIL == iMsg)
		printf("\nmain.hft stn_hft_FIX_op_channel_get_next_msg (...) failed");
	else 
		printf("received some imsg val=%d\n", iMsg);

	fflush(stdout);
	return iMsg;
}



// - - - - - - - - - - - - - - - -
// process fix order replace for the fix operation testing with exchange
int fix_op_test_process_order_replace(void* chnl_hdl,struct order_state_info* ord_info)
{
	uint8_t* msg;
	int msg_len;
 	int iMsg = 0;

	struct FIX_OR_variables_s FIX_OR_Constants =
	{
		.tag_34_msg_seq_num = seqNum+1,
		//.tag_22_alternative_SecurityID ="SECID",
		//.tag_11_client_order_id = "MCX02",
		.tag_21_floor_broker_instr = '1',
		//.tag_37_order_id ="MCXORDER00101",
		.tag_38_order_qty = 2,
		.tag_40_order_type = '2',
		//.tag_41_orig_clor_id = "MCX01",
		.tag_44_price =618075,
		 .tag_9366_strat_id=20,
		//.tag_48_instrument_code ="INST",
		//.tag_54_side ='1',
		//.tag_55_symbol = "GOOG",
		//.tag_167_instrument_type="INSTCODE",
		//.tag_200_mat_year_mth = "201203",
		//.tag_205_mat_day ='2',
	};

	FIX_OR_Constants.tag_34_msg_seq_num = seqNum+1;
	char rplNum[16] = "";
	bzero(rplNum, sizeof(rplNum));
				sprintf(rplNum, "MCXSX%06d", seqNum+1);
				
	strcpy(FIX_OR_Constants.tag_11_client_order_id, rplNum);
	strcpy(FIX_OR_Constants.tag_37_order_id, ord_info->tag37);
	strcpy(FIX_OR_Constants.tag_41_orig_clor_id, ord_info->tag41);
	printf("setting transTime=%s\n", ord_info->tag61);
	fflush(stdout);
	strcpy(FIX_OR_Constants.tag_60_message_creation_time, ord_info->tag61);
	printf("set transTime=%s\n", ord_info->tag61);
	fflush(stdout);
	stn_hft_FIX_op_channel_send_order_replace(chnl_hdl,&FIX_OR_Constants);
	printf("sent replace request\n");
	fflush(stdout); 
	// retrieve two message as echo server would be replying
	sleep(2);
	// assign it back from the replace order number
	strcpy(ord_info->tag41,rplNum);
	
	
	iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);
	if( STN_ERRNO_SUCCESS == iMsg)
		{
		printf("\nRPLD Recieved Message :%d %s\n",msg_len, msg);
		int transTimeFound = 0;
		int num = 0;
		for(num=0; num<msg_len; num++)
			{
			if(msg[num] == '6' && msg[num+1] == '0' && msg[num+2] == '=')
				{
				transTimeFound = 1;
				memcpy(ord_info->tag61, msg+num+3, 17);
				ord_info->tag61[17] = 0;
				break;
				}
		}
		if(transTimeFound)
				printf("transTime=%s\n", ord_info->tag61);
		else
				printf("transtime not found\n");
		}
	else if(STN_ERRNO_FAIL == iMsg)
		printf("\nmain.hft closure of channel");

	fflush(stdout);
	return iMsg;	
}


// need exchange id - 37, 41 and 60 to be pass along with the order
// process order cancel message for the fix operation testing
int fix_op_test_process_order_cancel(void *chnl_hdl,struct order_state_info* ord_info)
{
	
	uint8_t* msg;
	int msg_len;
	int iMsg = 0;

	struct FIX_OC_variables_s FIX_OC_Constants =
	{
		.tag_34_msg_seq_num = seqNum+2,
		.tag_9366_strat_id=20,
	};
	
	char cxlNum[16] = "";
	bzero(cxlNum, sizeof(cxlNum));
	sprintf(cxlNum, "MCXSX%06d", seqNum+2);
	strcpy(FIX_OC_Constants.tag_11_client_order_id, cxlNum);
	strcpy(FIX_OC_Constants.tag_37_order_id, ord_info->tag37);
	strcpy(FIX_OC_Constants.tag_41_orig_clor_id, ord_info->tag41);
	strcpy(FIX_OC_Constants.tag_60_message_creation_time, ord_info->tag61);
	
	stn_hft_FIX_op_channel_send_order_cancel(chnl_hdl,&FIX_OC_Constants); 
	printf("sent cxl req\n");
	fflush(stdout);
	sleep(2);
	// retrieve two message as echo server would be replying
	iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);

	if( STN_ERRNO_SUCCESS == iMsg)
		printf("\nCXLD Recieved Message :%d %s\n",msg_len, msg);
	else if(STN_ERRNO_FAIL == iMsg)
		printf("\nmain.hft stn_hft_FIX_op_channel_get_next_msg (...) failed");

	fflush(stdout);
	return iMsg;
	
}

int hft_mcx_fix_heartbeat_thread (void* hdl)
{
	while(bAppRunning == 1)
		{

		stn_hft_FIX_op_channel_send_hb(hdl);

		sleep(30);//sleep for 30 seconds
		}

	
}


int hft_mcx_fix_op_test(uint8_t* hw_iface_ip, uint8_t* exchg_FIX_gway_ip, uint16_t exchg_port)
{
	struct stn_hft_FIX_op_channel_public_attrib_s FIX_op_chnl_attrb ;
	void 	*chnl_hdl;
	uint8_t	*msg;
	int 	msg_len;
    int 	overall_loop = 1;
	int 	iMsg = 0;
	int 	loggedIn = 0;
	struct order_state_info ord_info;
	pthread_t hb_thread;

	

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


	printf("\nconfiguration details ...\ninterface: %s",hw_iface_ip);
	printf("\nfix GW: %s",exchg_FIX_gway_ip);
	printf("\nfix port: %u\n",exchg_port);
	
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
		printf("\nstn_hft_FIX_op_channel_create failed");
		return 0;
		}
		stn_hft_FIX_op_channel_start(chnl_hdl);

		for(;overall_loop > 0; overall_loop--)
			{
			// login messages
			loggedIn = hft_mcx_fix_channel_process_login(chnl_hdl);
			if(!loggedIn)
				break;

//			hft_mcx_fix_wait_for_dnld_msg(chnl_hdl);

			// start the heartbeat thread
			pthread_create(&hb_thread,
							NULL,
							hft_mcx_fix_heartbeat_thread,
							chnl_hdl);
							
/*							
			// just wait for the message to download
			while((iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len)) == STN_ERRNO_FAIL)
				{
				if(iMsg == STN_ERRNO_SUCCESS)
					{
					printf("\n%s",msg);
					fflush(stdout);
					}
				else if(iMsg == STN_ERRNO_NODATARECV)
					sleep(1);
				
				}

			iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);

			// delete the channel and come out.
*/			
			// send order new:
			// wait for the ack on the order new
			sleep(5);
			if(STN_ERRNO_FAIL == fix_op_test_process_order_new(chnl_hdl,&ord_info))
				break;
			// wait some random time
			sleep(2);

			// send order replace
			// wait for the ack on the order replace
			if(STN_ERRNO_FAIL == fix_op_test_process_order_replace(chnl_hdl,&ord_info))
				break;
			
			// wait some random time
			sleep(2);

			// send order cancel
			// wait for the ack on the order cancel.
			if(STN_ERRNO_FAIL == fix_op_test_process_order_cancel(chnl_hdl,&ord_info))
				break;

			}
		
		while(bShutdown == 0)
			{
			int iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);
			if( STN_ERRNO_SUCCESS == iMsg)
				{
				printf("\nmain.hft Recieved Message :%d %s\n",msg_len, msg);
				continue;
				}
			else if(STN_ERRNO_FAIL == iMsg)
				{
				printf("\nmain.hft stn_hft_FIX_op_channel_get_next_msg (...) failed");
				break;
				}
			sleep(2);
			}

		printf("\ndeleting channel...");
		stn_hft_FIX_op_channel_delete(chnl_hdl);
		bAppRunning = 0;
		sleep(5);
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


	hft_read_config_file();
	
	ans = hft_print_config_info_ask_answer();
	
	if(ans != 1)
		return 0;
	
	printf("\nDo you want to provide initial sequence number?(default 1)");
	scanf("%d",&seqNum);


	printf("\nInitial sequence Number:%d",seqNum);
	
	
	
/*
	while ((c = stn_getopt (argc, argv, "s:")) != -1)
	{
		switch (c)
		{

			case 's':
				seqNum = atoi(optarg);
				printf("seqNum=%d\n", seqNum);
				break;
		}
	}
*/

	signal(SIGINT,sig_handler);
	
	printf("\nCtrl+c to exit\n");

	exchg_fix_port = atoi(g_hft_config.g_fix_port);
	exchg_mcast_port = atoi(g_hft_config.g_multicast_port);
	
	hft_mcx_fix_op_test(g_hft_config.g_interface, g_hft_config.g_fix_gw, exchg_fix_port);	// -i 192.168.1.34  -g 192.168.1.230 -p 2002 -o 2
	
	
	return 0;


}



