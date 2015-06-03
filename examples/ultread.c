#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

int unblockio_read(int fd, void *buf, int count) {

  int data; //returns != 0 if data is available
  ssize_t nread;

  if (!FD_ISSET(fd, &fds)) {
    FD_SET(fd, &fds);       //Creating fd_set for select()
  }

  //set time select() is going to wait
  tv_str.tv_sec = 0;
  tv_str.tv_usec = 20;

  //is data available?
  data = select(fd + 1, &fds, NULL, NULL, &tv_str);
  
  if (data) {
    nread = read(fd, buf, count);   //data is available - read it!
    FD_CLR(fd, &fds);
    return nread;
  } else {
    //choose the next thread
    tcb next = run_get_next();
    tcb_setcontext(&next);
  }
  
  return -1;

}
