#include <stdio.h>

void *addr;
int i;

void my_setjmp()
{
	printf("(foo) ret add: %p\n", __builtin_return_address(0));
	addr = __builtin_return_address(0);
}

void my_longjmp()
{
	void **p = (void**)&p;
	register void** s asm("bp");
	printf("(foo2) ret add: %p %p &p: %p S: %p\n", __builtin_return_address(0),p[4], p, s);
	for( i = -10; i < 10; i++) {
	  printf("%d: %p %p\n",i,p[i], s[i]);
	}
	s[1] = addr;
}



int main()
{
	printf("(main) &main:  %p\n", &main);

main1:
	my_setjmp();
main2:

	printf("(main) &main1: %p\n", &&main1);
	printf("(main) &main2: %p\n", &&main2);

	my_longjmp();

	return 0;
}
