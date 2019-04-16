# include <signal.h>
# include <setjmp.h>

#ifndef _THREAD_H
#define _THREAD_H

#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define STACK_SIZE 4096 /* stack size per thread (in bytes) */

enum State {READY, BLOCKED, RUNNING};

class Thread
{
 private:
    int tid, quantums;
    char stack[STACK_SIZE];
    State cur_state;
    sigjmp_buf env;
 public:
    Thread(void (*f)(void));
    ~Thread();
    int get_quantums() {return quantums;}
    int get_tid() {return tid;}
    sigjmp_buf* get_env() {return &env;}
    void inc_quantums() {quantums++;}
    void set_state(State st) {cur_state = st;}
    State get_state() {return cur_state;}
    static Thread* get_th(int tid);
    static void kill_all();
}; 

#endif
