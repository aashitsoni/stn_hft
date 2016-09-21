
#include "pax_tests.h"


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

int get_nic_stats(char* device, int hw_id)
{
	char *nic_vid_template = (5 == hw_id)? "PAX-D-NIC-5-X-YYYYY" : ((3 == hw_id)?"PAX-D-NIC-3-X-YYYYY" : 0);
	struct pax_lib_param_s pax_lib_parm = {
				.ip_ip_pkt_pool = 0, // default..
				.message_bus_size =0, // default
				.num_pkt_dma_blocks = 24,
				.dma_page_mode = PAX_DMA_PAGE_MODE_HUGE_PTE
	};


	struct pax_device_creation_param_s pdc_params = {
				.no_data_timeout_ms = 500,
				.steering_policy = PAX_NIC_LB_POLICY_FREE,
				.stream_merge_policy = PAX_NIC_MERGE_ALL_PORTS,
				.device_open_mode = PAX_DEVICE_OPEN_MODE_DIAGNOSTICS,
				.master.nic_PCIE_info.remapped_device_number = 0
			};

	struct pax_device_info_s dev_info_vec [16] ={0};
	int num_devices_in_chassis =0;
	uint8_t* pax_devices_handles[PAX_MAX_DEVICES_PER_CHASSIS] = {0};
	uint8_t hw_descr_type;

	strcpy (pdc_params.master.nic_series_string,nic_vid_template);
	strcpy(pdc_params.master.nic_name,device);

	//now get the list of the NIC's in the chassis..
	pax_get_devices_in_chassis( pdc_params.master.nic_series_string, &dev_info_vec[0], &num_devices_in_chassis);

	int idx;
	//num_devices_in_chassis =1;
	for (idx =0; idx <num_devices_in_chassis; idx ++ )
	{
		memcpy(&pdc_params.master.nic_PCIE_info, &dev_info_vec[idx],sizeof(struct pax_device_info_s));
		// initialize the HARDWARE..
		if (STN_ERRNO_SUCCESS != pax_device_init(&pdc_params,&pax_devices_handles[idx],&hw_descr_type))
			return -1;
	}

	for (;;)
	{
		for (idx =0; idx <num_devices_in_chassis; idx ++ )
			pax_print_port_stats (pax_devices_handles[idx]);
		sleep(1);
	}

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

int get_pci_bus_read_latency(char* device, int hw_id)
{
	char *nic_vid_template = (5 == hw_id)? "PAX-D-NIC-5-X-YYYYY" : ((3 == hw_id)?"PAX-D-NIC-3-X-YYYYY" : 0);
	struct pax_lib_param_s pax_lib_parm = {
				.ip_ip_pkt_pool = 0, // default..
				.message_bus_size =0, // default
				.num_pkt_dma_blocks = 24,
				.dma_page_mode = PAX_DMA_PAGE_MODE_HUGE_PTE
	};


	struct pax_device_creation_param_s pdc_params = {
				.no_data_timeout_ms = 500,
				.steering_policy = PAX_NIC_LB_POLICY_FREE,
				.stream_merge_policy = PAX_NIC_MERGE_ALL_PORTS,
				.device_open_mode = PAX_DEVICE_OPEN_MODE_DIAGNOSTICS,
				.master.nic_PCIE_info.remapped_device_number = 0
			};

	struct pax_device_info_s dev_info_vec [16] ={0};
	int num_devices_in_chassis =0;
	uint8_t* pax_devices_handles[PAX_MAX_DEVICES_PER_CHASSIS] = {0};
	uint8_t hw_descr_type;

	strcpy (pdc_params.master.nic_series_string,nic_vid_template);
	strcpy(pdc_params.master.nic_name,device);

	//now get the list of the NIC's in the chassis..
	pax_get_devices_in_chassis( pdc_params.master.nic_series_string, &dev_info_vec[0], &num_devices_in_chassis);

	int idx;
	//num_devices_in_chassis =1;
	for (idx =0; idx <num_devices_in_chassis; idx ++ )
	{
		memcpy(&pdc_params.master.nic_PCIE_info, &dev_info_vec[idx],sizeof(struct pax_device_info_s));
		// initialize the HARDWARE..
		if (STN_ERRNO_SUCCESS != pax_device_init(&pdc_params,&pax_devices_handles[idx],&hw_descr_type))
			return -1;
	}

	for (;;)
	{
		for (idx =0; idx <num_devices_in_chassis; idx ++ )
				pax_print_pci_bus_read_latency (pax_devices_handles[idx]);
		sleep(1);
	}

}
