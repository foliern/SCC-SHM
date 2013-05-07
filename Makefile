SHELL=sh

default:
		@echo "Usage: make scc "
		@echo "Usage: make lib "
		@echo "Usage: make mailbox "
		@echo "       make clean"


scc: scc.c SCCAPI.o sccmalloc.o 
	gcc -g -o scc scc.c SCCAPI.o sccmalloc.o

mailbox: mailbox.c SCCAPI.o scc.o sccmalloc.o
	gcc -g -o mailbox mailbox.c SCCAPI.o scc.o sccmalloc.o
	
lib: SCCAPI.o scc.o
	gcc -g -shared -o libsccapi.so SCCAPI.o scc.o -lm
	cp libsccapi.so /shared/nil/snetInstallCQ/lib/
						
SCCAPI.o: SCCAPI.c SCCAPI.h
	gcc -g -c SCCAPI.c -o SCCAPI.o

scc.o: scc.c scc.h
	gcc -g -c scc.c -o scc.o -std=gnu99
	
sccmalloc.o: sccmalloc.c sccmalloc.h
	gcc -g -c sccmalloc.c -o sccmalloc.o


clean:
	@ rm -f *.o *.so lock mailbox scc /shared/nil/snetInstallCQ/lib/libsccapi.so 
