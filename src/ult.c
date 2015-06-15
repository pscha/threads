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

struct timeval tv_str; // for read datatypes
static fd_set fds;
tcb* tcb_array[MAX_TCB];
int idmarker = 20;

tcb_list* tlist;  // first TCB  AkA  current_tcb
tcb_list* zlist; // last element in Zombie-queue
tcb_list* wlist; // waiting-List 

tcb* scheduler_tcb; // schedulerTCB  auch als Prozzess-TCB beaknnt braucht nur gespeichert zu werden

jmp_buf sheduler;
	

/*
 This function only exists to tell the process to use an empty stack for the thread
 */
void signalHandlerSpawn( int arg )
{	
	printf("Signalhandler-aufruf Stackdaten:\n");
	tcb_contextprint();
	printf("tcb:%p\n",tlist->tcb);	
	if (setjmp(tlist->tcb->env)){
		tlist->tcb->func(); // hier funktion ausfuehren in dem tcb in dem wir gerade sind :: wie auch immer das mit den Pointern geht
	}

}

void tcb_makecontext(tcb *t){
	// malloc der stacksize zum erzeugen des Stacks
	struct sigaction* sa = malloc(sizeof(struct sigaction));
	printf("\tStackpointer:%p\n",&t->stack);	
	printf("\tsapointer:%p\n",sa);	
	// Create the new stack
	t->stack.ss_flags = 0;
	t->stack.ss_size = STACKSIZE;
	t->stack.ss_sp = malloc(STACKSIZE);
	if ( t->stack.ss_sp == NULL ) {
		perror( "Could not allocate stack." );
		exit( 1 );
	}
	sigaltstack( &t->stack, 0 );

	// Set up the custom signal handler
	sa->sa_handler = &signalHandlerSpawn;
	sa->sa_flags = SA_ONSTACK;
	sigemptyset( &sa->sa_mask );
	sigaction( SIGUSR1, sa, 0 );
	
	printf("\tStackpointerpinter:%p\n",t->stack.ss_sp);	
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
	tcb_list* tmp = tlist;
	printf("%p\t tlist 1\n",tlist);
	/* intialize the tcb_list entry*/
	printf("1. malloc\n");	
	tcb_list* tcb= malloc(sizeof(tcb_list));
	printf("2. malloc\n");	
	tcb->tcb = malloc(sizeof(tcb));
	printf("denfine next\n");	
	tcb->next = tlist;
	printf("denfine tlist\n");	
	tlist = tcb;
	tlist->tcb->Thread_ID = idmarker;
	printf("%p  der jmp_buff von dem Thread \n", tlist->tcb->env);
	idmarker++; // id count up
		
	printf("denfine execute function\n");	
	/* make the thread do something */
	tcb->tcb->func = f; // schreibe func-pointer in den TCB ab hier sind Kontext und ausfphrung mitenander verbunden
		printf("spawn 1\n");
		tcb_contextprint();

	tcb_makecontext(tcb->tcb); // hier wird dann der stack gesetzt, sodass ab hier der TCB block fertig sein m�sste
		printf("spawn 2\n");
		tcb_contextprint();
	
	// ab diesem Zeitpunkt befinden wir uns noch in dem Kontext in welcher die fkt ult_spawn aufgerufen wurde und haben einen voll funktions-
	// t�chtigen TCB mit einer neuen Funktion erstellt.
	
	// noch thread ID setzen einf�gen
	
	printf("end of spawn\n");
	if(tmp){
		tmp->next = tlist;
		tlist->next = tmp->next;
		tlist=tmp;
	}
	else{
		tlist = tcb;
	}
	printf("%p\t tlist 2\n",tlist);
	

	return tcb->tcb->Thread_ID;		
}


// yield the CPU to another thread
/*	Die Funktion ult_yield() gibt innerhalb eines Threads freiwillig den
 Prozessor auf. Es erfolgt der Aufruf des Schedulers und die Umschaltung
 auf den vom Scheduler ausgewaehlten Thread.
 */
void ult_yield() {
	printf("in yield\nSpringe von:");
	tcb_contextprint();
	printf("tlist_tcb: %d	%p\n",tlist->tcb->Thread_ID ,tlist->tcb);
	printf("tlist_tcb: %d	%p\n",scheduler_tcb->Thread_ID ,scheduler_tcb);
	tcb_swapcontext(tlist->tcb,scheduler_tcb);
	// hier gehts dann weiter, wenn der Thread wieder aufgerufen wird.
}

/*	Wird innerhalb eines Threads ult_exit() aufgerufen, so wird der Thread
 zum Zombie und der Exit-Status wird abgespeichert.
 */
void ult_exit(int status) {
	tcb_list *prev_element;
	tcb_list *zombie;
	
	tlist->tcb->status = status; // Exitcode wird gesetzt
	
	prev_element = tlist;
	while (1){
		prev_element = prev_element->next; // itrieren
		if(prev_element->next == tlist){
			prev_element->next = tlist->next;
			break;
		}
	}
	// prev-element haelt nun das element vor tlist. 
	
	zombie = tlist; 
	tlist = prev_element; // setzen von tlist neu auf das element vor tlist
	
	zombie->next = zlist; // in die Zombiequeue einfuegen
	zlist= zombie;
	
	
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
	 * TODO: hier muss eine ueberpruefung der Zombielemente vorgenommen werden, wenn das Element ein zombie ist, so returne den exit-status
	 * Wenn jedoch das Element NICHT in der Zlist zu finden ist so erstelle ein Waiting-element und fuelle eine Waiting-list danach fuehre ein 
	 * yield aus, 
	 */
	 tcb_list *zombie;
	 tcb_list *prev_element;
	 tcb_list *tmp_wlist;
	 zombie = zlist; //damit wir den zlistpointer nicht veraendern
	 
	 
	 
	 while (zombie != NULL){ // exestieren zombies dann...
		 if(tid == zombie->tcb->Thread_ID){ // wenn gesuchte ID gefunden wurde dann...
			 status= &zombie->tcb->status;  
			 return 0; // ich returne null weil alles gut funktioniert hat, wir muessen das Interface einhalten daher den status NICHT returnen
		 }
		 printf("ult_waitpid wartet auf %d - findet in Zombie_list: %d ", tid, zombie->tcb->status);
		 zombie = zombie->next; // itterieren der Zombieliste
	 }
	printf("keine Z gefunden\n");
	// wenn ich hier hinkomme so wurde das Element nicht gefunden
	
	tlist->tcb->Wait_ID = tid;  // setzt das Flag das markiert, das der Thread in die Waitingquee verschoben wurde
	
	
	// close the cycle
	prev_element = tlist;
	while (1){
		prev_element = prev_element->next; // itrieren
		if(prev_element->next == tlist->next){
			prev_element->next = tlist->next->next;
			break;
		}
	}
	
	printf("cycle geschlossen\n");
	
	tmp_wlist = tlist;
	if (wlist != NULL){
		// wliste ist nicht leere also müssen wir das Element an das letzte element kleben:
		while(wlist->next){
			wlist = wlist->next; // iteriere
		}
		wlist->next = tlist; // an des letzte element wurde die tlist rangehangen
	}else{
		wlist = tlist;
		printf("element angefuegt\n");
	}
	wlist = tmp_wlist;
	
	// wlist hat nun ein Element welches auf das Element zeigt was auch auf prev_element zeigt. 
	
	
	
	ult_yield(); // danach springe zum sheduler und komm erst wieder, wenn 
	// sind wiedergekommen! 
	
	
	
	while (zombie != NULL){ // exestieren zombies dann...
		 if(tid == zombie->tcb->Thread_ID){ // wenn gesuchte ID gefunden wurde dann...
			 status= &zombie->tcb->status;  
			 return 0; // ich returne null weil alles gut funktioniert hat, wir muessen das Interface einhalten daher den status NICHT returnen
		 }
		 printf("ult_waitpid wartet auf %d - findet in Zombie_list: %d ", tid, zombie->tcb->status);
		 zombie = zombie->next; // itterieren der Zombieliste
	 }
	
	
	
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
 werden. Sie erzeugt einen einzigen Thread (analog zum Init-Thread� bei Unix), aus
 welchem heraus dann alle anderen Threads erzeugt werden und welcher danach mit
 ult_waitpid() auf das Ende aller Threads wartet. 
 */
void ult_init(ult_func f) {
	tcb_list* tmp_tlist;
	tcb_list* tmp_zlist;
	tcb_list* tmp_wlist;
	tcb_list* prev_element;
	int i;
	zlist = NULL; // wir haben noch keine Zombies
	wlist = NULL; // waiting_list f�r sp�ter
	scheduler_tcb = malloc(sizeof(tcb)); // hole speicher f�r tcbblock
	scheduler_tcb->Thread_ID = 42;  // main thread ID wird auf 42 gesetzt  weil wir das k�nnen
	for(i = 0; i < MAX_TCB; i++){ // initialisierungen fuer unsere TCB_structs 
		tcb_array[i] = malloc(sizeof(tcb));
		tcb_array[i]->Thread_ID= i; // gleich ID zuweisung
	}
	
	ult_spawn(f); // hier wurde der erste Thread erzeugt, tcb_array[0] hat also Valide werrte
	
	printf("%p  der jmp_buff von scheduler\n", scheduler_tcb->env);

	printf("before swap\n");
	tcb_swapcontext(scheduler_tcb,tlist->tcb); // f�hre thread "my init" aus und starte den Sheduler
	/* make tlist a circular list */
	tmp_tlist = tlist;
	// go to last element in tlist
	while(tmp_tlist->next != NULL){	
		tmp_tlist = tmp_tlist->next; // iterieren durch die qeue 
	}
	// next element of last element is first element
	tmp_tlist->next = tlist;
	//------------------------------------------------------------------

	/* tmp_tlist ain't needed anymore */
	tmp_tlist = NULL;
	tlist = tlist->next;
	printf("erster Thread vom scheduler %d\n",tlist->tcb->Thread_ID);
	/* scheduling method: always run the next one */
	while(tlist){
		printf("\t\tswap context\n");
		tcb_swapcontext(scheduler_tcb, tlist->tcb); // scheduler_tcb repr�sentiert unseren Main Thread
		// hier m�ssen instruktionen hin damit der sheduler korrekt weiterarbeitet. der n�chste Tcb wird dann im folgenden Schleifendurchlauf aufgerufen.
		//--------------------------------------------------------------
		// Waiting-list bereinigen!
		
		//--------------------------------------------------------------
		tmp_zlist = zlist;
		tmp_wlist = wlist;
		tmp_tlist = tlist;
		while(wlist){
			while(zlist){
				if(wlist->tcb->Wait_ID == zlist->tcb->Thread_ID){
					wlist->next = tlist;
					while(tlist->next != wlist->next){
						tlist->next = tlist->next->next;
					}
					tlist->next = wlist; //close cycle
					tlist->next = tlist->next->next;
				}
				zlist = zlist->next;
			}
			wlist = wlist->next;
		}
		
		wlist = tmp_wlist;
		zlist = tmp_zlist;
		
		if (tlist->tcb->Wait_ID){
			tlist->next = NULL;
			tlist = prev_element->next; // sonderbahndlung fuer waiting threads
		}else{
			prev_element = tlist;
			tlist = tlist->next;
		}
		tlist = tlist->next;
	}

	
		
		/*
		 * Hier wird in abh�ngigkeit von I etwas angestellt
		 */
	
	/* Longjumps in der ult.c? */
	// longjmp(tcb_array[0]->env,1); 
	// hier kommen wir nur hin, wenn wir bestimmtes ausf�hren.
	
}	
