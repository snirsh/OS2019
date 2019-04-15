/*
 * test1.cpp
 *
 *	test suspends and resume
 *
 *  Created on: Apr 6, 2015
 *      Author: roigreenberg
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <deque>
#include <list>
#include <assert.h>
#include "uthreads.h"
//#include "libuthread.a"
#include <iostream>

using namespace std;

void wait() {
    int k;
    for(int i=0;i<100000;++i){
        for(int j=0;j<10000;++j){
            k=i*j;
        }
    }
}

void f (void)
{

}

void g (void)
{
    
}


int main(void)
{
    if (uthread_init(100) == -1)
    {
        return 0;
    }

    uthread_spawn(f);
    uthread_spawn(g);
    wait();
    uthread_block(1);
    wait();
    uthread_block(2);
    wait();
    uthread_resume(2);
    wait();
    uthread_resume(1);

    for(;;){}
}
