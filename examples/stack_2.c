#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

// 64kB stack
#define STACK 1024*64


void signalHandler( int arg )
{
  long stackp = 0;
  long basep = 0;

  __asm__ __volatile__ ( "mov %%ebp, %%eax":"=a" (basep));
  __asm__ __volatile__ ( "mov %%esp, %%eax":"=a" (stackp));
  
  printf("Signalhandler Stackpointer : %lu Basepointer: %lu\n",stackp,basep);

  return;
}


void newStack() {
  stack_t stack;
  struct sigaction sa;
  
  // Create the new stack
  stack.ss_flags = 0;
  stack.ss_size = STACK;
  stack.ss_sp = malloc(STACK );
  if ( stack.ss_sp == 0 ) {
    perror( "Could not allocate stack." );
    exit( 1 );
  }
  sigaltstack( &stack, 0 );

  // Set up the custom signal handler
  sa.sa_handler = &signalHandler;
  sa.sa_flags = SA_ONSTACK;
  sigemptyset( &sa.sa_mask );
  sigaction( SIGUSR1, &sa, 0 );

  printf( "raise signal\n" );
  raise( SIGUSR1 );
}

int main()
{
  long stackp = 0;
  long basep = 0;

  __asm__ __volatile__ ( "mov %%ebp, %%eax":"=a" (basep));
  __asm__ __volatile__ ( "mov %%esp, %%eax":"=a" (stackp));
  
  printf("Stackpointer : %lu Basepointer: %lu\n",stackp,basep);

  newStack();
  newStack();

  return 0;
}

