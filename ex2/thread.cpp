#include "thread.h"
#include "uthreads.h"
#include <map>
using namespace std;

static map<int, Thread*> th_map;
static bool av_tids[MAX_THREAD_NUM] = {true};

/* ctors */
Thread::Thread() {
    av_tids[0] = false;
}
Thread::Thread(void (*f)(void)) {
    for (int i=1; i < MAX_THREAD_NUM; i++) {
        if (av_tids[i]) {
            tid = i;
            av_tids[i] = false;
            break;
        }
    }
    // what the func???
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
