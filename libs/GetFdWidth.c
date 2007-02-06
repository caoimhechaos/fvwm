#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../configure.h"


int GetFdWidth(void)
{
#ifdef HAVE_SYSCONF
  return sysconf(_SC_OPEN_MAX);
#else
  return getdtablesize();
#endif
}
