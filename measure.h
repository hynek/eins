//#define NPROVE 54
#define WARM_UP 2
#define DISCARD_WORST 2
#define NINTERVALS 5000
#define STEP 1.0e-2

typedef struct {
        unsigned long hi;
        unsigned long lo;
} time_586;

#define gamma_time(x) \
__asm__ __volatile__("rdtsc" : "=d" (x.hi), "=a" (x.lo))

void init_timer(void);
double gamma_time_diff( time_586, time_586);
void mean_variance(int, double *, double *, double *, double *, double *);
