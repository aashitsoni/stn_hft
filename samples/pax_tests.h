
#ifndef PAX_SAMPLES_H
#define PAX_SAMPLES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dirent.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>

#include "pax_errno.h"
#include "pax_lib_public.h"
#include "pax_hash_table_public.h"
#include "pax_hs_pme_public.h"
#include "pax_flow_structure.h"
#include "pkt_hdrs.h"
#include "pax_dma_hdrs.h"
#include "pax_eb_public.h"
#include "pax_list_macros.h"
#include "pax_device_public.h"
#include "pax_numa_steering_public.h"
#include "pax_flow_update.h"

#define PAX_TEST_SLEEP_16K		16*1000	// in seconds


/*
 for the sake of our tests we will use only the following for hash purposes
*/
struct pkt_fields_s {
	uint32_t ip_a;
	uint32_t ip_b;
	uint32_t port_a;
	uint32_t port_b;
	uint8_t ip_proto; // for frags etc..
	uint16_t vlan_id;
};


struct __sample_pkt_proc_thr_param_s {
	uint8_t* pax_device_handle;
	struct pax_flow_pool_header_s* 
			flow_pool_hdr;
	uint8_t* hashtbl_handle;
	uint8_t quit;
	uint8_t descr_type;
	struct pax_latency_log_s latency_log;
};


int stn_getopt(int nargc,char **nargv, char*ostr);

int start_ht_test	  (char*);
int start_hs_pme_test (char* device);
int start_hs_pme_hw_test(char* device,int pme_type, int hw_id);
int start_eb_test	  (int argc, char**argv);
int start_pkt_steering_test
					  (char* device);
int start_flow_updates_test
					  (char* device, int hw_id);
int start_nic_sanity_tests(char* device, int hw_id);

int start_pax_navl_test(char* device, int slice_len, uint16_t classfn_disp, int num_threads, uint16_t dma_queued, 
						int num_devices_to_test, int start_index, uint8_t inline_mode, int flow_record_type, uint8_t* nrc_ip);

int get_nic_stats (char* device, int hw_id);

int __sample_setup_ps_rules(uint8_t* pax_device_handle, uint8_t hw_descr_type, uint16_t* ps_ring_ids_enb_arr, uint8_t* num_rings_setup);

uint8_t* __sample_setup_event_bus(uint8_t* pax_device_handle);
uint32_t __sample_event_bus_cbf_event_id_dma_block_done(uint8_t* pax_device_handle, uint16_t record_type, uint16_t record_len, uint8_t* dma_block_ptr);
uint32_t __sample_event_bus_cbf_event_id_hspme_hit(uint8_t* users_ctxt, uint16_t event_id, uint16_t record_len, uint8_t*record_value);

uint8_t* __sample_setup_pax_pme_engines_boyer_moore(uint8_t* pax_device_handle);
uint8_t* __sample_setup_pax_pme_pcre_engines(uint8_t* pax_device_handle);



#endif