
CC = g++-4.9.2
CFLAGS = -O3 -lpthread
TMFLAGS = -O3 -lpthread -fgnu-tm

all: randtrack randtrack_element_lock randtrack_global_lock randtrack_list_lock randtrack_parallel randtrack_reduction randtrack_tm

randtrack: list.h hash.h defs.h randtrack.cc
	$(CC) $(CFLAGS) randtrack.cc -o randtrack

randtrack_element_lock: list.h hash.h defs.h randtrack_element_lock.cc
	$(CC) $(CFLAGS) randtrack_element_lock.cc -o randtrack_element_lock

randtrack_global_lock: list.h hash.h defs.h randtrack_global_lock.cc
	$(CC) $(CFLAGS) randtrack_global_lock.cc -o randtrack_global_lock

randtrack_list_lock: list.h hash.h defs.h randtrack_list_lock.cc
	$(CC) $(CFLAGS) randtrack_list_lock.cc -o randtrack_list_lock

randtrack_parallel: list.h hash.h defs.h randtrack_parallel.cc
	$(CC) $(CFLAGS) randtrack_parallel.cc -o randtrack_parallel

randtrack_reduction: list.h hash.h defs.h randtrack_reduction.cc
	$(CC) $(CFLAGS) randtrack_reduction.cc -o randtrack_reduction

randtrack_tm: list.h hash.h defs.h randtrack_tm.cc
	$(CC) $(TMFLAGS) randtrack_tm.cc -o randtrack_tm

clean:
	rm -f *.o randtrack randtrack_element_lock randtrack_global_lock randtrack_list_lock randtrack_parallel randtrack_reduction randtrack_tm 
