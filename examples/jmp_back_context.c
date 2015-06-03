#include <stdio.h>
#include <setjmp.h>
#include <ucontext.h>
 
static ucontext_t con;


int main() {   
	  int a = 0;
  	printf("a=%d\n",a);
  	printf("getcontext\n",a);
	  
	  getcontext (&con);
	  
    if (a == 0) {
			a++;
    	printf("a=%d\n",a);
    	printf("setcontext\n",a);
			setcontext(&con);
    } else { 
			a++;
    	printf("a=%d\n",a);
    	printf("else\n",a);
    }
     
  	a++;
  	printf("a=%d\n",a);
   	printf("exit\n",a);
        
 
    return 0;
}
