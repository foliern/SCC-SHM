
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
	
	//get_addr(mbox)->ID				 = 101;
	//mbox->testString = (char *) get_ptr(SCCMallocPtr(9*sizeof(char)));
	//strcpy(mbox->testString,"Hello\0");
		
	return mbox;
}

void main(int argc, char** argv){

	SccInit(NULL);
	
/*
	if (node_location == 0){
		char *temp = (char*)SCCMallocPtr(9*sizeof(char));
		strcpy(temp,"Hello\0");
		
		uintptr_t addr = get_ptr(temp);
		printf("\naddress for master addr (offset):  %p\n",addr);		
		cpy_mem_to_mpb(0, (void *)&addr, sizeof(uintptr_t));
	}else{
		sleep(5);
		
	  	uintptr_t addr;
	  	cpy_mpb_to_mem(0, (void *)&addr, sizeof(uintptr_t));
	 	printf("\naddress for master addr (offset):  %p\n",addr);

		char *core1 = (char*) get_addr(addr);
		
		printf("From core 1: %s\n",core1);
	}
		
	
	
*/	if (node_location == 0){

	printf("\n\n FIRST ROUND CREATE MAILBOX ON CORE0!!!\n\n");

		mailbox_t *mbox = LpelMailboxCreate();
		
		uintptr_t addr  = get_ptr(mbox);
	
		mbox->ID = 102;
		mbox->testString = (char*)SCCMallocPtr(10*sizeof(char));
		strcpy(mbox->testString,"Griasti");	

		printf("\naddress for master addr (offset):  	%p\n",addr); 
		printf("shmem_start_address:	  	     	%p\n",shmem_start_address); 	
		printf("address for master mailbox:        	%p\n",mbox);
	  	printf("ID value from master: 			%d\n",mbox->ID);
		printf("string value from master: 		%s\n",mbox->testString);
		//char *strTemp = get_addr(mboxTemp->testString);
	  	//printf("string value from master %s\n\n",strTemp);
		cpy_mem_to_mpb(0, (void *)&addr, sizeof(uintptr_t));
//		sleep(10);
		
		int i=0;
		while(!strcmp(mbox->testString,"Griasti")){
			i++;
		}		
		printf("%d: string value from master: 		%s\n",i,mbox->testString);

		sleep(10);	
	printf("\n\n SECOND ROUND READ AND SET VALUE IN CORE1'S MAILBOX!!!\n\n");

                mailbox_t *MASTERMAIL;

                uintptr_t addr2;
                cpy_mpb_to_mem(1, (void *)&addr2, sizeof(uintptr_t));

                printf("\naddress for master addr (offset): 	%p\n",addr2);
                printf("shmem_start_address: 	              	%p\n",shmem_start_address);

                MASTERMAIL= (mailbox_t *)get_addr(addr2);

                printf("address for master mailbox:        	%p\n",MASTERMAIL);
                printf("ID value from master: 			%d\n",MASTERMAIL->ID);
                printf("string value from master: 		%s\n",MASTERMAIL->testString);
                strcpy(MASTERMAIL->testString,"CIAO");
                printf("string value from master: 		%s\n",MASTERMAIL->testString);	

	
	}else{

	printf("\n\n FIRST ROUND READ AND SET VALUE IN CORE0'S MAILBOX!!!\n\n");

		mailbox_t *MASTERMAIL;
		
		uintptr_t addr;
         	cpy_mpb_to_mem(0, (void *)&addr, sizeof(uintptr_t));
	
		printf("\naddress for master addr (offset):  	%p\n",addr);
                printf("shmem_start_address:  		        %p\n",shmem_start_address);
	
		MASTERMAIL= (mailbox_t *)get_addr(addr);

		printf("address for master mailbox:     	%p\n",MASTERMAIL);
                printf("ID value from master:	 		%d\n",MASTERMAIL->ID);
		printf("string value from master: 		%s\n",MASTERMAIL->testString);
		strcpy(MASTERMAIL->testString,"Hoila");
		printf("string value from master:		%s\n",MASTERMAIL->testString);

		sleep(5);
	
	printf("\n\n SECOND ROUND CREATE MAILBOX ON CORE1!!!\n\n");
		
		mailbox_t *mbox = LpelMailboxCreate();

                uintptr_t addr2  = get_ptr(mbox);

                mbox->ID = 102;
                mbox->testString = (char*)SCCMallocPtr(10*sizeof(char));
                strcpy(mbox->testString,"TSCHUSS");

                printf("\naddress for master addr (offset): 	%p\n",addr2);
                printf("shmem_start_address:               	%p\n",shmem_start_address);

                printf("address for master mailbox:        	%p\n",mbox);
                printf("ID value from master: 			%d\n",mbox->ID);
                printf("string value from master: 		%s\n\n",mbox->testString);
                //char *strTemp = get_addr(mboxTemp->testString);
                //printf("string value from master %s\n\n",strTemp);
                cpy_mem_to_mpb(1, (void *)&addr2, sizeof(uintptr_t));
//              sleep(10);

                int i=0;
                while(!strcmp(mbox->testString,"TSCHUSS")){
                        i++;
                }
                printf("%d: string value from master: 		%s\n",i,mbox->testString);


	}
	
}



