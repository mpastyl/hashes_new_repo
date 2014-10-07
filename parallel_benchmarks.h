#ifndef PARALLEL_BENCHMARKS_H_
#define PARALLEL_BENCHMARKS_H_

#include <pthread.h>
#include <stdlib.h>
#include "tsc.h"
#ifdef TSX
#	include "rtm-le.h"
#endif

typedef struct {
	
    unsigned tid;
    char pad0[(64-sizeof(unsigned))/sizeof(char)];
	unsigned nr_operations;
    char pad1[(64-sizeof(unsigned))/sizeof(char)];

	unsigned operations, 
	         lookups, 
	         insertions, 
	         deletions,
	         real_insertions,
	         real_deletions;
    char pad2[(64-6*sizeof(unsigned))/sizeof(char)];
    long long int value_sum;        
    char pad3[(64-sizeof(long long int))/sizeof(char)];
#ifdef WORKLOAD_TIME
	unsigned int time_to_leave;
    char pad4[(64-sizeof(unsigned int))/sizeof(char)];
#endif

#ifdef SPLIT_ORDERED
    unsigned long long *prev;
    char pad5[(64-sizeof(unsigned long long *))/sizeof(char)];
    unsigned long long curr;
    char pad6[(64-sizeof(unsigned long long ))/sizeof(char)];
    unsigned long long next;
    char pad7[(64-sizeof(unsigned long long))/sizeof(char)];
#endif

#ifdef TSX
	txstats_t txstats;
    char pad8[(64-sizeof(txstats_t))/sizeof(char)];
#endif

	tsc_t insert_lock_set_tsc;
    char pad9[(64-sizeof(tsc_t))/sizeof(char)];
	tsc_t insert_timer;
    char pad10[(64-sizeof(tsc_t))/sizeof(char)];
	tsc_t lookup_timer;
    char pad11[(64-sizeof(tsc_t))/sizeof(char)];

//	tsc_t operations_tsc;
} params_t;

double pthreads_benchmark();

#endif /* PARALLEL_BENCHMARKS_H_ */
