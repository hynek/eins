#include "measure.h"

#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

void
init_timer(void)
{
	// NOP
}


void get_time(time_586 *time)
{
	if (clock_gettime(CLOCK_MONOTONIC, time) == -1)
	{
		perror("get_time failure");
	}
}

double
time_diff(time_586 b, time_586 a)
{
	/* calculate the difference between timestamps in microseconds (us) */
	/* following lines are adapted from http://www.guyrutenberg.com/2007/09/22/profiling-code-using-clock_gettime/ */
	time_586 tmp;

	/* assumption b is in future of a */
	if (a.tv_nsec > b.tv_nsec) {
		/* fix naive subtraction error */
		tmp.tv_sec = (b.tv_sec - a.tv_sec) - 1;
		tmp.tv_nsec = (b.tv_nsec - a.tv_nsec) + 1E+9;
	} else {
		/* no special case, just do the subtraction */
		tmp.tv_sec = b.tv_sec - a.tv_sec;
		tmp.tv_nsec = b.tv_nsec - a.tv_nsec;
	}

	/* convert nanoseconds to microseconds */
	return (tmp.tv_sec * 1.0E+6) + (tmp.tv_nsec / 1000.0);
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
