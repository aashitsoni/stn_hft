
#include <numa.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sched.h>
#include <unistd.h>
#include <sched.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "stn_numa_impl.h"


pid_t gettid()
{
	long ret = syscall(__NR_gettid);
	return(pid_t)ret;
	
}

int get_cpu_mask(int cpu_id)
{
	return (1<<cpu_id);
}


int __stn_hft_get_numa_node(int cpu_id)
{
	int node_id = -1;

	
	node_id = numa_node_of_cpu(cpu_id);

	if(-1 == node_id)
		{
		printf("Error: Invalid cpu id : %d",cpu_id);
		return node_id;
		}
	return node_id;
	

}

int __stn_hft_set_thread_affinity(int cpu_id)
{

	pid_t	tid = 0;

	unsigned int cpu_mask = get_cpu_mask(cpu_id);
	unsigned int len = sizeof(cpu_mask);
	
	tid = gettid();
	
	if(sched_setaffinity(tid,len,(cpu_set_t*)&cpu_mask))
		{
		printf("Error: Setting up cpu_affinity :%d,mask :0x%x\n",cpu_id,cpu_mask);
		return -1;
		}
	return 0;

}


void __stn_numa_node_release_memory(unsigned char* pBlock,unsigned long size)
{
    if(pBlock)
        free(pBlock);
    return;
#if 0
	unsigned long modulo = size % TWOMB;
	unsigned long RoundedSize = (size-modulo)+TWOMB;
	if(munmap(pBlock,size) == -1)
	{
		printf("\nnuma_node_release_memory:unmap failed");
		return;
	}
#endif    
}


unsigned char* __stn_numa_node_allocate_memory(int requested_node,unsigned long neededBytes,int flag)
{

    unsigned char* pBlock = 0;
    pBlock = calloc(1,neededBytes);
   

#if 0 
	struct bitmask* requested_node_mask;
	struct bitmask* old_node_mask;
	int huge_page_fs_fd = open("/hugetlbfs/hugepagefile",O_CREAT|O_RDWR,07555);

	unsigned long modulo = neededBytes % TWOMB;
	unsigned long RoundedSize = (neededBytes-modulo)+TWOMB;


	if(huge_page_fs_fd == -1)
	{
		printf("\numa_node_allocate_memory:open hugepage failed");
		return 0;

	}
	old_node_mask = numa_get_membind();

	requested_node_mask = numa_bitmask_alloc(8);

	if(requested_node_mask == 0)
	{
		printf("\nnuma_node_allocate_mememory failed : node mask allocation");
		return 0;

	}
	
	numa_bitmask_setbit(requested_node_mask,requested_node);

	numa_set_membind(requested_node_mask);

	pBlock = mmap(0,neededBytes,PROT_READ|PROT_WRITE,flag,huge_page_fs_fd,0);	
	if(MAP_FAILED == pBlock)
	{
 		close(huge_page_fs_fd);
		numa_set_membind(old_node_mask);
		printf("\nnuma_node_allocate: memory mapping failed:%u",RoundedSize);
		return 0;
	}

	numa_set_membind(old_node_mask);
	close(huge_page_fs_fd);

#endif
	return pBlock;
}

