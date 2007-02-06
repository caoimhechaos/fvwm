#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "fvwmlib.h"
/***************************************************************************
 *
 * A simple routine to copy a string, stripping spaces and mallocing
 * space for the new string 
 ***************************************************************************/
void CopyString(char **dest, char *source)
{
  int len;
  char *start;
  
  while(((isspace(*source))&&(*source != '\n'))&&(*source != 0))
    {
      source++;
    }
  len = 0;
  start = source;
  while((*source != '\n')&&(*source != 0))
    {
      len++;
      source++;
    }
  
  source--;
  while((isspace(*source))&&(*source != 0)&&(len >0))
    {
      len--;
      source--;
    }
  *dest = safemalloc(len+1);
  strncpy(*dest,start,len);
  (*dest)[len]=0;	  
}


