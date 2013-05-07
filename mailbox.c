
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "scc.h"
#include "sccmalloc.h"


/* mailbox structures */
typedef struct mailbox_t {
	int 							ID;
	char* 						testString;
}mailbox_t;

mailbox_t *LpelMailboxCreate(void)
{
	mailbox_t *mbox = (mailbox_t *) get_ptr(SCCMallocPtr(sizeof(mailbox_t)));
	
	//get_addr(mbox)->ID				 = 101;
	//mbox->testString = (char *) get_ptr(SCCMallocPtr(9*sizeof(char)));
	//strcpy(mbox->testString,"Hello\0");
		
	return mbox;
}

void main(int argc, char** argv){

	SccInit(NULL);
	
	if (node_location == 0){
		char *temp = (char*)SCCMallocPtr(9*sizeof(char));
		strcpy(temp,"Hello\0");
		
		uintptr_t addr = get_ptr(temp);
		
		cpy_mem_to_mpb(0, (void *)&addr, sizeof(uintptr_t));
	}else{
		sleep(5);
		
	  uintptr_t addr;
	  cpy_mpb_to_mem(0, (void *)&addr, sizeof(uintptr_t));
	 
		char *core1 = (char*) get_addr(addr);
		
		printf("From core 1: %s",core1);
	}
		
	
	/*
	if (node_location == 0){
		mailbox_t *mbox = LpelMailboxCreate();
		
		mailbox_t *mboxTemp = get_addr(mbox);
	
		mboxTemp->ID = 102;
		
		printf("\naddress for master mailbox %p\n",mboxTemp);
	  printf("ID value from master %d\n",mboxTemp->ID);
		char *strTemp = get_addr(mboxTemp->testString);
	  printf("string value from master %s\n\n",strTemp);
		
		
	}else{
		mailbox_t *MASTERMAIL;
		
	 uintptr_t addr;
	 cpy_mpb_to_mem(0, (void *)&addr, sizeof(uintptr_t));
	 printf("addr= %u, &addr= %u, shmem_start_address= %u\n",addr,&addr,shmem_start_address);
	 
	 MASTERMAIL = (mailbox_t*) get_addr(addr);

	 printf("address for master mailbox %p\n",MASTERMAIL);
	 printf("ID value from master %d\n",MASTERMAIL->ID);
	 printf("string value from master %s\n",(char*)get_addr((uintptr_t)MASTERMAIL->testString));
	}
	*/
}



