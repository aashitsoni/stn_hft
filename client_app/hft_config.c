
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014

SwiftTrade Network LLC

File Name  : client_app/hft_config.c

Author      Date				Comments
------      -----				---------
mt			10/20/2016			Created
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

#include "fix_server_test.h"
#include "console_log.h"





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

int hft_print_config()
{
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

