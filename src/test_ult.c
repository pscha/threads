#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include "ult.h"

int end_copying;
long copyed_bytes = 0;
time_t start_time;

void threadA()
{
	char c[256];
	FILE* rnd;
	int fd;	
	
	start_time = time(NULL);
	rnd = fopen("/dev/random","r");
	fd = fileno(rnd);

	while(!end_copying)
	{
		ult_read(fd,c,256); 
		copyed_bytes = copyed_bytes + 256;			
	}
	ult_exit(0);
}

void threadB()
{
	char* in;
	while(!end_copying)
	{
		printf("> ");
		getline(&in,0,NULL);
			
		if(strncmp("exit",in,4))
		{
			end_copying = 1;
		}
		else if(strncmp("status",in,6))
		{
			printf("time = %f\nbytes = %li\n",difftime(time(NULL) ,start_time),copyed_bytes);
		}
		free(in);
	}
	ult_exit(0);
}

void myInit()
{
    int cpid[2], i, status;
    printf("spawn A\n"); fflush(stdout);
    cpid[0] = ult_spawn(threadA);
    printf("spawn B\n"); fflush(stdout);
    cpid[1] = ult_spawn(threadB);
    for (i = 0; i < 2; i++) {
        printf("waiting for cpid[%d] = %d\n", i, cpid[i]); fflush(stdout);
        if (ult_waitpid(cpid[i], &status) == -1) {
            fprintf(stderr, "waitpid for %d failed\n", cpid[i]);
            ult_exit(-1);
        }
        printf("(status = %d)\n", status);
    }
    puts("ciao!");
    ult_exit(0);
}

int main()
{
    printf("starting myInit\n"); fflush(stdout);
    ult_init(myInit);
    exit(0);
}


