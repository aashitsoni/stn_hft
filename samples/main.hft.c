
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Network LLC

Author      Date				Comments
------      -----				---------
mt			01/11/2012			init rev
mt			01/11/2012			mcast data recv from hardware sample with callback
mt			01/21/2012			mcast data recv from hardware sample with Huge Page buffer
mi			03/27/2012			MCX FIX order processing

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
	char g_fix_tag_554[64];
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
		else if (strncmp(txt,"fix_tag_554",11) == 0)
			pszText = g_hft_config.g_fix_tag_554;
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
			printf("\nmain.hft Recieved Message :%d %s\n",msg_len, msg);
			//if(strstr(msg, "96=-1"))
			if(strstr(msg, "35=A")) // successful logon response
			{
				loggedIn=1;
				printf("successfully logged in..\n");
				break;
			}
			if(strstr(msg,"35=5")) // unsucessful logon message
				{
				loggedIn=-1;
				printf("login request recieved unsuccessful response\n");
				break;
				}
			else
				{
				printf("login request failed..\n");
				loggedIn = 1;
				}
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
		.tag_141_Reset_seq_num = 'N',
		.tag_204_cust_or_firm = 1,
		.tag_59_time_in_force = 0,			
		.tag_34_msg_seq_num=15,
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

#if MCX_TRADING

			hft_mcx_fix_wait_for_dnld_msg(chnl_hdl);

			// start the heartbeat thread
			pthread_create(&hb_thread,
							NULL,
							hft_mcx_fix_heartbeat_thread,
							chnl_hdl);

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
#endif

			iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);

			// delete the channel and come out.
			
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



//--test two pair strategies
int hft_mcx_fix_multi_pair_test(uint8_t* hw_iface_ip, uint8_t* exchg_FIX_gway_ip, uint16_t exchg_port)
{
	struct stn_hft_FIX_op_channel_public_attrib_s FIX_op_chnl_attrb ;
	void *chnl_hdl,*pair_hdl1, *pair_hdl2;
	uint8_t* msg;
	int msg_len,iReturn;
	pthread_t	sender_thread;
	struct channel_trd_attribs_s thrd;

	struct stn_hft_pair_strategy_attrib_s FIX_pair_chnl_attrib1, FIX_pair_chnl_attrib2;
	
		
	char msg_p[1024]={ 0x39,0x39,0x32,0x3d,0x54,0x46,0x01,//random tag
	 0x39,0x36,0x39,0x3d,0x59,0x47,0x46,0x01,//random tag
					0x00};
	
	// SETUP the session constants for this channel
	struct FIX_session_constants_s FIX_session_constants = 
	{
		.tag_90_encrptd_digest_length = 32,
                .tag_95_raw_data_length = 21,
                .tag_98_encrpt_methd = 0,
                .tag_141_Reset_seq_num = 'N',
                .tag_204_cust_or_firm = 1,
                .tag_59_time_in_force = 0,
                .tag_34_msg_seq_num=15,
	};


	FIX_pair_chnl_attrib1.x = 1;
	FIX_pair_chnl_attrib1.y = 25;
	FIX_pair_chnl_attrib1.Price_B = 618000;
	FIX_pair_chnl_attrib1.qty_A = 100;
	FIX_pair_chnl_attrib1.qty_B = 100;
	FIX_pair_chnl_attrib1.order_size = 1;
	FIX_pair_chnl_attrib1.net_quantity = 345234;
	FIX_pair_chnl_attrib1.direction = BUY_A_SELL_B;
	FIX_pair_chnl_attrib1.lot_A = 1;
	FIX_pair_chnl_attrib1.lot_B = 1;
	FIX_pair_chnl_attrib1.tick_A = 25;
	FIX_pair_chnl_attrib1.tick_B = 25;
	FIX_pair_chnl_attrib1.strat_id = 20;
	
	FIX_pair_chnl_attrib2.x = 1;
	FIX_pair_chnl_attrib2.y = 25;
	FIX_pair_chnl_attrib2.Price_B = 618050;
	FIX_pair_chnl_attrib2.qty_A = 100;
	FIX_pair_chnl_attrib2.qty_B = 100;
	FIX_pair_chnl_attrib2.order_size = 1;
	FIX_pair_chnl_attrib2.net_quantity = 345234;
	FIX_pair_chnl_attrib2.direction = SELL_A_BUY_B;
	FIX_pair_chnl_attrib2.lot_A = 1;
	FIX_pair_chnl_attrib2.lot_B = 1;
	FIX_pair_chnl_attrib2.tick_A = 25;
	FIX_pair_chnl_attrib2.tick_B = 25;
	FIX_pair_chnl_attrib2.strat_id = 20;


	strcpy(FIX_pair_chnl_attrib1.InstrumentCode_A,g_hft_config.g_instrument_A);
	strcpy(FIX_pair_chnl_attrib2.InstrumentCode_A,g_hft_config.g_instrument_A);
	strcpy(FIX_pair_chnl_attrib1.InstrumentCode_B,g_hft_config.g_instrument_B);
	strcpy(FIX_pair_chnl_attrib2.InstrumentCode_B,g_hft_config.g_instrument_B);
	//strcpy(FIX_pair_chnl_attrib.tag_1_account_name,"PAIR_ACCOUNT");


	strcpy(FIX_session_constants.tag_8_FIXString, "FIX.4.2");
	strcpy(FIX_session_constants.tag_91_encrpted_digest, g_hft_config.g_fix_tag_91);
	strcpy(FIX_session_constants.tag_96_raw_data, g_hft_config.g_fix_tag_96);
	strcpy(FIX_session_constants.tag_49_sender_comp_id, g_hft_config.g_fix_tag_49);
	//TODO: Proficient values:
	strcpy(FIX_session_constants.tag_56_target_comp_id, g_hft_config.g_fix_tag_56);
	strcpy(FIX_session_constants.tag_57_target_comp_sub_id , g_hft_config.g_fix_tag_57);
	strcpy(FIX_session_constants.mcx_tag_9227_terminal_info, g_hft_config.g_fix_tag_9227);
	
	// SETUP the attribs for this channel
	strcpy(FIX_op_chnl_attrb.FIX_gway_addr,exchg_FIX_gway_ip);
	FIX_op_chnl_attrb.FIX_gway_port = exchg_port;
	strcpy(FIX_op_chnl_attrb.local_interface_ip,hw_iface_ip);
	FIX_op_chnl_attrb.recv_cpu_id = 0;
	FIX_op_chnl_attrb.log_stats_flag = 0; // make sure if you want to log the events
	FIX_op_chnl_attrb.fix_op_chnl_generic_bfrs_needed = 2;
		
	
	if (STN_ERRNO_SUCCESS != stn_hft_FIX_op_channel_create (&FIX_op_chnl_attrb, 
								&FIX_session_constants, 
								seqNum,
								&chnl_hdl))
		return 0;
	

	if(STN_ERRNO_SUCCESS !=	stn_hft_FIX_op_channel_start(chnl_hdl))
		return 0;

	if(STN_ERRNO_SUCCESS !=	stn_hft_FIX_op_channel_login (chnl_hdl))
		return 0;
	int loggedIn = 0;
	int iMsg;
	while(!loggedIn)
	{
		while((iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len)) == STN_ERRNO_NODATARECV);
		if( STN_ERRNO_SUCCESS == iMsg)
		{
			printf("\nmain.hft Recieved Message :%d %s\n",msg_len, msg);
			if(strstr(msg, "96=0"))
			{
				loggedIn=1;
				printf("Logged in successfully\n");
			}
		}
		else if(STN_ERRNO_FAIL == iMsg)
		{
			printf("\nmain.hft closure of channel");
			break;
		}
		sleep(1);
	}
	iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);
	printf("received test msg %s\n", msg);




	// Now the channel is setup, start with the pair creation and see how it goes.
	iReturn = stn_hft_create_pair_strategy(&FIX_pair_chnl_attrib1,chnl_hdl,&pair_hdl1);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		// log the error
		printf("\nstn_hft_create_pair_strat failed %x",iReturn);
		return 0;
		}

	printf("created pair strategy 1\n");
	fflush(stdout);
	 iReturn = stn_hft_create_pair_strategy(&FIX_pair_chnl_attrib2,chnl_hdl,&pair_hdl2);
	 if(STN_ERRNO_SUCCESS != iReturn)
	 {
		 // log the error
		 printf("\nstn_hft_create_pair_strat failed %x",iReturn);
		 return 0;
	 }

	 printf("created pair strategy 2\n");
	 fflush(stdout);
                

	iReturn = stn_hft_update_price_of_B(618025,pair_hdl1);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_update_price_of_B failed: %x",iReturn);
		return 0;
		}
	iReturn = stn_hft_update_price_of_B(618025,pair_hdl2);
	printf("updated price of B\n");
	fflush(stdout);
/*
	FIX_pair_chnl_attrib.net_quantity = 43434343;
	iReturn = stn_hft_update_pair_strategy(&FIX_pair_chnl_attrib,pair_hdl,NET_QTY_CHANGE);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_update_pair_strat failed: %x",iReturn);
		return 0;
		}
*/
	sleep(10);// let the thread execute


	int i = 1;
	for(i = 1; i < 100; i++)
	{
		iReturn = stn_hft_update_price_of_B(618025+i*25,pair_hdl1);
        	if(STN_ERRNO_SUCCESS != iReturn)
        	{
        	        printf("\nstn_hft_update_price_of_B failed: %x",iReturn);
        	        return 0;
        	}
		iReturn = stn_hft_update_price_of_B(618050+i*25,pair_hdl2);
		if(STN_ERRNO_SUCCESS != iReturn)
                {
                        printf("\nstn_hft_update_price_of_B failed: %x",iReturn);
                        return 0;
                }

        	printf("updated %d. price of B\n", i);
        	fflush(stdout);
		sleep(1);
	}

	iReturn = stn_hft_update_price_of_B(618050,pair_hdl1);
	sleep(5);
	iReturn = stn_hft_pause_pair_execution(pair_hdl1);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_pause_pair_exectution failed: %x",iReturn);
		return 0;
		}
	iReturn = stn_hft_resume_pair_execution(pair_hdl1);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_resume_pair_execution failed: %x",iReturn);
		return 0;
		}
	iReturn = stn_hft_delete_pair_strategy(pair_hdl1);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_resume_pair_execution failed: %x",iReturn);
		return 0;
		}
	// once done, clear the channel and pair, and then come out.
	fflush(stdout);
	stn_hft_FIX_op_channel_delete(chnl_hdl);
	sleep(5);
			
}
//end multipair testing.



//-------------START HFT PAIR TESTING

int hft_mcx_fix_pair_test(uint8_t* hw_iface_ip, uint8_t* exchg_FIX_gway_ip, uint16_t exchg_port)
{
	struct stn_hft_FIX_op_channel_public_attrib_s FIX_op_chnl_attrb ;
	void *chnl_hdl,*pair_hdl;
	uint8_t* msg;
	int msg_len,iReturn;
	pthread_t	sender_thread;
	struct channel_trd_attribs_s thrd;

	struct stn_hft_pair_strategy_attrib_s FIX_pair_chnl_attrib;
	
		
	char msg_p[1024]={ 0x39,0x39,0x32,0x3d,0x54,0x46,0x01,//random tag
	 0x39,0x36,0x39,0x3d,0x59,0x47,0x46,0x01,//random tag
					0x00};
	
	// SETUP the session constants for this channel
	struct FIX_session_constants_s FIX_session_constants = 
	{
		.tag_90_encrptd_digest_length = 32,
                .tag_95_raw_data_length = 21,
                .tag_98_encrpt_methd = 0,
                .tag_141_Reset_seq_num = 'N',
                .tag_204_cust_or_firm = 1,
                .tag_59_time_in_force = 0,
                .tag_34_msg_seq_num=15,
	};


	FIX_pair_chnl_attrib.x = 1;
	FIX_pair_chnl_attrib.y = -24000;
	FIX_pair_chnl_attrib.Price_B = 639000;
	FIX_pair_chnl_attrib.qty_A = 100;
	FIX_pair_chnl_attrib.qty_B = 100;
	FIX_pair_chnl_attrib.order_size = 1;
	FIX_pair_chnl_attrib.net_quantity = 345234;
	FIX_pair_chnl_attrib.direction = BUY_A_SELL_B;
	FIX_pair_chnl_attrib.lot_A = 1;
	FIX_pair_chnl_attrib.lot_B = 1;
	FIX_pair_chnl_attrib.tick_A = 25;
	FIX_pair_chnl_attrib.tick_B = 25;
	FIX_pair_chnl_attrib.strat_id = 20;

	strcpy(FIX_pair_chnl_attrib.InstrumentCode_A,g_hft_config.g_instrument_A);
	strcpy(FIX_pair_chnl_attrib.InstrumentCode_B,g_hft_config.g_instrument_B);
	//strcpy(FIX_pair_chnl_attrib.tag_1_account_name,"PAIR_ACCOUNT");



	strcpy(FIX_session_constants.tag_8_FIXString, "FIX.4.2");
	strcpy(FIX_session_constants.tag_91_encrpted_digest, g_hft_config.g_fix_tag_91);
	strcpy(FIX_session_constants.tag_96_raw_data, g_hft_config.g_fix_tag_96);
	strcpy(FIX_session_constants.tag_49_sender_comp_id, g_hft_config.g_fix_tag_49);
	//TODO: Proficient values:
	strcpy(FIX_session_constants.tag_56_target_comp_id, g_hft_config.g_fix_tag_56);
	strcpy(FIX_session_constants.tag_57_target_comp_sub_id , g_hft_config.g_fix_tag_57);
	strcpy(FIX_session_constants.mcx_tag_9227_terminal_info, g_hft_config.g_fix_tag_9227);
	
	
	// SETUP the attribs for this channel
	strcpy(FIX_op_chnl_attrb.FIX_gway_addr,exchg_FIX_gway_ip);
	FIX_op_chnl_attrb.FIX_gway_port = exchg_port;
	strcpy(FIX_op_chnl_attrb.local_interface_ip,hw_iface_ip);
	FIX_op_chnl_attrb.recv_cpu_id = 0;
	FIX_op_chnl_attrb.log_stats_flag = 0; // make sure if you want to log the events
	FIX_op_chnl_attrb.fix_op_chnl_generic_bfrs_needed = 2;
		
	
	if (STN_ERRNO_SUCCESS != stn_hft_FIX_op_channel_create (&FIX_op_chnl_attrb, 
								&FIX_session_constants, 
								seqNum,
								&chnl_hdl))
		return 0;
	

	if(STN_ERRNO_SUCCESS !=	stn_hft_FIX_op_channel_start(chnl_hdl))
		return 0;

	if(STN_ERRNO_SUCCESS !=	stn_hft_FIX_op_channel_login (chnl_hdl))
		return 0;
	int loggedIn = 0;
	int iMsg;
	while(!loggedIn)
	{
		while((iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len)) == STN_ERRNO_NODATARECV);
		if( STN_ERRNO_SUCCESS == iMsg)
		{
			printf("\nmain.hft Recieved Message :%d %s\n",msg_len, msg);
			if(strstr(msg, "96=0"))
			{
				loggedIn=1;
				printf("Logged in successfully\n");
			}
		}
		else if(STN_ERRNO_FAIL == iMsg)
		{
			printf("\nmain.hft closure of channel");
			break;
		}
		sleep(1);
	}
	iMsg= stn_hft_FIX_op_channel_get_next_msg(chnl_hdl,&msg,&msg_len);
	printf("received test msg %s\n", msg);




	// Now the channel is setup, start with the pair creation and see how it goes.
	iReturn = stn_hft_create_pair_strategy(&FIX_pair_chnl_attrib,chnl_hdl,&pair_hdl);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		// log the error
		printf("\nstn_hft_create_pair_strat failed %x",iReturn);
		return 0;
		}

	printf("created pair strategy\n");
	fflush(stdout);

	iReturn = stn_hft_update_price_of_B(639025,pair_hdl);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_update_price_of_B failed: %x",iReturn);
		return 0;
		}
	printf("updated price of B\n");
	fflush(stdout);
/*
	FIX_pair_chnl_attrib.net_quantity = 43434343;
	iReturn = stn_hft_update_pair_strategy(&FIX_pair_chnl_attrib,pair_hdl,NET_QTY_CHANGE);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_update_pair_strat failed: %x",iReturn);
		return 0;
		}
*/
	sleep(10);// let the thread execute


	int i = 1;
	for(i = 1; i < 100; i++)
	{
		iReturn = stn_hft_update_price_of_B(639025+i*25,pair_hdl);
        	if(STN_ERRNO_SUCCESS != iReturn)
        	{
        	        printf("\nstn_hft_update_price_of_B failed: %x",iReturn);
        	        return 0;
        	}
        	printf("updated 2. price of B\n");
        	fflush(stdout);
		sleep(1);
	}

	iReturn = stn_hft_update_price_of_B(639050,pair_hdl);
	sleep(5);
	iReturn = stn_hft_pause_pair_execution(pair_hdl);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_pause_pair_exectution failed: %x",iReturn);
		return 0;
		}
	iReturn = stn_hft_resume_pair_execution(pair_hdl);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_resume_pair_execution failed: %x",iReturn);
		return 0;
		}
	iReturn = stn_hft_delete_pair_strategy(pair_hdl);
	if(STN_ERRNO_SUCCESS != iReturn)
		{
		printf("\nstn_hft_resume_pair_execution failed: %x",iReturn);
		return 0;
		}
	// once done, clear the channel and pair, and then come out.
	fflush(stdout);
	stn_hft_FIX_op_channel_delete(chnl_hdl);
	sleep(5);
			
}



//------------END HFT PAIR TESTING

//- - - - - - - - - - - - - - - - - - - 

// dummy message structure used for testing hft api's 
struct nse_mkt_msg_header_s {
        int num_msgs_in_pkt;    // Number of messages in a batched packet..
        int total_msg_size;             // True message before LZO compression....
};

struct nse_mkt_data_msg_s {
        char msg_type;
        long sequence_nbr;
        long time_stamp;        // milli from Jan 1 1980 00:00:00
        char symbol[10];
        char mkt_msg[0];
};

struct composite_s {
        struct nse_mkt_msg_header_s             _hdr;
        struct nse_mkt_data_msg_s               _msg;
};

//- - - - - - - - - - - - - - - - - - - - - - 
/*
NOTE:
This is blocking call and if this call back function does not return in real time hardware is going to get stuck and 
as a result it may start dropping packets.
*/

/*User defined callback routine, Need to be passed in creating mcast channel.*/
int user_defined_mkt_msg_cbfn(void* user_mkt_msg_handle, uint8_t* mkt_data_bfr, uint16_t mkt_data_bfr_len, uint32_t reserved)
{
	static int callback = 0;
	
	//todo : user defined code to handle packet and do whatever they want.
	if(callback = 0)
		{
		printf("\nRecieved callback from the pax: %u\n",mkt_data_bfr_len);
		callback =1;
		
		}
	
	return;
	
}
//- - - - - - - - - - - - - - - - - - - - 

int hft_mcx_mkt_data_test(uint8_t* hw_iface_ip, uint8_t* exchange_mkt_ip,  uint16_t exchg_mcast_port)
{
	struct stn_hft_mkt_channel_public_s stn_mkt_chnl = 
	{
		.exchange_id = stn_hft_enum_exchange_id_mcx_e,
		.channel_type = STN_HFT_MKT_CHANNEL_TYPE_MUDP,
		.mkt_data_port = exchg_mcast_port,
		.log_interval =-1, // no logging
		.log_stats_flag =0,
		.recv_cpu_id = 3
	};

	struct 	composite_s* recv_msg=0;
	void *stn_hdl = 0;	
	unsigned char * pkt;
	unsigned int pkt_len;
	unsigned int i = 0;	

	int ret = 0;

	strcpy(stn_mkt_chnl.mkt_data_addr,exchange_mkt_ip);
	strcpy(stn_mkt_chnl.local_interface_ip,hw_iface_ip);

	ret = stn_hft_mkt_data_channel_mcast_create(&stn_mkt_chnl,0,0,user_defined_mkt_msg_cbfn,0,(void*)&stn_hdl);

	if (STN_ERRNO_SUCCESS == ret)
		stn_hft_mkt_data_channel_start ((void*)stn_hdl);		
	while(bShutdown == 0)
		{
		if( stn_hft_mkt_data_channel_get_next_msg(stn_hdl,&pkt, &pkt_len) == STN_ERRNO_SUCCESS)
			{

				/* This is application message hence no need to do regular packet decoding. 
				 type cast to the message type we expect. As for example here we use on of the expected
				 */
				recv_msg = (struct composite_s*) pkt;
				/*
				Please classify or do whatever one wants here with the application payload 
				*/
				printf("Recieved Msg Sequence No :%u\n",recv_msg->_msg.sequence_nbr);
				/*
				Clients can have their own routine to classify and do application analysis
				*/
			}
		else
			{
				sleep(1); 
				/* 
				please sleep for one sec so that other thread gets some chance.
				 It depends on how the client wants to do it. 
				 */
			}
			// call for packets and do the classification as needed
		}
	stn_hft_mkt_data_channel_delete((void*)stn_hdl);
	return 0;
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


	hft_read_config_file();
	
		// gcc getopt(...) has bugs on certain ops.. so use the hand rolled from stn_ api's
	while ((c = stn_getopt (argc, argv, "i:m:p:o:g:s:")) != -1)
	{
		switch (c)
		{

			case 'o':
				ops = atoi(optarg);
				break;

			case 's':
				seqNum = atoi(optarg);
				printf("seqNum=%d\n", seqNum);
				break;
		}
	}

	signal(SIGINT,sig_handler);
	
	printf("\nCtrl+c to exit\n");

	exchg_fix_port = atoi(g_hft_config.g_fix_port);
	exchg_mcast_port = atoi(g_hft_config.g_multicast_port);
	
	if (2 == ops)
		hft_mcx_fix_op_test(g_hft_config.g_interface, g_hft_config.g_fix_gw, exchg_fix_port);	// -i 192.168.1.34  -g 192.168.1.230 -p 2002 -o 2
	else if (1 == ops)
		hft_mcx_mkt_data_test (g_hft_config.g_interface, g_hft_config.g_multicast_ip, exchg_mcast_port);	// -i 172.21.73.212 -m 234.21.34.5 -p 23000
	else if ( 3 == ops)
		hft_mcx_fix_pair_test(g_hft_config.g_interface, g_hft_config.g_fix_gw, exchg_fix_port);
	else if (4 == ops)
		hft_mcx_fix_multi_pair_test(g_hft_config.g_interface, g_hft_config.g_fix_gw, exchg_fix_port);
	
	
	return 0;


}


