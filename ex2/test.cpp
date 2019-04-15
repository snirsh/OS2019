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
    int i = 1;
    int j = 0;
    while(1)
    {
        if (i == uthread_get_quantums(uthread_get_tid()))
        {
            cout << "f" << "  q:  " << i << endl;
            if (i == 3 && j == 0)
            {
                j++;
                cout << "          f suspend by f" << endl;
                uthread_block(uthread_get_tid());
            }
            if (i == 6 && j == 1)
            {
                j++;
                cout << "          g resume by f" << endl;
                uthread_resume(2);
            }
            if (i == 8 && j == 2)
            {
                j++;
                cout << "          **f end**" << endl;
                uthread_terminate(uthread_get_tid());
                return;
            }
            i++;
        }
    }
}

void g (void)
{
    int i = 1;
    int j = 0;
    while(1)
    {
        if (i == uthread_get_quantums(uthread_get_tid()))
        {
            cout << "g" << "  q:  " << i << endl;
            if (i == 11 && j == 0)
            {
                j++;
                cout << "          **g end**" << endl;
                uthread_terminate(uthread_get_tid());
                return;
            }
            i++;
        }
    }
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
