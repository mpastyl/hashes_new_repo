#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h> /* INT32_MAX */
#include "clargs.h"

/* Default command line arguments */
#define ARGUMENT_DEFAULT_NUM_THREADS 1
#define ARGUMENT_DEFAULT_INIT_HASH_SIZE 512
#define ARGUMENT_DEFAULT_INIT_INSERTIONS ( 16 * ARGUMENT_DEFAULT_INIT_HASH_SIZE )
#define ARGUMENT_DEFAULT_MAX_KEY INT32_MAX
#define ARGUMENT_DEFAULT_LOOKUP_FRAC 80
#define ARGUMENT_DEFAULT_INSERT_FRAC 10
#define ARGUMENT_DEFAULT_INIT_SEED 1024
#define ARGUMENT_DEFAULT_THREAD_SEED 128
#define ARGUMENT_DEFAULT_VERIFY 0
#ifdef WORKLOAD_TIME
#  define ARGUMENT_DEFAULT_RUN_TIME_SEC 5
#endif
#ifdef __STRIPED_H
#  define ARGUEMENT_DEFAULT_STARTING_LOCKS 16
#endif

static char *opt_string = "ht:s:m:i:l:r:e:j:z:v:x:";
static struct option long_options[] = {
	{ "help",            no_argument,       NULL, 'h' },
	{ "num-threads",     required_argument, NULL, 't' },
	{ "init-hash-size",  required_argument, NULL, 's' },
	{ "init-insertion",  required_argument, NULL, 'z' },
	{ "max-key",         required_argument, NULL, 'm' },
	{ "lookup-frac",     required_argument, NULL, 'l' },
	{ "insert-frac",     required_argument, NULL, 'i' },
	/* FIXME better short options for these, or no short */
	{ "init-seed",       required_argument, NULL, 'e' },
	{ "thread-seed",     required_argument, NULL, 'j' },
    { "verify",          required_argument, NULL, 'v' },
#ifdef WORKLOAD_TIME
	{ "run-time-sec",    required_argument, NULL, 'r' },
#endif
#ifdef __STRIPED_H
    { "starting-locks",  required_argument, NULL, 'x' },
#endif
	{ NULL, 0, NULL, 0 }
};

clargs_t clargs = {
	ARGUMENT_DEFAULT_NUM_THREADS,
	ARGUMENT_DEFAULT_INIT_HASH_SIZE,
	ARGUMENT_DEFAULT_INIT_INSERTIONS,
	ARGUMENT_DEFAULT_MAX_KEY,
	ARGUMENT_DEFAULT_LOOKUP_FRAC,
	ARGUMENT_DEFAULT_INSERT_FRAC,
	ARGUMENT_DEFAULT_INIT_SEED,
	ARGUMENT_DEFAULT_THREAD_SEED,
    ARGUMENT_DEFAULT_VERIFY
#ifdef WORKLOAD_TIME
	,ARGUMENT_DEFAULT_RUN_TIME_SEC
#endif
#ifdef __STRIPED_H
    ,ARGUMENT_DEFAULT_STARTING_LOCKS
#endif

};

static void clargs_print_usage(char *progname)
{
	printf("usage: %s [options]\n"
	       "  possible options:\n"
	       "    -h,--help  print this help message\n"
	       "    -t,--num-threads  number of threads [%d]\n"
	       "    -s,--init-hash-size  number of buckets the hash table initially contains [%d]\n"
		   "    -z,--init-insertions number of insertions to be performed for initialization [%d]\n"
		   "    -m,--max-key  max key to lookup,insert,delete [%d]\n"
	       "    -l,--lookup-frac  lookup fraction of operations [%d%%]\n"
	       "    -i,--insert-frac  insert fraction of operations [%d%%]\n"
	       "    -e,--init-seed    the seed that is used for the hash initializion [%d]\n"
	       "    -j,--thread-seed  the seed that is used for the thread operations [%d]\n"
           "    -v,--verify verify results at end of run [%d]\n",
	       progname, ARGUMENT_DEFAULT_NUM_THREADS, ARGUMENT_DEFAULT_INIT_HASH_SIZE,
		   ARGUMENT_DEFAULT_INIT_INSERTIONS,
	       ARGUMENT_DEFAULT_MAX_KEY, ARGUMENT_DEFAULT_LOOKUP_FRAC, 
	       ARGUMENT_DEFAULT_INSERT_FRAC,
		   ARGUMENT_DEFAULT_INIT_SEED, ARGUMENT_DEFAULT_THREAD_SEED,
           ARGUMENT_DEFAULT_VERIFY);

#ifdef WORKLOAD_TIME
	printf("    -r,--run-time-sec execution time [%d sec]\n",
			ARGUMENT_DEFAULT_RUN_TIME_SEC);
#endif
#ifdef __STRIPED_H
    printf("    -x--starting locks for striped implementation[%d]\n",
            ARGUMENT_DEFAULT_STARTING_LOCKS);
#endif
    
}

void clargs_init(int argc, char **argv)
{
	char c;
	int i;

	while (1) {
		i = 0;
		c = getopt_long(argc, argv, opt_string, long_options, &i);
		if (c == -1)
			break;

		switch(c) {
		case 'h':
			clargs_print_usage(argv[0]);
			exit(1);
		case 't':
			clargs.num_threads = atoi(optarg);
			break;
		case 's':
			clargs.init_hash_size = atoi(optarg);
			break;
		case 'z':
			clargs.init_insertions = atoi(optarg);
		case 'm':
			clargs.max_key = atoi(optarg);
			break;
		case 'l':
			clargs.lookup_frac = atoi(optarg);
			break;
		case 'i':
			clargs.insert_frac = atoi(optarg);
			break;
		case 'e':
			clargs.init_seed = atoi(optarg);
			break;
		case 'j':
			clargs.thread_seed = atoi(optarg);
			break;
        case 'v':
            clargs.verify = atoi(optarg);
            break;
#ifdef WORKLOAD_TIME
		case 'r':
			clargs.run_time_sec = atoi(optarg);
			break;
#endif
#ifdef __STRIPED_H
        case 'x':
            clargs.starting_locks = atoi(optarg);
            break;
#endif
		default:
			clargs_print_usage(argv[0]);
			exit(1);
		}
	}

	/* Sanity checks. */
	assert(clargs.lookup_frac + clargs.insert_frac <= 100);
}

void clargs_print()
{
	printf("Inputs:\n"
	       "====================\n"
	       "  num_threads: %d\n"
	       "  init_hash_size: %d\n"
		   "  init_insertions: %d\n"
	       "  max_key: %d\n"
	       "  lookup_frac: %d\n"
	       "  insert_frac: %d\n"
	       "  init_seed: %d\n"
	       "  thread_seed: %d\n",
	       clargs.num_threads, clargs.init_hash_size, 
	       clargs.init_insertions, clargs.max_key,
	       clargs.lookup_frac, clargs.insert_frac,
		   clargs.init_seed, clargs.thread_seed);

#ifdef WORKLOAD_TIME
	printf("  run_time_sec: %d\n", clargs.run_time_sec);
#endif
#ifdef __STRIPED_H
    printf("  starting locks: %d\n",clargs.starting_locks);
#endif
}
