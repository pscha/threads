#include <signal.h>
#include <setjmp.h>

#define STACKSIZE	64*1024	//approriate stack size

struct tcb_str;	//forward declaration

typedef struct tcb_str {
	long tcb_ip ; 	// instruktion pointer
					// register speichern
	long tcb_bp ;   // basepointer
	//fill this struct with statusinformations
	stack_t	stack;	//stack for local vars
	/* rest */
} tcb;
