
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
	mailbox_t *mbox = (mailbox_t *) SCCMallocPtr(sizeof(mailbox_t));
	
	mbox->ID				 = 101;
	mbox->testString = (char *) SCCMallocPtr(9*sizeof(char));
	strcpy(mbox->testString,"Hello\0");
	
	/*
	mailbox_t *mboxTemp = get_addr(mbox);
	
	mboxTemp->ID				 = 101;
	mboxTemp->testString = (char *) get_ptr(SCCMallocPtr(9*sizeof(char)));
	strcpy(mboxTemp->testString,"Hello\0");
	*/
	mbox = get_ptr(mbox);
  return mbox;
}

void main(int argc, char** argv){

	SccInit(NULL);
	
	
	if (node_location == 0){
		mailbox_t *mbox = LpelMailboxCreate();
		
		
		//mbox = get_addr(master->mailbox);
		//mbox->ID = 102;l
		
		uintptr_t addr = (uintptr_t) mbox;
		cpy_mem_to_mpb(0, (void *)&addr, sizeof(uintptr_t));
		
		printf("addr= %u, &addr= %u, shmem_start_address= %u\n",addr,&addr,shmem_start_address);
		
		printf("\naddress for master mailbox %p\n",get_addr(mbox));
	  printf("ID value from master %d\n",get_addr(mbox->ID));
	  printf("string value from master %s\n\n",get_addr(mbox->testString));
		
		/*
		printf("\naddress for master mailbox %p\n",mbox);
	  printf("ID value from master %d\n",mbox->ID);
	  printf("string value from master %s\n\n",mbox->testString);
		*/
		
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
}



