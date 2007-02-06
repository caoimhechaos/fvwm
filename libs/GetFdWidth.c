#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <config.h>


int GetFdWidth(void)
{
#ifdef HAVE_SYSCONF
  return sysconf(_SC_OPEN_MAX);
#else
  return getdtablesize();
#endif
}
