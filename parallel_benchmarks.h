#ifndef PARALLEL_BENCHMARKS_H_
#define PARALLEL_BENCHMARKS_H_

#include <pthread.h>
#include <stdlib.h>
#include "tsc.h"

typedef struct {
	unsigned tid;

	unsigned nr_operations;

	unsigned operations, 
	         lookups, 
	         insertions, 
	         deletions,
	         real_insertions,
	         real_deletions;

#ifdef WORKLOAD_TIME
	unsigned int time_to_leave;
#endif

	tsc_t insert_lock_set_tsc;

//	tsc_t operations_tsc;
} params_t;

double pthreads_benchmark();

#endif /* PARALLEL_BENCHMARKS_H_ */
