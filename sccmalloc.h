#ifndef SCCMALLOC_H
#define SCCMALLOC_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define LOCAL_LUT   0x14
#define REMOTE_LUT  (LOCAL_LUT + local_pages)


extern void *remote;
extern unsigned char local_pages;
extern uintptr_t shmem_start_address;

typedef struct {
  unsigned char node, lut;
  uint32_t offset;
} lut_addr_t;

lut_addr_t SCCPtr2Addr(void *p);
void *SCCAddr2Ptr(lut_addr_t addr);

void SCCInit(unsigned char size);
void SCCStop(void);

void *SCCMallocPtr(size_t size);
unsigned char SCCMallocLut(size_t size);
void SCCFree(void *p);

uintptr_t get_ptr(void* addr);
void* get_addr(uintptr_t i_addr);

/*
 * Allocates memory of size s.
 * RETURNS: pointer to memory.
 */

void *SNetMemAlloc( size_t s);

/*
 * Frees the memory pointed to by ptr.
 * RETURNS: nothing.
 */

void SNetMemFree( void *ptr);


#endif
