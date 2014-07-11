#include <string.h>

extern int AbortMSGLength;
extern char *abortMSG;
extern int ExitCode;

char NewAbortMSG(char *str)
  {
  int t;

  t=strlen(str);
  if (t<80)
    {
    strcpy(abortMSG,str);
    AbortMSGLength=t;
    return(TRUE);
    }
  return(FALSE);
  }

