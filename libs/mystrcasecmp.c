#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int mystrcasecmp(char *s1,char *s2)
{
  int c1,c2;
  int n,n2;

  n=strlen(s1);
  n2=strlen(s2);
  if(n!=n2)
    return 1;

  for (;;)
    {
      c1 = *s1; 
      c2 = *s2;
      if (!c1 || !c2) 
	return(c1 - c2);
      if (isupper(c1)) 
	c1 = 'a' - 1 + (c1 & 31);
      if (isupper(c2)) 
	c2 = 'a' - 1 + (c2 & 31);
      if (c1 != c2) 
	return(c1 - c2);
      n--,s1++,s2++;
    }
}
