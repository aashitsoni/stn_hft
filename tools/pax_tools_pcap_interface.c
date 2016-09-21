
#include <stdio.h>
#include <stdlib.h>
#include "pax_errno.h"
#include "pax_tools_pcap_interface.h"


/*
-------<<< NOTE >>>
------- * THIS DMA emulator supports only supports IP packets. 
------------- * If user application wants to support other than IP then they can extend the function
------- * DMA emulator does not support VLAN/ ISL tunnel encapsulations..

*/

uint32_t  pax_tools_init_pcap_interface(struct pax_tools_pcap_iface_hdl_s *prx_args)
{		
	uint32_t ret = PAX_ERRNO_DEVICE_OPEN_FAIL;

	char errbuf[512];		/* error buffer */

	prx_args->quit = 0;
	/* open capture device */
	prx_args->pcap_handle= pcap_open_live(prx_args->pcap_interface_name, MAX_FRAME_LEN, ENABLE_PROMISCUOUS, PACKET_RECV_TIMEOUT, errbuf);
	if (prx_args->pcap_handle == NULL) {
		fprintf(stderr, "Couldn't open NIC %s: %s\n", prx_args->pcap_interface_name, errbuf);
		return ret;
	}

	ret = STN_ERRNO_SUCCESS;
	/* 
		Check for type ETHERNET 802.x
		In LINUX DLT_EN10MB is used for all flavors of standard Ethernet (10Mb,
        100Mb, 1000Mb, and presumably 10000Mb), and you determine
        whether a DLT_EN10MB packet uses Ethernet or 802.3 encapsulation
        by looking at the type/length field;
	*/
	if (pcap_datalink(prx_args->pcap_handle) != DLT_EN10MB) 
	{
		fprintf(stderr, "0x%x is not an valid Ethernet\n", prx_args->pcap_handle);
		pcap_close(prx_args->pcap_handle);
		ret = PAX_ERRNO_DEVICE_INVALID;
	}

	if (-1 == pcap_setnonblock (prx_args->pcap_handle, 1, errbuf))
	{
		fprintf(stderr, "Non-block set failed on device");
		pcap_close(prx_args->pcap_handle);
		ret = PAX_ERRNO_DEVICE_INVALID;
	}

	return ret;
}


void* pax_tools_pcap_promiscuous_thread_fn(void* thread_args)
{
	struct pax_tools_pcap_iface_hdl_s *prx_args  = thread_args;
	int32_t ret = 0;
	
	uint8_t thrd_name[] = "px_pcap_if_thr";

	#include <sys/prctl.h>
	prctl(PR_SET_NAME,thrd_name,0,0,0);

	/*
	NOTE: time out does not seem to function... 
	So may have to setup pcap_setnonblock and then look at the num secs/ millisec elapsed to post a timeout..
	See: http://uw714doc.sco.com/en/man/html.3/libpcap.3.html
	*/


	while(1) {
		ret = pcap_dispatch(prx_args->pcap_handle, 0, prx_args->pcap_promiscuous_rx_cbf, prx_args->caller_args);
		if (ret <=0) { // -2 is returned on pcap_breakloop..
			if (prx_args->quit) break;
			else { // timeout or some other error occured..
				prx_args->pcap_promiscuous_rx_cbf(prx_args->caller_args,0,0);
				continue; 
			}
		};
		if (prx_args->quit) break;
	}
	
	prx_args->pcap_promiscuous_rx_cbf(prx_args->caller_args,0,0);

	pcap_close(prx_args->pcap_handle);
}

uint32_t  pax_tools_pcap_interface_teardown(struct pax_tools_pcap_iface_hdl_s* handle)
{
	handle->quit = 1;
	pcap_breakloop(handle->pcap_handle);
}


