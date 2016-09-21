#include <sys/types.h>   /* for type definitions */
#include <sys/socket.h>  /* for socket API function calls */
#include <netinet/in.h>  /* for address structs */
#include <arpa/inet.h>   /* for sockaddr_in */
#include <stdio.h>       /* for printf() */
#include <stdlib.h>      /* for atoi() */
#include <string.h>      /* for strlen() */
#include <unistd.h>      /* for close() */

#define MAX_LEN  1024    /* maximum string size to send */
#define MIN_PORT 1024    /* minimum port allowed */
#define MAX_PORT 65535   /* maximum port allowed */


struct nse_mkt_msg_header_s {
	int num_msgs_in_pkt;	// Number of messages in a batched packet..
	int total_msg_size;		// True message before LZO compression....
};

struct nse_mkt_data_msg_s {
	char msg_type;
	long sequence_nbr;
	long time_stamp;	// milli from Jan 1 1980 00:00:00
	char symbol[10];
	char mkt_msg[0];
};

struct composite_s {
	struct nse_mkt_msg_header_s		_hdr;
	struct nse_mkt_data_msg_s		_msg;
};


int main(int argc, char *argv[]) {

  int sock;                   /* socket descriptor */
  char send_str[MAX_LEN];     /* string to send */
  struct sockaddr_in mc_addr; /* socket address structure */
  unsigned int send_len;      /* length of string to send */
  char* mc_addr_str;          /* multicast IP address */
  unsigned short mc_port;     /* multicast port */
  unsigned char mc_ttl=1;     /* time to live (hop count) */
  struct composite_s  _dummy_msg;
  int idx;
	
  /* validate number of arguments */
  if (argc != 3) {
    fprintf(stderr, 
            "Usage: %s <Multicast IP> <Multicast Port>\n", 
            argv[0]);
    exit(1);
  }

  mc_addr_str = argv[1];       /* arg 1: multicast IP address */
  mc_port     = atoi(argv[2]); /* arg 2: multicast port number */

  /* validate the port range */
  if ((mc_port < MIN_PORT) || (mc_port > MAX_PORT)) {
    fprintf(stderr, "Invalid port number argument %d.\n",
            mc_port);
    fprintf(stderr, "Valid range is between %d and %d.\n",
            MIN_PORT, MAX_PORT);
    exit(1);
  }

  /* create a socket for sending to the multicast address */
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket() failed");
    exit(1);
  }
  
  /* set the TTL (time to live/hop count) for the send */
  if ((setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, 
       (void*) &mc_ttl, sizeof(mc_ttl))) < 0) {
    perror("setsockopt() failed");
    exit(1);
  } 
  
  /* construct a multicast address structure */
  memset(&mc_addr, 0, sizeof(mc_addr));
  mc_addr.sin_family      = AF_INET;
  mc_addr.sin_addr.s_addr = inet_addr(mc_addr_str);
  mc_addr.sin_port        = htons(mc_port);

  /*
  printf("Begin typing (return to send, ctrl-C to quit):\n");

  // clear send buffer 
  memset(send_str, 0, sizeof(send_str));

  while (fgets(send_str, MAX_LEN, stdin)) {
    send_len = strlen(send_str);

    // send string to multicast address 
    if ((sendto(sock, send_str, send_len, 0, 
         (struct sockaddr *) &mc_addr, 
         sizeof(mc_addr))) != send_len) {
      perror("sendto() sent incorrect number of bytes");
      exit(1);
    }
	    // clear send buffer 
    memset(send_str, 0, sizeof(send_str));
  */

  for (idx =0; idx < 5000; idx ++) 
  {
	send_len = sizeof(_dummy_msg);
	_dummy_msg._hdr.num_msgs_in_pkt = 7;//+idx;
	_dummy_msg._hdr.total_msg_size = 514;
	_dummy_msg._msg.msg_type = 'M';
	
	// send string to multicast address 
	if ((sendto(sock, (char*)&_dummy_msg, send_len, 0, 
			(struct sockaddr *) &mc_addr, 
			sizeof(mc_addr))) != send_len) 
	{
		perror("sendto() sent incorrect number of bytes");
	}
		usleep(80);
  }
	
  close(sock);  

  exit(0);
}

