#include "measure.h"
#include <math.h>
#include <sys/time.h>
#include <unistd.h>

static double CLOCK;

static long long
rdtsc(void)
{
	unsigned int a, b;
	__asm__ __volatile__("rdtsc" : "=d" (a), "=a" (b));
	return ((long long) a << 32) + (long long) b;
}

void
init_timer(void)
{
	long long tsc_start, tsc_end;
	struct timeval tv_start, tv_end;

	tsc_start = rdtsc();
	gettimeofday(&tv_start, NULL);
	usleep(100000); /* delay must be < 1000000 to be portable */
	tsc_end = rdtsc();
	gettimeofday(&tv_end, NULL);

	CLOCK = (double) (tsc_end-tsc_start) /
		(1000000 * (tv_end.tv_sec - tv_start.tv_sec) + (tv_end.tv_usec - tv_start.tv_usec));
}

double
gamma_time_diff(time_586 b, time_586 a)
{
        double db, da, res;

	db =   (double) b.hi
	     * (double) (1<<16)
	     * (double) (1<<16)
	     + (double) b.lo;

	da =   (double) a.hi
	     * (double) (1<<16)
	     * (double) (1<<16)
	     + (double) a.lo;

	if (db < da) {
		res = (    (double) (1<<16)
			 * (double) (1<<16)
			 * (double) (1<<16)
			 * (double) (1<<16)
			 + db-da
			) /(double)CLOCK;
	} else {
		res = (db-da)/(double)CLOCK;
	}
	return res;
}


static void
sort(double *a, int num)
{
	int i, j;
	double app;
	for (i=0; i<num-1; i++)
		for (j=i+1; j<num; j++)
			if (a[i]>a[j]) {
				app = a[i];
				a[i] = a[j];
				a[j] = app;
			}
}


void
mean_variance(int num,
	      double *record, 
	      double *min, 
	      double *max, 
	      double *med, 
	      double *var)
{

	double  s, sq, meanv, meanq;
	int     i;

	*min = 1e9;
	*max = 0;
	s    = 0;
	sq   = 0;


        for (i=0; i<WARM_UP; i++)
                record[i] = 0;

        sort(record, num);

	for (i=WARM_UP; i < num - DISCARD_WORST; i++) {
		*min = ( record[i] < *min ?record[i]:*min);
		*max = ( record[i] > *max ?record[i]:*max);
		s   +=   record[i];
		sq  +=   record[i] * record[i];
	}

	meanv  = s  / ( num - WARM_UP - DISCARD_WORST );
	meanq  = sq / ((float)num-WARM_UP - DISCARD_WORST);

	*var   = meanq - ((meanv) * (meanv));

	*med   = (record[ (int)ceil((float) num/2) - 1 ] + record[(int)ceil((float) num/2)]) /2;
}
