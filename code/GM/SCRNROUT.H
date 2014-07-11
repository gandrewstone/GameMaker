/* scrnrout.asm header file    by Andy Stone  Dec 26,1990 */
extern "C"
  {
  extern int far writestr     ( int x, int y, int col, const char *str);
  extern int far writech      ( int x, int y, int col, int ch);
  extern int far readch       ( int x, int y);
  extern int far copypage     ( int source,int destination);
  extern int far drawbox      ( int x, int y, int x1, int y1, int col, int ltype, int shadcol);
  extern int far savebox      ( int x, int y, int x1, int y1, char *copyto);
  extern int far restorebox   ( int x, int y, int x1, int y1, char *copyfrom);
  extern int far xchgbox      ( int x, int y, int x1, int y1, char *window);
  extern int far clrbox       ( int x, int y, int x1, int y1, int attrib);
  }
/* end of header file */

