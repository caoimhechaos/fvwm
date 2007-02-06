/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified 
 * by Rob Nation 
 ****************************************************************************/
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/


/***********************************************************************
 *
 * fvwm menu code
 *
 ***********************************************************************/
#include "../configure.h"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <X11/keysym.h>
#include <sys/types.h>
#include <sys/time.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

int menu_on=0;

MenuRoot *ActiveMenu = NULL;		/* the active menu */
MenuItem *ActiveItem = NULL;		/* the active menu item */

int menuFromFrameOrWindowOrTitlebar = FALSE;

extern int Context,Button;
extern FvwmWindow *ButtonWindow, *Tmp_win;
extern XEvent Event;
int Stashed_X, Stashed_Y,MenuY=0;

void DrawTrianglePattern(Window,GC,GC,GC,int,int,int,int);
void DrawSeparator(Window, GC,GC,int, int,int,int,int);
void DrawUnderline(Window w, GC gc, int x, int y, char *txt, int off);
int UpdateMenu(void); 

extern XContext MenuContext;

/****************************************************************************
 *
 * Initiates a menu pop-up
 *
 ***************************************************************************/
int do_menu (MenuRoot *menu)
{
  int prevStashedX=0,prevStashedY=0;
  MenuRoot *PrevActiveMenu=0;
  MenuItem *PrevActiveItem=0;
  int retval=MENU_NOP;
  int x,y;

  /* this condition could get ugly */
  if(menu->in_use)
    return MENU_ERROR;

  /* In case we wind up with a move from a menu which is
   * from a window border, we'll return to here to start
   * the move */
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x, &y, &JunkX, &JunkY, &JunkMask);    

  if(menu_on)
    {
      prevStashedX = Stashed_X;
      prevStashedY = Stashed_Y;

      PrevActiveMenu = ActiveMenu;
      PrevActiveItem = ActiveItem;
      if(ActiveMenu)
	if(Scr.flags & MWMMenus)
	  x = Stashed_X+ (ActiveMenu->width >> 1) + (menu->width >> 1) - 3;
      if(ActiveItem)
	y = ActiveItem->y_offset + MenuY + (Scr.EntryHeight >>1);
    }
  else
    {
      if(!GrabEm(MENU))
	{
	  XBell(dpy,Scr.screen);
	  return MENU_DONE;
	}
      if(Scr.flags & MWMMenus)
	x += (menu->width >> 1) - 3;
    }
  if (PopUpMenu (menu, x, y))
    {
      retval = UpdateMenu();
    }
  else
    XBell (dpy, Scr.screen);

  ActiveMenu = PrevActiveMenu;
  ActiveItem = PrevActiveItem;
  if((ActiveItem)&&(menu_on))
    ActiveItem->state = 1;
  Stashed_X = prevStashedX;
  Stashed_Y = prevStashedY;


  if(!menu_on)
    {
      UngrabEm();
      WaitForButtonsUp();
    }
  return retval;
}

/***********************************************************************
 *
 *  Procedure:
 *	RelieveRectangle - add relief lines to a rectangular window
 *
 ***********************************************************************/
void RelieveRectangle(Window win,int x,int y,int w, int h,GC Hilite,GC Shadow)
{
  XDrawLine(dpy, win, Hilite, x, y, w+x-1, y);
  XDrawLine(dpy, win, Hilite, x, y, x, h+y-1);

  XDrawLine(dpy, win, Shadow, x, h+y-1, w+x-1, h+y-1);
  XDrawLine(dpy, win, Shadow, w+x-1, y, w+x-1, h+y-1);
}

/***********************************************************************
 *
 *  Procedure:
 *	RelieveHalfRectangle - add relief lines to the sides only of a
 *      rectangular window
 *
 ***********************************************************************/
void RelieveHalfRectangle(Window win,int x,int y,int w,int h,
			  GC Hilite,GC Shadow)
{
  XDrawLine(dpy, win, Hilite, x, y-1, x, h+y);
  XDrawLine(dpy, win, Hilite, x+1, y, x+1, h+y-1);

  XDrawLine(dpy, win, Shadow, w+x-1, y-1, w+x-1, h+y);
  XDrawLine(dpy, win, Shadow, w+x-2, y, w+x-2, h+y-1);
}


/***********************************************************************
 *
 *  Procedure:
 *      PaintEntry - draws a single entry in a poped up menu
 *
 ***********************************************************************/
void PaintEntry(MenuRoot *mr, MenuItem *mi)
{
  int y_offset,text_y,d, y_height;
  GC ShadowGC, ReliefGC, currentGC;

  y_offset = mi->y_offset;
  y_height = mi->y_height;
  text_y = y_offset + Scr.StdFont.y;

  ShadowGC = Scr.MenuShadowGC;
  if(Scr.d_depth<2)
    ReliefGC = Scr.MenuShadowGC;
  else
    ReliefGC = Scr.MenuReliefGC;


  if(Scr.flags & MWMMenus)
    {
      if((!mi->prev)||(!mi->prev->state))
	XClearArea(dpy, mr->w, 0,y_offset-1,mr->width,y_height+2,0);
      else
	XClearArea(dpy, mr->w, 0,y_offset+1,mr->width,y_height-1,0);
      if ((mi->state)&&(mi->func != F_TITLE)&&(mi->func != F_NOP)&&*mi->item)
	{
	  RelieveRectangle(mr->w, 3, y_offset, mr->width-5, mi->y_height,
			   ReliefGC,ShadowGC);
	  RelieveRectangle(mr->w, 2, y_offset-1, mr->width-3, mi->y_height+2,
			   ReliefGC,ShadowGC);
	}
      RelieveHalfRectangle(mr->w, 0, y_offset-1, mr->width,
			   y_height+2, ReliefGC, ShadowGC);
    }
  else
    {
      XClearArea(dpy, mr->w, 0,y_offset,mr->width,y_height,0);
      if ((mi->state)&&(mi->func != F_TITLE)&&(mi->func != F_NOP)&&*mi->item)
	RelieveRectangle(mr->w, 2, y_offset, mr->width-4, mi->y_height,
			 ReliefGC,ShadowGC);
      RelieveHalfRectangle(mr->w, 0, y_offset, mr->width,
			   y_height, ReliefGC, ShadowGC);
    }
  if(mi->func == F_TITLE)
    {
      if(Scr.flags & MWMMenus)
	{
	  text_y += HEIGHT_EXTRA>>1;
	  XDrawLine(dpy, mr->w, ShadowGC, 2, y_offset+y_height-2,
		    mr->width-3, y_offset+y_height-2);
	  XDrawLine(dpy, mr->w, ShadowGC, 2, y_offset+y_height-4,
		    mr->width-3, y_offset+y_height-4);
	}
      else
	{
	  if(mi->next != NULL)
	    {
	      DrawSeparator(mr->w,ShadowGC,ReliefGC,5, y_offset+y_height-3,
			    mr->width-6, y_offset+y_height-3,1);
	    }
	  if(mi != mr->first)
	    {
	      text_y += HEIGHT_EXTRA_TITLE>>1;
	      DrawSeparator(mr->w,ShadowGC,ReliefGC,5, y_offset+1,
			    mr->width-6, y_offset+1,1);
	    }
	}
    }
  else
    text_y += HEIGHT_EXTRA>>1;
  if(mi->func == F_NOP && *mi->item==0) 
    {
      if(Scr.flags & MWMMenus)
	DrawSeparator(mr->w,ShadowGC,ReliefGC,2,y_offset-1+HEIGHT_SEPARATOR/2,
		      mr->width-3,y_offset-1+HEIGHT_SEPARATOR/2,0);
      else
	DrawSeparator(mr->w,ShadowGC,ReliefGC,5,y_offset-1+HEIGHT_SEPARATOR/2,
		      mr->width-6,y_offset-1+HEIGHT_SEPARATOR/2,1);
    }
  if(mi->next == NULL) 
    DrawSeparator(mr->w,ShadowGC,ShadowGC,1,mr->height-2,
		  mr->width-2, mr->height-2,1);
  if(mi == mr->first) 
    DrawSeparator(mr->w,ReliefGC,ReliefGC,0,0, mr->width-1,0,-1);

  if(check_allowed_function(mi))
    currentGC = Scr.MenuGC;
  else
    /* should be a shaded out word, no just re-colored. */
    currentGC = Scr.MenuStippleGC;    

  if(*mi->item)
    XDrawString(dpy, mr->w, currentGC,mi->x,text_y, mi->item, mi->strlen);
  if(mi->strlen2>0)
    XDrawString(dpy, mr->w, currentGC,mi->x2,text_y, mi->item2,mi->strlen2);

  /* pete@tecc.co.uk: If the item has a hot key, underline it */
  if (mi->hotkey > 0)
    DrawUnderline(mr->w, currentGC,mi->x,text_y,mi->item,mi->hotkey - 1);
  if (mi->hotkey < 0)
    DrawUnderline(mr->w, currentGC,mi->x2,text_y,mi->item2, -1 - mi->hotkey);

  d=(Scr.EntryHeight-7)/2;
  if(mi->func == F_POPUP)
    if(mi->state)
      DrawTrianglePattern(mr->w, ShadowGC, ReliefGC, ShadowGC,mr->width-d-8,
			  y_offset+d-1, mr->width-d-1, y_offset+d+7);
    else
      DrawTrianglePattern(mr->w, ReliefGC, ShadowGC, ReliefGC,mr->width-d-8,
			  y_offset+d-1, mr->width-d-1, y_offset+d+7);
  return;
}

/****************************************************************************
 * Procedure:
 *	DrawUnderline() - Underline a character in a string (pete@tecc.co.uk)
 *
 * Calculate the pixel offsets to the start of the character position we
 * want to underline and to the next character in the string.  Shrink by
 * one pixel from each end and the draw a line that long two pixels below
 * the character...
 *
 ****************************************************************************/
void  DrawUnderline(Window w, GC gc, int x, int y, char *txt, int posn) 
{
  int off1 = XTextWidth(Scr.StdFont.font, txt, posn);
  int off2 = XTextWidth(Scr.StdFont.font, txt, posn + 1) - 1;
  
  XDrawLine(dpy, w, gc, x + off1, y + 2, x + off2, y + 2);
}
/****************************************************************************
 *
 *  Draws two horizontal lines to form a separator
 *
 ****************************************************************************/
void DrawSeparator(Window w, GC TopGC, GC BottomGC,int x1,int y1,int x2,int y2,
		   int extra_off)
{
  XDrawLine(dpy, w, TopGC   , x1,           y1,  x2,          y2);
  XDrawLine(dpy, w, BottomGC, x1-extra_off, y1+1,x2+extra_off,y2+1);
}
    
/****************************************************************************
 *
 *  Draws a little Triangle pattern within a window
 *
 ****************************************************************************/
void DrawTrianglePattern(Window w,GC GC1,GC GC2,GC GC3,int l,int u,int r,int b)
{
  int m;

  m = (u + b)/2;

  XDrawLine(dpy,w,GC1,l,u,l,b);

  XDrawLine(dpy,w,GC2,l,b,r,m);
  XDrawLine(dpy,w,GC3,r,m,l,u);
}

/***********************************************************************
 *
 *  Procedure:
 *	PaintMenu - draws the entire menu
 *
 ***********************************************************************/
void PaintMenu(MenuRoot *mr, XEvent *e)
{
  MenuItem *mi;
  
  for (mi = mr->first; mi != NULL; mi = mi->next)
    {
      /* be smart about handling the expose, redraw only the entries
       * that we need to
       */
      if (e->xexpose.y < (mi->y_offset + mi->y_height) &&
	  (e->xexpose.y + e->xexpose.height) > mi->y_offset)
	{
	  PaintEntry(mr, mi);
	}
    }
  XSync(dpy, 0);
  return;
}

MenuRoot *PrevMenu = NULL;
MenuItem *PrevItem = NULL;
int PrevY=0;


/***********************************************************************
 *
 *  Procedure:
 *	Updates menu display to reflect the highlighted item
 *
 ***********************************************************************/
int FindEntry(void)
{
  MenuItem *mi;
  MenuRoot *actual_mr;
  int retval = MENU_NOP;
  MenuRoot *PrevPrevMenu;
  MenuItem *PrevPrevItem;
  int PrevPrevY;
  int x, y, ChildY;
  Window Child;

  XQueryPointer( dpy, Scr.Root, &JunkRoot, &Child,
		&JunkX,&ChildY, &x, &y, &JunkMask);
  XQueryPointer( dpy, ActiveMenu->w, &JunkRoot, &JunkChild,
		&JunkX, &ChildY, &x, &y, &JunkMask);

  /* look for the entry that the mouse is in */
  for(mi=ActiveMenu->first; mi; mi=mi->next)
    if(y>=mi->y_offset && y<mi->y_offset+mi->y_height)
      break;
  if(x<0 || x>ActiveMenu->width)
    mi = NULL;


  /* if we weren't on the active entry, let's turn the old active one off */
  if ((ActiveItem)&&(mi!=ActiveItem))
    {
      ActiveItem->state = 0;
      PaintEntry(ActiveMenu, ActiveItem);
    }
  /* if we weren't on the active item, change the active item and turn it on */
  if ((mi!=ActiveItem)&&(mi != NULL))
    {
      mi->state = 1;
      PaintEntry(ActiveMenu, mi);
    }
  ActiveItem = mi;

  if(ActiveItem)
    {
      /* create a new sub-menu */
      if((ActiveItem->func == F_POPUP)&&
	 ((Scr.flags & MWMMenus)||(x>(3*ActiveMenu->width>>2))))
	{
	  PrevPrevMenu = PrevMenu;
	  PrevPrevItem = PrevItem;
	  PrevPrevY = PrevY;
	  PrevY = MenuY;
	  PrevMenu = ActiveMenu;
	  PrevItem = ActiveItem;
	  retval = do_menu(ActiveItem->menu);
	  /* Unfortunately, this is needed (why?) for multi-screen operation */
	  flush_expose(ActiveMenu->w);
	  for (mi = ActiveMenu->first; mi != NULL; mi = mi->next)
	    {
	      PaintEntry(ActiveMenu, mi);
	    }
	  XSync(dpy, 0);
	  MenuY = PrevY;
	  PrevMenu = PrevPrevMenu;
	  PrevItem = PrevPrevItem;
	  PrevY = PrevPrevY;
	}
    }
  /* end a sub-menu */ 
  if (XFindContext (dpy, Child,MenuContext,(caddr_t *)&actual_mr)==XCNOENT)
    {
      return retval;
    }

  if(actual_mr != ActiveMenu)
    {
      if(actual_mr == PrevMenu)
	{
	  if((PrevItem->y_offset + PrevY > ChildY)||
	     ((PrevItem->y_offset+PrevItem->y_height + PrevY) < ChildY))
	    {
	      return SUBMENU_DONE;
	    }
	}
      else
	return SUBMENU_DONE;
    }
  return retval;
}

/***********************************************************************
 * Procedure
 * 	menuShortcuts() - Menu keyboard processing (pete@tecc.co.uk)
 *
 * Function called from UpdateMenu instead of Keyboard_Shortcuts()
 * when a KeyPress event is received.  If the key is alphanumeric,
 * then the menu is scanned for a matching hot key.  Otherwise if
 * it was the escape key then the menu processing is aborted.
 * If none of these conditions are true, then the default processing
 * routine is called.
 ***********************************************************************/
void menuShortcuts(XEvent *ev) 
{
  MenuItem *mi;
  KeySym keysym = XLookupKeysym(&ev->xkey,0);
  
  /* Try to match hot keys */
  if (((keysym >= XK_a) && (keysym <= XK_z)) ||	/* Only consider alphabetic */
      ((keysym >= XK_0) && (keysym <= XK_9)))	/* ...or numeric keys	*/
    {
      /* Search menu for matching hotkey */
      for (mi = ActiveMenu->first; mi; mi = mi->next) 
	{
	  char key;
	  if (mi->hotkey == 0) continue;	/* Item has no hotkey	*/
	  key = (mi->hotkey > 0) ?		/* Extract hot character */
	    mi->item[mi->hotkey - 1] : mi->item2[-1 - mi->hotkey];
	  
	  /* Convert to lower case to match the keysym */
	  if (isupper(key)) key = tolower(key);
	  
	  if (keysym == key)
	    {		/* Are they equal?		*/
	      ActiveItem = mi;		/* Yes: Make this the active item */
	      ev->type = ButtonRelease;	/* Force a menu exit		*/
	      return;
	    }
	}
    }
  
  switch(keysym)		/* Other special keyboard handling	*/
    {
    case XK_Escape:		/* Escape key pressed. Abort		*/
      ActiveItem = NULL;	/* No selection				*/
      ev->type = ButtonRelease;	/* Make the menu exit			*/
      break;
      
      /* Nothing special --- Allow other shortcuts (cursor movement)	*/
    default:
      Keyboard_shortcuts(ev,ButtonRelease);
      break;
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	Updates menu display to reflect the highlighted item
 * 
 *  Returns:
 *      0 on error condition
 *      1 on return from submenu to parent menu
 *      2 on button release return
 *
 ***********************************************************************/
int UpdateMenu(void)
{
  int done,func;
  int retval;
  MenuRoot *actual_mr;

  FindEntry();
  while (TRUE)
    {
      /* block until there is an event */
      XMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|ExposureMask | 
		 KeyPressMask|VisibilityChangeMask|ButtonMotionMask, &Event);
      StashEventTime(&Event);
      done = 0;
      if (Event.type == MotionNotify) 
	{
	  /* discard any extra motion events before a release */
	  while((XCheckMaskEvent(dpy,ButtonMotionMask|ButtonReleaseMask,
				 &Event))&&(Event.type != ButtonRelease));
	}
      /* Handle a limited number of key press events to allow mouseless
       * operation */
      if(Event.type == KeyPress)
	menuShortcuts(&Event);

      switch(Event.type)
	{
	case ButtonRelease:
	  /* The following lines holds the menu when the button is released */
	  if ((Scr.flags & MWMMenus) && !ActiveItem && (menu_on > 1))
	    {
	      int x,y;
	      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY, &x, &y, &JunkMask);
	      if((XFindContext(dpy, JunkChild,MenuContext,
			      (caddr_t *)&actual_mr)!=XCNOENT)&&
		 (actual_mr != ActiveMenu))
		{
		  done = 1;
		  break;
		}
	    }
	  PopDownMenu();
	  if(ActiveItem)
	    {
	      func = ActiveItem->func;
	      done = 1;
	      if(ButtonWindow)
		{
		  ExecuteFunction(func, ActiveItem->action,
				  ButtonWindow->frame,
				  ButtonWindow, &Event, Context,
				  ActiveItem->val1,ActiveItem->val2,
				  ActiveItem->val1_unit,ActiveItem->val2_unit,
				  ActiveItem->menu,-1);
		}
	      else
		{
		  ExecuteFunction(func,ActiveItem->action,
				  None,None, &Event, 
				  Context,ActiveItem->val1,
				  ActiveItem->val2,
				  ActiveItem->val1_unit,ActiveItem->val2_unit,
				  ActiveItem->menu,-1);

		}
	    }
	  ActiveItem = NULL;
	  ActiveMenu = NULL;
	  menuFromFrameOrWindowOrTitlebar = FALSE;
	  return MENU_DONE;

	case KeyPress:
	case VisibilityNotify:
	case ButtonPress:
	  done=1;
	  break;

	case MotionNotify:
	  done = 1;

	  retval = FindEntry();
	  if((retval == MENU_DONE)||(retval == SUBMENU_DONE))
	    {
	      PopDownMenu();	      
	      ActiveItem = NULL;
	      ActiveMenu = NULL;
	      menuFromFrameOrWindowOrTitlebar = FALSE;
	    }

	  if(retval == MENU_DONE)
	    return MENU_DONE;
	  else if (retval == SUBMENU_DONE)
	    return MENU_NOP;

	  break;

	case Expose:
	  /* grab our expose events, let the rest go through */
	  if((XFindContext(dpy, Event.xany.window,MenuContext,
			   (caddr_t *)&actual_mr)!=XCNOENT))
	    {
	      PaintMenu(actual_mr,&Event);
	      done = 1;
	    }
	  break;

	default:
	  break;
	}
      
      if(!done)DispatchEvent();
      XFlush(dpy);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	PopUpMenu - pop up a pull down menu
 *
 *  Inputs:
 *	menu	- the root pointer of the menu to pop up
 *	x, y	- location of upper left of menu
 *      center	- whether or not to center horizontally over position
 *
 ***********************************************************************/
Bool PopUpMenu (MenuRoot *menu, int x, int y)
{

  if ((!menu)||(menu->w == None)||(menu->items == 0)||(menu->in_use))
    return False;

  menu_on++;
  InstallRootColormap();

  Stashed_X = x;
  Stashed_Y = y;
  
  /* pop up the menu */
  ActiveMenu = menu;
  ActiveItem = NULL;
  
  x -= (menu->width >> 1);
  y -= (Scr.EntryHeight >> 1);
  
  if((Tmp_win)&&(menu_on == 1)&&(Context&C_LALL))
    {
      y = Tmp_win->frame_y+Tmp_win->boundary_width+Tmp_win->title_height+1;
      x = Tmp_win->frame_x + Tmp_win->boundary_width + 
	Button*Tmp_win->title_height+1;
    }
  if((Tmp_win)&&(menu_on==1)&&(Context&C_RALL))
    {
      y = Tmp_win->frame_y+Tmp_win->boundary_width+Tmp_win->title_height+1;
      x = Tmp_win->frame_x +Tmp_win->frame_width - Tmp_win->boundary_width-
	Button*Tmp_win->title_height - menu->width+1;
    }
  if((Tmp_win)&&(menu_on==1)&&(Context&C_TITLE))
    {
      y = Tmp_win->frame_y+Tmp_win->boundary_width+Tmp_win->title_height+1;
      if(x < Tmp_win->frame_x + Tmp_win->title_x)
	x = Tmp_win->frame_x + Tmp_win->title_x;
      if((x + menu->width) >
	 (Tmp_win->frame_x + Tmp_win->title_x +Tmp_win->title_width))
	x = Tmp_win->frame_x + Tmp_win->title_x +Tmp_win->title_width-
	  menu->width +1;
    }

  /* clip to screen */
  if (x + menu->width > Scr.MyDisplayWidth-2) 
    x = Scr.MyDisplayWidth - menu->width-2;
  if (x < 0) x = 0;

  if (y + menu->height > Scr.MyDisplayHeight-2) 
    {
      y = Scr.MyDisplayHeight - menu->height-2;
      /* Warp pointer to middle of top line */
      /* Not with MWMMenus! */
      if((!(Scr.flags & MWMMenus))||(menu_on < 2))
	XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
		     Scr.MyDisplayHeight, 
		     x + (menu->width>>1), (y + (Scr.EntryHeight >> 1)));
    }
  if (y < 0) y = 0;

  MenuY = y;
  XMoveWindow(dpy, menu->w, x, y);
  XMapRaised(dpy, menu->w);
  menu->in_use = True;
  return True;
}


/***********************************************************************
 *
 *  Procedure:
 *	PopDownMenu - unhighlight the current menu selection and
 *		take down the menus
 *
 ***********************************************************************/
void PopDownMenu()
{
  if (ActiveMenu == NULL)
    return;
  
  menu_on--;
  if (ActiveItem)
    ActiveItem->state = 0;
  
  XUnmapWindow(dpy, ActiveMenu->w);

  UninstallRootColormap();
  XFlush(dpy);
  if (Context & (C_WINDOW | C_FRAME | C_TITLE | C_SIDEBAR))
    menuFromFrameOrWindowOrTitlebar = TRUE;
  else
    menuFromFrameOrWindowOrTitlebar = FALSE;
  ActiveMenu->in_use = FALSE;
}

/***************************************************************************
 * 
 * Wait for all mouse buttons to be released 
 * This can ease some confusion on the part of the user sometimes 
 * 
 * Discard superflous button events during this wait period.
 *
 ***************************************************************************/
void WaitForButtonsUp()
{
  Bool AllUp = False;
  XEvent JunkEvent;
  unsigned int mask;

  while(!AllUp)
    {
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &JunkX, &JunkY, &mask);    
      
      if((mask&
	  (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask))==0)
	AllUp = True;
    }
  XSync(dpy,0);
  while(XCheckMaskEvent(dpy,
			ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
			&JunkEvent))
    {
      StashEventTime (&JunkEvent);
      XAllowEvents(dpy,ReplayPointer,CurrentTime);
    }

}



