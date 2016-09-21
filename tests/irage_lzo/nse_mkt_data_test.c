/* testmini.c -- very simple test program for the miniLZO library

   This file is part of the LZO real-time data compression library.

   Copyright (C) 2011 Markus Franz Xaver Johannes Oberhumer
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/time.h>


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

#define TOTAL_MKT_MSGS_TO_PRODUCE	(4 * 1000*1000)


struct nse_md_msg_s{
	int num_messages;
	int bytes_written; // includes header + lzo compressed length
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


int decompress_data ()
{
	struct nse_md_msg_s *p_nse_mkt_msg =0;
	unsigned char __LZO_MMODEL in_scratch  [ IN_LEN ];
	FILE* fp = fopen("nse_mkt_data.dat","r");
	struct stat stat_buf ;
	uint64_t file_size = 0;
	int items_read = 0, idx;
	struct timeval	tv1, tv2;

	if (!fp)
	{
		//printf("unable to open market msg file: %s\n", strerror(errno));
		return 0;
	}

	fstat(fileno(fp), &stat_buf);
	file_size = stat_buf.st_size;
	
	p_nse_mkt_msg = (struct nse_md_msg_s *)malloc (file_size);
	items_read = fread (p_nse_mkt_msg,file_size,1,fp);
	if (!items_read) {
		int error = ferror(fp);
		fprintf(stderr, "Error reading file_header. Code %d\n", error);
	}


	gettimeofday(&tv1, NULL);

	for (idx =0; idx < TOTAL_MKT_MSGS_TO_PRODUCE; idx ++)
	{

	/*
	* Step 4: decompress again, now going from 'out' to 'in'
	*/
		unsigned long new_len = IN_LEN;
		int r1 = lzo1x_decompress(&p_nse_mkt_msg->mkt_updt_message[0],p_nse_mkt_msg->compressed_len,in_scratch,&new_len,NULL);

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
			return 1;
		}
		p_nse_mkt_msg = (struct nse_md_msg_s *) (((unsigned char*)p_nse_mkt_msg) + p_nse_mkt_msg->bytes_written);
	}

	gettimeofday(&tv2, NULL);

    if (tv1.tv_usec > tv2.tv_usec)
    {
		// USEC component of the 1st measurement can be numerically greater than the USEC component of the 2nd measurement
		// So adjust it..
        tv2.tv_sec--;
        tv2.tv_usec += 1000000;
    }
	fprintf (stderr, "decompression done.\n\n");

	{
		float divisor = (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec)/ 1000000.00;
		float processing_rate = ((TOTAL_MKT_MSGS_TO_PRODUCE) / divisor) ;

		fprintf(stderr,"Result(sec.usec)         : %ld.%ld.\n", tv2.tv_sec - tv1.tv_sec, tv2.tv_usec - tv1.tv_usec);
		fprintf(stderr,"Processing rate          : %f mmps\n",processing_rate);
	}

	fclose (fp);
	return 0;
}

int main(int argc, char *argv[])
{
    int r2,r3;
    lzo_uint in_len;
    lzo_uint out_len;
    lzo_uint delta_len;
	struct nse_md_msg_s nse_mkt_msg = {1,0,0,{0}}; 
	FILE *fp;
	int num_mkt_msgs_to_produce = TOTAL_MKT_MSGS_TO_PRODUCE;
	time_t seconds;
	unsigned long num_messages = 0, num_msg_bytes = 0, num_compressed_bytes = 0;


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


/*
 * Step 2: prepare the input block that will get compressed.
 *         We just fill it with zeros in this example program,
 *         but you would use your real-world data here.
 */
    in_len = IN_LEN;
    lzo_memset(in,0,in_len);

	fp = fopen("nse_mkt_data.dat","w");
	if (!fp)
	{
		//printf("unable to open market msg file: %s\n", strerror(errno));
		return 0;
	}
	
	do 
	{
		unsigned long bytes_to_write = 0;

	/*	
		Step 2.1
		Initialize the input buffer with some random set of data
	*/
		generate_mkt_message(in,in_len);

#if 0
		printf("printing uncompressed data\n");
		print_data((unsigned char*)in,in_len);
#endif

	/*
	* Step 3: compress from 'in' to 'out' with LZO1X-1
	*/		
		num_messages++;

		out_len = 0;
		r2 = lzo1x_1_compress(in,
							in_len,
							&nse_mkt_msg.mkt_updt_message[0],
							&nse_mkt_msg.compressed_len,
							wrkmem);

		if (r2 == LZO_E_OK) 
		{
			//printf("compressed %lu bytes into %lu bytes\n",
			//	(unsigned long) in_len, (unsigned long) out_len);
			//printf("compressed %lu bytes into mkt_dat array %lu bytes\n",
			//	(unsigned long) in_len, (unsigned long) nse_mkt_msg.compressed_len);
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
			printf("internal error - compression failed: %d\n", r2);
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
			Subtract it from the market message record byte array and then commit 
		the market message to a file
		*/
		delta_len = OUT_LEN - nse_mkt_msg.compressed_len;
		bytes_to_write = sizeof(nse_mkt_msg)-delta_len;
		nse_mkt_msg.bytes_written = bytes_to_write;
		//printf ("delta_len: %lu, bytes_to_write:%lu\n", delta_len, bytes_to_write);
		r3 = fwrite (&nse_mkt_msg,bytes_to_write,1,fp);
		if (0 == r3) 
		{
			printf ("error writing to file: %s\n", strerror(ferror(fp)));
		}

	} while (--num_mkt_msgs_to_produce > 0);

	printf("------\n");
	printf ("total_mkt_messages:%lu, total_in_bytes: %lu KB, total_compressed_bytes:%lu KB\n", 
				num_messages, 
				num_msg_bytes < 1024 ? num_msg_bytes:num_msg_bytes/1024,
				num_compressed_bytes < 1024 ? num_compressed_bytes: num_compressed_bytes/1024 );
	
	fclose (fp);

	printf("begining decompression...\n");
	decompress_data ();

    printf("\nminiLZO simple compression test passed.\n");
    return 0;
}

/*
vi:ts=4:et
*/

