#include <stdio.h>
#include <string.h>


/************************************************************************
 *
 * Concatentates 3 strings
 *
 *************************************************************************/
char CatS[256];

char *CatString3(char *a, char *b, char *c)
{
  if (strlen(a)+strlen(b)+strlen(c) > 255)
    {
      return NULL;
    }
  strcpy(CatS, a);
  strcat(CatS, b);
  strcat(CatS, c);
  return CatS;
}
