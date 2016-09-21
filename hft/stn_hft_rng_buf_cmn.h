
#ifndef STN_HFT_RNG_BUF_COMMON_H
#define STN_HFT_RNG_BUF_COMMON_H

#include <stdint.h>

#define STN_HFT_CHNL_HP_BUFFER_SIZE_MAX				(64*1024*1024) // 64 MB buffer size
#define STN_HFT_CHNL_MSG_SIZE_MAX					(1520+8) // mkt_channel_data_s structure
#define STN_HFT_CHNL_HP_BUFFER_CNT_MAX				(STB_HFT_CHNL_HP_BUFFER_SIZE_MAX/STN_HFT_CHNL_MSG_SIZE_MAX)


struct __hft_channel_rng_buf_entry_s {
	uint64_t data_length;	// payload length received
	uint8_t data [1520];	// aligned to 8 byte
};


#endif