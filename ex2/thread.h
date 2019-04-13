# include <signal.h>
# include <setjmp.h>
# include "uthreads.h"

#ifndef _THREAD_H
#define _THREAD_H

enum State {READY, BLOCKED, RUNNING};

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
		"rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5 

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif
class Thread
{
 private:
    int tid, quantums;
    char stack[STACK_SIZE];
    State cur_state;
    sigjmp_buf env;
 public:
    Thread();
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
