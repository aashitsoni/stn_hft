
/*
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Copyright (c) 2014 , SwiftTrade Network LLC , MA

File Name : common/console_log.h

Author      Date				Comments
------      -----				---------
mt			10/20/2016			Created
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/
#ifndef _CONSOLE_LOG_H_
#define _CONSOLE_LOG_H_


#define CONSOLE_LOG_SUCCESS 1
#define CONSOLE_LOG_FILE_OPEN_ERROR  -1
#define CONSOLE_LOG_ALREADY_OPEN 2
#define CONSOLE_LOG_ERROR_FILE_NOT_OPEN 3


int console_log_open();
int console_log_close();
void console_log_write(char*,...);


#endif