#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include "timers_lib.h"
#include "alloc.h"
#include "parallel_benchmarks.h"
#include "clargs.h"
#include "aff.h"

//#include "global_lock.h"
//#include "striped.h"
#include "refinable.h"

/* The Hash Table */
struct HashSet ht;

static pthread_t *threads;
static params_t *params;
static pthread_barrier_t barrier;
int elements_count=0;
long long int total_value_sum=0;

static void params_print()
{
	int i;

	int total_operations = 0, total_lookups = 0, 
	    total_insertions = 0, total_deletions = 0;
	for (i=0; i < clargs.num_threads; i++) {
		printf("Thread %2d: %8d ( %8d %8d %8d )\n", 
		       params[i].tid, params[i].operations,
		       params[i].lookups, params[i].insertions,
		       params[i].deletions);
		total_operations += params[i].operations;
		total_lookups += params[i].lookups;
		total_insertions += params[i].insertions;
		total_deletions += params[i].deletions;
        if (clargs.verify == 1){
            elements_count += params[i].real_insertions;
            elements_count -= params[i].real_deletions;
            total_value_sum += params[i].value_sum;
        }
	}

        
    if (clargs.verify==1){
        printf("\nTest1 start!\n");
        int expected_count = find_elements_count(&ht);
        if(expected_count == elements_count) printf("Test1 PASSED\n\n");
        else{
            printf("Test1 FAILED\n");
            printf(" elements in HT == %d , elements_counted == %d\n\n",expected_count,elements_count);
        }
        printf("\nTest2 start!\n");
        long long int expected_sum = find_elements_sum(&ht);
        if(expected_sum == total_value_sum) printf("Test2 PASSED\n\n");
        else{
            printf("Test2 FAILED\n");
            printf(" expected sum == %lld , sum found== %lld\n\n",expected_sum,total_value_sum);
        }
    }
	printf("%10s %8d ( %8d %8d %8d )\n", "TotalStat",
	       total_operations, total_lookups, 
	       total_insertions, total_deletions);
	printf("\n");

	printf("Verbose timers: insert_lock_set_tsc\n");
	for (i=0; i < clargs.num_threads; i++) {
		printf("  Thread %2d: %4.2lf\n", params[i].tid,
		       tsc_getsecs(&params[i].insert_lock_set_tsc));
	}
	printf("\n");

#ifdef WORKLOAD_TIME
	printf("\033[31mOps/usec:\033[0m %2.4lf\n",
	       (double) total_operations / (clargs.run_time_sec * 1000000));
	printf("\n");
#endif
}

timer_tt *wall_timer;

void *thread_fn(void *args)
{
	unsigned int i;
	params_t *params = args;
	struct drand48_data drand_buffer;
	long int drand_res;
	int choice, key;

	setaffinity_oncpu(params->tid);

	srand48_r(params->tid * clargs.thread_seed, &drand_buffer);
	tsc_init(&params->insert_lock_set_tsc);

	pthread_barrier_wait(&barrier);
	if (params->tid == 0)
		timer_start(wall_timer);
	pthread_barrier_wait(&barrier);

	for (i=0; i < params->nr_operations; i++) {
#if defined(WORKLOAD_TIME) || defined (WORKLOAD_ONE_THREAD)
		if (params->time_to_leave)
			break;
#endif
		params->operations++;

		lrand48_r(&drand_buffer, &drand_res);
		choice = drand_res % 100;
		lrand48_r(&drand_buffer, &drand_res);
		key = drand_res % clargs.max_key;

		if (choice < clargs.insert_frac) {         /* insert   */
			params->insertions++;
			if( Insert(&ht, key, params)) {
                params->real_insertions++;
                params->value_sum += key;
            }
		} else if (choice < clargs.insert_frac + 
		                    clargs.lookup_frac) {  /* lookup   */
			params->lookups++;
			Lookup(&ht, key, params);
		} else {                                   /* deletion */
			params->deletions++;
			if( Erase(&ht, key, params) ) {
                params->real_deletions++;
                params->value_sum -= key;
            }
		}
	}

	pthread_barrier_wait(&barrier);
	if (params->tid == 0)
		timer_stop(wall_timer);

	return NULL;
}

double pthreads_benchmark()
{
	params_t init_params;
	int nthreads = clargs.num_threads;
	unsigned nr_operations = 1000000; /* FIXME */
	int i;

	printf("Pthreads Benchmark...\n");
	printf("nthreads: %d nr_operations: %d ( %d %d %d )\n", clargs.num_threads, 
	       nr_operations, clargs.lookup_frac, clargs.insert_frac, 
	       100 - clargs.lookup_frac - clargs.insert_frac);
#ifdef WORKLOAD_TIME
	printf("run_time_sec: %d\n", clargs.run_time_sec);
#endif

	XMALLOC(threads, clargs.num_threads);
	XMALLOC(params, clargs.num_threads);

	/* Initialize the Hash Table */
	printf("Initializing at %d\n", clargs.init_hash_size);
#ifdef __STRIPED_H
	initialize(&ht, clargs.init_hash_size,clargs.starting_locks);
	//initialize(&ht, clargs.init_hash_size,16);
#else
	initialize(&ht, clargs.init_hash_size);
#endif
    int res;
	for (i=0; i < clargs.init_insertions; i++) {
		res = Insert(&ht, i, &init_params);
        if(clargs.verify==1) {
            if (res){
                elements_count++;
                total_value_sum+=i;
            }
        }
	}

	wall_timer = timer_init();
	if (pthread_barrier_init(&barrier, NULL, nthreads)) {
		perror("pthread_barrier_init");
		exit(1);
	}

	for (i=0; i < nthreads; i++) {
		memset(&params[i], 0, sizeof(*params));
		params[i].tid = i;
#ifdef WORKLOAD_FIXED
		params[i].nr_operations = nr_operations / nthreads;
#elif WORKLOAD_TIME
		params[i].nr_operations = UINT32_MAX;
#else
		params[i].nr_operations = nr_operations;
#endif
	}

	for (i=0; i < nthreads; i++)
		pthread_create(&threads[i], NULL, thread_fn, &params[i]);

#ifdef WORKLOAD_TIME
	sleep(clargs.run_time_sec);
	for (i=0; i < nthreads; i++)
		params[i].time_to_leave = 1;
#endif
	
	for (i=0; i < nthreads; i++)
		pthread_join(threads[i], NULL);

	params_print();

	if (pthread_barrier_destroy(&barrier)) {
		perror("pthread_barrier_destroy");
		exit(1);
	}

	print_set_length(&ht);

	return timer_report_sec(wall_timer);
}
