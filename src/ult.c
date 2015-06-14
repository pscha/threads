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
#define THREAD_IS_ALIVE 1337

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

tcb_list* tlist;  // first TCB  AkA  current_tcb
zombie_list* zlist; // last element in Zombie-queue
waiting_list* wlist; // waiting-List 

tcb* scheduler_tcb; // schedulerTCB  auch als Prozzess-TCB beaknnt braucht nur gespeichert zu werden

jmp_buf sheduler;
	

/*
 This function only exists to tell the process to use an empty stack for the thread
 */
void signalHandlerSpawn( int arg )
{	
	printf("Signalhandler-aufruf Stackdaten:\n");
	tcb_contextprint();
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
	printf("in yield\nSpringe von:");
	tcb_contextprint();
	tcb_swapcontext(tlist->tcb,scheduler_tcb);
	// hier gehts dann weiter, wenn der Thread wieder aufgerufen wird.
}

/*	Wird innerhalb eines Threads ult_exit() aufgerufen, so wird der Thread
 zum Zombie und der Exit-Status wird abgespeichert.
 */
void ult_exit(int status) {
	tlist->tcb->zombie_flag = status; // zombieflag wird gesetzt
	
	// Zombieelement Appenden
	zombie_list zombie; 
	zombie.thread_id= tlist->tcb->Thread_ID; // Thread ID wurde zum Zombie
	zombie.status = status; // setze exit_status
	zombie.nextzombie = zlist; // Append an liste, wenn erstes Element dann = NULL
	zlist = &zombie; 
	// zombie_list hat ein neuen Element
	free(tlist->tcb->stack.ss_sp); // Free the Stack!
	free(tlist->tcb); // free the tcb der Thread exestiert nicht mehr alle Pointer und jmp_buf löschen
	
	longjmp(scheduler_tcb->env,100); // springe zum sheduler 	
}

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
	/*
	 * TODO: hier muss eine Überprüfung der Zombielemente vorgenommen werden, wenn das Element ein zombie ist, so returne den exit-status
	 * Wenn jedoch das Element NICHT in der Zlist zu finden ist so erstelle ein Waiting-element und fürlle eine Waiting-list danach führe ein 
	 * yield aus, 
	 */
	 zombie_list *zombie;
	 zombie = zlist; //damit wir den zlistpointer nicht verändern
	 
	 while (zombie != NULL){ // exestieren zombies dann...
		 if(tid == zombie->thread_id){ // wenn gesuchte ID gefunden wurde dann...
			 status= &zombie->status; // nur existent wenn das Zombieelement exestiert ?!?!!!??!?!???? 
			 return 0; // ich returne null weil alles gut funktioniert hat, wir muessen das Interface einhalten daher den status NICHT returnen
		 }
		 printf("ult_waitpid wartet auf %d - findet in Zombie_list: %d ", tid, zombie->status);
		 zombie = zombie->nextzombie; // itterieren der Zombieliste
	 }
	
	// wenn ich hier hinkomme so wurde das Element nicht gefunden
	
	
	
	return -1;	//return 'error'
}

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

/*	Die Funktion ult_init() initialisiert die Bibliothek (insbesondere die Datenstrukturen
 des Schedulers wie z.B. Thread-Listen) und muss vor allen anderen Funktionen aufgerufen
 werden. Sie erzeugt einen einzigen Thread (analog zum Init-Threadâ bei Unix), aus
 welchem heraus dann alle anderen Threads erzeugt werden und welcher danach mit
 ult_waitpid() auf das Ende aller Threads wartet. 
 */
void ult_init(ult_func f) {
	zlist = NULL; // wir haben noch keine Zombies
	wlist = NULL; // waiting_list für später
	scheduler_tcb = malloc(sizeof(tcb)); // hole speicher für tcbblock
	tcb_list* tmp_tlist;
	int i;
	scheduler_tcb->Thread_ID = 42;  // main thread ID wird auf 42 gesetzt  weil wir das können
	for(i = 0; i < MAX_TCB; i++){ // initialisierungen fuer unsere TCB_structs 
		tcb_array[i] = malloc(sizeof(tcb));
		tcb_array[i]->Thread_ID= i; // gleich ID zuweisung
	}

	ult_spawn(f); // hier wurde der erste Thread erzeugt, tcb_array[0] hat also Valide werrte

	tcb_swapcontext(scheduler_tcb,tlist->tcb); // führe thread "my init" aus und starte den Sheduler

	/* make tlist a circular list */
	tmp_tlist = tlist;
	// go to last element in tlist
	while(tmp_tlist->next != NULL){	
		tmp_tlist = tmp_tlist->next; // iterieren durch die qeue 
	}
	// next element of last element is first element
	tmp_tlist->next = tlist;
	//------------------------------------------------------------------

	/* scheduling method: always run the next one */
	while(tlist){
		tcb_swapcontext(scheduler_tcb, tlist->tcb); // scheduler_tcb repräsentiert unseren Main Thread
		// hier müssen instruktionen hin damit der sheduler korrekt weiterarbeitet. der nächste Tcb wird dann im folgenden Schleifendurchlauf aufgerufen.
		
		/*
		 * TODO: ACHTUNG ABSOLUT WICHTIG !!!!!!!!!!!!!!!!!!!
		 * tlist muss IMMER auf den tcb zeigen in dem Kontext wir uns befinden, der Thread muss darauf achten!
		 */
		
		
		/*
		 *  TODO:  Wenn ein Zombieflag gesetzt ist so tue folgendes: Lösche den Thread aus der Running_queue 
		 * 	
		 */
		 
		 /*
		  * TODO: Wenn sich in der Zombieliste etwas befindet, so wird in der Waiting-list nachgeschaut... wenn die Threadids identisch sind, so kehre
		  * in den hinterlegten tcb zurück welcher in einem Waiting-struct gespechertn worden ist, außerdem lösche das Zombie-element, dann ist jeglicher hinweis
		  * auf das Leben des Thread vernichtet worden.
		  */
		tlist = tlist->next;
	}

	
		
		/*
		 * Hier wird in abhängigkeit von I etwas angestellt
		 */
	
	/* Longjumps in der ult.c? */
	// longjmp(tcb_array[0]->env,1); 
	// hier kommen wir nur hin, wenn wir bestimmtes ausführen.
	
}	
