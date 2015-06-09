
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "tcb.h"




/**
 * Speichert den aktuellen Kontext in der übergebenen tcb-strukt
 * wird der kontext geladen geht es hinter diesem Befehl weiter (wegen IP) 
 */
int tcb_getcontext(tcb *t){
	//  int long ip;
	//  __asm__ __volatile__ ( "mov %%eip, %%eax":"=a" (ip)); // speichere ip in ip
	return 0;
}


/**
 * setzt den aktuellen kontext aus der übergebenen Strucktur. Befehle nach diesem werden nicht ausgelöst, da 
 * der IP neu gesetzt wird
 */
int tcb_setcontext(const tcb *t){

	return 0;
}


/**
 * Wechselt die TCB-kontexte. beim erneuten Laden taucht das Programm aus dieser methode auf (wie Yield quasi)
 */
int tcb_swapcontext(tcb *ourTcb, tcb *newTcb){
	return 0;
}

/**
 * erzeugt einen Neuen Kontext mit der übergebenen FUnktion. Wenn der Kontext aufgerufen wird wird die FUnktion ausgeführt.
 */
int tcb_makecontext(tcb *t, void *func(), int argc, ...){
	return 0;
}
