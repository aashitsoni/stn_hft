
//# pax_numa_impl.h


#ifndef _PAX_NUMA_IMPL_H_
#define _PAX_NUMA_IMPL_H_


#define CPU1 0x1
#define CPU2 0x2
#define CPU3 0x4
#define CPU4 0x8
#define CPU5 0x10
#define CPU6 0x20
#define CPU7 0x40
#define CPU8 0x80


#define NUMA_SHARED		MAP_SHARED
#define NUMA_PRIVATE	MAP_PRIVATE
#define TWOMB			2048

unsigned char* __stn_numa_node_allocate_memory(int,unsigned long,int);
void __stn_numa_node_release_memory(unsigned char*,unsigned long);
int __stn_hft_get_numa_node(int cpu_id);
int __stn_hft_set_thread_affinity(int cpu_id);


#endif

