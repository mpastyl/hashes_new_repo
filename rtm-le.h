#ifndef RMT_LE_H
#define RMT_LE_H

#include <stdlib.h>
#include <math.h> /* pow() */
#include <pthread.h>
#include "rtm.h"
#include "timers_lib.h"

#define EXP_THRESHOLD 2
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define EXP_POW(f,c,m) (MIN((c) * (f), (m)))

typedef struct {
	int tid;
	int txstarts,
	    txcommits,
	    txaborts,
	    txaborts_cap,
	    txaborts_con,
	    txaborts_exp,
	    txaborts_unk;
	int lock_acqs;

} txstats_t;

static void txstats_abort_update(txstats_t *stats, int xbegin_status)
{
	stats->txaborts++;
	if (xbegin_status & _XABORT_CAPACITY)
		stats->txaborts_cap++;
	else if (xbegin_status & _XABORT_CONFLICT)
		stats->txaborts_con++;
	else if (xbegin_status & _XABORT_EXPLICIT)
		stats->txaborts_exp++;
	else
		stats->txaborts_unk++;
}

extern int rtm_elide_max_retries;
static int rtm_elided_lock(pthread_spinlock_t *spinlock, txstats_t *stats)
{
	int aborts = 0;
	int status;

	while (1) {
		if (stats)
			stats->txstarts++;

#ifdef TSX_SPIN_ON_ABORT
		/* Avoid Lemming effect. */
		while (*spinlock == 0)
			/* do nothing */;
#endif

		status = _xbegin();
		if (_XBEGIN_STARTED == (unsigned)status) {
			/* TX region */
			if (*spinlock == 0) /* lock taken by someone else */
				_xabort(0xff);

			return aborts;
		}
	
		/* Aborted. */
		aborts++;
		if (stats)
			txstats_abort_update(stats, status);

		/* If threshold exceeded grab the lock and return. */
		if (aborts >= rtm_elide_max_retries) {
			if (stats)
				stats->lock_acqs++;
			pthread_spin_lock(spinlock);

			return aborts;
		}

		/**
		 * Next step based on abort reason:
		 *   capacity: 1 abort
		 *   explicit: wait for lock to be freed and retry.
		 *   unknown:  1 abort
		 **/
		if (status & _XABORT_CAPACITY && aborts >= 1) {
			if (stats)
				stats->lock_acqs++;
			pthread_spin_lock(spinlock);

			return aborts;
		} else if (status & _XABORT_EXPLICIT || status & _XABORT_CONFLICT) {
		} else { /* Unknown abort */
			if (stats)
				stats->lock_acqs++;
			pthread_spin_lock(spinlock);

			return aborts;
		}
	}

	return -1;
}

static void rtm_elided_unlock(pthread_spinlock_t *spinlock, txstats_t *stats)
{
	if (*spinlock == 1) { /* lock is free */
		_xend();
		if (stats)
			stats->txcommits++;
	} else {
		pthread_spin_unlock(spinlock);
	}

#ifdef TSX_EXP_BACKOFF
	if (stats && stats->failures > 0)
		stats->failures = 0;
#endif
}

#endif /* RMT_LE_H */
