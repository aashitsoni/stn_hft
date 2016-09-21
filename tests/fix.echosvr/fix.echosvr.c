	/*
	
	  ECHOSERV.C
	  ==========
	  (c) Paul Griffiths, 1999
	  Email: mail@paulgriffiths.net
	  
	  Simple TCP/IP echo server.
	
	*/
	
	
#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */
	
#include "helper.h"           /*  our own helper functions  */
	
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
	
	

	/*	Global constants  */
#define FIX_MSG_SIZE		4096	
#define ECHO_PORT          (2002)
#define MAX_LINE           (1000)
#define FIX_MSG_OFFSET		128
#define FIX_BUFF_SIZE            FIX_MSG_SIZE-FIX_MSG_OFFSET

static char *FIX_4_2_Generic_Msg_Template="8=FIX.4.2%c"
                                             "9=%04u%c"
                                             "35=%c%c"           /*Message type*/
                                             "34=%04u%c"         /*Sequence number*/ 
                                             "49=%s%c"           /*sender comp id*/
                                             "52=%s%c"           /*time */
                                             "56=%s";             /*target comp id*/
                                            /*doesnt have last 0x01, encoding in here. Because that will be fixed in the message 
                                                                        send*/

static char *FIX_4_2_ER_Template="11=%s%c"  /* ClOrdId*/
									"14=%u%c"  //  CumQty
									"17=%s%c"  //  ExecutionId
									"20=%c%c"  // ExecutionTransType
									"37=%u%c"  //  OrderId
									"38=%u%c"  // OrderQty
									"39=%c%c"  //  OrdStatus
									"54=%c%c"  // Side
									"55=%s%c"  // Symbol
									"150=%c%c"; //ExecType

											
struct FIX_Order_Ack_constants
{
	uint8_t		tag_11_ClOrdId[48];
	uint32_t	tag_38_order_qty;
	uint8_t		tag_39_ord_status;
	uint8_t		tag_54_side;
	uint8_t		tag_55_symbol[16];
	uint8_t		tag_150_exec_type;
};


int 	fix_msg_seq_num = 10202;

#define EXECUTION_REPORT '8'
#define LOGON_RESPONSE 'A'


#define TAG_8_LEN   10       // 8=FIX.4.2. begine string
#define TAG_9_LEN   7       // 9=0000. message length
#define TAG_35_LEN  5       // 35=x.  message type
#define TAG_52_LEN  21      // 52=99990011-22:33:55.  time stamp
#define TAG_INIT_LEN    4


#define tag_49_sender_comp_id "FIXECHO"
#define tag_56_target_comp_id "HFTFIX"

#define FIX_MSG_HDR_LEN (TAG_8_LEN + TAG_9_LEN + TAG_35_LEN + TAG_52_LEN)


int FIX_msg_hdr_fixed_length;

#define EXEC_TRANS_TYPE_NEW '0'
#define ORD_STATUS_NEW '0'
#define ORD_STATUS_PARTIAL '1'

#define EXEC_TYPE_NEW '0'
#define EXEC_TYPE_PARTIAL '1'



char* FormatCurrTime(char* szTime)
{
    time_t curr_time;
    struct tm* gm_time;
    
    
    time(&curr_time);
    gm_time = gmtime(&curr_time);
    sprintf(szTime,"%d%02d%02d-%02d:%02d:%02d",gm_time->tm_year+1900,
                        gm_time->tm_mon+1,
                        gm_time->tm_mday,
                        gm_time->tm_hour,
                        gm_time->tm_min,
                        gm_time->tm_sec);

    return szTime;
}

int get_fix_msg_seq_len(int sequenceno)
{
    if(sequenceno < 9999)
        return 4;
    else if(sequenceno < 99999)
        return 5;
    else if(sequenceno < 999999)
        return 6;
    else if(sequenceno < 9999999)
        return 7;
    else if(sequenceno < 99999999)
        return 8;
    else if(sequenceno < 999999999)
        return 9;
    else                        // TODO: if it can really go beyond 10
        return 10;
    
}


char *GenerateCheckSum( char *buf, long bufLen, char* checkSum )
{
    long idx;
    unsigned int cks;
    for( idx = 0L, cks = 0; idx < bufLen; cks += (unsigned int)buf[ idx++ ] );
    sprintf( checkSum, "%03d", (unsigned int)( cks % 256 ) );
    return checkSum;
}

/*
send generic message to the outgoing FIX channel

*/
int _fix_echo_send_generic_msg(int sock,unsigned char msg_type, unsigned char* fix_msg, int msg_size, char* time_str)
{
	unsigned char* msg = fix_msg;
	char checksum[10],time_str_lcl[20] = "\0";// this shall not be more than 20 bytes
	uint16_t len_msg_header = 0,fix_msg_length = 0,total_fix_msg_len = 0;
	uint16_t buff_len = 0;


	   
	// Increment sequence number
	fix_msg_seq_num++;
	

	if (0 == time_str) {
		FormatCurrTime(time_str_lcl);
		time_str = &time_str_lcl[0];
	}


	// this is length to offset in the buffer to copy the header.
	len_msg_header = FIX_msg_hdr_fixed_length + get_fix_msg_seq_len(fix_msg_seq_num);

	// this is lenght to encode in to the FIX message
	fix_msg_length = len_msg_header - TAG_8_LEN - TAG_9_LEN + msg_size ;
//						  + 1;	// to adjust the 0x01 that we will encode on the null byte

	// change the message buffer to the reserver aread where we have bytes we can use to format the message
	msg = fix_msg - len_msg_header; // 1 byte extra to accomodate null character, which will be overwritten by 0x01 later on.

	buff_len = sprintf((char*)msg, FIX_4_2_Generic_Msg_Template,
						0x01,
						fix_msg_length,0x01,
						msg_type,0x01,
						fix_msg_seq_num,0x01,
						tag_49_sender_comp_id,0x01,
						time_str,0x01,
						tag_56_target_comp_id);

	msg[buff_len]=0x01;  // set this one up to get rid of the null character posted by previous sprintf

	

	/**/
	total_fix_msg_len = len_msg_header + msg_size;/*account for the 0x01 added up*/

	GenerateCheckSum((char*)msg,total_fix_msg_len,checksum);
	total_fix_msg_len +=  sprintf((char*)&fix_msg[msg_size],"10=%s%c", checksum,0x01);	   // pad checksum

	
	if(send(sock, msg, total_fix_msg_len, 0) == -1)
		return -1;

	return total_fix_msg_len;
}





int _fix_send_order_ack (int sock,char* fix_msg,int fix_msg_len )
{
	 uint8_t oc_msg [FIX_MSG_SIZE];
	 uint16_t msg_len = 0;
	 char time_str[100];
	 struct FIX_Order_Ack_constants order_ack;
	 
		// TODO: decode the fix incoming message
		
	 FormatCurrTime(time_str);

		
	 
	 strcpy(order_ack.tag_11_ClOrdId,"HFT_TEST");
	 order_ack.tag_150_exec_type = EXEC_TYPE_NEW;
	 order_ack.tag_38_order_qty = 10;
	 order_ack.tag_39_ord_status = ORD_STATUS_NEW;
	 order_ack.tag_54_side = 1;
	 strcpy(order_ack.tag_55_symbol,"MyTestSym");
	 
	 	

 
	 msg_len = snprintf (&oc_msg[FIX_MSG_OFFSET],FIX_BUFF_SIZE,
						 FIX_4_2_ER_Template,
						 order_ack.tag_11_ClOrdId, 0x01,	//ClOrdId
						 0,0x01, 	//CumQty
						 time_str,0x01,	//ExectuionId
						 EXEC_TRANS_TYPE_NEW,0x01,	//Execution Trans Type NEW
						 time_str,0x01,	// Order Id
						 order_ack.tag_38_order_qty,0x01,	// OrderQty
						 order_ack.tag_39_ord_status,0x01, // OrdStatus NEW
						 order_ack.tag_54_side,0x01, 	// side
						 order_ack.tag_55_symbol,0x01, // Symbol
						 order_ack.tag_150_exec_type,0x01);

	return _fix_echo_send_generic_msg(sock,
											EXECUTION_REPORT,
											&oc_msg[FIX_MSG_OFFSET],
											msg_len, time_str);

	
}


int _fix_send_logon_response(int sock,char* fix_msg,int fix_msg_len)
{
	 uint8_t logon_resp_msg [FIX_MSG_SIZE];
	 uint16_t msg_len = 0;
	 char time_str[100];
	 struct FIX_Order_Ack_constants order_ack;
	 
		// TODO: decode the fix incoming message
		
	 FormatCurrTime(time_str);

		
	 
	 
 
	 msg_len = snprintf (&logon_resp_msg[FIX_MSG_OFFSET],FIX_BUFF_SIZE,
						 "98=0%c"
						 "95=10%c"
						 "96=0%c"
						 ,
						 0x01,
						 0x01,
						 0x01);

	return _fix_echo_send_generic_msg(sock,
											LOGON_RESPONSE,
											&logon_resp_msg[FIX_MSG_OFFSET],
											msg_len, time_str);


}

int _fix_send_order_fill (int sock,char* fix_msg,int fix_msg_len )
{
	 uint8_t oc_msg [FIX_MSG_SIZE];
	 uint16_t msg_len = 0;
	 char time_str[100];
	 struct FIX_Order_Ack_constants order_ack;
	 
		// TODO: decode the fix incoming message
		
	 FormatCurrTime(time_str);

		
	 
	 strcpy(order_ack.tag_11_ClOrdId,"HFT_TEST");
	 order_ack.tag_150_exec_type = EXEC_TYPE_PARTIAL;
	 order_ack.tag_38_order_qty = 10;
	 order_ack.tag_39_ord_status = ORD_STATUS_PARTIAL;
	 order_ack.tag_54_side = 1;
	 strcpy(order_ack.tag_55_symbol,"MyTestSym");
	 
	 	

 
	 msg_len = snprintf (&oc_msg[FIX_MSG_OFFSET],FIX_BUFF_SIZE,
						 FIX_4_2_ER_Template,
						 order_ack.tag_11_ClOrdId, 0x01,	//ClOrdId
						 0,0x01, 	//CumQty
						 time_str,0x01,	//ExectuionId
						 EXEC_TRANS_TYPE_NEW,0x01,	//Execution Trans Type NEW
						 time_str,0x01,	// Order Id
						 order_ack.tag_38_order_qty,0x01,	// OrderQty
						 order_ack.tag_39_ord_status,0x01, // OrdStatus NEW
						 order_ack.tag_54_side,0x01, 	// side
						 order_ack.tag_55_symbol,0x01, // Symbol
						 order_ack.tag_150_exec_type,0x01);

	return _fix_echo_send_generic_msg(sock,
											EXECUTION_REPORT,
											&oc_msg[FIX_MSG_OFFSET],
											msg_len, time_str);

	
}



	
int main(int argc, char *argv[]) {
	int 	  list_s;				 /*  listening socket		   */
	int 	  conn_s;				 /*  connection socket		   */
	short int port; 				 /*  port number			   */
	struct	  sockaddr_in servaddr;  /*  socket address structure  */
	char	  buffer[MAX_LINE]; 	 /*  character buffer		   */
	char	 *endptr;				 /*  for strtol()			   */
	int bytes_read;



	FIX_msg_hdr_fixed_length = FIX_MSG_HDR_LEN + 
									TAG_INIT_LEN + // 4 lenght for	sender id sign 
									TAG_INIT_LEN + // 4 lenght byte for target id signature
									TAG_INIT_LEN + // 4 length bytes for sequence number signature
									strlen(tag_49_sender_comp_id)+
									strlen(tag_56_target_comp_id);

	/*	Get port number from the command line, and
		set to default port if no arguments were supplied  */

	if ( argc == 2 ) {
	port = strtol(argv[1], &endptr, 0);
	if ( *endptr ) {
		fprintf(stderr, "ECHOSERV: Invalid port number.\n");
		exit(EXIT_FAILURE);
	}
	}
	else if ( argc < 2 ) {
	port = ECHO_PORT;
	}
	else {
	fprintf(stderr, "ECHOSERV: Invalid arguments.\n");
	exit(EXIT_FAILURE);
	}

	
	/*	Create the listening socket  */

	if ( (list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
	fprintf(stderr, "ECHOSERV: Error creating listening socket.\n");
	exit(EXIT_FAILURE);
	}


	/*	Set all bytes in socket address structure to
		zero, and fill in the relevant data members   */

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family 	 = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port		 = htons(port);


	/*	Bind our socket addresss to the 
	listening socket, and call listen()  */

	if ( bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
	fprintf(stderr, "ECHOSERV: Error calling bind()\n");
	exit(EXIT_FAILURE);
	}

	if ( listen(list_s, LISTENQ) < 0 ) {
	fprintf(stderr, "ECHOSERV: Error calling listen()\n");
	exit(EXIT_FAILURE);
	}

	
	/*	Enter an infinite loop to respond
		to client requests and echo input  */


	/*	Wait for a connection, then accept() it  */

	if ( (conn_s = accept(list_s, NULL, NULL) ) < 0 ) {
		fprintf(stderr, "ECHOSERV: Error calling accept()\n");
		exit(EXIT_FAILURE);
	}

	// dont reply to log on
	bytes_read = recv(conn_s,(void*)buffer,MAX_LINE,0);
	
	if(bytes_read <=0)
		{
			printf("\nClosing Socket, nothing to read\n");
			return 0;
		}

	printf ("Bytes read:%d\nSending Logon Response", bytes_read);
	fflush(stdout);

	bytes_read = _fix_send_logon_response(conn_s,(void*)buffer,strlen(buffer));
	
	if(-1==bytes_read)
		printf("\nError Sending logon response messages");
	else
		printf("\nLogon Response Sent bytes:%d",bytes_read);
	fflush(stdout);

		

	while ( 1 ) 
		{

	/*	Retrieve an input line from the connected socket
		then simply write it back to the same socket.	  */
		bytes_read = recv(conn_s,(void*)buffer,MAX_LINE,0);
	
		if(bytes_read <=0)
			{
				printf("\nClosing Socket, nothing to read\n");
				break;
			}
/*		int bytes_read = Readline(conn_s, buffer, MAX_LINE-1);*/
		printf ("Bytes read:%d\n", bytes_read);
		fflush(stdout);
/*		Writeline(conn_s, buffer, strlen(buffer));*/
//		bytes_read =send(conn_s,(void*)buffer,strlen(buffer),0);

		/*
		1. Decode the incoming fix Message and create a structure that can hold
			ClOrdId,Symbol, Quantity, Price
		*/

		bytes_read = _fix_send_order_ack(conn_s,buffer,strlen(buffer)); // sending order ack

		if(-1==bytes_read)
			printf("\nError Sending messages");
		else
			printf("\nOrder Ack Sent bytes:%d",bytes_read);
		fflush(stdout);
		
		
		bytes_read = _fix_send_order_fill(conn_s,buffer,strlen(buffer)); // sending order ack

		if(-1==bytes_read)
			printf("\nError Sending messages");
		else
			printf("\nOrder Fill Sent bytes:%d",bytes_read);

		fflush(stdout);

		}


	/*	Close the connected socket	*/

	if ( close(conn_s) < 0 ) {
		fprintf(stderr, "ECHOSERV: Error calling close()\n");
		exit(EXIT_FAILURE);
	}
}

