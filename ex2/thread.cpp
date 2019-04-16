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

static Thread* th_map[MAX_THREAD_NUM];

/* ctors */

Thread::Thread(void (*f)(void)) : quantums(0), cur_state(READY) {
    for (int i=0; i < MAX_THREAD_NUM; i++) {
        if (th_map[i] == nullptr) {
            tid = i;
            break;
        }
    }
    th_map[tid] = this;
    if (tid==0) {quantums ++;}
    
    address_t sp, pc;
    sp = (address_t)stack + STACK_SIZE - sizeof(address_t);
    pc = (address_t)f;
    int ret_val = sigsetjmp(env, 1);
    if (ret_val != 0) {
        cout<<"thread init: sigsetjmp error";
    }
    (env->__jmpbuf)[JB_SP] = translate_address(sp);
    (env->__jmpbuf)[JB_PC] = translate_address(pc);
    ret_val = sigemptyset(&env->__saved_mask); 
    if (ret_val != 0) {
        cout<<"thread init: sigemptyset error";
    }
    // TODO: check ret vals??
    
}

Thread::~Thread() {
    th_map[tid] = nullptr;
}

/* funcs */
Thread* Thread::get_th(int tid) {
    return th_map[tid];
}

void Thread::kill_all() {
    for (int i=0; i < MAX_THREAD_NUM; ++i) {
        if (th_map[i] != nullptr) {
            th_map[i]->~Thread();
        }
    }
}
