#ifndef CLARGS_H_
#define CLARGS_H_

#include <unistd.h>
#include <getopt.h>

typedef struct {
	int num_threads,
	    init_hash_size,
	    init_insertions,
	    max_key,
	    lookup_frac,
		insert_frac,
	    init_seed,
	    thread_seed;
#ifdef WORKLOAD_TIME
	int run_time_sec;
#endif
} clargs_t;
extern clargs_t clargs;

void clargs_init(int argc, char **argv);
void clargs_print();

#endif /* CLARGS_H_ */