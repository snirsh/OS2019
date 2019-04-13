#include "uthreads.h"
#include <iostream>
using namespace std;

void foo() {
    cout<<"## FOO ##"<<endl;
}

int main(int argc, char* argv[]) {
    cout<<endl<<"############### TESTING ###############"<<endl;
    uthread_init(1000);
    uthread_spawn(&foo);
    uthread_spawn(&foo);
    uthread_spawn(&foo);
    uthread_spawn(&foo);
    
    return 0;
}