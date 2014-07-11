#include <stdio.h>
#include <stdlib.h>
static void cleanup(void);

int main(void)
  {
  printf("Dummy program worked!\n");
  atexit(cleanup);
  exit(1);
  }

static void cleanup(void)
  {
  printf("Cleanup fn.\n");
  }

