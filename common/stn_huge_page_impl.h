

#ifndef _STN_HUGE_PAGEIMPL_H_
#define _STN_HUGE_PAGEIMPL_H_

#include <stdint.h>

unsigned char* stn_huge_page_memory_allocate(unsigned long long neededBytes);
void stn_huge_page_memory_release(unsigned char* pBlock,unsigned long long size);


#endif

