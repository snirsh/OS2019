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
    uthread_init(50000);
    uthread_spawn(&foo);
    uthread_spawn(&foo);


    for(;;){}
    return 0;
}