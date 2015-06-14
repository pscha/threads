#include <signal.h>
#include <setjmp.h>

#define STACKSIZE	64*1024	//approriate stack size

struct tcb;	//forward declaration

typedef void (*ult_func)();


typedef struct tcb {
	jmp_buf env; // register und alles wird gespeichert
	int Thread_ID; // id des Threads
	int zombie_flag;
	int waiting_flag;
	ult_func func;
	//fill this struct with statusinformations
	stack_t	stack;	//stack for local vars
	/* rest */
} tcb;

/**
 * diese funktion printet uns mal die Pointer von dem verwendeten Stack
 * reines Debugging
 */
extern int tcb_contextprint();


/**
 * Speichert den aktuellen Kontext in der übergebenen tcb-strukt
 * wird der kontext geladen geht es hinter diesem Befehl weiter (wegen IP) 
 */
extern int tcb_getcontext(tcb *t);

/**
 * setzt den aktuellen kontext aus der übergebenen Strucktur. Befehle nach diesem werden nicht ausgelöst, da 
 * der IP neu gesetzt wird
 */
extern int tcb_setcontext(tcb *t);

/**
 * Wechselt die TCB-kontexte. beim erneuten Laden taucht das Programm aus dieser methode auf (wie Yield quasi)
 * Wenn das erste mal getkontext aufgerufen wird wird der state gespeichert und es wird der nächste gesetzt, 
 * wenn dann wieder zurückgekehrt wird wird das bemerkt und um eine Schleife zu verhindern wird dann kein Setkontext
 * ausgeführt, der Caller sieht das so, als ob er diese FUnktion einfach ausführt. 
 * 
 */
extern int tcb_swapcontext(tcb *ourTcb, tcb *newTcb);


/**
 * erzeugt einen Neuen Kontext mit der übergebenen FUnktion. Wenn der Kontext aufgerufen wird wird die FUnktion ausgeführt.
 */
//extern void tcb_makecontext(tcb *t);


