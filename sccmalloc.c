#include <fcntl.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>

#include "scc.h"
#include "sccmalloc.h"
#include "bool.h"
//#include "configuration.h"





typedef union block {
  struct {
    union block *next;
    size_t size;
  } hdr;
  uint32_t align;   // Forces proper allignment
} block_t;

typedef struct {
  unsigned char free;
  unsigned char size;
} lut_state_t;

void *remote;
unsigned char local_pages;

static void *local;
static int mem, cache;
static block_t *freeList;
static lut_state_t *lutState;
static unsigned char remote_pages;
uintptr_t shmem_start_address;

lut_addr_t SCCPtr2Addr(void *p)
{
  uint32_t offset;
  unsigned char lut;

  if (local <= p && p <= local + local_pages * PAGE_SIZE) {
    offset = (p - local) % PAGE_SIZE;
    lut = LOCAL_LUT + (p - local) / PAGE_SIZE;
  } else if (remote <= p && p <= remote + remote_pages * PAGE_SIZE) {
    offset = (p - remote) % PAGE_SIZE;
    lut = REMOTE_LUT + (p - remote) / PAGE_SIZE;
  } else {
    printf("Invalid pointer\n");
  }

  lut_addr_t result = {node_location, lut, offset};
  return result;
}

void *SCCAddr2Ptr(lut_addr_t addr)
{
  if (LOCAL_LUT <= addr.lut && addr.lut < LOCAL_LUT + local_pages) {
    return (void*) ((addr.lut - LOCAL_LUT) * PAGE_SIZE + addr.offset + local);
  } else if (REMOTE_LUT <= addr.lut && addr.lut < REMOTE_LUT + remote_pages) {
    return (void*) ((addr.lut - REMOTE_LUT) * PAGE_SIZE + addr.offset + remote);
  } else {
    printf("Invalid SCC LUT address\n");
  }

  return NULL;
}

void SCCInit(unsigned char size)
{
  local_pages = size;
  remote_pages = remap ? MAX_PAGES - size : 1;

  /* Open driver device "/dev/rckdyn011" to map memory in write-through mode */
  mem = open("/dev/rckdyn011", O_RDWR|O_SYNC);
  printf("mem: %i\n", mem);
  if (mem < 0) {
	printf("Opening /dev/rckdyn011 failed!\n");
  }	
  cache = open("/dev/rckdcm", O_RDWR|O_SYNC);
  printf("cache: %i\n",cache);
  if (cache < 0) {
	 printf("Opening /dev/rckdcm failed!\n");
  }

  unsigned int *nkAddr = 2572472320;
  
  //local = mmap(NULL, local_pages * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, cache, LOCAL_LUT << 24);
  local = mmap((void*)nkAddr, local_pages * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, cache, LOCAL_LUT << 24);
  if (local == NULL) printf("Couldn't map memory!");

	printf("nkAddr:		%p\n",nkAddr);
	printf("local:		%p\n",local);

  remote = mmap(NULL, remote_pages * PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, cache, REMOTE_LUT << 24);
  if (remote == NULL) printf("Couldn't map memory!");
	//printf("\n\nlocal_pages * PAGE_SIZE = (%d * %d) = %d \n\n",local_pages,PAGE_SIZE,(local_pages*PAGE_SIZE));
	shmem_start_address = (uintptr_t)local;
  freeList = local+0x10;
  freeList->hdr.next = freeList;
  freeList->hdr.size = (size * PAGE_SIZE) / sizeof(block_t);

  if (remap) {
    lutState = SNetMemAlloc(remote_pages * sizeof(lut_state_t));
    lutState[0].free = 1;
    lutState[0].size = remote_pages;
    lutState[remote_pages - 1] = lutState[0];
  }
}

void SCCStop(void)
{
  munmap(remote, remote_pages * PAGE_SIZE);
  munmap(local, local_pages * PAGE_SIZE);

  close(mem);
  close(cache);
}

void *SCCMallocPtr(size_t size)
{
  size_t nunits;
  block_t *curr, *prev, *new;


  if (freeList == NULL) printf("Couldn't allocate memory!");

  prev = freeList;
  curr = prev->hdr.next;
  nunits = (size + sizeof(block_t) - 1) / sizeof(block_t) + 1;

  do {
    if (curr->hdr.size >= nunits) {
      if (curr->hdr.size == nunits) {
        if (prev == curr) prev = NULL;
        else prev->hdr.next = curr->hdr.next;

      } else if (curr->hdr.size > nunits) {
	new = curr + nunits;
	*new = *curr;
        new->hdr.size -= nunits;
  	curr->hdr.size = nunits;

        if (prev == curr) prev = new;{
		prev->hdr.next = new;
      	}
      }
      freeList = prev;
      return (void*) (curr + 1);
     }
  } while (curr != freeList && (prev = curr, curr = curr->hdr.next));


  printf("Couldn't allocate memory!");
  return NULL;
}

void SCCFreePtr(void *p)
{
  block_t *block = (block_t*) p - 1,
          *curr = freeList;

  if (freeList == NULL) {
    freeList = block;
    freeList->hdr.next = freeList;
    return;
  }

  while (!(block > curr && block < curr->hdr.next)) {
    if (curr >= curr->hdr.next && (block > curr || block < curr->hdr.next)) break;
    curr = curr->hdr.next;
  }


  if (block + block->hdr.size == curr->hdr.next) {
    block->hdr.size += curr->hdr.next->hdr.size;
    if (curr == curr->hdr.next) block->hdr.next = block;
    else block->hdr.next = curr->hdr.next->hdr.next;
  } else {
    block->hdr.next = curr->hdr.next;
  }

  if (curr + curr->hdr.size == block) {
    curr->hdr.size += block->hdr.size;
    curr->hdr.next = block->hdr.next;
  } else {
    curr->hdr.next = block;
  }

  freeList = curr;
}

unsigned char SCCMallocLut(size_t size)
{
  lut_state_t *curr = lutState;

  do {
    if (curr->free && curr->size >= size) {
      if (curr->size == size) {
        curr->free = 0;
        curr[size - 1].free = 0;
      } else {
        lut_state_t *next = curr + size;

        next->free = 1;
        next->size = curr->size - size;
        next[next->size - 1] = next[0];

        curr->free = 0;
        curr->size = size;
        curr[curr->size - 1] = curr[0];
      }

      return REMOTE_LUT + curr - lutState;
    }

    curr += curr->size;
  } while (curr < lutState + remote_pages);

  printf("Not enough available LUT entries!\n");
  return 0;
}

void SCCFreeLut(void *p)
{
  lut_state_t *lut = lutState + (p - remote) / PAGE_SIZE;

  if (lut + lut->size < lutState + remote_pages && lut[lut->size].free) {
    lut->size += lut[lut->size].size;
  }

  if (lutState < lut && lut[-1].free) {
    lut -= lut[-1].size;
    lut->size += lut[lut->size].size;
  }

  lut->free = 1;
  lut[lut->size - 1] = lut[0];
}

void SCCFree(void *p)
{
  if (local <= p && p <= local + local_pages * PAGE_SIZE) {
    SCCFreePtr(p);
  } else if (remote <= p && p <= remote + remote_pages * PAGE_SIZE) {
    SCCFreeLut(p);
  }
}

uintptr_t get_ptr(void* addr)
{
  return ((uintptr_t) addr - shmem_start_address);
}


void* get_addr(uintptr_t i_addr)
{
  return (void*) (shmem_start_address + i_addr);
}

void *SNetMemAlloc( size_t s) {
  
  void *ptr;

  if( s == 0) {
    ptr = NULL;
  }
  else {
    ptr = malloc( s);
    if( ptr == NULL) {
      printf("\n\n** Fatal Error ** : Unable to Allocate Memory.\n\n");
      exit(1);
    }
  }

  return( ptr);
}


void SNetMemFree( void *ptr) {

  free( ptr);
  ptr = NULL;
}
