#include <iostream>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#include <list>
#include <map>
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

#define ENTER(func) cout<<"## entering: "<<func<<" ##"<<endl;
#define PARAM(p,v) cout<<"## "<<p<<" = "<<v<<" ##"<<endl;
#define EXIT(func) cout<<"## exiting: "<<func<<" ##"<<endl<<endl;
#define ERR(msg) cerr<<msg<<endl;
#define MSG(msg) cout<<msg<<endl;

/* GLOBALS */
list<Thread*> ready_list, blocked_list;
SleepingThreadsList sleep_list;
struct sigaction sa_real = {0};
struct sigaction sa_virt = {0};
struct itimerval timer;
int quantum, total_qu;

/* inner funcs */
void switch_threads(int sig)
{
    Thread* cur_th = ready_list.front();
    cur_th->set_state(READY);
    ready_list.pop_front();
    ready_list.push_back(cur_th);
    Thread* new_th = ready_list.front();
    new_th->set_state(RUNNING);
    new_th->inc_quantums();

    int ret_val = sigsetjmp(*cur_th->get_env(),1);
    printf("SWITCH: now running=%d\n", new_th->get_tid()); 
    if (ret_val == 1) {
        return;
    }
    // TODO: check ret val?
    //siglongjmp(*new_th->get_env(),1);

    timer.it_value.tv_usec = quantum;
    if (setitimer (ITIMER_VIRTUAL, &timer, NULL)) {
        printf("setitimer error.");
    }
}

void wake(int sig)
{
    ENTER("wake")
    wake_up_info* wk = sleep_list.peek();
    int tid = wk->id;
    timeval old_tv = wk->awaken_tv;
    sleep_list.pop();

    wake_up_info* next = sleep_list.peek();
    if (next != nullptr) {
        timeval new_tv = next->awaken_tv;
        timersub(&new_tv, &old_tv, &timer.it_value);
        if (setitimer (ITIMER_REAL, &timer, NULL)) {
            printf("setitimer error.");
        }
    }
    uthread_resume(tid);
    EXIT("wake")
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
    ENTER("init")
    PARAM("quantum_usecs", quantum_usecs)

    // install wake handler
    sa_real.sa_handler = &wake;
	if (sigaction(SIGALRM, &sa_real,NULL) < 0) {
		printf("sigaction error (real)");
	}

    // install virtual timer handler
    sa_virt.sa_handler = &switch_threads;
	if (sigaction(SIGVTALRM, &sa_virt,NULL) < 0) {
		printf("sigaction error (virtual)");
	}

    quantum = quantum_usecs;
    Thread* main_th = new Thread();
    if (main_th == nullptr) {
        // no thread created error
        return -1;
    }
    ready_list.push_front(main_th);
    main_th->inc_quantums();
    total_qu = 1;

    // set the first virtual timer to get things going
	timer.it_value.tv_usec = quantum;
    if (setitimer (ITIMER_VIRTUAL, &timer, NULL)) {
        printf("setitimer error.");
    }

    EXIT("init")
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
    ENTER("spawn")
    PARAM("func", f)

    Thread* new_th = new Thread(f);
    if (new_th == nullptr) {
        // no thread created error
        return -1;
    }
    ready_list.push_back(new_th);

    MSG("spawned tid: "<<new_th->get_tid())
    EXIT("spawn")
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
    ENTER("terminate")
    PARAM("tid", tid)
    if (tid > 99 || tid < 0) {
        // invalid id
        return -1;
    }
    if (tid == 0) {
        // terminate all other threads and clean up
        Thread::kill_all();
        ready_list.clear();
        blocked_list.clear();
        exit(0);
    }
    Thread* th = Thread::get_th(tid);
    if (th == nullptr) {
        // no such thread
        return -1;
    }
    State st = th->get_state();
    switch(st) {
        case READY:
            ready_list.remove(th);
        case RUNNING:
            switch_threads(0);
            ready_list.remove(th);
        case BLOCKED:
            blocked_list.remove(th);
    }
    th->~Thread();
    
    EXIT("terminate")
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
    ENTER("block")
    PARAM("tid", tid)

    if (tid > 99 || tid <= 0) {
        // invalid id
        ERR("block: invalid tid")
        return -1;
    }
    Thread* th = Thread::get_th(tid);
    if (th == nullptr) {
        // no such thread
        ERR("block: invalid tid 2")
        return -1;
    }
    State st = th->get_state();
    switch(st) {
        case BLOCKED:
            return 0;
        case RUNNING:
            switch_threads(0);
    }
    ready_list.remove(th);
    blocked_list.push_front(th);
    th->set_state(BLOCKED);
    
    EXIT("block")
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
    ENTER("resume")
    PARAM("tid", tid)

    if (tid > 99 || tid < 0) {
        // invalid id
        return -1;
    }
    Thread* th = Thread::get_th(tid);
    if (th == nullptr) {
        // no such thread
        return -1;
    }
    State st = th->get_state();
    if (st == BLOCKED) {
        blocked_list.remove(th);
        ready_list.push_back(th);
        th->set_state(READY);
    }
    EXIT("resume")
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
    ENTER("sleep")
    PARAM("usec", usec)

    int tid = uthread_get_tid();
    if (tid == 0) {
        ERR("can't sleep main")
        return -1;
    }
    
    wake_up_info* wk = sleep_list.peek();

    // adding the first timer
    if (wk == nullptr) {
        timer.it_value = calc_wake_up_timeval(usec);
        if (setitimer (ITIMER_REAL, &timer, NULL)) {
            ERR("sleep: setitimer error.");
            return -1;
	    }
        sleep_list.add(tid, timer.it_value);
        uthread_block(tid);
        MSG("sleep: first timer")
    
    } else {
        sleep_list.add(tid, calc_wake_up_timeval(usec));
        uthread_block(tid);
        wk = sleep_list.peek();
        if (wk->id == tid) {
            timer.it_value = calc_wake_up_timeval(usec);
            if (setitimer (ITIMER_REAL, &timer, NULL)) {
                ERR("sleep: setitimer error.");
                return -1;
            }
            MSG("sleep: not first timer")
        }
    }
    EXIT("sleep")
    return 0;
}

/*
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid() {
    ENTER("get_tid")
    
    EXIT("get_tid")
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
    ENTER("get_total_quantums")

    return total_qu;

    EXIT("get_total_quantums")
    return 0;
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
    ENTER("get_quantums")
    PARAM("tid", tid)

    if (tid > 99 || tid < 0) {
        // invalid id
        return -1;
    }
    Thread* th = Thread::get_th(tid);
    if (th == nullptr) {
        // no such thread
        return -1;
    }
    EXIT("get_quantums")
    return th->get_quantums();

}