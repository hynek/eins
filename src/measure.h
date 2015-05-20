//#define NPROVE 54
#define WARM_UP 2
#define DISCARD_WORST 2
#define MIN_VALS (WARM_UP + DISCARD_WORST)
#define NINTERVALS 5000
#define STEP 1.0e-2

#include <time.h>

/*
typedef struct {
        unsigned long hi;
        unsigned long lo;
} time_586;
*/

typedef struct timespec time_586;

/*
#define get_time(x) \
__asm__ __volatile__("rdtsc" : "=d" (x.hi), "=a" (x.lo))
*/


/** init the timer */
void init_timer(void);


/** get the current time from the monotonic clock and store the result at the given address  */
void get_time(time_586 *time);


/** calculate the difference between two time events in microseconds */
double time_diff( time_586, time_586);


/** determine the min and max value as well as the arithmetic mean and the variance 
  * from the given number of doubles */
void mean_variance(int, double *, double *, double *, double *, double *);
