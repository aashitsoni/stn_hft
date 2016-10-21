/*******************************************************************************************************
Copyright (c) 2014 , SwiftTrade Network LLC, MA

File Name : common/console_log.c

Author      Date				Comments
------      -----				---------
mt			10/20/2016			Created


********************************************************************************************************/

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include "console_log.h"

FILE* fp_console_log_file = 0;



void format_console_log_filename(char* szFileName)
{
	time_t	curr_time;
	struct tm* local_time;
	
	time(&curr_time);
	local_time = localtime(&curr_time);
	
	sprintf(szFileName,"FIXConsole_Log_%2u%2u%2u_%2u%2u%2u.log",local_time->tm_mon+1,local_time->tm_mday,local_time->tm_year-100,
								local_time->tm_hour,local_time->tm_min, local_time->tm_sec);
	return;
	
}

int console_log_open()
{
	char consoleLogFile[64];
	
	if(fp_console_log_file != NULL)
		return CONSOLE_LOG_ALREADY_OPEN;
	
	format_console_log_filename(consoleLogFile);
	
	fp_console_log_file = fopen(consoleLogFile,"w+");
	if(fp_console_log_file == NULL)
		return CONSOLE_LOG_FILE_OPEN_ERROR;
		
	return CONSOLE_LOG_SUCCESS;
}
int console_log_close()
{
	if(fp_console_log_file != NULL)
	{
		fclose(fp_console_log_file);
		fp_console_log_file = NULL;
		return CONSOLE_LOG_SUCCESS;
	}
	
	return CONSOLE_LOG_ERROR_FILE_NOT_OPEN;
	
}

void console_log_write(char* str,...)
{
	va_list valist;
	va_start(valist,str);
	vfprintf(fp_console_log_file,str,valist);
	va_end(valist);
	return;
}


