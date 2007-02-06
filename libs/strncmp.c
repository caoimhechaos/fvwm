#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int mystrncasecmp(char *s1,char *s2,int n)
{
  register int c1,c2;
  
  for (;;)
    {
      if (!n) return(0);
      c1 = *s1,c2 = *s2;
      if (!c1 || !c2) return(c1 - c2);
      if (isupper(c1)) c1 = 'a' - 1 + (c1 & 31);
      if (isupper(c2)) c2 = 'a' - 1 + (c2 & 31);
      if (c1 != c2) return(c1 - c2);
      n--,s1++,s2++;
    }
}
