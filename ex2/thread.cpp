#include "thread.h"
#include <map>
#include <iostream>
#include <setjmp.h>
using namespace std;

typedef unsigned long address_t;
#ifdef __x86_64__
/* code for 64 bit Intel arch */

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
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}
#endif

static map<int, Thread*> th_map;
static bool av_tids[MAX_THREAD_NUM];

/* ctors */
Thread::Thread() : quantums(0), cur_state(READY) {
    fill_n(av_tids, MAX_THREAD_NUM, true);
    av_tids[0] = false;
    th_map[tid] = this;
}
Thread::Thread(void (*f)(void)) : quantums(0), cur_state(READY) {
    for (int i=1; i < MAX_THREAD_NUM; i++) {
        if (av_tids[i] == true) {
            tid = i;
            av_tids[i] = false;
            break;
        }
    }
    th_map[tid] = this;
    /*
    address_t sp, pc;
    sp = (address_t)stack + STACK_SIZE - sizeof(address_t);
    pc = (address_t)f;
    sigsetjmp(env, 1);
    (env->__jmpbuf)[JB_SP] = translate_address(sp);
    (env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask); 
    // TODO: check ret vals??
    */
}

Thread::~Thread() {
    av_tids[tid] = true;
    th_map.erase(tid);
}

/* funcs */
Thread* Thread::get_th(int tid) {
    // return nullptr?

    return th_map[tid];
}

void Thread::kill_all() {
    for (map<int, Thread*>::iterator it=th_map.begin(); it != th_map.end(); ++it) {
        it->second->~Thread();
    }
}
