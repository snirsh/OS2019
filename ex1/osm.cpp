#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>


/* calling a system call that does nothing */
#define OSM_NULLSYSCALL asm volatile( "int $0x80 " : : \
        "a" (0xffffffff) /* no such syscall */, "b" (0), "c" (0), "d" (0) /*:\
        "eax", "ebx", "ecx", "edx"*/)


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
    struct timeval t;
    gettimeofday(&t, NULL);
    double start = t.tv_sec*1000000000.0 + (t.tv_usec*1000.0);
    for (unsigned int i=0; i < iterations; ++i) {
        
    }
    gettimeofday(&t, NULL);
    double end = t.tv_sec*1000000000.0 + (t.tv_usec*1000.0);
    return end - start;
}

void stupid(){}

/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_function_time(unsigned int iterations) {
    struct timeval t;
    gettimeofday(&t, NULL);
    double start = t.tv_sec*1000000000.0 + (t.tv_usec*1000.0);
    for (unsigned int i=0; i < iterations/10; ++i) {
        stupid();
        stupid();
        stupid();
        stupid();
        stupid();
        stupid();
        stupid();
        stupid();
        stupid();
        stupid();
    }
    gettimeofday(&t, NULL);
    double end = t.tv_sec*1000000000.0 + (t.tv_usec*1000.0);
    return end - start;
}


/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   */
double osm_syscall_time(unsigned int iterations) {
    struct timeval t;
    gettimeofday(&t, NULL);
    double start = t.tv_sec*1000000000.0 + (t.tv_usec*1000.0);
    for (unsigned int i=0; i < iterations/10; ++i) {
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
    }
    gettimeofday(&t, NULL);
    double end = t.tv_sec*1000000000.0 + (t.tv_usec*1000.0);
    return end - start;
}

int main(int argc, char* argv[]) {
    int iter = 1000;
    if (argc == 1) {
        iter = atoi(argv[0]); 
    }
    double t1 = osm_operation_time(iter);
    double t2 = osm_function_time(iter);
    double t3 = osm_syscall_time(iter);
    printf("%f %f %f\n", t1, t2, t3);
}
