//pssh -h /shared/nil/LNX/hosts/hosts1 -t -1 -P -p 1 /shared/nil/air/air
// air.c 
#include "SCCAPI.h"
#include <stdio.h>

#define CORES 48

typedef volatile struct _AIR {
        int *counter;
        int *init;
} AIR;

AIR atomic_inc_regs[2*CORES];
int node_location;

//--------------------------------------------------------------------------------------
// FUNCTION: atomic_inc
//--------------------------------------------------------------------------------------
// Increments an AIR register and returns its privious content
//--------------------------------------------------------------------------------------
void atomic_inc(AIR reg, int *value)
{
  value = reg->counter;
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
void test(int counterID)
{
	int i = -1;
	
	printf("Value of i at start %d\n",i);
	
	atomic_read(&atomic_inc_regs[counterID], &i);
	printf("Read value of counter without modifying shoud be 0, value %d\n",i);
	
	atomic_inc(&atomic_inc_regs[counterID], &i);
	printf("Read value of counter with increment shoud be 0, value %d\n",i);
	
	atomic_inc(&atomic_inc_regs[counterID], &i);
	printf("Read value of counter with increment shoud be 1, value %d\n",i);
	
	atomic_read(&atomic_inc_regs[counterID], &i);
	printf("Read value of counter without modifying shoud be 2, value %d\n",i);
	
	atomic_dec(&atomic_inc_regs[counterID], 0);
	atomic_read(&atomic_inc_regs[counterID], &i);
	printf("Just wrote 0, counter value shoud decrease to 1, value %d\n",i);
	
}

void test1()
{
	int indx,i = -1;
	
	if(node_location == 0){
		for(indx=0;indx<10;indx++){
			atomic_inc(&atomic_inc_regs[1], &i);
			printf("counter 1, value after inc %d\n",i);
		}
		atomic_inc(&atomic_inc_regs[0], &i);
	}
	else{
		atomic_read(&atomic_inc_regs[0], &i);	
		while(!i)
		{atomic_read(&atomic_inc_regs[0], &i);}
		atomic_read(&atomic_inc_regs[1], &i);	
		printf("counter 1, value should be 10, %d\n",i);
	}	
}

int main(int argc, char *argv []) {
  int x, y, z;
	
  // Initialize API
  InitAPI(0);

  // Find out who I am...
  z = ReadConfigReg(CRB_OWN+MYTILEID);
  x = (z >> 3) & 0x0f; // bits 06:03
  y = (z >> 7) & 0x0f; // bits 10:07
  z = (z     ) & 0x07; // bits 02:00
  node_location = PID(x, y, z);
  
	int *air_baseE = (int *) MallocConfigReg(FPGA_BASE + 0xE000);
	int *air_baseF = (int *) MallocConfigReg(FPGA_BASE + 0xF000);
	int i;
	// Assign and Initialize Two Set of Atomic Increment Registers
  for (i = 0; i < CORES; i++)
  {
		//First Set of AIR
    atomic_inc_regs[i].counter = air_baseE + 2*i;
    atomic_inc_regs[i].init = air_baseE + 2*i + 1;
		if(node_location == 0){
			// only one core needs to call this
			*atomic_inc_regs[i].init = 0;
		}
		
		//Second Set of AIR
		atomic_inc_regs[CORES+i].counter = air_baseF + 2*i;
    atomic_inc_regs[CORES+i].init = air_baseF + 2*i + 1;
		if(node_location == 0){
			// only one core needs to call this
			*atomic_inc_regs[CORES+i].init = 0;
		}
  }
	
		printf("Start Test\n\n");
		test(0);
		//test1();
	
	return 0;
}
