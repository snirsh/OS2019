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
            if((5<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=8))
            {
                std::cout<< " f sleep for 1 sec\n";
                uthread_sleep(1000000);
            }
            if((30<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=35))
            {
                std::cout<< " f sleep for 1 sec again\n";
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
            if((5<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=8))
            {
                std::cout << " g try to wake f:\n";
                if(uthread_resume(1))
                {
                    std::cout << " cant wake f!\n";
                }
            }
            if((10<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=12))
            {
                std::cout << " g go to sleep for 3 sec:\n";
                uthread_sleep(3000000);
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
            if((10<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=12))
            {
                std::cout << " z try to resume g but g is sleeping:\n";
                uthread_resume(2);
            }
            if((16<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=19))
            {
                std::cout << " z block g:\n";
                uthread_block(2);
            }
            if((45<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=50))
            {
                std::cout<< " z try to resume f\n";
                if(uthread_resume(1))
                {
                    std::cout << " f doesnt exist\n";
                }
                std::cout << " z create new f's\n";
                uthread_spawn(f);
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
            if((16<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=17))
            {
                std::cout << " main resume g but g is sleeping:\n";
                uthread_resume(2);
            }
            if((34<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=35))
            {
                std::cout<< " main terminated f\n";
                uthread_terminate(1);
            }
            if((50<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=53))
            {
                std::cout << " main resume g:\n";
                uthread_resume(2);
            }
            if((60<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=62))
            {
                std::cout<< " main terminated f (1) \n";
                uthread_terminate(1);
            }
            if((65<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=68))
            {
                std::cout<< " main terminated g (2) \n";
                uthread_terminate(2);
            }
            if((71<= uthread_get_total_quantums()) && (uthread_get_total_quantums()<=75))
            {
                std::cout<< " main terminated the main (end of prog) \n";
                uthread_terminate(0);
            }
        }
    }
}
