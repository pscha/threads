#include <stdio.h>
#include <setjmp.h>
 
static jmp_buf buf;
  
int main() {   
	  int a = 0;
  	printf("a=%d\n",a);
  	printf("Setjmp\n",a);
	  
    if (!setjmp(buf)) {
			a++;
    	printf("a=%d\n",a);
    	printf("longjmp\n",a);
			longjmp(buf,1);
    } else {
			a++;
    	printf("a=%d\n",a);
    	printf("setjmp - else\n",a);
    }
    
  	a++;
  	printf("a=%d\n",a);
   	printf("exit\n",a);
        
 
    return 0;
}
