#include <iostream>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include <list>
#include <map>
#include <string>
#include "uthreads.h"
#include "thread.h"
#include "sleeping_threads_list.h"
using namespace std;

/*
 * User-Level Threads Library (uthreads)
 * Author: OS, os@cs.huji.ac.il
 */

#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define STACK_SIZE 4096 /* stack size per thread (in bytes) */

#define ERR_SYS(msg) cerr << "system error: " << msg << endl;
#define ERR_LIB(msg) cerr << "thread library error: " << msg << endl;

/* GLOBALS */
timeval now;
list<Thread*> ready_list, blocked_list;
SleepingThreadsList sleep_list;
struct sigaction sa_real = {0};
struct sigaction sa_virt = {0};
struct itimerval timer;
struct timeval quantum;
int total_qu;
static sigset_t set;

/* inner funcs */
void sig_block()
{
    if (sigprocmask(SIG_BLOCK, &set, nullptr)) {
        Thread::kill_all();
        ERR_SYS("sig_block: sigprocmask error")
        exit(1);
    }
}

void sig_unblock() {
    if (sigprocmask(SIG_UNBLOCK, &set, nullptr)) {
        Thread::kill_all();
        ERR_SYS("sig_unblock: sigprocmask error")
        exit(1);
    }
}

void set_vtimer() {
    timer.it_value = quantum;
    timer.it_interval = quantum;
    if (setitimer (ITIMER_VIRTUAL, &timer, NULL)) {
        ERR_SYS("set_vtimer: setitimer error");
        exit(1);
    }
}

void switch_threads(int sig)
{
    sig_block();
    // in case of only one thread running
    if (ready_list.size() == 1) {
        ready_list.front()->inc_quantums();
        total_qu++;
        sig_unblock();
        return;
    }

    // otherwise we need to switch
    Thread* cur_th = ready_list.front();
    cur_th->set_state(READY);
    ready_list.pop_front();
    ready_list.push_back(cur_th);
    Thread* new_th = ready_list.front();
    new_th->set_state(RUNNING);
    new_th->inc_quantums();
    total_qu ++;

    int ret_val = sigsetjmp(*cur_th->get_env(), 1);
    if (ret_val != 0) {
        ERR_SYS("switch: sigsetjmp error");
        exit(1);
    }
    siglongjmp(*(new_th->get_env()), 1);
    sig_unblock();
}

void wake(int sig)
{
    sig_block();
    wake_up_info* wk = sleep_list.peek();
    int tid = wk->id;
    timeval old_tv = wk->awaken_tv;
    sleep_list.pop();
    
    wake_up_info* next = sleep_list.peek();
    if (next != nullptr) {
        timeval new_tv = next->awaken_tv;
        timersub(&new_tv, &old_tv, &timer.it_value);
        timer.it_interval.tv_sec = 1000000;
	    timer.it_interval.tv_usec = 0;
        if (setitimer (ITIMER_REAL, &timer, NULL)) {
            ERR_SYS("wake: setitimer error");
            exit(1);
        }
    }
    uthread_resume(tid);
    sig_unblock();
}

timeval calc_wake_up_timeval(int usecs_to_sleep)
{
	timeval now, time_to_sleep, wake_up_timeval;
	gettimeofday(&now, nullptr);
	time_to_sleep.tv_sec = usecs_to_sleep / 1000000;
	time_to_sleep.tv_usec = usecs_to_sleep % 1000000;
	timeradd(&now, &time_to_sleep, &wake_up_timeval);
	return wake_up_timeval;
}


/* External interface */
/*
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {

    if (quantum_usecs <= 0) {
        ERR_LIB("init: invalid quantum value");
        return -1;
    }

    // set signals
    if (sigemptyset(&set) || sigaddset(&set, SIGVTALRM) || sigaddset(&set, SIGALRM))
    {
        Thread::kill_all();
        ERR_SYS("init: sigset error")
        exit(1);
    }

    // install wake handler
    sa_real.sa_handler = &wake;
	if (sigaction(SIGALRM, &sa_real,NULL) < 0) {
		ERR_SYS("init: sigaction error (real)");
        exit(1);
	}

    // install virtual timer handler
    sa_virt.sa_handler = &switch_threads;
	if (sigaction(SIGVTALRM, &sa_virt,NULL) < 0) {
		ERR_SYS("init: sigaction error (virtual)");
        exit(1);
	}

    // configure quantum timeval
    quantum.tv_sec = quantum_usecs / 1000000;
	quantum.tv_usec = quantum_usecs % 1000000;

    // create main thread
    Thread* main_th = new Thread(0);
    if (main_th == nullptr) {
        ERR_LIB("init: can't create main thread")
        return -1;
    }
    ready_list.push_front(main_th);
    main_th->inc_quantums();
    total_qu = 1;

    // set the virtual timer to get things going
	set_vtimer();
    return 0;
}

/*
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void)) {
    sig_block();
    Thread* new_th = new Thread(f);
    if (new_th == nullptr) {
        ERR_LIB("spawn: can't spawn new thread")
        return -1;
    }
    ready_list.push_back(new_th);
    sig_unblock();
    return new_th->get_tid();
}

/*
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    sig_block();
    if (tid > 99 || tid < 0) {
        ERR_LIB("terminate: inavlid tid")
        sig_unblock();
        return -1;
    }
    if (tid == 0) {
        Thread::kill_all();
        ready_list.clear();
        blocked_list.clear();
        exit(0);
    }
    Thread* th = Thread::get_th(tid);
    if (th == nullptr) {
        ERR_LIB("terminate: no such thread")
        sig_unblock();
        return -1;
    }
    State st = th->get_state();
    switch(st) {
        case READY:
            ready_list.remove(th);
        case RUNNING:
            switch_threads(0);
            set_vtimer();
            ready_list.remove(th);
        case BLOCKED:
            blocked_list.remove(th);
    }
    th->~Thread();  
    sig_unblock();  
    return 0;
}

/*
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {
    sig_block();
    if (tid > 99 || tid <= 0) {
        ERR_LIB("block: invalid tid")
        sig_unblock();
        return -1;
    }
    Thread* th = Thread::get_th(tid);
    if (th == nullptr) {
        ERR_LIB("block: no such thread")
        sig_unblock();
        return -1;
    }
    State st = th->get_state();
    switch(st) {
        case BLOCKED:
            sig_unblock();
            return 0;
        case RUNNING:
        case READY:
            set_vtimer();
            switch_threads(0);
    }
    th->set_state(BLOCKED);
    ready_list.remove(th);
    blocked_list.push_front(th);
    sig_unblock();
    return 0;
}

/*
 * Description: This function resumes a blocked thread with ID tid and moves
 * it to the READY state. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid) {
    sig_block();
    if (tid > 99 || tid < 0) {
        ERR_LIB("resume: invalid tid")
        sig_unblock();
        return -1;
    }
    Thread* th = Thread::get_th(tid);
    if (th == nullptr) {
        ERR_LIB("resume: no such thread")
        sig_unblock();
        return -1;
    }
    State st = th->get_state();
    if (st == BLOCKED) {
        blocked_list.remove(th);
        ready_list.push_back(th);
        th->set_state(READY);
    }
    sig_unblock();
    return 0;

}

/*
 * Description: This function blocks the RUNNING thread for usecs micro-seconds in real time (not virtual
 * time on the cpu). It is considered an error if the main thread (tid==0) calls this function. Immediately after
 * the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_sleep(unsigned int usec) {
    sig_block();
    if (usec <= 0) {
        ERR_LIB("sleep: invalid sleep value");
        sig_unblock();
        return -1;
    }
    int tid = uthread_get_tid();
    if (tid == 0) {
        ERR_LIB("sleep: can't sleep main")
        sig_unblock();
        return -1;
    }
    
    wake_up_info* wk = sleep_list.peek();

    // adding the first timer
    if (wk == nullptr) {
        sleep_list.add(tid, calc_wake_up_timeval(usec));
        uthread_block(tid);
        timer.it_value.tv_sec = usec / 1000000;
	    timer.it_value.tv_usec = usec % 1000000;
        timer.it_interval.tv_sec = 1000000;
	    timer.it_interval.tv_usec = 0;
        if (setitimer (ITIMER_REAL, &timer, NULL)) {
            ERR_SYS("sleep: setitimer error");
            exit(1);
	    }
    
    } else {
        sleep_list.add(tid, calc_wake_up_timeval(usec));
        uthread_block(tid);
        wk = sleep_list.peek();
        if (wk->id == tid) {
            timeval wk_tv = calc_wake_up_timeval(usec);
            timer.it_value.tv_sec = wk_tv.tv_sec;
            timer.it_value.tv_usec = wk_tv.tv_usec;
            if (setitimer (ITIMER_REAL, &timer, NULL)) {
                ERR_SYS("sleep: setitimer error");
                exit(1);
            }
        }
    }
    set_vtimer();
    sig_unblock();
    return 0;
}

/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid() {
    return ready_list.front()->get_tid();
}

/*
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums() {
    return total_qu;
}


/*
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
int uthread_get_quantums(int tid) {
    if (tid > 99 || tid < 0) {
        ERR_LIB("get_quantums: invalid tid")
        return -1;
    }
    Thread* th = Thread::get_th(tid);
    if (th == nullptr) {
        ERR_LIB("get_quantums: no such thread")
        return -1;
    }
    return th->get_quantums();
}
