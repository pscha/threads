#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include "ult.h"
#include "tcb.h"

#define MAX_TCB 10
#define DEBUG 0

//TODO: Scheduler bitte richtig schreiben :)
//TODO: Coding style: Variablennamen bitte klein schreiben

/* 
 * for use in vim:
 *
 * :%s/sheduler/scheduler/Ig
 * :%s/Sheduler/Scheduler/Ig
 *
 * :%s/next_TCB/next_tcb/Ig
 */

struct timeval tv_str; // for read datatypes
static fd_set fds;
tcb* tcb_array[MAX_TCB];

tcb_list* tlist; 






tcb* next_TCB;  // globale Variable für den nächsten Thread Sheduler 
tcb* current_tcb; // der tcb der momentan ausgeführt wird.

jmp_buf sheduler;
	

/*
 This function only exists to tell the process to use an empty stack for the thread
 */
void signalHandlerSpawn( int arg )
{	// so habe ich mir das Vorgestellt 
	if (setjmp(tlist->tcb->env)){
		tlist->tcb->func(); // hier funktion ausführen in dem tcb in dem wir gerade sind :: wie auch immer das mit den Pointern geht
	}

}

void tcb_makecontext(tcb *t){
	// malloc der stacksize zum erzeugen des Stacks
  struct sigaction sa;
  
  // Create the new stack
  t->stack.ss_flags = 0;
  t->stack.ss_size = STACKSIZE;
  t->stack.ss_sp = malloc(STACKSIZE);
  if ( t->stack.ss_sp == 0 ) {
    perror( "Could not allocate stack." );
    exit( 1 );
  }
  sigaltstack( &t->stack, 0 );

  // Set up the custom signal handler
  sa.sa_handler = &signalHandlerSpawn;
  sa.sa_flags = SA_ONSTACK;
  sigemptyset( &sa.sa_mask );
  sigaction( SIGUSR1, &sa, 0 );

  printf( "raise signal\n" );
  raise( SIGUSR1 );
}

/*
 * ---------------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------
 * ---------------------------------------------------------------------------------------------------------------
 */


// spawn a new thread, return its ID 
/*	Die Funktion ult_spawn() erzeugt einen Thread-Control-Block (TCB) fuer
 die Funktion f() und stellt den Thread in die Run-Queue. Sie liefert
 den ID des erzeugten Threads zurueck. An dieser Stelle erfolgt kein
 Scheduling, sondern die Abarbeitung wird mit dem laufenden Thread
 fortgesetzt, um die Erzeugung mehrerer Threads zu ermoeglichen.
 */
int ult_spawn(ult_func f) {	
	/* intialize the tcb_list entry*/
	printf("1. malloc\n");	
	tcb_list* tcb= malloc(sizeof(tcb_list));
	printf("2. malloc\n");	
	tcb->tcb = malloc(sizeof(tcb));
	printf("denfine next\n");	
	tcb->next = tlist;
	
	printf("denfine tlist\n");	
	tlist = tcb;
		
	printf("denfine execute function\n");	
	/* make the thread do something */
	tcb->tcb->func = f; // schreibe func-pointer in den TCB ab hier sind Kontext und ausfphrung mitenander verbunden

	tcb_makecontext(tcb->tcb); // hier wird dann der stack gesetzt, sodass ab hier der TCB block fertig sein müsste
	
	// ab diesem Zeitpunkt befinden wir uns noch in dem Kontext in welcher die fkt ult_spawn aufgerufen wurde und haben einen voll funktions-
	// tüchtigen TCB mit einer neuen Funktion erstellt.
	
	// noch thread ID setzen einfügen
	
	printf("end of spawn\n");
	

	return 0;		
}


// yield the CPU to another thread
/*	Die Funktion ult_yield() gibt innerhalb eines Threads freiwillig den
 Prozessor auf. Es erfolgt der Aufruf des Schedulers und die Umschaltung
 auf den vom Scheduler ausgewaehlten Thread.
 */
void ult_yield() {
	printf("in yield\n");
	if(!tcb_getcontext(current_tcb)){ // beim setzen der sprungmarke gehen wir in die schleife, sonst nicht.
		printf("in if\n");
		longjmp(sheduler,current_tcb->Thread_ID); // gib die ID nach oben 
	} 
	// hier gehts dann weiter, wenn der Thread wieder aufgerufen wird.
}


// current thread exits
/*	Wird innerhalb eines Threads ult_exit() aufgerufen, so wird der Thread
 zum Zombie und der Exit-Status wird abgespeichert.
 */
void ult_exit(int status) {
}


// thread waits for termination of another thread
// returns 0 for success, -1 for failure
// exit-status of terminated thread is obtained
/*	Mit ult_waitpid() kann abgefragt werden, ob der Thread mit dem angegebenen
 ID bereits beendet wurde. Ist der Thread bereits beendet, so kehrt die
 Funktion sofort zurueck und liefert in status den Exit-Status des Threads
 (welcher als Argument an ult_exit() uebergeben wurde). Laeuft der Thread noch,
 so soll der aktuelle Thread blockieren und es muss auf einen anderen Thread
 umgeschaltet werden. Bei nachfolgenden Aufrufen des Schedulers soll ueberprueft
 werden, ob der Thread mittlerweile beendet wurde; ist dies der Fall, so soll
 der aktuelle Thread geweckt werden (am besten so, dass er auch als naechster
 Thread die CPU erhaelt).
 */
int ult_waitpid(int tid, int *status) {
	return -1;	//return 'error'
}



// read from file, block the thread if no data available
/*	Hinter ult_read() verbirgt sich die Funktion read() der Unix-API, welche Daten
 aus einer Datei (File-Deskriptor fd) in einen Puffer (buf) liest. Die Funktion
 ult_read() ist eine Wrapper-Funktion fuer read(), welche zusaetzlich ueberprueft, ob
 ausreichend Daten verfuegbar sind. Ist dies nicht der Fall, so wird der Thread
 blockiert und ein anderer Thread erhaelt die CPU. Bei nachfolgenden Aufrufen des
 Schedulers soll ueberprueft werden, ob mittlerweile ausreichend Daten vorliegen; ist
 dies der Fall, so soll der aktuelle Thread geweckt werden (s.o.). Dies loest ein
 Problem mit Systemrufen, welche im Kernel blockieren koennen (wie z.B. read()).
 Ohne diesen Mechanismus wuerde die gesamte User-Level-Thread-Bibliothek, die aus Sicht
 des Kernels ein Prozess ist, blockieren (selbst wenn andere User-Level-Threads
 lauffaehig waeren). Wir kuemmern uns hier nur um die read()-Funktion, obwohl auch andere
 Systemrufe blockieren koennen.
 */
int ult_read(int fd, void *buf, int count) {
	
  int data; //returns != 0 if data is available
  ssize_t nread;
  if (!FD_ISSET(fd, &fds)) {
    FD_SET(fd, &fds);       //Creating fd_set for select()
  }

	printf("\n");
  //set time select() is going to wait
  tv_str.tv_sec = 0;
  tv_str.tv_usec = 200;

  //is data available?
  data = select(fd + 1, &fds, NULL, NULL, &tv_str);
  
  if (data) {
    nread = read(fd, buf, count);   //data is available - read it!
    FD_CLR(fd, &fds);
    return nread;
  }

  return -1;
}


// start scheduling and spawn a thread running function f
/*	Die Funktion ult_init() initialisiert die Bibliothek (insbesondere die Datenstrukturen
 des Schedulers wie z.B. Thread-Listen) und muss vor allen anderen Funktionen aufgerufen
 werden. Sie erzeugt einen einzigen Thread (analog zum Init-Threadâ bei Unix), aus
 welchem heraus dann alle anderen Threads erzeugt werden und welcher danach mit
 ult_waitpid() auf das Ende aller Threads wartet. 
 */
void ult_init(ult_func f) {
	tcb_list* tmp_tlist;
	int i;
	for(i = 0; i < MAX_TCB; i++){ // initialisierungen fuer unsere TCB_structs 
		tcb_array[i] = malloc(sizeof(tcb));
		tcb_array[i]->Thread_ID= i; // gleich ID zuweisung
	}
	
		
	
	

	ult_spawn(f); // hier wurde der erste Thread erzeugt, tcb_array[0] hat also Valide werrte
	/*
	 * Der sheduler wird den Thread 0 starten und diesen bis zum 
	 * Ult-Wait nicht verlassen. Wobei nach dem 
	 * verlassen von thread 0 nur noch threadA und threadB abgearbeitet
	 * werden bis einer der beiden Blockiert.
	 * 
	 * Daher macht es sinn, thread0 thread 0 nach dem ult-Wait in eine andere Qeue zu schieben. 
	 */
	
	/* make tlist a circular list */
	tmp_tlist = tlist;
	// go to last element in tlist
	while(tmp_tlist->next != NULL){
		tmp_tlist = tmp_tlist->next;
	}
	// next element of last element is first element
	tmp_tlist->next = tlist;


	/* scheduler */
	/*i = setjmp(sheduler);
	if (i){
		if (i==1){
			next_TCB= tcb_array[2]; // thread a
		}
		else if (i==2){
			
			next_TCB= tcb_array[1]; // thread b
		}
	*/

	/* scheduling method: always run the next one */
	while(tlist){
		tcb_swapcontext(tlist->tcb, tlist->next->tcb);
		tlist = tlist->next;
	}

	
		
		/*
		 * Hier wird in abhängigkeit von I etwas angestellt
		 */
	
	/* Longjumps in der ult.c? */
	// longjmp(tcb_array[0]->env,1); 
	// hier kommen wir nur hin, wenn wir bestimmtes ausführen.
	
}	
