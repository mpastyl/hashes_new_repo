CC = gcc
CFLAGS = -Wall -Wextra -g 

CFLAGS += -pthread
#WORKLOAD = -DWORKLOAD_TIME
WORKLOAD = -DWORKLOAD_FIXED
CFLAGS += $(WORKLOAD)
CFLAGS += -DCPU_MHZ_SH=\"./cpu_mhz.sh\"

#CFLAGS += -DVERBOSE_STATISTICS

#LDFLAGS = -L/home/users/jimsiak/local/lib/ -ltcmalloc

PROGS =  main.global_lock main.striped main.refinable main.split_ordered \
		 main.cuckoo main.non_blocking \
		 main.tsx

all: $(PROGS)

SRC = main.c clargs.c parallel_benchmarks.c aff.c

main.global_lock: $(SRC)
	$(CC) $(CFLAGS) -DGLOBAL_LOCK -o $@ $^

main.striped: $(SRC)
	$(CC) $(CFLAGS) -DSTRIPED -o $@ $^

main.refinable: $(SRC)
	$(CC) $(CFLAGS) -DREFINABLE -o $@ $^

main.split_ordered: $(SRC)
	$(CC) $(CFLAGS) -DSPLIT_ORDERED -o $@ $^

main.cuckoo: $(SRC)
	$(CC) $(CFLAGS) -DCUCKOO -o $@ $^

main.non_blocking: $(SRC)
	$(CC) $(CFLAGS) -DNON_BLOCKING -o $@ $^

main.tsx: $(SRC)
	$(CC) $(CFLAGS) -DTSX -DTSX_SPIN_ON_ABORT -o $@ $^

clean:
	rm -f $(PROGS)
