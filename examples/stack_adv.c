#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <errno.h>


jmp_buf back, main_jb;

// 64kB stack
#define STACK 1024*64*1024

stack_t stack;
stack_t stackadv;

void signalHandlerAdv( int arg )
{
  long stackp = 0;
  long basep = 0;

  __asm__ __volatile__ ( "mov %%ebp, %%eax":"=a" (basep));
  __asm__ __volatile__ ( "mov %%esp, %%eax":"=a" (stackp));
  
  printf("SignalhandlerAdv Stackpointer : %lu Basepointer: %lu\n",stackp,basep);

  printf("Init Thread 2 end\n");

  return;
}


void signalHandler( int arg )
{
  long stackp = 0;
  long basep = 0;
  int foo;

  __asm__ __volatile__ ( "mov %%ebp, %%eax":"=a" (basep));
  __asm__ __volatile__ ( "mov %%esp, %%eax":"=a" (stackp));
  
  printf("Signalhandler Stackpointer : %lu Basepointer: %lu\n",stackp,basep);
  
  if ( setjmp(back) ) {
 
    printf("Longjmp (run Thread 1)\n");
 
    struct sigaction sa;

    // Create the new stack
    stackadv.ss_flags = 0;
    stackadv.ss_size = STACK;
    stackadv.ss_sp = malloc(STACK );
    if ( stackadv.ss_sp == 0 ) {
      perror( "Could not allocate stack for Thread 2." );
      exit( 1 );
    }
    foo = sigaltstack( &stackadv, 0 );
     printf("Sigstackres 2: %d %d\n",foo,errno);

    // Set up the custom signal handler
    sa.sa_handler = &signalHandlerAdv;
    sa.sa_flags = SA_ONSTACK;
    sigemptyset( &sa.sa_mask );
    foo = sigaction( SIGUSR1, &sa, 0 );
    printf("Sigactkres2: %d\n",foo);

    printf("Raise Sig (Init Thread 2)\n");

    raise( SIGUSR1 );

    printf("longjmp end (Run Thread 1)\n");

    longjmp(main_jb,1);
    
    return;

  }

  printf("Sig end (Init Thread 1)\n");

  return;
}



int main()
{
  stack_t current_stack;
  struct sigaction current_sa;
	
  int foo;		 

  struct sigaction sa;
  long stackp = 0;
  long basep = 0;

  // Create the new stack
  stack.ss_flags = 0;
  stack.ss_size = STACK;
  stack.ss_sp = malloc(STACK );
  if ( stack.ss_sp == 0 ) {
    perror( "Could not allocate stack." );
    exit( 1 );
  }
  foo = sigaltstack( &stack, &current_stack );
  printf("WSigstackres: %d\n",foo);

  // Set up the custom signal handler
  sa.sa_handler = &signalHandler;
  sa.sa_flags = SA_ONSTACK;
  sigemptyset( &sa.sa_mask );
  foo = sigaction( SIGUSR1, &sa, &current_sa );
  printf("Sigactkres: %d\n",foo);

  __asm__ __volatile__ ( "mov %%ebp, %%eax":"=a" (basep));
  __asm__ __volatile__ ( "mov %%esp, %%eax":"=a" (stackp));
  
  printf("Stackpointer : %lu Basepointer: %lu\n",stackp,basep);

  // Send the signal to call the function on the new stack
  printf( "raise signal (Init thread 1)\n" );
  raise( SIGUSR1 );

  //wiederherstellen des alten stacks und handlers
//  sigaltstack(&current_stack, 0);
//  sigaction(SIGUSR1, &current_sa, 0);

  printf( "Longjmp (Run thread 1)\n" );

  if ( setjmp(main_jb) == 0 ) {
    longjmp(back,1);
  }
  
  return 0;
}

