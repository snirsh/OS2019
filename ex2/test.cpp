#include "uthreads.h"
#include <iostream>
using namespace std;

void foo() {
    cout<<"## FOO ##"<<endl;
}

int main(int argc, char* argv[]) {
    cout<<"#### testing ####"<<endl;
    uthread_init(1000);
    
    return 0;
}