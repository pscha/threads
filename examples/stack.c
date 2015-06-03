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

void signalHandler2( int arg )
{
  long stackp = 0;
  long basep = 0;

  __asm__ __volatile__ ( "mov %%ebp, %%eax":"=a" (basep));
  __asm__ __volatile__ ( "mov %%esp, %%eax":"=a" (stackp));
  
  printf("Signalhandler2 Stackpointer : %lu Basepointer: %lu\n",stackp,basep);

  return;
}

int main()
{
  stack_t stack;
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
  sigaltstack( &stack, 0 );

  // Set up the custom signal handler
  sa.sa_handler = &signalHandler;
  sa.sa_flags = SA_ONSTACK;
  sigemptyset( &sa.sa_mask );
  sigaction( SIGUSR1, &sa, 0 );

  signal(SIGUSR2,signalHandler2);

  __asm__ __volatile__ ( "mov %%ebp, %%eax":"=a" (basep));
  __asm__ __volatile__ ( "mov %%esp, %%eax":"=a" (stackp));
  
  printf("Stackpointer : %lu Basepointer: %lu\n",stackp,basep);

  // Send the signal to call the function on the new stack
  printf( "raise signal\n" );
  raise( SIGUSR1 );
  printf( "raise signal2\n" );
  raise( SIGUSR2 );

  return 0;
}

