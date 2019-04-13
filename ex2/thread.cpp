#include "thread.h"
#include "uthreads.h"
#include <map>
#include <setjmp.h>
using namespace std;

static map<int, Thread*> th_map;
static bool av_tids[MAX_THREAD_NUM] = {true};

/* ctors */
Thread::Thread() : quantums(0), cur_state(READY) {
    av_tids[0] = false;
}
Thread::Thread(void (*f)(void)) : quantums(0), cur_state(READY) {
    for (int i=1; i < MAX_THREAD_NUM; i++) {
        if (av_tids[i]) {
            tid = i;
            av_tids[i] = false;
            break;
        }
    }
    address_t sp, pc;
    sp = (address_t)stack + STACK_SIZE - sizeof(address_t);
    pc = (address_t)f;
    sigsetjmp(env, 1);
    (env->__jmpbuf)[JB_SP] = translate_address(sp);
    (env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&env->__saved_mask); 
    // TODO: check ret vals??
}

Thread::~Thread() {
    av_tids[this->get_tid()] = true;
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
