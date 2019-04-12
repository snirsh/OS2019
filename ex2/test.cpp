#include "uthreads.h"
#include <iostream>
using namespace std;

void foo() {
    cout<<"## FOO ##"<<endl;
}

int main(int argc, char* argv[]) {
    
    uthread_init(100);
    uthread_spawn(&foo);
    uthread_terminate(1);
    uthread_block(2);
    uthread_resume(3);
    uthread_sleep(100);
    uthread_get_tid();
    uthread_get_total_quantums();
    uthread_get_quantums(4);

    return 0;
}