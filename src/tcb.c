
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "tcb.h"


/**
 * diese funktion printet uns mal die Pointer von dem verwendeten Stack
 * reines Debugging
 */
int tcb_contextprint(tcb *t){
	

	return 0;
}


/**
 * Speichert den aktuellen Kontext in der übergebenen tcb-strukt
 * wird der kontext geladen geht es hinter diesem Befehl weiter (wegen IP) 
 */
int tcb_getcontext(tcb *t){
	if (setjmp(t->env)){ // speichert die Umgebungsvar in den jmp_buf des zugewiesenen TCB 
		printf("Thread %d wird weiter bearbeitet",t->Thread_ID);
		return 1; // wenn wir hier wieder kommen signalisieren wir das mit einer 1
	}
 	// hier gehts dann weiter, wenn der TCB wieder geladen wird.
 	return 0; // Wenn wir etwas gespeichert haben zeigen wir das mit einer 0
}


/**
 * setzt den aktuellen kontext aus der übergebenen Strucktur. Befehle nach diesem werden nicht ausgelöst, da 
 * der IP neu gesetzt wird
 */
int tcb_setcontext(tcb *t){
	longjmp(t->env,1); // ?? ob das klappt?   
	// nach dem Longjmp werden ja alle Pointer und Register geladen und es geht innerhalb des Threads weiter
	// das return wird nicht erreicht, da nach dem Longjmp das
	
	return 0;
}


/**
 * Wechselt die TCB-kontexte. beim erneuten Laden taucht das Programm aus dieser methode auf (wie Yield quasi)
 * Wenn das erste mal getkontext aufgerufen wird wird der state gespeichert und es wird der nächste gesetzt, 
 * wenn dann wieder zurückgekehrt wird wird das bemerkt und um eine Schleife zu verhindern wird dann kein Setkontext
 * ausgeführt, der Caller sieht das so, als ob er diese FUnktion einfach ausführt. 
 * 
 */
int tcb_swapcontext(tcb *ourTcb, tcb *newTcb){
	if  (!tcb_getcontext(ourTcb)){ //
		tcb_setcontext(newTcb);
	}
	tcb_setcontext(newTcb);
	
	return 0;
}

/**
 * erzeugt einen Neuen Kontext mit der übergebenen FUnktion. Wenn der Kontext aufgerufen wird wird die FUnktion ausgeführt.
 */
int tcb_makecontext(tcb *t, void *func(), int argc, ...){
	// malloc der stacksize zum erzeugen des Stacks
	
	
	return 0;
}
