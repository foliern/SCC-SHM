#ifndef LPEL_shmalloc_H
#define LPEL_shmalloc_H

#include "SCCAPI.h"
#include "scc.h"
#include "bool.h"

typedef struct lpel_block {
  t_vcharp space;          // pointer to space for data in block             
  size_t free_size;        // actual free space in block (0 or whole block)  
  struct lpel_block *next; // pointer to next block in circular linked list 
} LPEL_BLOCK;

typedef struct {
  LPEL_BLOCK *tail;     // "last" block in linked list of blocks           
} LPEL_BLOCK_S;

void LPEL_shmalloc_init();
t_vcharp LPEL_shmalloc();
void LPEL_shfree();

#endif /*RCCE_shmalloc_h*/
