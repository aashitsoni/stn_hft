/* testmini.c -- very simple test program for the miniLZO library


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
#include <sys/time.h>
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

//static unsigned char __LZO_MMODEL in  [ IN_LEN ];
//static unsigned char __LZO_MMODEL out [ OUT_LEN ];


/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);


struct comp_digest_s{
	lzo_uint compressed_len;
	unsigned char *orig_msg;
	unsigned char *comp_msg;
};

/*************************************************************************
//
**************************************************************************/


void* decompress_mkt_data_data_thrd_fn (void* thrd_arg)
{
	struct comp_digest_s *p_compr_digest = (struct comp_digest_s *)thrd_arg;
	unsigned char __LZO_MMODEL decomp_scratch  [ 12*1024 ];
	unsigned long idx = 0;
	int r1; 
	lzo_uint decomp_length;
	struct timeval	tv1, tv2;

	gettimeofday(&tv1, NULL);

	for (idx =0; idx < 10; idx++)
	{
		decomp_length = 0;

		r1 = lzo1x_decompress(p_compr_digest->comp_msg, 
							  p_compr_digest->compressed_len, 
							  decomp_scratch, 
							  &decomp_length,
							  NULL);
#if 0
		if (r1 == LZO_E_OK) 
		{
			printf("data de-compressed successfully. decomp size: %lu\n", decomp_length);
			if (0 == memcmp(p_compr_digest->orig_msg,decomp_scratch,decomp_length))
				printf("decomp data matches\n");
			else 
				printf("decomp data does not match\n");
		}
		else
		{
			/* this should NEVER happen */
			printf("internal error - decompression failed: %d\n", r1);
			return 0;
		}
#endif
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
		float total_time= (tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec)/ 1000000.00;
		float avg_processing_time = ((total_time) / 10) ;

		fprintf(stderr,"Total (sec.usec) : %ld.%ld, Proc time: %f\n", tv2.tv_sec - tv1.tv_sec, tv2.tv_usec - tv1.tv_usec, avg_processing_time);
	}


	return 0;
}


int irage_test_comp()
{
	FILE *fp;
	unsigned char __LZO_MMODEL test_buff_in[12*1024] = {0};
	unsigned char __LZO_MMODEL test_buff_out[12*1024] = {0};
	int r2;
	lzo_uint comp_len = 0;

	fp = fopen("uncompPkt.bin","r");

	if (!fp)
	{
		printf("unable to open irage test file: %s\n", strerror(errno));
		return 0;
	}
	
	do 
	{
		struct stat stat_buf ;
		uint64_t file_size = 0;
		struct comp_digest_s comp_dgst = {0};
		
		fstat(fileno(fp), &stat_buf);
		file_size = stat_buf.st_size;
	
		fread( test_buff_in, file_size, 1, fp);;
		
		/*
		 * Step 3: compress from 'in' to 'out' with LZO1X-1
		 */		

		r2 = lzo1x_1_compress(test_buff_in,
						file_size,
						&test_buff_out[0],
						&comp_len,
						wrkmem);

		if (r2 == LZO_E_OK) 
		{
			printf("original size: %lu, compressed size = %lu\n", file_size, comp_len);
		}
		else
		{
			/* This should NEVER happen */
			printf("internal error - compression failed: %d\n", r2);
			return 2;
		}
		/* check for an incompressible block */
		if (comp_len >= file_size)
		{
			printf("This block contains incompressible data.\n");
			return 0;
		}
		comp_dgst.orig_msg = &test_buff_in[0];
		comp_dgst.comp_msg = &test_buff_out[0];
		comp_dgst.compressed_len = comp_len;
		decompress_mkt_data_data_thrd_fn(&comp_dgst);

	} while (0);

	fclose (fp);
	
	return 0;
}

int main(int argc, char *argv[])
{
 	time_t seconds;

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

	irage_test_comp();
    printf("\nminiLZO simple compression test passed.\n");
    return 0;
}

/*
vi:ts=4:et
*/

