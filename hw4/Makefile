
CC = g++-4.9
CFLAGS = -O3 -lpthread
TMFLAGS = -O3 -lpthread -fgnu-tm

all: randtrack randtrack_basic randtrack_element_lock randtrack_global_lock randtrack_list_lock randtrack_reduction randtrack_tm

randtrack: list.h hash.h defs.h randtrack.cc
	$(CC) $(CFLAGS) randtrack.cc -o randtrack

randtrack_basic: list.h hash.h defs.h randtrack_basic.cc
	$(CC) $(CFLAGS) randtrack_basic.cc -o randtrack_basic

randtrack_element_lock: list.h hash_lock.h defs.h randtrack_element_lock.cc
	$(CC) $(CFLAGS) randtrack_element_lock.cc -o randtrack_element_lock

randtrack_global_lock: list.h hash.h defs.h randtrack_global_lock.cc
	$(CC) $(CFLAGS) randtrack_global_lock.cc -o randtrack_global_lock

randtrack_list_lock: list.h hash_lock.h defs.h randtrack_list_lock.cc
	$(CC) $(CFLAGS) randtrack_list_lock.cc -o randtrack_list_lock

randtrack_reduction: list.h hash.h defs.h randtrack_reduction.cc
	$(CC) $(CFLAGS) randtrack_reduction.cc -o randtrack_reduction

randtrack_tm: list.h hash.h defs.h randtrack_tm.cc
	$(CC) $(TMFLAGS) randtrack_tm.cc -o randtrack_tm

clean:
	rm -f *.o randtrack randtrack_basic randtrack_element_lock randtrack_global_lock randtrack_list_lock randtrack_reduction randtrack_tm 
