#include <signal.h>
#include <setjmp.h>

#define STACKSIZE	64*1024	//approriate stack size

struct tcb_str;	//forward declaration

typedef struct tcb_str {
	//fill this struct with statusinformations
	stack_t	stack;	//stack for local vars
	/* rest */
} tcb;
