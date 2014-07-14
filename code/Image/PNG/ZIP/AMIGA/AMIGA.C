/* OS specific routines for AMIGA platform.
 *
 * John Bush    <JBush@landfill.east.sun.com>  BIX: jbush
 * Paul Kienitz <Paul.Kienitz@f28.n125.z1.fidonet.org>
 *
 * History:
 *
 * Date     DoBee    Comments
 * -------  -------- -----------------------------------------------
 * 21Jan93  JBush    Original coding.
 *                   Incorporated filedate.c (existing routine).
 *
 * 31Jan93  JBush    Made filedate.c include unconditional.
 *
 * 18Jul93  PaulK    Moved Aztec _abort() here from stat.c because we
 *                   can't share the same one between Zip and UnZip.
 *                   Added close_leftover_open_dirs() call to it.
 */


/* ============================================================ */
/* filedate.c is an external file, since it's shared with UnZip */

#include "amiga/filedate.c"

/* ============================================================ */
/* the same applies to stat.c, but only for Aztec               */

#ifdef AZTEC_C
#  include "amiga/stat.c"

/* the following handles cleanup when a ^C interrupt happens: */

#  include "ziperr.h"
#  define echon() set_con()     /* make sure this matches crypt.h */
void err(int c, char *h);

void _abort(void)               /* called when ^C is pressed */
{
    close_leftover_open_dirs();
    echon();
    err(ZE_ABORT, "^C");
}

#endif /* AZTEC_C */

