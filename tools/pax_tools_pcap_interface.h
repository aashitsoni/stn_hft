
#ifndef PAX_TOOLS_PCAP_INTERFACE_H
#define PAX_TOOLS_PCAP_INTERFACE_H
#include <stdint.h>
#include <pcap.h>
#include <pthread.h>
#include "pax_dma_hdrs.h"
#include "pkt_hdrs.h"

/*
-------<<< NOTE >>>
------- THIS DMA emulator supports ONLY upto a number of packets as defined by the PAX_HACK_MAX_PKTS_FOR_TESTING define
------- It only supports IP packets. If user application wants to support other than IP then they can extend the function
------- DMA emulator does not support VLAN/ ISL tunnel encapsulations..

*/


struct pax_tools_pcap_iface_hdl_s {
	pcap_t* pcap_handle;
	uint8_t pcap_interface_name[16];
	void* caller_args;
	pthread_t thr_hdl;
	uint8_t quit;
	void (*pcap_promiscuous_rx_cbf) (uint8_t *args, const struct pcap_pkthdr *header, const uint8_t *packet);
};


uint32_t  pax_tools_init_pcap_interface(struct pax_tools_pcap_iface_hdl_s *prx_args);
void*	  pax_tools_pcap_promiscuous_thread_fn(void* thread_args);
uint32_t  pax_tools_pcap_interface_teardown(struct pax_tools_pcap_iface_hdl_s* handle);



#endif