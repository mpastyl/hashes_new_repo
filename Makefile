CC = gcc
CFLAGS = -Wall -Wextra -g 

CFLAGS += -pthread
WORKLOAD = -DWORKLOAD_TIME
CFLAGS += $(WORKLOAD)
CFLAGS += -DCPU_MHZ_SH=\"./cpu_mhz.sh\"

#CFLAGS += -DVERBOSE_STATISTICS

#LDFLAGS = -L/home/users/jimsiak/local/lib/ -ltcmalloc

PROGS =  main.global_lock main.striped main.refinable main.split_ordered

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

clean:
	rm -f $(PROGS)
