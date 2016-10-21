#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "stn_huge_page_impl.h"
#include "console_log.h"

#define TWO_MB_PAGE	(2*1024*1024)

void stn_huge_page_memory_release(unsigned char* pBlock,unsigned long long size)
{
	unsigned long modulo = size % TWO_MB_PAGE;
	unsigned long RoundedSize = (size-modulo)+TWO_MB_PAGE;
	if(munmap(pBlock,size) == -1)
	{
		console_log_write("%s:%d numa_node_release_memory:unmap failed\n",__FILE__,__LINE__);
		return;
	}
}


unsigned char* stn_huge_page_memory_allocate(unsigned long long neededBytes)
{
	unsigned char* pBlock = 0;
	int huge_page_fs_fd = open("/hugetlbfs/hugepagefile",O_CREAT|O_RDWR,07555);

	unsigned long modulo = neededBytes % TWO_MB_PAGE;
	unsigned long RoundedSize = (neededBytes-modulo)+TWO_MB_PAGE;

	if(huge_page_fs_fd == -1)
	{
		console_log_write("%s:%d stn_huge_page_memory_allocate: open hugepage failed\n",__FILE__,__LINE__);
		fflush(stdout);
		return 0;
	}

	pBlock = mmap(0, RoundedSize, PROT_READ|PROT_WRITE, MAP_PRIVATE, huge_page_fs_fd, 0);	
	if(MAP_FAILED == pBlock)
	{
 		close(huge_page_fs_fd);
		console_log_write("%s:%d stn_huge_page_memory_allocate: memory mapping failed:%u\n",__FILE__,__LINE__,RoundedSize);
		fflush(stdout);
		return 0;
	}
	bzero(pBlock,RoundedSize);
	close(huge_page_fs_fd);
	
	return pBlock;
}

