

#ifndef __PKT_HEADERS_H
#define __PKT_HEADERS_H

#include <stdint.h>
#include <arpa/inet.h>

#define CISCO_ISL_HEADER_SIZE	26
#define MAX_FRAME_LEN			2048
/* default timeout in milliseconds to wait for the capture device to open */
#define PACKET_RECV_TIMEOUT		5000
#define ENABLE_PROMISCUOUS		1
#define RECV_10_PKTS_FOR_TEST	10

#pragma pack (push)
#pragma pack(1)

typedef union U4x3_U
{
    uint8_t   u1;
    int8_t    s1;
    uint16_t  u2;
    int16_t   s2;
    uint32_t  u4;
    int32_t   s4;
    uint8_t   u1a[12];
    int8_t    s1a[12];
    uint16_t  u2a[6];
    int16_t   s2a[6];
    uint32_t  u4a[3];
    int32_t   s4a[3];
}
U4x3_T, *U4x3_P;
typedef U4x3_T  U1x12_T;
typedef U4x3_T* U1x12_P;
typedef U4x3_T  U2x6_T;
typedef U4x3_T* U2x6_P;


struct u4_u2_s {
	uint32_t u4a;
	uint16_t u2a;
};



typedef struct pcap_hdr_s {
    uint32_t magic_number;   /* magic number */
    uint16_t version_major;  /* major version number */
    uint16_t version_minor;  /* minor version number */
    int      thiszone;       /* GMT to local correction */
    uint32_t sigfigs;        /* accuracy of timestamps */
    uint32_t snaplen;        /* max length of captured packets, in octets */
    uint32_t network;        /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_usec;        /* timestamp microseconds */
    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t orig_len;       /* actual length of packet */
	u_char   payload[0];	 /* actual start of the packet payload */
} pcaprec_hdr_t;


#define L4_PROTO_TCP	6
#define L4_PROTO_UDP	17
#define L4_PROTO_ICMP	1

struct mac_hdr_s {
	struct u4_u2_s mac_addr_a;
	struct u4_u2_s mac_addr_b;
	uint16_t mac_proto;
};

struct iphdr_s {
   unsigned char   ip_hl:4; /* both fields are 4 bits */
   unsigned char   ip_v:4;
   uint8_t        ip_tos;
   uint16_t       ip_len;
   uint16_t       ip_id;
   uint16_t       ip_off;
   uint8_t        ip_ttl;
   uint8_t        ip_p;
   uint16_t       ip_sum;
   uint32_t		 ip_src;
   uint32_t		 ip_dst;
};

struct tcphdr_s {
        uint16_t src_port;
        uint16_t dst_port;
        uint32_t seq;
        uint32_t ack_seq;       
        #  if __BYTE_ORDER == __LITTLE_ENDIAN
        unsigned short res1:4;
        unsigned short doff:4;
        unsigned short fin:1;
        unsigned short syn:1;
        unsigned short rst:1;
        unsigned short psh:1;
        unsigned short ack:1;
        unsigned short urg:1;
        unsigned short res2:2;
        #  elif __BYTE_ORDER == __BIG_ENDIAN
        unsigned short doff:4;
        unsigned short res1:4;
        unsigned short res2:2;
        unsigned short urg:1;
        unsigned short ack:1;
        unsigned short psh:1;
        unsigned short rst:1;
        unsigned short syn:1;
        unsigned short fin:1;
        #  endif
        unsigned short window;       
        unsigned short check;
        unsigned short urg_ptr;
};

struct udphdr_s {
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t length;
  uint16_t checksum;
};

struct pkt_hdr_swab_s {
	U4x3_T  mac;
	uint16_t l3proto;
	struct iphdr_s iphdr;
    uint16_t src_port;
    uint16_t dst_port;
};

struct transports_hdr_port_info_swab_s {
	uint16_t src_port;
	uint16_t dst_port;
};

#pragma pack(pop)

/*http://www.koders.com/c/fid4C49ED857A29D795F03ED1471158B7866B83BC4E.aspx*/

#endif