/* testmini.c -- very simple test program for the miniLZO library

   This file is part of the LZO real-time data compression library.

   Copyright (C) 2011 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2010 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2009 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2008 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2007 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2006 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2005 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2004 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2003 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <pthread.h>

/*************************************************************************
// This program shows the basic usage of the LZO library.
// We will compress a block of data and decompress again.
//
// For more information, documentation, example programs and other support
// files (like Makefiles and build scripts) please download the full LZO
// package from
//    http://www.oberhumer.com/opensource/lzo/
**************************************************************************/

/* First let's include "minizo.h". */

#include "minilzo.h"


/* We want to compress the data block at 'in' with length 'IN_LEN' to
 * the block at 'out'. Because the input block may be incompressible,
 * we must provide a little more output space in case that compression
 * is not possible.
 */

#if defined(__LZO_STRICT_16BIT)
//#define IN_LEN      (8*1024u)
#define IN_LEN      (64)
#elif defined(LZO_ARCH_I086) && !defined(LZO_HAVE_MM_HUGE_ARRAY)
//#define IN_LEN      (60*1024u)
#define IN_LEN      (128)
#else
//#define IN_LEN      (128*1024ul)
#define IN_LEN      (256)
#endif
#define OUT_LEN     (IN_LEN + IN_LEN / 16 + 64 + 3)

static unsigned char __LZO_MMODEL in  [ IN_LEN ];
static unsigned char __LZO_MMODEL out [ OUT_LEN ];


/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);

#define TOTAL_MKT_MSGS_TO_PRODUCE	(1000*1000)
#define NUM_THREADS					8

struct nse_md_msg_s{
	int num_messages;
	lzo_uint compressed_len;
	unsigned char __LZO_MMODEL mkt_updt_message [ OUT_LEN ];
};

/*************************************************************************
//
**************************************************************************/

int generate_mkt_message(unsigned char* mkt_updt_message, int in_len)
{
	unsigned long idx;
	unsigned char* itr = &mkt_updt_message[0];
	for (idx=0;idx <in_len/sizeof(int);idx++ )
		itr[idx] = rand();
	return 0;
}


void print_data (unsigned char* ptr, unsigned long len)
{
	unsigned long j;

	for (j = 1; j <= len; j++)
		{
			printf("%02x ", (uint8_t)*ptr++);
			if (!(j%32))
				printf("\n");
		}
	printf("\n");
}

struct lzo_thread_node_s {
	unsigned long thread_start_idx;
	unsigned long thread_end_idx;
	struct nse_md_msg_s* p_md_msg_arr;
};

void* decompress_mkt_data_data_thrd_fn (void* thrd_arg)
{
	struct lzo_thread_node_s *p_lzo_thrd_node = (struct lzo_thread_node_s *)thrd_arg;
	struct nse_md_msg_s* p_md_msg_arr = p_lzo_thrd_node->p_md_msg_arr;
	unsigned char __LZO_MMODEL in_scratch  [ IN_LEN ];
	unsigned long idx = 0;
    unsigned long new_len = IN_LEN;
	int r1;
	
	for (idx =p_lzo_thrd_node->thread_start_idx; idx < p_lzo_thrd_node->thread_end_idx; idx++, p_md_msg_arr++;)
	{

		r1 = lzo1x_decompress(&p_md_msg_arr->mkt_updt_message[0],p_md_msg_arr->compressed_len,in_scratch,&new_len,NULL);
		if (r1 == LZO_E_OK) 
		{
	#if 0
			printf("printing de-compressed data\n");
			print_data(in_scratch,new_len);
	#endif
		}
		else
		{
			/* this should NEVER happen */
			printf("internal error - decompression failed: %d\n", r1);
			return 0;
		}
	}
}

int decompress_mkt_data_file()
{
	struct stat stat_buf ;
	uint64_t file_size = 0, read_size = 0;
	FILE *fp = fopen("nse_mkt_data.dat","r");
	struct nse_md_msg_s* p_nse_md_hive_nodes [NUM_THREADS];
	struct nse_md_msg_s* p_mkt_file_bfr =0;
	unsigned char* bfr =0;
	unsigned long msgs_parsed = 0, thrd_idx;
	pthread_t thr [NUM_THREADS]; 
	struct lzo_thread_node_s 
	//int mkt_msg_offset_in_node = (int)(&((struct nse_md_msg_s*)0)->mkt_updt_message[0]);
	struct lzo_thread_node_s* p_lzo_thread_hive [NUM_THREADS];

	int sizeof_node = sizeof(struct nse_md_msg_s*);

	if (!fp)
	{
		//printf("unable to open market msg file: %s\n", strerror(errno));
		return 0;
	}

	fstat(fileno(fp), &stat_buf);
	file_size = stat_buf.st_size;
	bfr = (unsigned char*) malloc(file_size);
	p_mkt_file_bfr =  (struct nse_md_msg_s*)(bfr);

	for (thrd_idx=0; thrd_idx< NUM_THREADS; thrd_idx++)
	{
		p_nse_md_hive_nodes [thrd_idx] = (struct nse_md_msg_s*) malloc (sizeof_node * (TOTAL_MKT_MSGS_TO_PRODUCE/NUM_THREADS) );
		p_lzo_thread_hive[thrd_idx]->p_md_msg_arr = p_nse_md_hive_nodes [thrd_idx];
	}

	while (msgs_parsed < (TOTAL_MKT_MSGS_TO_PRODUCE/NUM_THREADS) && thrd_idx < NUM_THREADS)
	{
		msgs_parsed++;
		if (msgs_parsed >= (TOTAL_MKT_MSGS_TO_PRODUCE/NUM_THREADS)) 
		{
			thrd_idx++;
			msgs_parsed = 0;
		}
		
	}

	for (thrd_idx=0; thrd_idx< NUM_THREADS; thrd_idx++)
		pthread_create(&thr[thrd_idx], NULL/*&threadAttributes*/, decompress_mkt_data_data_thrd_fn, (void*)p_md_msg_arr[thrd_idx]);

	// wait for all threads to finish...
	for (thrd_idx=0; thrd_idx< NUM_THREADS; thrd_idx++)
		pthread_join(thr[thrd_idx], NULL);


}

int prepapre_mkt_data_file()
{
    lzo_uint in_len;
    lzo_uint out_len;
    lzo_uint delta_len;
	struct nse_md_msg_s nse_mkt_msg = {1,0,{0}}; 
	FILE *fp;
	int num_mkt_msgs_to_produce = TOTAL_MKT_MSGS_TO_PRODUCE;
	time_t seconds;
	unsigned long num_messages = 0, num_msg_bytes = 0, num_compressed_bytes = 0;

	fp = fopen("nse_mkt_data.dat","w");

	if (!fp)
	{
		//printf("unable to open market msg file: %s\n", strerror(errno));
		return 0;
	}
	
	do 
	{
		unsigned long bytes_to_write = 0, j;
		unsigned char* ptr = &in[0];


	/*	
		Step 2.1
		Initialize the input buffer with some random set of data
	*/
		generate_mkt_message(in,in_len);

#if 0
		printf("printing uncompressed data\n");
		print_data(ptr,in_len);
#endif

	/*
	* Step 3: compress from 'in' to 'out' with LZO1X-1
	*/		
		num_messages++;

			r2 = lzo1x_1_compress(in,
							in_len,
							&nse_mkt_msg.mkt_updt_message[0],
							&nse_mkt_msg.compressed_len,
							wrkmem);

		if (r2 == LZO_E_OK) 
		{
			num_msg_bytes += in_len;
			num_compressed_bytes += nse_mkt_msg.compressed_len;
#if 0
			printf("printing compressed data. len = %lu\n",nse_mkt_msg.compressed_len );
			ptr = &nse_mkt_msg.mkt_updt_message[0];
			print_data(&nse_mkt_msg.mkt_updt_message[0],nse_mkt_msg.compressed_len);
#endif
		}
		else
		{
			/* this should NEVER happen */
			printf("internal error - compression failed: %d\n", r1);
			return 2;
		}
		/* check for an incompressible block */
		if (nse_mkt_msg.compressed_len >= in_len)
		{
			printf("This block contains incompressible data.\n");
			return 0;
		}

		/*
			Step 3.1: 
			Compute the difference in the in record length's after compression
			Subtract it from the market message record byte array and then commit the market message to a file
		*/
		delta_len = OUT_LEN - nse_mkt_msg.compressed_len;
		bytes_to_write = sizeof(nse_mkt_msg)-delta_len;
		r3 = fwrite (&nse_mkt_msg,bytes_to_write,1,fp);
		if (0 == r3) 
		{
			printf ("error writing to file: %s\n", strerror(ferror(fp)));
		}

	} while (--num_mkt_msgs_to_produce > 0);

	printf("------\n");
	printf ("total_mkt_messages:%lu, total_in_bytes: %lu, total_compressed_bytes:%lu\n", 
				num_messages, 
				num_msg_bytes < 1024 ? num_msg_bytes:num_msg_bytes/1024,
				num_compressed_bytes < 1024 ? num_compressed_bytes: num_compressed_bytes/1024 );
	
	fclose (fp);

}

int main(int argc, char *argv[])
{
    int r1,r2,r3;
    lzo_uint in_len;
    lzo_uint out_len;
    lzo_uint new_len, delta_len;


    if (argc < 0 && argv == NULL)   /* avoid warning about unused args */
        return 0;

    printf("\nLZO real-time data compression library (v%s, %s).\n",
           lzo_version_string(), lzo_version_date());
    printf("Copyright (C) 1996-2011 Markus Franz Xaver Johannes Oberhumer\nAll Rights Reserved.\n\n");


/*
 * Step 1: initialize the LZO library
 */
    if (lzo_init() != LZO_E_OK)
    {
        printf("internal error - lzo_init() failed !!!\n");
        printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable '-DLZO_DEBUG' for diagnostics)\n");
        return 3;
    }

	time(&seconds);
	srand((unsigned int) seconds);

	prepapre_mkt_data_file();



    printf("\nminiLZO simple compression test passed.\n");
    return 0;
}

/*
vi:ts=4:et
*/

