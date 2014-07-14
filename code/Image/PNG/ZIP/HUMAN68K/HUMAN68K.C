#include <string.h>
#include <sys/stat.h>

int h68_stat(char *name, struct stat *st)
{
  int ret;

#if 0   /* This is not needed.  LIBC is debugged. */
  if (strchr(name, '*') != NULL || strchr(name, '?') != NULL)
    return -1;
#endif
  ret = stat(name, st);
  if (ret == -1)
    return -1;
  if (S_ISEXBIT(st->st_mode))
    st->st_mode |= S_IEXEC;
  else
    st->st_mode &= ~S_IEXEC;
  st->st_mode &= (~022 & 0177777);
  return ret;
}
