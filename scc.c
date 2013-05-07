#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h> /*for uint16_t*/
#include <signal.h> /*for sigmask*/

#include "SCCAPI.h"
#include "scc.h"
#include "bool.h"
#include "sccmalloc.h"

int node_location;
static int num_nodes = 8;
bool remap =false;

t_vcharp mpbs[CORES];
t_vcharp locks[CORES];
volatile int *irq_pins[CORES];
volatile uint64_t *luts[CORES];
AIR atomic_inc_regs[2*CORES];

void SccInit(void *info){
//void main(int argc, char** argv){

  //(void) info; /* NOT USED */
  int x, y, z, address;
	int *air_baseE, *air_baseF;
	
  InitAPI(0);
  z = ReadConfigReg(CRB_OWN+MYTILEID);
  x = (z >> 3) & 0x0f; // bits 06:03
  y = (z >> 7) & 0x0f; // bits 10:07
  z = z & 7; // bits 02:00
  node_location = PID(x, y, z);
	
	air_baseE = (int *) MallocConfigReg(FPGA_BASE + 0xE000);
	air_baseF = (int *) MallocConfigReg(FPGA_BASE + 0xF000);
	printf("going to for loop to ini all\n");
	unsigned char cpu;
  for (cpu = 0; cpu < CORES; cpu++) {
    x = X_PID(cpu);
    y = Y_PID(cpu);
    z = Z_PID(cpu);

    if (cpu == node_location) address = CRB_OWN;
    else address = CRB_ADDR(x, y);
    
		irq_pins[cpu] = MallocConfigReg(address + (z ? GLCFG1 : GLCFG0));
    locks[cpu] = (t_vcharp) MallocConfigReg(address + (z ? LOCK1 : LOCK0));//printf("locks for %d\n",cpu);
		luts[cpu] = (uint64_t*) MallocConfigReg(address + (z ? LUT1 : LUT0));//printf("luts for %d\n",cpu);
		
    MPBalloc(&mpbs[cpu], x, y, z, cpu == node_location);//printf("mpballoc for %d\n",cpu);	
		
		//First Set of AIR
    atomic_inc_regs[cpu].counter = air_baseE + 2*cpu;
    atomic_inc_regs[cpu].init = air_baseE + 2*cpu + 1;
		if(node_location == 0){
			// only one core needs to call this
			*atomic_inc_regs[cpu].init = 0;
		}
		 //printf ("first air for %d\n",cpu);
		//Second Set of AIR
		atomic_inc_regs[CORES+cpu].counter = air_baseF + 2*cpu;
    atomic_inc_regs[CORES+cpu].init = air_baseF + 2*cpu + 1;
		if(node_location == 0){
			// only one core needs to call this
			*atomic_inc_regs[CORES+cpu].init = 0;
		}//printf ("second air for %d\n",cpu);
  }
	
	printf("going to for loop to ini luts\n");
	//variables for the LUT init
	remap=true;
	int i, lut;
	unsigned char num_pages = PAGES_PER_CORE - (LINUX_PRIV_PAGES+1);
	//int max_pages = MAX_PAGES - 1;
	int max_pages = remap ? MAX_PAGES/2 : MAX_PAGES - 1;

	for (lut = 0; lut < 10; lut++) {
		printf("Copy to %i  node's LUT entry Nr.: %i / %x from= %i node's LUT entry Nr.: %i / %x \n",
                  node_location,num_pages,num_pages,num_nodes+1,  lut, lut);
		LUT(node_location,num_pages++) = LUT(num_nodes+1, lut);		
	}

		
	/*
		#define false 0
		#define true  1
	*/
	flush();
  START(node_location) = 0;
  END(node_location) = 0;
  /* Start with an initial handling run to avoid a cross-core race. */
  HANDLING(node_location) = 1;
  WRITING(node_location) = false;
	
	SCCInit(num_pages);
	
  FOOL_WRITE_COMBINE;
  unlock(node_location);

	
}

int SccGetNodeId(void) { 
  return node_location; 
}

bool SccIsRootNode(void) {
  return node_location == 0; 
}

void SccCleanup(){
	unsigned char cpu;
  for (cpu = 0; cpu < CORES; cpu++) {
    MPBunalloc(&mpbs[cpu]);
    FreeConfigReg((int*) locks[cpu]);
  }
}

//--------------------------------------------------------------------------------------
// FUNCTION: atomic_inc
//--------------------------------------------------------------------------------------
// Increments an AIR register and returns its privious content
//--------------------------------------------------------------------------------------
void atomic_inc(AIR *reg, int *value)
{
  (*value) = (*reg->counter);
}

//--------------------------------------------------------------------------------------
// FUNCTION: atomic_dec
//--------------------------------------------------------------------------------------
// Decrements an AIR register and returns its privious content
//--------------------------------------------------------------------------------------
void atomic_dec(AIR *reg, int value)
{
  (*reg->counter) = value;
}

//--------------------------------------------------------------------------------------
// FUNCTION: atomic_read
//--------------------------------------------------------------------------------------
// Returns the current value of an AIR register with-out modification to AIR
//--------------------------------------------------------------------------------------
void atomic_read(AIR *reg, int *value)
{
  (*value) = (*reg->init);
}

//--------------------------------------------------------------------------------------
// FUNCTION: atomic_write
//--------------------------------------------------------------------------------------
// Initializes an AIR register by writing a start value
//--------------------------------------------------------------------------------------
void atomic_write(AIR *reg, int value)
{
  (*reg->init) = value;
}


//***************************************************************************************
// Optimized memcpy routines from and to MPB
//***************************************************************************************
//
// Author: RWTH Aachen, adapted for RCCE by Rob F. Van der Wijngaart
//         Intel Corporation
// Date:   12/22/2010
//
//***************************************************************************************
// 
// Copyright 2010 Intel Corporation
// 
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
// 

//--------------------------------------------------------------------------------------
// FUNCTION: memcpy_get
//--------------------------------------------------------------------------------------
// optimized memcpy for copying data from MPB to private memory
//--------------------------------------------------------------------------------------
inline void *memcpy_get(void *dest, const void *src, size_t count)
{
        int h, i, j, k, l, m;     

        asm volatile (
                "cld;\n\t"
                "1: cmpl $0, %%eax ; je 2f\n\t"
                "movl (%%edi), %%edx\n\t"
                "movl 0(%%esi), %%ecx\n\t"
                "movl 4(%%esi), %%edx\n\t"
                "movl %%ecx, 0(%%edi)\n\t"
                "movl %%edx, 4(%%edi)\n\t"
                "movl 8(%%esi), %%ecx\n\t"
                "movl 12(%%esi), %%edx\n\t"
                "movl %%ecx, 8(%%edi)\n\t"
                "movl %%edx, 12(%%edi)\n\t"
                "movl 16(%%esi), %%ecx\n\t"
                "movl 20(%%esi), %%edx\n\t"
                "movl %%ecx, 16(%%edi)\n\t"
                "movl %%edx, 20(%%edi)\n\t"
                "movl 24(%%esi), %%ecx\n\t"
                "movl 28(%%esi), %%edx\n\t"
                "movl %%ecx, 24(%%edi)\n\t"
                "movl %%edx, 28(%%edi)\n\t"
                "addl $32, %%esi\n\t"
                "addl $32, %%edi\n\t"
                "dec %%eax ; jmp 1b\n\t"
                "2: movl %%ebx, %%ecx\n\t" 
                "movl (%%edi), %%edx\n\t"
                "andl $31, %%ecx\n\t"
                "rep ; movsb\n\t" 
                : "=&a"(h), "=&D"(i), "=&S"(j), "=&b"(k), "=&c"(l), "=&d"(m)
                : "0"(count/32), "1"(dest), "2"(src), "3"(count)  : "memory");

        return dest;
}

//--------------------------------------------------------------------------------------
// FUNCTION: memcpy_put
//--------------------------------------------------------------------------------------
// optimized memcpy for copying data from private memory to MPB
//--------------------------------------------------------------------------------------
inline void *memcpy_put(void *dest, const void *src, size_t count)
{
        int i, j, k;

        asm volatile (
                "cld; rep ; movsl\n\t"
                "movl %4, %%ecx\n\t" 
                "andl $3, %%ecx\n\t"
                "rep ; movsb\n\t" 
                : "=&c"(i), "=&D"(j), "=&S"(k) 
                : "0"(count/4), "g"(count), "1"(dest), "2"(src) : "memory");

        return dest;
}

