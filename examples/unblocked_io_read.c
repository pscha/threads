#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

struct timeval tv_str;
static fd_set fds;

int unblock_io_read(int fd, void *buf, int count) {

  int data; //returns != 0 if data is available
  ssize_t nread;

  if (!FD_ISSET(fd, &fds)) {
    FD_SET(fd, &fds);       //Creating fd_set for select()
  }

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

int main(int argc, char **argv) {
  int fd_read;
  int read_count, max_read;
  int res;
  
  char buf[10];
  
	FD_ZERO(&fds);

	if((fd_read = open("/dev/random", O_RDONLY)) == -1) {
		printf("ERROR: Can't open file to read.\n");
		exit(-1);
	}
	
	if ( argc > 1 ) {
	  max_read = atoi(argv[1]);
  } else {
    max_read = 100;
  }
  
  read_count = 0;
  
  while ( read_count < max_read ) {
    res = unblock_io_read(fd_read, buf, 10);
    
    printf("Read: %d Bytes Overall read: %d of %d Bytes\n",res, read_count, max_read);

    usleep(1000);
    
    if ( res != -1 ) {
      read_count += res;
    }
  }
	
	close(fd_read);
}
