#include <process.h>
#include <stdio.h>

void main(int argc,char *argv[])
  {
  printf("Chaining to %s.",argv[1]);
  spawnl(P_WAIT,argv[1],argv[1],NULL);
  //execl(P_WAIT,argv[1],argv[1],NULL); 
  }