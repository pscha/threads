#include <malloc.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>


void print_stack_pointer( char *info )
{
  long stackp = 0;
  long basep = 0;

  __asm__ __volatile__ ( "mov %%ebp, %%eax":"=a" (basep));
  __asm__ __volatile__ ( "mov %%esp, %%eax":"=a" (stackp));
  
  printf("%s Stackpointer : %lu Basepointer: %lu\n",info,stackp,basep);

  return;
}


// 64kB stack
#define STACK 1024*64

jmp_buf child, parent;

// The child thread will execute this function
void threadFunction()
{
  print_stack_pointer("---Stack (thread):");
  printf( "Child thread yielding to parent\n" );
  if ( setjmp( child ) ) {
    print_stack_pointer("---Stack (thread2):");
    printf( "Child thread exiting\n" );
    longjmp( parent, 1 );
  }

  longjmp( parent, 1 );
}

void signalHandler( int arg )
{
  print_stack_pointer("---Stack (Sighandler):");

  if ( setjmp( child ) ) {
    threadFunction();
  }

  return;
}

int main()
{
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

  // Send the signal to call the function on the new stack
  print_stack_pointer("---Stack (Main):");
  printf( "Creating child thread\n" );
  raise( SIGUSR1 );

  // Execute the child context
  print_stack_pointer("---Stack (Main 2):");
  printf( "Switching to child thread\n" );
  if ( setjmp( parent ) ) {
    print_stack_pointer("---Stack (Main 3):");
    printf( "Switching to child thread again\n" );
    if ( setjmp( parent ) == 0 ) {
      longjmp( child, 1 );
    }
  } else {
    longjmp( child, 1 );
  }

  // Free the stack
  free( stack.ss_sp );
  printf( "Child thread returned and stack freed\n" );
  return 0;
}

