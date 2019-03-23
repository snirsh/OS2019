#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "osm.h"

#define DEFAULT_ITER 1000
#define UNROLL 10
#define GET_NANOSECS(timeval) (double)(timeval.tv_sec*1000000000.0 + timeval.tv_usec*1000.0)
#define GET_TIME(timeval) if (gettimeofday(&timeval, NULL) < 0) { return -1; }
#define GET_NEW_ITER(iter) (iter % UNROLL == 0 ? iter / UNROLL : (iter / UNROLL) + UNROLL)
#define DO_TEN(x) x;x;x;x;x;x;x;x;x;x;

/* Initialization function that the user must call
 * before running any other library function.
 * The function may, for example, allocate memory or
 * create/open files.
 * Pay attention: this function may be empty for some desings. It's fine.
 * Returns 0 uppon success and -1 on failure
 */
int osm_init() {
    return 0;
}

/* finalizer function that the user must call
 * after running any other library function.
 * The function may, for example, free memory or
 * close/delete files.
 * Returns 0 uppon success and -1 on failure
 */
int osm_finalizer() {
    return 0;
}

/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_operation_time(unsigned int iterations) {
    if (iterations < 1) {
        return -1;
    }
    struct timeval t_s, t_e;
    int x[10] = {0};
    GET_TIME(t_s)
    for (unsigned int i=0; i < GET_NEW_ITER(iterations); ++i) {
        x[0] += 2;
        x[1] += 2;
        x[2] += 2;
        x[3] += 2;
        x[4] += 2;
        x[5] += 2;
        x[6] += 2;
        x[7] += 2;
        x[8] += 2;
        x[9] += 2;
    }
    GET_TIME(t_e)
    double start = GET_NANOSECS(t_s);
    double end = GET_NANOSECS(t_e);
    return (end - start) / iterations;
}

void null_func(){}

/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_function_time(unsigned int iterations) {
    if (iterations < 1) {
        return -1;
    }
    struct timeval t_s, t_e;
    GET_TIME(t_s)
    for (unsigned int i=0; i < GET_NEW_ITER(iterations); ++i) {
        DO_TEN(null_func())
    }
    GET_TIME(t_e)
    double start = GET_NANOSECS(t_s);
    double end = GET_NANOSECS(t_e);
    return (end - start) / iterations;
}

/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_syscall_time(unsigned int iterations) {
    if (iterations < 1) {
        return -1;
    }
    struct timeval t_s, t_e;
    GET_TIME(t_s)
    for (unsigned int i=0; i < GET_NEW_ITER(iterations); ++i) {
        DO_TEN(OSM_NULLSYSCALL)
    }
    GET_TIME(t_e)
    double start = GET_NANOSECS(t_s);
    double end = GET_NANOSECS(t_e);
    return (end - start) / iterations;
}

int main(int argc, char* argv[]) {
    unsigned int iter = DEFAULT_ITER;
    if (argc > 1) {
        iter = atoi(argv[1]);
        if (iter < 1) {
            iter = DEFAULT_ITER;
        }
    }
    double t1 = osm_operation_time(iter);
    double t2 = osm_function_time(iter);
    //double t3 = osm_syscall_time(iter);
    double t3 = 0;
    printf("op: %f\nfunc: %f\nsyscall: %f\n", t1, t2, t3);
}
