CC = gcc
CFLAGS = -Wall -Wextra -g 

CFLAGS += -pthread
WORKLOAD = -DWORKLOAD_TIME
CFLAGS += $(WORKLOAD)
CFLAGS += -DCPU_MHZ_SH=\"./cpu_mhz.sh\"

#CFLAGS += -DVERBOSE_STATISTICS

#LDFLAGS = -L/home/users/jimsiak/local/lib/ -ltcmalloc

PROGS =  main

all: $(PROGS)

SRC = main.c clargs.c parallel_benchmarks.c aff.c

main: $(SRC) 
	$(CC) $(CFLAGS) -o $@ $^ 

clean:
	rm -f $(PROGS)
