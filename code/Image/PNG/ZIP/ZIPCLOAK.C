/*
   Dummy version of Info-ZIP zipcloak.c encryption code.  See "Where"
   file for sites from which to obtain full encryption sources.
 */

#include "tailor.h"

void warn  OF((char *msg1, char *msg2));
void err   OF((int code, char *msg));
int  main  OF((void));

void warn(msg1, msg2)
    char *msg1, *msg2;        /* message strings juxtaposed in output */
{
    fprintf(stderr, "zipcloak warning: %s%s\n", msg1, msg2);
}

void err(code, msg)
    int code;               /* error code from the ZE_ class */
    char *msg;              /* message about how it happened */
{
    if (code) warn(msg, "");
}

int main()
{
    fprintf(stderr, "\
This version of ZipCloak does not support encryption.  Get zcrypt20.zip (or\n\
a later version) and recompile.  The Info-ZIP file `Where' lists sites.\n");
    exit(1);
    return 1; /* avoid warning */
}
