#include "uthreads.h"
#include <iostream>
using namespace std;

void foo() {
    cout<<"## FOO ##"<<endl;
}

void wait() {
    int k;
    for(int i=0;i<100000;++i){
        for(int j=0;j<10000;++j){
            k=i*j;
        }
    }
}

int main(int argc, char* argv[]) {
    cout<<endl<<"############### TESTING ###############"<<endl;
    uthread_init(100000);
    uthread_spawn(&foo);
    wait();
    uthread_spawn(&foo);
    wait();
    uthread_spawn(&foo);
    wait();
    uthread_block(1);
    wait();
    uthread_resume(1);
    
    for(;;){}
    return 0;
}