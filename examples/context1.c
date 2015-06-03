/*
The following program provides an example of using the
getcontext(2), setcontext(2) and makecontext(3C) functions
introduced in IRIX 5.x. These functions are useful for
implementing user level context switching between multiple
threads of control within a single IRIX process.

Each thread's context is defined in ucontext(5). A thread's 
context is maintained and created by the operating system 
primarily because it includes the signal mask (which is 
maintained by the operating system).


Compile this example as:
	cc context1.c -o context1

You must supply 3 numerical parameters:
	context1 a b c

	a = Number of threads to create and execute.
	b = Time slice for each thread, how many milliseconds a thread
	    will be allowed to execute before another thread is allowed to run.
	    These units must be in units of 10.
	c = Number of loop iterations to perform, per thread, in the
	    workfunc() function.


Sample execution on a 100 MHZ IP20 processor, IRIX 5.3:

./context1 13 20 1000000

thread number '3' finished first
total number of dispatches '131'

thread   num_loops
------------------
 0        945927
 1        943358
 2        933348
 3        1000000
 4        940210
 5        937799
 6        964359
 7        956506
 8        962444
 9        966233
 10        962684
 11        965896
 12        968186

*/

#include <ucontext.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define QWORD 16		/* bytes per Quadword */
#define DSTKSIZ 1024*QWORD	/* Align stack to QWORD boundary */

int num_thrds;		/* number of threads to create */
long looptims;		/* times to loop in work loop */
int num_disp;		/* current number of dispatches */
int disp_cycle;		/* current dispatch cycle */
clock_t tslice_ms;	/* time slice per thread in milliseconds */
int cte;		/* current thread executing */
int nttd;		/* next thread to dispatch */
int done_early;		/* indicates if program terminated with control-C */

ucontext_t *ca;		/* global context array, used by all threads */
ucontext_t when_done;	/* context used when a thread completes all work */
long *gblary;		/* work loop iteration array, 1 entry per thread */

/* This function is called when the first thread completes its loop
   execution in the workfunc function.
*/
void all_done(void) {
        int i;

	if (done_early) {
		printf("\nprogram terminated before any threads completed\n\n");
		printf("below are current loop iterations for each thread\n");
	} else {
		printf("\nthread number '%d' finished first\n",nttd);
		printf("total number of dispatches '%d'\n\n",num_disp);
	}
	printf("thread   num_loops\n");
	printf("------------------\n");
        for(i=0;i<num_thrds;i++) {
		printf(" %d        %d\n",i,gblary[i]);
	}
        exit(0);
}

/* This function is called if the program is interrupted by the user
   hitting control C.
*/
void not_done(void) {
	done_early = 1;
	all_done();
}

/* This is the work function performed by each thread. It will execute
   for tslice_ms milliseconds before being interrupted by a SIGALRM signal
   attached to an itimer. When the itimer fires, the context of the thread
   that was executing will be passed to dispatcher(). This thread will not
   execute on the CPU until it is dispatched at a later time.
*/
void workfunc(long *i) {
	/* Loop and update per thread data. */
	while(*i<looptims) {
		*i += 1;
	}

	/* Disable any more SIGALRMs from being sent to this process.
	   The first thread to complete its work wins. The way this
	   example is coded, there is a race condition between the disabling
	   of the itimer and the possibility of the itimer firing. For sake
	   of simplicity, just live with it for now. No need to clutter this
	   example with locks or semaphores.
	*/
	alarm(0);
}

/* This is the 'dispatch a thread' function. */
void dispatcher(int sig, siginfo_t *sip, ucontext_t *up) {

        /* Figure out the thread that was executing before the dispatch
           cycle (when SIGALRM arrived). Increment number of dispatches.
        */
	num_disp++;
	cte = disp_cycle % num_thrds;
	disp_cycle++;
	nttd = disp_cycle % num_thrds;

	/* Save the thread's context at time of SIGALRM, and dispatch
	   the next thread.
	*/
	ca[cte] = *up;
	if (setcontext(&ca[nttd]) == -1) {
		perror("setcontext in dispatcher failed");
		exit(-1);
	}
}

/* Setup a signal handler for signal 'sig' at function 'func'.
   Set the signal mask to block out SIGALRM signals when 'func' is executing.
*/
void trap_sig(const int sig, void *func) {
        struct sigaction act;

        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask,SIGALRM);
        act.sa_handler = (void*)func;
        act.sa_flags = SA_SIGINFO;

        if (sigaction(sig,&act,NULL) < 0) {
                perror("sigaction");
                exit(-1);
        }
}

/* Establish a signal handler for SIGALRM. When the SIGALRM is
   delivered to the process, the operating system  will pass the
   context of the executing thread at the time of the SIGALRM.
   Set up an itimer to fire every ms milliseconds. The itimer is used
   by the dispatcher function as the time slice interval.
*/
void set_timer(const clock_t ms, void *func) {
	struct itimerval it;
	struct sigaction act;
	clock_t s,us;

	sigemptyset(&act.sa_mask);
	act.sa_handler = (void *)func;
	act.sa_flags = SA_SIGINFO;

	if (sigaction(SIGALRM,&act,NULL) < 0) {
		perror("sigaction");
                exit(-1);
        }

	/* Convert the milliseconds parameter to seconds and microseconds.
	   Set up itimer to go off every tslice_ms time slice.
	*/
	s = ms*1000 / 1000000;
	us = ms*1000 % 1000000;

        it.it_interval.tv_sec = s;
        it.it_interval.tv_usec = us;
        it.it_value.tv_sec = s;
        it.it_value.tv_usec = us;

	if (setitimer(ITIMER_REAL,&it,NULL) < 0) {
		perror("setitimer(ITIMER_REAL)");
		exit(-1);
	}
}

/* Create a stack for a thread. */
void makestack( int size, stack_t *uc_stack ) {
        /* Align the stack on a QWORD boundary so this code will
           run on all SGI platforms.
        */
        /* Round size to QWORD byte boundary. */
        size += QWORD - 1;
        size /= QWORD;
        size *= QWORD;

	/* Allocate memory to be used as the stack for a thread, plus
	   a little fudge.
	*/
        if ((uc_stack->ss_sp = (char *)memalign(QWORD,size+QWORD)) == NULL) {
                perror("malloc");
                exit(-1);
        }

	/* Initialize stack environment within context structure of thread. */
        uc_stack->ss_sp += size+QWORD;
        uc_stack->ss_size = size;
        uc_stack->ss_flags = 0;
}

/* This is the initialization function. It allocates space for
   num_thrds ucontext_t types; allocates space for global array;
   sets up a context for each thread to execute.
*/
void init1(ucontext_t *when_done, void *workfunc) {
	int i;

        /* Allocate space for num_thrds ucontext_t types. */
        if ((ca = (ucontext_t*)calloc(num_thrds,sizeof(ucontext_t)))==NULL) {
                perror("calloc failed\n");
                exit(-1);
        }

	/* Allocate space for global array, 1 entry per thread (num_thrds). */
        if ((gblary = (long *)calloc(num_thrds,sizeof(long)))==NULL) {
                perror("calloc failed\n");
                exit(-1);
        }

	/* Setup context (a function's execution entry point) in case one of
	   the threads complete its work before SIGINT is sent to the process.
	   Allocate a stack, too. The function all_done will be invoked if any
	   thread completes its work before SIGINT is sent to the process.
	*/
        if (getcontext(when_done)!=0) {
                perror("getcontext 1 failed");
                exit(-1);
        }
        makestack(DSTKSIZ, &when_done->uc_stack);
        when_done->uc_link = NULL;
        makecontext(when_done,all_done,0);

	/* Setup contexts for num_thrds threads. Allocate a stacks, too. The
	   uc_link field is pointed at the context that will be invoked if any
	   thread completes its work before SIGINT is sent to the process.
	*/
        for(i=0;i<num_thrds;i++) {
                if (getcontext(&ca[i])!=0) {
                        perror("getcontext 1 failed");
                        exit(-1);
                }

                makestack(DSTKSIZ, &ca[i].uc_stack);
                ca[i].uc_link = when_done;
                makecontext (&ca[i],workfunc,1,&gblary[i]);
        }
}

/* Main function. */
int main(int argc, char *argv[]) {

        if (argc !=4) {  /* Need 3 arguments to program */
        	printf("usage: %s num_threads tslice_ms loop_times\n",argv[0]);
        	exit(-1);
	}

	num_thrds = atoi(argv[1]);
	tslice_ms = atol(argv[2]);
	looptims = atol(argv[3]);
	num_disp = 0;
	done_early = 0;

	/* create some randomness as to which thread runs first */
	disp_cycle = (int)(time(NULL) % (time_t)num_thrds);

	/* Initialize global arrays and context structures. */
	init1(&when_done,workfunc);

	/* Establish a signal handler for control-C. This can be used to
	   prematurely terminate the program.
	   Set up the time slice via an itimer which will invoke dispatcher()
	   every time slice, tslice_ms milliseconds.
	   Invoke dispatcher to start the first thread of execution.
	*/
	trap_sig(SIGINT,not_done);
	set_timer(tslice_ms,dispatcher);
	dispatcher(0,NULL,&ca[disp_cycle]);

	/* Not reached */
	return (0);
}
