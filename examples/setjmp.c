#include <malloc.h>
#include <setjmp.h>
#include <stdio.h>

jmp_buf child, parent;

// The child thread will execute this function
void threadFunction()
{
    printf( "Child thread yielding to parent\n" );
if ( setjmp( child ) )
         {
                 printf( "Child thread exiting\n" );
                 longjmp( parent, 1 );
         }

         longjmp( parent, 1 );
}

void signalHandler( int arg )
{
         if ( setjmp( child ) )
         {
                 threadFunction();
         }

         return;
}

int main()
{
  jmp_buf gate;
  int setjmp_res;
  int i = 0;
  
  printf("i=%d\n",i);
  
  printf("Setze marker, falls noch etwas fehlt\n");
  setjmp_res = setjmp(gate); //--- Marker 1 ----

  if ( setjmp_res ) {
    printf("Noch was vergessen. i=i+1 (%d)\n",setjmp_res);
    i++;
  }
  
  if ( i == 0 ) {
    printf("i=0!!! Sollte aber 1 sein.\n");
    longjmp( gate, 2 );
  }

  printf( "Jetzt aber: i = %d\n",i);
  return 0;
}

