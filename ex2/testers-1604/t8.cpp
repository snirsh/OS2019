#include "uthreads.h"
#include <stdio.h>
#include <iostream>

int gotit1 = 0;
int gotit2 = 0;
int gotit3 = 0;
int gotit4 = 1;
int gotit5 = 0;
int gotit6 = 0;
int gotit7 = 0;

void f(void) {
    while (1) {
        if (gotit1) {
            std::cout << "in f id: " <<  uthread_get_tid() << " " << uthread_get_quantums
                    (uthread_get_tid()) << std::endl;
            gotit1 = 0;
            gotit2 = 1;
            gotit3 = 1;
            gotit4 = 1;
            if((5<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=7))
            {
                std::cout<< " f sleep for 100000 usec\n";
                uthread_sleep(100000);
            }
            if((20<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=21))
            {
                std::cout<< " f sleep for 1 sec\n";
                uthread_sleep(1000000);
            }
        }
    }
}

void g(void) {
    while (1) {
        if (gotit2) {
            std::cout << "in g id: "  << uthread_get_tid() << " " << uthread_get_quantums
                    (uthread_get_tid()) << std::endl;
            gotit2 = 0;
            gotit1 = 1;
            gotit3 = 1;
            gotit4 = 1;
            if((6<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=8))
            {
                std::cout<< " g terminate  f\n";
                uthread_terminate(1);
            }
            if((9<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=10))
            {
                std::cout<< " g spawn new f\n";
                uthread_spawn(f);
            }
            if((21<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=22))
            {
                std::cout<< " g sleep for 1 sec\n";
                uthread_sleep(1000000);
            }
        }
    }
}
void z(void) {
    while (1) {
        if (gotit3)
        {
            std::cout << "in z id: " << uthread_get_tid() << " " << uthread_get_quantums
                    (uthread_get_tid
                             ()) << std::endl;
            gotit3 = 0;
            gotit1 = 1;
            gotit2 = 1;
            gotit4 = 1;
            if((4<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=5))
            {
                std::cout<< " z sleep for 0 usec\n";
                uthread_sleep(0);
            }
            if((23<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=24))
            {
                std::cout<< " z sleep for 1 sec\n";
                uthread_sleep(1000000);
            }

        }
    }
}


int main() {
    uthread_init(100000);
    uthread_spawn(f);
    uthread_spawn(g);
    uthread_spawn(z);
    for (;;) {
        if (gotit4) {
            std::cout << "in main id: " << uthread_get_tid() << " "  << uthread_get_quantums
                    (uthread_get_tid()) << std::endl;
            std::cout << "Prog q count " << uthread_get_total_quantums() << std::endl;
            gotit4 = 0;
            gotit1 = 1;
            gotit2 = 1;
            gotit3 = 1;
        }
        if((52<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=53))
        {
            std::cout<< " end of the program\n";
            uthread_terminate(0);
        }

    }
}
