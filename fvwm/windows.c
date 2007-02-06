/****************************************************************************
 * This module is all new
 * by Rob Nation
 * A little of it is borrowed from ctwm.
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/
/***********************************************************************
 *
 * fvwm window-list popup code
 *
 ***********************************************************************/

#include "../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

/* I tried to include "limits.h" to get these values, but it
 * didn't work for some reason */
/* Minimum and maximum values a `signed int' can hold.  */
#define MY_INT_MIN (- MY_INT_MAX - 1)
#define MY_INT_MAX 2147483647

extern XContext MenuContext;
/*
 * Change by PRB (pete@tecc.co.uk), 31/10/93.  Prepend a hot key
 * specifier to each item in the list.  This means allocating the
 * memory for each item (& freeing it) rather than just using the window
 * title directly.  */
void do_windowList(int val1, int val2)
{
#ifndef NO_WINDOWLIST
  MenuRoot *mr;
  MenuItem *mi,*tmp;
  FvwmWindow *t;
  char *tname;
  char loc[40],*name=NULL;
  int dwidth,dheight;
  char tlabel[50];
  int last_desk_done = MY_INT_MIN;
  int next_desk;

  char *t_hot;			/* Menu label with hotkey added */
  char scut = '0';		/* Current short cut key */

  sprintf(tlabel,"CurrentDesk: %d",Scr.CurrentDesk);
  mr=NewMenuRoot(tlabel);
  AddToMenu(mr, tlabel, "Geometry", NULL, F_TITLE,0,0,'s','s');      

  next_desk = 0;
  while(next_desk != MY_INT_MAX)
    {
      /* Sort window list by desktop number */
      if((val1 < 2)&&(val1 > -2))
	{
	  next_desk = MY_INT_MAX;
	  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	    {
	      if((t->Desk >last_desk_done)&&(t->Desk < next_desk))
		next_desk = t->Desk;
	    }
	}
      else if((val1 <4)&&(val1 > -4))
	{
	  if(last_desk_done  == MY_INT_MIN)
	    next_desk = Scr.CurrentDesk;
	  else
	    next_desk = MY_INT_MAX;
	}
      else 
	{
	  if(last_desk_done  == MY_INT_MIN)
	    next_desk = val2;
	  else
	    next_desk = MY_INT_MAX;
	}
      last_desk_done = next_desk;
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  if((t->Desk == next_desk)&&
	    (!(t->flags & WINDOWLISTSKIP)))
	    {
	      if (++scut == ('9' + 1)) scut = 'A';	/* Next shortcut key */
	      if(val1%2 != 0)
		name = t->icon_name;
	      else
		name = t->name;
	      t_hot = safemalloc(strlen(name) + 8);
	      sprintf(t_hot, "&%c.  %s", scut, name); /* Generate label */
	      
	      tname = safemalloc(40);
	      tname[0]=0;
	      if(t->flags & ICONIFIED)
		strcpy(tname, "(");
	      sprintf(loc,"%d:",t->Desk);
	      strcat(tname,loc);
	      if(t->frame_x >=0)
		sprintf(loc,"+%d",t->frame_x);
	      else
		sprintf(loc,"%d",t->frame_x);
	      strcat(tname, loc);
	      if(t->frame_y >=0)
		sprintf(loc,"+%d",t->frame_y);
	      else
		sprintf(loc,"%d",t->frame_y);
	      strcat(tname, loc);
	      dheight = t->frame_height - t->title_height - 2*t->boundary_width;
	      dwidth = t->frame_width - 2*t->boundary_width;
	      
	      dwidth -= t->hints.base_width;
	      dheight -= t->hints.base_height;
	      
	      dwidth /= t->hints.width_inc;
	      dheight /= t->hints.height_inc;

	      sprintf(loc,"x%d",dwidth);
	      strcat(tname, loc);
	      sprintf(loc,"x%d",dheight);
	      strcat(tname, loc);
	      if(t->flags & ICONIFIED)
		strcat(tname, ")");
		  
	      AddToMenu(mr, t_hot, tname, NULL, F_RAISE_IT,
			(long)t,(long)(t->w),'s','s');
	    }
	}
    }
  MakeMenu(mr);

  do_menu(mr);

  XDestroyWindow(dpy,mr->w);
  XDeleteContext(dpy, mr->w, MenuContext);  
  /* need to free the window list ? */
  mi = mr->first;
  while(mi != NULL)
    {
      tmp = mi->next;
      if (mi->func != F_TITLE)
	{
	  if (mi->item != NULL) free(mi->item);
	  if (mi->item2 != NULL) free(mi->item2);
	}
      free(mi);
      mi = tmp;
    }
  free(mr);
#endif
}


