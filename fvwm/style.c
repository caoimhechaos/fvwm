/****************************************************************************
 * This module is all original code 
 * by Rob Nation 
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for parsing the fvwm style command
 *
 ***********************************************************************/
#include "../configure.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "../version.h"

void ParseStyle(char *text, FILE *fd, char **list, int *junk)
{
  extern char *orig_tline;
  char *name, *line;
  char *restofline,*tmp;
  char *icon_name = NULL;
  char *forecolor = NULL;
  char *backcolor = NULL;
  unsigned long off_buttons=0;
  unsigned long on_buttons=0;
  int butt;

  int len,desknumber = 0,bw=0, nobw = 0;
  unsigned long off_flags = 0;
  unsigned long on_flags = 0;
  
  name = stripcpy2(text,FALSE,TRUE);
  /* in case there was no argument! */
  if(name == NULL)
    return;

  restofline = stripcpy3(text,FALSE);
  line = restofline;

  if(restofline == NULL)return;
  while((*restofline != 0)&&(*restofline != '\n'))
    {
      while(isspace(*restofline))restofline++;
      if(mystrncasecmp(restofline,"ICON",4)==0)
	{
	  restofline +=4;
	  while(isspace(*restofline))restofline++;
	  tmp = restofline;
	  len = 0;
	  while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&(*tmp != '\n'))
	    {
	      tmp++;
	      len++;
	    }
	  if(len > 0)
	    {
	      icon_name = safemalloc(len+1);
	      strncpy(icon_name,restofline,len);
	      icon_name[len] = 0;
	      off_flags |= ICON_FLAG;
	      on_flags |= SUPPRESSICON_FLAG;
	    }
	  else
	    on_flags |= SUPPRESSICON_FLAG;	    
	  restofline = tmp;
	}
      if(mystrncasecmp(restofline,"COLOR",5)==0)
	{
	  restofline +=5;
	  while(isspace(*restofline))restofline++;
	  tmp = restofline;
	  len = 0;
	  while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
		(*tmp != '\n')&&(*tmp != '/')&&(!isspace(*tmp)))
	    {
	      tmp++;
	      len++;
	    }
	  if(len > 0)
	    {
	      forecolor = safemalloc(len+1);
	      strncpy(forecolor,restofline,len);
	      forecolor[len] = 0;
	      off_flags |= FORE_COLOR_FLAG;
	    }

	  while(isspace(*tmp))tmp++;
	  if(*tmp == '/')
	    {
	      tmp++;
	      while(isspace(*tmp))tmp++;
	      restofline = tmp;
	      len = 0;
	      while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
		    (*tmp != '\n')&&(*tmp != '/')&&(!isspace(*tmp)))
		{
		  tmp++;
		  len++;
		}
	      if(len > 0)
		{
		  backcolor = safemalloc(len+1);
		  strncpy(backcolor,restofline,len);
		  backcolor[len] = 0;
		  off_flags |= BACK_COLOR_FLAG;
		}
	    }
	  restofline = tmp;
	}
      if(mystrncasecmp(restofline,"FORECOLOR",9)==0)
	{
	  restofline +=9;
	  while(isspace(*restofline))restofline++;
	  tmp = restofline;
	  len = 0;
	  while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
		(*tmp != '\n')&&(*tmp != '/')&&(!isspace(*tmp)))
	    {
	      tmp++;
	      len++;
	    }
	  if(len > 0)
	    {
	      forecolor = safemalloc(len+1);
	      strncpy(forecolor,restofline,len);
	      forecolor[len] = 0;
	      off_flags |= FORE_COLOR_FLAG;
	    }
	  restofline = tmp;
	}

      if(mystrncasecmp(restofline,"BACKCOLOR",9)==0)
	{
	  restofline +=9;
	  while(isspace(*restofline))restofline++;
	  tmp = restofline;
	  len = 0;
	  while((tmp != NULL)&&(*tmp != 0)&&(*tmp != ',')&&
		(*tmp != '\n')&&(*tmp != '/')&&(!isspace(*tmp)))
	    {
	      tmp++;
	      len++;
	    }
	  if(len > 0)
	    {
	      backcolor = safemalloc(len+1);
	      strncpy(backcolor,restofline,len);
	      backcolor[len] = 0;
	      off_flags |= BACK_COLOR_FLAG;
	    }
	  restofline = tmp;
	}
      else if(mystrncasecmp(restofline,"NoIconTitle",11)==0)
	{
	  off_flags |= NOICON_TITLE_FLAG;
	  restofline +=11;
	}	
      else if(mystrncasecmp(restofline,"IconTitle",9)==0)
	{
	  on_flags |= NOICON_TITLE_FLAG;
	  restofline +=9;
	}	
      else if(mystrncasecmp(restofline,"NOICON",6)==0)
	{
	  restofline +=6;
	  off_flags |= SUPPRESSICON_FLAG;
	}
      else if(mystrncasecmp(restofline,"NOTITLE",7)==0)
	{
	  restofline +=7;
	  off_flags |= NOTITLE_FLAG;
	}
      else if(mystrncasecmp(restofline,"TITLE",5)==0)
	{
	  restofline +=5;
	  on_flags |= NOTITLE_FLAG;
	}
      else if(mystrncasecmp(restofline,"NOHANDLES",9)==0)
	{
	  restofline +=9;
	  off_flags |= NOBORDER_FLAG;
	}	
      else if(mystrncasecmp(restofline,"HANDLES",7)==0)
	{
	  restofline +=7;
	  on_flags |= NOBORDER_FLAG;
	}
      else if (mystrncasecmp(restofline,"NOBUTTON",8)==0)
      {
        restofline +=8;

        sscanf(restofline,"%d",&butt);
        while(isspace(*restofline))restofline++;
        while((!isspace(*restofline))&&(*restofline!= 0)&&
              (*restofline != ',')&&(*restofline != '\n'))
          restofline++;
        while(isspace(*restofline))restofline++;

        off_buttons |= (1<<(butt-1));
      }
      else if (mystrncasecmp(restofline,"BUTTON",6)==0)
	{
	  restofline +=6;
	  
	  sscanf(restofline,"%d",&butt);
	  while(isspace(*restofline))restofline++;
	  while((!isspace(*restofline))&&(*restofline!= 0)&&
		(*restofline != ',')&&(*restofline != '\n'))
	    restofline++;
	  while(isspace(*restofline))restofline++;
	  
	  on_buttons |= (1<<(butt-1));        
	}
      else if(mystrncasecmp(restofline,"WindowListSkip",14)==0)
	{
	  restofline +=14;
	  off_flags |= LISTSKIP_FLAG;
	}	
      else if(mystrncasecmp(restofline,"WindowListHit",13)==0)
	{
	  restofline +=13;
	  on_flags |= LISTSKIP_FLAG;
	}	
      else if(mystrncasecmp(restofline,"CirculateSkip",13)==0)
	{
	  restofline +=13;
	  off_flags |= CIRCULATESKIP_FLAG;
	}	
      else if(mystrncasecmp(restofline,"CirculateHit",12)==0)
	{
	  restofline +=12;
	  on_flags |= CIRCULATESKIP_FLAG;
	}	
      else if(mystrncasecmp(restofline,"StartIconic",11)==0)
	{
	  restofline +=11;
	  off_flags |= START_ICONIC_FLAG;
	}	
      else if(mystrncasecmp(restofline,"StartNormal",11)==0)
	{
	  restofline +=11;
	  on_flags |= START_ICONIC_FLAG;
	}	
      else if(mystrncasecmp(restofline,"StaysOnTop",10)==0)
	{
	  restofline +=10;
	  off_flags |= STAYSONTOP_FLAG;	  
	}	
      else if(mystrncasecmp(restofline,"StaysPut",8)==0)
	{
	  restofline +=8;
	  on_flags |= STAYSONTOP_FLAG;	  
	}	
      else if(mystrncasecmp(restofline,"Sticky",6)==0)
	{
	  off_flags |= STICKY_FLAG;	  
	  restofline +=6;
	}	
      else if(mystrncasecmp(restofline,"Slippery",8)==0)
	{
	  on_flags |= STICKY_FLAG;	  
	  restofline +=8;
	}	
      else if(mystrncasecmp(restofline,"BorderWidth",11)==0)
	{
	  restofline +=11;
	  off_flags |= BW_FLAG;
	  sscanf(restofline,"%d",&bw);
	  while(isspace(*restofline))restofline++;
	  while((!isspace(*restofline))&&(*restofline!= 0)&&
		 (*restofline != ',')&&(*restofline != '\n'))
	    restofline++;
	  while(isspace(*restofline))restofline++;
	}
      else if(mystrncasecmp(restofline,"HandleWidth",11)==0)
	{
	  restofline +=11;
	  off_flags |= NOBW_FLAG;
	  sscanf(restofline,"%d",&nobw);
	  while(isspace(*restofline))restofline++;
	  while((!isspace(*restofline))&&(*restofline!= 0)&&
		 (*restofline != ',')&&(*restofline != '\n'))
	    restofline++;
	  while(isspace(*restofline))restofline++;
	}
      else if(mystrncasecmp(restofline,"STARTSONDESK",12)==0)
	{
	  restofline +=12;
	  off_flags |= STAYSONDESK_FLAG;
	  sscanf(restofline,"%d",&desknumber);
	  while(isspace(*restofline))restofline++;
	  while((!isspace(*restofline))&&(*restofline!= 0)&&
		 (*restofline != ',')&&(*restofline != '\n'))
	    restofline++;
	  while(isspace(*restofline))restofline++;
	}
      else if(mystrncasecmp(restofline,"STARTSANYWHERE",14)==0)
	{
	  restofline +=14;
	  on_flags |= STAYSONDESK_FLAG;
	}
      while(isspace(*restofline))restofline++;
      if(*restofline == ',')
	restofline++;
      else if((*restofline != 0)&&(*restofline != '\n'))
	{
	  fvwm_err("bad style command in line %s at %s",
		   orig_tline,restofline,NULL);
	  return;
	}
    }
  /* capture default icons */
  if(strcmp(name,"*") == 0)
    {
      if(off_flags & ICON_FLAG)
	Scr.DefaultIcon = icon_name;
      off_flags &= ~ICON_FLAG;
      icon_name = NULL;
    }

  /* capture default colors */
  if(strcmp(name,"*") == 0)
    {
      extern char *Stdfore, *Stdback;

      if(off_flags & FORE_COLOR_FLAG)
	Stdfore = forecolor;
      off_flags &= ~FORE_COLOR_FLAG;
      forecolor = NULL;
      if(off_flags & BACK_COLOR_FLAG)
	Stdback = backcolor;
      off_flags &= ~BACK_COLOR_FLAG;
      backcolor = NULL;
    }
  free(line);
  AddToList(name,icon_name,off_flags,on_flags,desknumber,bw,nobw,
	    forecolor,backcolor,off_buttons,on_buttons);
}


void AddToList(char *name, char *icon_name, unsigned long off_flags, 
	       unsigned long on_flags, int desk, int bw, int nobw,
	       char *forecolor, char *backcolor,
               unsigned long off_buttons, unsigned long on_buttons)
{
  name_list *nptr,*lastptr = NULL;

  if((name == NULL)||((off_flags == 0)&&(on_flags == 0)&&(on_buttons == 0)&&
		      (off_buttons == 0)))
    {
      if(name)
	free(name);
      if(icon_name)
	free(icon_name);
      return;
    }

  /* used to merge duplicate entries, but that is no longer
   * appropriate since conficting styles are possible, and the
   * last match should win! */
  for (nptr = Scr.TheList; nptr != NULL; nptr = nptr->next)
    {
      lastptr=nptr;
    }

  nptr = (name_list *)safemalloc(sizeof(name_list));
  nptr->next = NULL;
  nptr->name = name;
  nptr->on_flags = on_flags;
  nptr->off_flags = off_flags;
  nptr->value = icon_name;
  nptr->Desk = desk;
  nptr->border_width = bw;
  nptr->resize_width = nobw;
  nptr->ForeColor = forecolor;
  nptr->BackColor = backcolor;

  nptr->off_buttons = off_buttons;
  nptr->on_buttons = on_buttons;

  if(lastptr != NULL)
    lastptr->next = nptr;
  else
    Scr.TheList = nptr;
}    

