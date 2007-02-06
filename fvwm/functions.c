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
#include <unistd.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

extern XEvent Event;
extern FvwmWindow *Tmp_win;
extern int menuFromFrameOrWindowOrTitlebar;
extern DoHandlePageing;

extern char **g_argv;



/***********************************************************************
 *
 *  Procedure:
 *	ExecuteFunction - execute a fvwm built in function
 *
 *  Inputs:
 *	func	- the function to execute
 *	action	- the menu action to execute 
 *	w	- the window to execute this function on
 *	tmp_win	- the fvwm window structure
 *	event	- the event that caused the function
 *	context - the context in which the button was pressed
 *      val1,val2 - the distances to move in a scroll operation 
 *
 ***********************************************************************/
void ExecuteFunction(int func,char *action, Window in_w, FvwmWindow *tmp_win, 
		     XEvent *eventp, unsigned long context,long val1,long val2,
		     int val1_unit, int val2_unit, MenuRoot *menu, int Module)
{
  FvwmWindow *t,*temp;
  char *junk, *junkC;
  unsigned long junkN;
  int junkD;
  int x,y;
  Window w;
#ifndef NON_VIRTUAL
  int delta_x,delta_y;
#endif
  int warp_x=0,warp_y=0;
#ifndef NO_PAGER
  Pixel TextColor,BackColor;
  Pixmap BackPixmap;
#endif

  /* Defer Execution may wish to alter this value */
  w = in_w;

  switch (func)
    {
    case F_NOP:
    case F_TITLE:
      break;
      
    case F_BEEP:
      XBell(dpy, Scr.screen);
      break;
      
    case F_RESIZE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, MOVE, ButtonPress))
	break;

      if(tmp_win == NULL)
	break;
      if(check_allowed_function2(func,tmp_win) == 0)
	{
	  XBell(dpy, Scr.screen);
	  break;
	}
      tmp_win->flags &= ~MAXIMIZED;
      resize_window(w,tmp_win,val1,val2,val1_unit,val2_unit);
      break;
      
    case F_MOVE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, MOVE,ButtonPress))
	break;

      if(tmp_win == NULL)
	break;

      move_window(eventp,w,tmp_win,context,val1,val2,val1_unit,val2_unit);
      break;

#ifndef NON_VIRTUAL      
    case F_SCROLL:
      if((val1 > -100000)&&(val1 < 100000))
	x=Scr.Vx + val1*val1_unit/100;
      else
	x = Scr.Vx + (val1/1000)*val1_unit/100;
	  
      if((val2 > -100000)&&(val2 < 100000))
	y=Scr.Vy + val2*val2_unit/100;
      else
	y = Scr.Vy + (val2/1000)*val2_unit/100;
      
      if(((val1 <= -100000)||(val1 >= 100000))&&(x>Scr.VxMax))
	{
	  x = 0;
	  y += Scr.MyDisplayHeight;
	  if(y > Scr.VyMax)
	    y=0;
	}
      if(((val1 <= -100000)||(val1 >= 100000))&&(x<0))
	{
	  x = Scr.VxMax;
	  y -= Scr.MyDisplayHeight;
	  if(y < 0)
	    y=Scr.VyMax;
	}
      if(((val2 <= -100000)||(val2>= 100000))&&(y>Scr.VyMax))
	{
	  y = 0;
	  x += Scr.MyDisplayWidth;
	  if(x > Scr.VxMax)
	    x=0;
	}
      if(((val2 <= -100000)||(val2>= 100000))&&(y<0))
	{
	  y = Scr.VyMax;
	  x -= Scr.MyDisplayWidth;
	  if(x < 0)
	    x=Scr.VxMax;
	}
      MoveViewport(x,y,True);
      break;
#endif
    case F_MOVECURSOR:
      XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &x,&y,&JunkX, &JunkY, &JunkMask);
#ifndef NON_VIRTUAL
      delta_x = 0;
      delta_y = 0;
      warp_x = 0;
      warp_y = 0;
      if(x >= Scr.MyDisplayWidth -2)
	{
	  delta_x = Scr.EdgeScrollX;
	  warp_x = Scr.EdgeScrollX - 4;
	}
      if(y>= Scr.MyDisplayHeight -2)
	{
	  delta_y = Scr.EdgeScrollY;
	  warp_y = Scr.EdgeScrollY - 4;      
	}
      if(x < 2)
	{
	  delta_x = -Scr.EdgeScrollX;
	  warp_x =  -Scr.EdgeScrollX + 4;
	}
      if(y < 2)
	{
	  delta_y = -Scr.EdgeScrollY;
	  warp_y =  -Scr.EdgeScrollY + 4;
	}
      if(Scr.Vx + delta_x < 0)
	delta_x = -Scr.Vx;
      if(Scr.Vy + delta_y < 0)
	delta_y = -Scr.Vy;
      if(Scr.Vx + delta_x > Scr.VxMax)
	delta_x = Scr.VxMax - Scr.Vx;
      if(Scr.Vy + delta_y > Scr.VyMax)
	delta_y = Scr.VyMax - Scr.Vy;
      if((delta_x!=0)||(delta_y!=0))
	{
	  MoveViewport(Scr.Vx + delta_x,Scr.Vy+delta_y,True);
	  XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
		       Scr.MyDisplayHeight, 
		       x - warp_x,
		       y - warp_y);
	}
#endif
      XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
		   Scr.MyDisplayHeight, x + val1*val1_unit/100-warp_x,
		   y+val2*val2_unit/100 - warp_y);
      
      break;
    case F_ICONIFY:
      if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT, ButtonRelease))
	break;

      if(tmp_win == NULL)
	break;
      if (tmp_win->flags & ICONIFIED)
	{
	  if(val1 <=0)
	    DeIconify(tmp_win);
	}
      else
	{
	  if(check_allowed_function2(func,tmp_win) == 0)
	    {
	      XBell(dpy, Scr.screen);
	      break;
	    }
	  if(val1 >=0)
	    {
	      if(check_allowed_function2(func,tmp_win) == 0)
		{
		  XBell(dpy, Scr.screen);
		  break;
		}
	      Iconify(tmp_win,eventp->xbutton.x_root-5,eventp->xbutton.y_root-5);
	    }
	}
      break;

    case F_RAISE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
	break;
      
      if(tmp_win)
	RaiseWindow(tmp_win);

      if (LookInList(Scr.TheList,tmp_win->name, &tmp_win->class, &junk, &junkD,
		     &junkD, &junkD, &junkC, &junkC, &junkN)& STAYSONTOP_FLAG)
	tmp_win->flags |= ONTOP;
      KeepOnTop();
      break;
      
    case F_LOWER:
      if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT, ButtonRelease))
	break;
      
      if(tmp_win == NULL)
	break;
      LowerWindow(tmp_win);

      tmp_win->flags &= ~ONTOP;
      break;
      
    case F_DESTROY:
      if (DeferExecution(eventp,&w,&tmp_win,&context, DESTROY, ButtonRelease))
	break;

      if(tmp_win == NULL)
	break;
      if(check_allowed_function2(func,tmp_win) == 0)
	{
	  XBell(dpy, Scr.screen);
	  break;
	}
#ifndef NO_PAGER
      /* Dont delete the pager - it crashes the program! */
      if((tmp_win->w == Scr.Pager_w)||(tmp_win == Scr.FvwmPager))
	break;
#endif

      if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		       &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	Destroy(tmp_win);
      else
	XKillClient(dpy, tmp_win->w);
      XSync(dpy,0);
      break;
      
    case F_DELETE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, DESTROY,ButtonRelease))
	break;

      if(tmp_win == NULL)
	break;
      if(check_allowed_function2(func,tmp_win) == 0)
	{
	  XBell(dpy, Scr.screen);
	  break;
	}

#ifndef NO_PAGER
      /* Dont delete the pager - it crashes the program! */
      if((tmp_win->w == Scr.Pager_w)||(tmp_win == Scr.FvwmPager))
	break;
#endif
      if (tmp_win->flags & DoesWmDeleteWindow)
	{
	  send_clientmessage (tmp_win->w, _XA_WM_DELETE_WINDOW, CurrentTime);
	  break;
	}
      else
	XBell (dpy, Scr.screen);
      XSync(dpy,0);
      break;

    case F_CLOSE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, DESTROY,ButtonRelease))
	break;

      if(tmp_win == NULL)
	break;
      if(check_allowed_function2(func,tmp_win) == 0)
	{
	  XBell(dpy, Scr.screen);
	  break;
	}

#ifndef NO_PAGER
      /* Dont delete the pager - it crashes the program! */
      if((tmp_win->w == Scr.Pager_w)||(tmp_win == Scr.FvwmPager))
	break;
#endif
      if (tmp_win->flags & DoesWmDeleteWindow)
	{
	  send_clientmessage (tmp_win->w, _XA_WM_DELETE_WINDOW, CurrentTime);
	  break;
	}
      else if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
			    &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
	Destroy(tmp_win);
      else
	XKillClient(dpy, tmp_win->w);
      XSync(dpy,0);
      break;
      
    case F_RESTART:
      Done(1, action);
      break;

    case F_EXEC:
      XGrabPointer(dpy, Scr.Root, True,
		   ButtonPressMask | ButtonReleaseMask,
		   GrabModeAsync, GrabModeAsync,
		   Scr.Root, Scr.FvwmCursors[WAIT], CurrentTime);
      XSync (dpy,0);

      if (!(fork())) /* child process */
	if (execl("/bin/sh", "/bin/sh", "-c", action, (char *)0)==-1)
	  exit(100);
      XUngrabPointer(dpy,CurrentTime);
      XSync (dpy,0);
      break;
      
    case F_REFRESH:
      {
	XSetWindowAttributes attributes;
	unsigned long valuemask;
	
	valuemask = (CWBackPixel);
	attributes.background_pixel = Scr.StdColors.fore;
	attributes.backing_store = NotUseful;
	w = XCreateWindow (dpy, Scr.Root, 0, 0,
			   (unsigned int) Scr.MyDisplayWidth,
			   (unsigned int) Scr.MyDisplayHeight,
			   (unsigned int) 0,
			   CopyFromParent, (unsigned int) CopyFromParent,
			   (Visual *) CopyFromParent, valuemask,
			   &attributes);
	XMapWindow (dpy, w);
	XDestroyWindow (dpy, w);
	XFlush (dpy);
      }
      break;

    case F_STICK:
      /* stick/unstick a window */
      if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
	break;

      if(tmp_win == NULL)
	break;

      if(tmp_win->flags & STICKY)
	{
	  tmp_win->flags &= ~STICKY;
	  if(tmp_win->BackPixel == Scr.StickyColors.back)
	    {
	      tmp_win->ReliefPixel = Scr.StdRelief.fore;
	      tmp_win->ShadowPixel = Scr.StdRelief.back;
	      tmp_win->BackPixel = Scr.StdColors.back;
	    }
	  if(tmp_win->TextPixel == Scr.StickyColors.fore)
	    tmp_win->TextPixel = Scr.StdColors.fore;
	}
      else
	{
	  tmp_win->flags |=STICKY;
	  if(tmp_win->BackPixel == Scr.StdColors.back)
	    {
	      tmp_win->BackPixel = Scr.StickyColors.back;
	      tmp_win->ShadowPixel = Scr.StickyRelief.back;
	      tmp_win->ReliefPixel = Scr.StickyRelief.fore;	  
	    }
	  if(tmp_win->TextPixel == Scr.StdColors.fore)
	    tmp_win->TextPixel = Scr.StickyColors.fore;
	}
      if(Scr.Hilite != tmp_win)
	{
	  /* Need to make SetBorder change the window back color */
	  temp=Scr.Hilite;
	  SetBorder(tmp_win,True,True,True,None);
	  SetBorder(tmp_win,False,True,True,None);
	  SetBorder(temp,True,True,True,None);
	}
      BroadcastConfig(M_CONFIGURE_WINDOW,tmp_win);
#ifndef NO_PAGER
      MoveResizePagerView(tmp_win);

      /* Need to re-draw pager_view in case the window
       * is unsticking */
      if(Scr.Hilite == tmp_win)
	{
	  TextColor = Scr.HiColors.fore;
	  BackPixmap= Scr.gray_pixmap;
	  BackColor = Scr.HiColors.back;
	}
      else
	{
	  TextColor = Scr.StdColors.fore;
	  BackPixmap = Scr.light_gray_pixmap;
	  BackColor = Scr.StdColors.back;
	}
      if(Scr.d_depth < 2)
	XSetWindowBackgroundPixmap(dpy,tmp_win->pager_view,BackPixmap);
      else
	XSetWindowBackground(dpy,tmp_win->pager_view,BackColor);
      XClearWindow(dpy,tmp_win->pager_view);
      if((tmp_win->icon_name != NULL)&&(Scr.PagerFont.height > 0))
	{
	  NewFontAndColor(Scr.PagerFont.font->fid,TextColor,BackColor);
	  XDrawString (dpy, tmp_win->pager_view,Scr.FontGC,2,Scr.PagerFont.y+2,
		       tmp_win->icon_name, strlen(tmp_win->icon_name));
	}
#endif
      break;

#ifndef NON_VIRTUAL
    case F_GOTO_PAGE:
      /* back up 1 virtual desktop page */
      x=val1*Scr.MyDisplayWidth;
      y=val2*Scr.MyDisplayHeight;
      MoveViewport(x,y,True);
      break;
#endif
#ifndef NON_VIRTUAL      
    case F_TOGGLE_PAGE:
      if (DoHandlePageing) 
	{
	  DoHandlePageing = 0; 
	  Broadcast(M_TOGGLE_PAGING,1,0,0,0,0,0,0,0);
	}
      else 
	{
	  DoHandlePageing = 1;
	  Broadcast(M_TOGGLE_PAGING,1,1,0,0,0,0,0,0);
	}
      checkPanFrames();
      break;
#endif

    case F_CIRCULATE_UP:
      t=Circulate(tmp_win,action,UP);
      if(t)FocusOn(t,0);
      break;

    case F_CIRCULATE_DOWN:
      t=Circulate(tmp_win,action,DOWN);
      if(t)FocusOn(t,0);
      break;

    case F_WARP:
      t=Circulate(tmp_win,action,DOWN);
      if((t)&&(t->flags & ICONIFIED))
	{
	  FocusOn(t,0);
	  DeIconify(t);
	}
      if(t)FocusOn(t,0);
      break;
      
    case F_WAIT:
      {
	Bool done = False;
	if(val1 == 0)
	  val1 = 1;
	while(!done)
	  {
	    if(My_XNextEvent(dpy, &Event))
	      {
		DispatchEvent ();
		if(Event.type == MapNotify)
		  {
		    if((Tmp_win)&&(matchWildcards(action,Tmp_win->name)==True))
		      done = True;
		    if((Tmp_win)&&(Tmp_win->class.res_class)&&
		       (matchWildcards(action,Tmp_win->class.res_class)==True))
		      done = True;
		    if((Tmp_win)&&(Tmp_win->class.res_name)&&
		       (matchWildcards(action,Tmp_win->class.res_name)==True))
		      done = True;
		  }
	      }
	  }
      }
      XSync(dpy,0);
      break;
    case F_RAISE_IT:
      if(val1 != 0)
	{
 	  FocusOn((FvwmWindow *)val1,0);
	  if (((FvwmWindow *)(val1))->flags & ICONIFIED)
	    {
	      DeIconify((FvwmWindow *)val1);
	      FocusOn((FvwmWindow *)val1,0);
	    }
	}
      
      break;
    case F_FOCUS:
      if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
	break;
      FocusOn(tmp_win,0);
      break;

    case F_CHANGE_WINDOWS_DESK:
      if (DeferExecution(eventp,&w,&tmp_win,&context,SELECT,ButtonRelease))
	break;
      if(tmp_win == NULL)
	break;

      changeWindowsDesk(tmp_win,val1);
      break;
    case F_DESK:
      changeDesks(val1,val2);
      break;
    case F_MODULE:
      if(eventp->type != KeyPress)
	UngrabEm();
      if(tmp_win)
	executeModule(action, (FILE *)NULL,(char **)tmp_win->w,(int *)context);
      else
	executeModule(action, (FILE *)NULL,(char **)0, (int *)context);
      /* If we execute a module, don't wait for buttons to come up,
       * that way, a pop-up menu could be implemented */
      Module = 0;
      break;

    case F_RAISELOWER:
      if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
	break;
      if(tmp_win == NULL)
	break;

      if((tmp_win == Scr.LastWindowRaised)||
	 (tmp_win->flags & VISIBLE))
	{
	  LowerWindow(tmp_win);
	  tmp_win->flags &= ~ONTOP;
	}
      else
	{
	  RaiseWindow(tmp_win);
	  if (LookInList(Scr.TheList,tmp_win->name, &tmp_win->class,&junk,
			 &junkD,&junkD, &junkD, &junkC,&junkC,&junkN)&STAYSONTOP_FLAG)
	    tmp_win->flags |= ONTOP;	    
	  KeepOnTop();
	}
      break;

    case F_POPUP:
      ActiveItem = NULL;
      ActiveMenu = NULL;
      menuFromFrameOrWindowOrTitlebar = FALSE;
      do_menu(menu);
      break;

     case F_MAXIMIZE:
      if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonRelease))
	break;
      if(tmp_win == NULL)
	break;

      if(check_allowed_function2(func,tmp_win) == 0)
	{
	  XBell(dpy, Scr.screen);
	  break;
	}
      Maximize(tmp_win,val1,val2,val1_unit,val2_unit);
      break; 

    case F_QUIT:
      Done(0, NULL);
      break;

    case F_WINDOWLIST:
      do_windowList(val1,val2);
      break;

    case F_FUNCTION:
      ComplexFunction(w, tmp_win, eventp, context,menu);
      break;

    case F_QUICKIE:
      QuickRestart();
      break;
    case F_SEND_WINDOW_LIST:
      if(Module >= 0)
	{
	  SendPacket(Module,M_TOGGLE_PAGING,1,DoHandlePageing,0,0,0,0,0,0);
	  SendPacket(Module,M_NEW_DESK,1,Scr.CurrentDesk,0,0,0,0,0,0);
	  SendPacket(Module,M_NEW_PAGE,3,Scr.Vx,Scr.Vy,Scr.CurrentDesk,
		     0,0,0,0);
	  if(Scr.Hilite != NULL)
	    SendPacket(Module,M_FOCUS_CHANGE,3,Scr.Hilite->w,Scr.Hilite->frame,
		       (unsigned long)Scr.Hilite,0,0,0,0);
	  else
	    SendPacket(Module,M_FOCUS_CHANGE,3,0,0,0,0,0,0,0);
	  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	    {
	      SendConfig(Module,M_CONFIGURE_WINDOW,t);
	      SendName(Module,M_WINDOW_NAME,t->w,t->frame,
		       (unsigned long)t,t->name);
	      SendName(Module,M_ICON_NAME,t->w,t->frame,
		       (unsigned long)t,t->icon_name);
	      SendName(Module,M_RES_CLASS,t->w,t->frame,
		       (unsigned long)t,t->class.res_class);
	      SendName(Module,M_RES_NAME,t->w,t->frame,
		       (unsigned long)t,t->class.res_name);

	      if((t->flags & ICONIFIED)&&(!(t->flags & ICON_UNMAPPED)))
		SendPacket(Module,M_ICONIFY,7,t->w,t->frame,
			   (unsigned long)t,
			   t->icon_x_loc,t->icon_y_loc,
			   t->icon_w_width, 
			   t->icon_w_height+t->icon_p_height);
	      if((t->flags & ICONIFIED) && (t->flags & ICON_UNMAPPED))
		SendPacket(Module,M_ICONIFY,7,t->w,t->frame,
			  (unsigned long)t,0,0,0,0);
	    }
	  if(Scr.Hilite == NULL)
	    {
	      Broadcast(M_FOCUS_CHANGE,3,0,0,0,0,0,0,0);
	    }
	  else 
	    {
	      Broadcast(M_FOCUS_CHANGE,3,Scr.Hilite->w,
			Scr.Hilite->frame,(unsigned long)Scr.Hilite,0,0,0,0);
	    }
	  SendPacket(Module,M_END_WINDOWLIST,0,0,0,0,0,0,0,0);
	}
    }

  /* Only wait for an all-buttons-up condition after calls from
   * regular built-ins, not from complex-functions or modules. */
  if(Module == -1)
    WaitForButtonsUp();

  return;
}

FvwmWindow *Circulate(FvwmWindow *tmp_win, char *action,Bool Direction)
{
  FvwmWindow *t,*selected;
  Bool found;
  int count,pass=1;
  int base, best;

  while(pass< 3)
    {
      if(tmp_win)
	base = tmp_win->focus_sequence;
      else
	base = -1;
      if(Direction == DOWN)
	best = -1;
      else 
	best = 10000;
      selected = tmp_win;
      
      /* move focus to the next window */
      found = FALSE;
      t = tmp_win;
      count = 0;
      while(count < 3)
	{
	  if(Direction == DOWN)
	    {
	      if((t == (FvwmWindow *)0)||(t->next == NULL))
		{
		  t = Scr.FvwmRoot.next;
		  count++;
		}
	      else
		t =t->next;
	    }
	  else /* Direction Up */
	    {
	      if ((t == (FvwmWindow *)0)||(t == &Scr.FvwmRoot)||
		  (t->prev == &Scr.FvwmRoot)||(t->prev == (FvwmWindow *)NULL))
		{
		  for(t=Scr.FvwmRoot.next;t->next != (FvwmWindow *)NULL;t=t->next);
		  count++;
		}
	      else
		t=t->prev;
	    }
	  found = TRUE;
	  
	  if(t->Desk != Scr.CurrentDesk)
	    found = False;
	  
	  if((!(Scr.flags & Lenience))&&
	     (t)&&(t->wmhints)&&(t->wmhints->flags&InputHint)&&
	     (t->wmhints->input == False)&&
	     !(t->flags & DoesWmTakeFocus))
	    found = False;
	  
	  if (t->flags & CIRCULATESKIP)
	    found = FALSE;
	  /* optional skip over icons */
	  
	  if((t->flags & ICONIFIED)&&(Scr.flags & CirculateSkipIcons))
	    found = FALSE;
	  
	  
	  /* Make CirculateUp and CirculateDown take args. by Y.NOMURA */
	  if (action && (strlen(action) > 0)&&
	      !(matchWildcards(action, t->name)) &&
	      !(matchWildcards(action, t->icon_name))&&
	      t->class.res_name &&
	      !(matchWildcards(action, t->class.res_name)))
	    found = FALSE;
	  if((found)&&(Direction == DOWN)&&(t->focus_sequence > best))
	    {
	      best = t->focus_sequence;
	      selected = t;
	    }
	  if((found)&&(Direction != DOWN)&&(t->focus_sequence < best)
	     &&(t->focus_sequence > base))
	    {
	      best = t->focus_sequence;
	      selected = t;
	    }
	}
      if((selected)&&(selected == tmp_win)&&(base > 0))
	{
	  if(Direction == DOWN)
	    {
	      ClearCirculatedFlag();
	      tmp_win->focus_sequence = 0;
	    }
	  else
	    {
	      FvwmWindow *temp;
	      
	      temp = Scr.FvwmRoot.next;	  
	      while(temp != NULL)
		{
		  temp->focus_sequence++;
		  if(temp == tmp_win)temp->focus_sequence = 0;
		  temp = temp->next;
		}
	    }	
	  pass++;
	}
      else
	pass = 3;
    }
      
  return selected;
}
	  

/***********************************************************************
 *
 *  Procedure:
 *	DeferExecution - defer the execution of a function to the
 *	    next button press if the context is C_ROOT
 *
 *  Inputs:
 *      eventp  - pointer to XEvent to patch up
 *      w       - pointer to Window to patch up
 *      tmp_win - pointer to FvwmWindow Structure to patch up
 *	context	- the context in which the mouse button was pressed
 *	func	- the function to defer
 *	cursor	- the cursor to display while waiting
 *      finishEvent - ButtonRelease or ButtonPress; tells what kind of event to
 *                    terminate on.
 *
 ***********************************************************************/
int DeferExecution(XEvent *eventp, Window *w,FvwmWindow **tmp_win,
		   unsigned long *context, int cursor, int FinishEvent)

{
  int done;
  int finished = 0;
  Window dummy;
  Window original_w;

  original_w = *w;

  if((*context != C_ROOT)&&(*context != C_NO_CONTEXT))
    {
      if((FinishEvent == ButtonPress)||((FinishEvent == ButtonRelease) &&
					(eventp->type != ButtonPress)))
	{
	  return FALSE;
	}
    }
  if(!GrabEm(cursor))
    {
      XBell(dpy,Scr.screen);
      return True;
    }
  
  while (!finished)
    {
      done = 0;
      /* block until there is an event */
      XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
		 ExposureMask |KeyPressMask | VisibilityChangeMask |
		 ButtonMotionMask| PointerMotionMask/* | EnterWindowMask | 
		 LeaveWindowMask*/, eventp);
      StashEventTime(eventp);

      if(eventp->type == KeyPress)
	Keyboard_shortcuts(eventp,FinishEvent);	
      if(eventp->type == FinishEvent)
	finished = 1;
      if(eventp->type == ButtonPress)
	{
	  XAllowEvents(dpy,ReplayPointer,CurrentTime);
	  done = 1;
	}
      if(eventp->type == ButtonRelease)
	done = 1;
      if(eventp->type == KeyPress)
	done = 1;
      
      if(!done)
	{
	  DispatchEvent();
	}

    }

  
  *w = eventp->xany.window;
  if(((*w == Scr.Root)||(*w == Scr.NoFocusWin))
      && (eventp->xbutton.subwindow != (Window)0))
    {
      *w = eventp->xbutton.subwindow;
      eventp->xany.window = *w;
    }
  if (*w == Scr.Root)
    {
      *context = C_ROOT;
      XBell(dpy,Scr.screen);
      UngrabEm();
      return TRUE;
    }
  if (XFindContext (dpy, *w, FvwmContext, (caddr_t *)tmp_win) == XCNOENT)
    {
      *tmp_win = NULL;
      XBell(dpy,Scr.screen);
      UngrabEm();
      return (TRUE);
    }

  if(*w == (*tmp_win)->Parent)
    *w = (*tmp_win)->w;

  if(original_w == (*tmp_win)->Parent)
    original_w = (*tmp_win)->w;
  
  /* this ugly mess attempts to ensure that the release and press
   * are in the same window. */
  if((*w != original_w)&&(original_w != Scr.Root)&&
     (original_w != None)&&(original_w != Scr.NoFocusWin))
    if(!((*w == (*tmp_win)->frame)&&
       (original_w == (*tmp_win)->w)))
      {
	*context = C_ROOT;
	XBell(dpy,Scr.screen);
	UngrabEm();
	return TRUE;
      }
  
  *context = GetContext(*tmp_win,eventp,&dummy);
  
  UngrabEm();
  return FALSE;
}


/****************************************************************************
 *
 * This is used to tell applications which windows on the screen are
 * top level appication windows, and which windows are the icon windows
 * that go with them.
 *
 ****************************************************************************/
void SetMapStateProp(FvwmWindow *tmp_win, int state)
{
  unsigned long data[2];		/* "suggested" by ICCCM version 1 */
  
  data[0] = (unsigned long) state;
  data[1] = (unsigned long) tmp_win->icon_w;
/*  data[2] = (unsigned long) tmp_win->icon_pixmap_w;*/
  
  XChangeProperty (dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32, 
		   PropModeReplace, (unsigned char *) data, 2);
  return;
}

/****************************************************************************
 *
 * Keeps the "StaysOnTop" windows on the top of the pile.
 * This is achieved by clearing a flag for OnTop windows here, and waiting
 * for a visibility notify on the windows. Eception: OnTop windows which are
 * obscured by other OnTop windows, which need to be raised here.
 *
 ****************************************************************************/
void KeepOnTop()
{
  FvwmWindow *t;

  /* flag that on-top windows should be re-raised */
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if((t->flags & ONTOP)&&!(t->flags & VISIBLE))
	{
	  RaiseWindow(t);
	  t->flags &= ~RAISED;
	}
      else
	t->flags |= RAISED;
    }
}


/***************************************************************************
 *
 *  Moves the viewport within thwe virtual desktop
 *
 ***************************************************************************/
#ifndef NON_VIRTUAL
void MoveViewport(int newx, int newy, Bool grab)
{
  FvwmWindow *t;
  int deltax,deltay;

  if(grab)
    XGrabServer(dpy);


  if(newx > Scr.VxMax)
    newx = Scr.VxMax;
  if(newy > Scr.VyMax)
    newy = Scr.VyMax;
  if(newx <0)
    newx = 0;
  if(newy <0)
    newy = 0;

  deltay = Scr.Vy - newy;
  deltax = Scr.Vx - newx;

  Scr.Vx = newx;
  Scr.Vy = newy;
  Broadcast(M_NEW_PAGE,3,Scr.Vx,Scr.Vy,Scr.CurrentDesk,0,0,0,0);

  if((deltax!=0)||(deltay!=0))
    {
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  /* If the window is iconified, and sticky Icons is set,
	   * then the window should essentially be sticky */
	  if(!((t->flags & ICONIFIED)&&(Scr.flags & StickyIcons)) &&
	     (!(t->flags & STICKY)))
	    {
              if(!(Scr.flags & StickyIcons))
		{
		  t->icon_x_loc += deltax;
		  t->icon_xl_loc += deltax;
		  t->icon_y_loc += deltay;
		  if(t->icon_pixmap_w != None)
		    XMoveWindow(dpy,t->icon_pixmap_w,t->icon_x_loc,
				t->icon_y_loc);
                  if(t->icon_w != None)
                    XMoveWindow(dpy,t->icon_w,t->icon_x_loc,
                                t->icon_y_loc+t->icon_p_height);
		  if(!(t->flags &ICON_UNMAPPED))
		  Broadcast(M_ICON_LOCATION,7,t->w,t->frame,
			    (unsigned long)t,
			    t->icon_x_loc,t->icon_y_loc,
			    t->icon_w_width, 
			    t->icon_w_height+t->icon_p_height);
		}
	    }
	  if(!(t->flags & STICKY))
	    SetupFrame (t, t->frame_x+ deltax, t->frame_y + deltay,
			t->frame_width, t->frame_height,FALSE);
	}
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  /* If its an icon, and its sticking, autoplace it so
	   * that it doesn't wind up on top a a stationary
	   * icon */
	  if(((t->flags & STICKY)||(Scr.flags & StickyIcons))&&
	     (t->flags & ICONIFIED)&&(!(t->flags & ICON_MOVED))&&
	     (!(t->flags & ICON_UNMAPPED)))
	    AutoPlace(t);
	}

    }
  /* fix up the viewport indicator */
  MoveResizeViewPortIndicator();
#ifndef NON_VIRTUAL
  checkPanFrames();

  /* do this with PanFrames too ??? HEDU */
  while(XCheckTypedEvent(dpy,MotionNotify,&Event))
    StashEventTime(&Event);    
#endif
  if(grab)
    XUngrabServer(dpy);
}
#endif


/**************************************************************************
 *
 * Moves focus to specified window 
 *
 *************************************************************************/
void FocusOn(FvwmWindow *t,int DeIconifyOnly)
{
#ifndef NON_VIRTUAL
  int dx,dy;
  int cx,cy;
#endif
  int x,y;

  if(t == (FvwmWindow *)0)
    return;

  if(t->Desk != Scr.CurrentDesk)
    {
      changeDesks(0,t->Desk);
    }

#ifndef NON_VIRTUAL
  if(t->flags & ICONIFIED)
    {
      cx = t->icon_xl_loc + t->icon_w_width/2;
      cy = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
    }
  else
    {
      cx = t->frame_x + t->frame_width/2;
      cy = t->frame_y + t->frame_height/2;
    }

  /* Put center of window on the visible screen */
  if((!DeIconifyOnly)&&(Scr.flags & CenterOnCirculate))
    {
      dx = cx - Scr.MyDisplayWidth/2 + Scr.Vx;
      dy = cy - Scr.MyDisplayHeight/2 + Scr.Vy;
    }
  else
    {
      dx = (cx + Scr.Vx)/Scr.MyDisplayWidth*Scr.MyDisplayWidth;
      dy = (cy +Scr.Vy)/Scr.MyDisplayHeight*Scr.MyDisplayHeight;
    }
  MoveViewport(dx,dy,True);
#endif

  if(t->flags & ICONIFIED)
    {
      x = t->icon_xl_loc + t->icon_w_width/2;
      y = t->icon_y_loc + t->icon_p_height + ICON_HEIGHT/2;
    }
  else
    {
      x = t->frame_x;
      y = t->frame_y;
    }
  if(!(Scr.flags & ClickToFocus))
    XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, x+2,y+2);
  RaiseWindow(t);
  KeepOnTop();

  /* If the window is still not visible, make it visible! */
  if(((t->frame_x + t->frame_height)< 0)||(t->frame_y + t->frame_width < 0)||
     (t->frame_x >Scr.MyDisplayWidth)||(t->frame_y>Scr.MyDisplayHeight))
    {
      SetupFrame(t,0,0,t->frame_width, t->frame_height,False);
      if(!(Scr.flags & ClickToFocus))
	XWarpPointer(dpy, None, Scr.Root, 0, 0, 0, 0, 2,2);
    }
  UngrabEm();
  SetFocus(t->w,t);
}


   
/***********************************************************************
 *
 *  Procedure:
 *	(Un)Maximize a window.
 *
 ***********************************************************************/
void Maximize(FvwmWindow *tmp_win,int val1,int val2, 
	      int val1_unit,int val2_unit)
{
  int new_width, new_height,new_x,new_y;

  if (tmp_win->flags & MAXIMIZED)
    {
      tmp_win->flags &= ~MAXIMIZED;
      SetupFrame(tmp_win, tmp_win->orig_x, tmp_win->orig_y, tmp_win->orig_wd,
		 tmp_win->orig_ht,TRUE);
      SetBorder(tmp_win,True,True,True,None);
    }
  else
    {
      new_width = tmp_win->frame_width;      
      new_height = tmp_win->frame_height;
      new_x = tmp_win->frame_x;
      new_y = tmp_win->frame_y;
      if(val1 >0)
	{
	  new_width = val1*val1_unit/100-2;
	  new_x = 0;
	}
      if(val2 >0)
	{
	  new_height = val2*val2_unit/100-2;
	  new_y = 0;
	}
      if((val1==0)&&(val2==0))
	{
	  new_x = 0;
	  new_y = 0;
	  new_height = Scr.MyDisplayHeight-2;
	  new_width = Scr.MyDisplayWidth-2;
	}
      tmp_win->flags |= MAXIMIZED;
      ConstrainSize (tmp_win, &new_width, &new_height);
      SetupFrame(tmp_win,new_x,new_y,new_width,new_height,TRUE);
      SetBorder(tmp_win,Scr.Hilite == tmp_win,True,True,tmp_win->right_w[0]);
    }
#ifndef NO_PAGER  
  RedrawPager();
#endif
}

/*****************************************************************************
 *
 * Grab the pointer and keyboard
 *
 ****************************************************************************/
Bool GrabEm(int cursor)
{
  int i=0,val=0;
  unsigned int mask;

  XSync(dpy,0);
  /* move the keyboard focus prior to grabbing the pointer to
   * eliminate the enterNotify and exitNotify events that go
   * to the windows */
  if(Scr.PreviousFocus == NULL)
    Scr.PreviousFocus = Scr.Focus;
  SetFocus(Scr.NoFocusWin,NULL);
  mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|PointerMotionMask
    | EnterWindowMask | LeaveWindowMask;
  while((i<1000)&&(val=XGrabPointer(dpy, Scr.Root, True, mask,
				    GrabModeAsync, GrabModeAsync, Scr.Root,
				    Scr.FvwmCursors[cursor], CurrentTime)!=
		   GrabSuccess))
    {
      i++;
      /* If you go too fast, other windows may not get a change to release
       * any grab that they have. */
      sleep_a_little(1000);
    }

  /* If we fall out of the loop without grabbing the pointer, its
     time to give up */
  XSync(dpy,0);
  if(val!=GrabSuccess)
    {
      return False;
    }
  return True;
}


/*****************************************************************************
 *
 * UnGrab the pointer and keyboard
 *
 ****************************************************************************/
void UngrabEm()
{
  Window w;

  XSync(dpy,0);
  XUngrabPointer(dpy,CurrentTime);

  if(Scr.PreviousFocus != NULL)
    {
      w = Scr.PreviousFocus->w;

      /* if the window still exists, focus on it */
      if (w)
	{
	  SetFocus(w,Scr.PreviousFocus);
	}
      Scr.PreviousFocus = NULL;
    }
  XSync(dpy,0);
}


/*****************************************************************************
 *
 * Waits Scr.ClickTime, or until it is evident that the user is not
 * clicking, but is moving the cursor
 *
 ****************************************************************************/
Bool IsClick(int x,int y,unsigned EndMask, XEvent *d)
{
  int xcurrent,ycurrent,total = 0;

  xcurrent = x;
  ycurrent = y;
  while((total < Scr.ClickTime)&&
	(x - xcurrent < 5)&&(x - xcurrent > -5)&&
	(y - ycurrent < 5)&&(y - ycurrent > -5))
    {
      sleep_a_little(10000);
      total+=10;
      if(XCheckMaskEvent (dpy,EndMask, d))
	{
	  StashEventTime(d);
	  return True;
	}
      if(XCheckMaskEvent (dpy,ButtonMotionMask|PointerMotionMask, d))
	{
	  xcurrent = d->xmotion.x_root;
	  ycurrent = d->xmotion.y_root;
	  StashEventTime(d);
	}
    }
  return False;
}

/*****************************************************************************
 *
 * Builtin which determines if the button press was a click or double click...
 *
 ****************************************************************************/
void ComplexFunction(Window w, FvwmWindow *tmp_win, XEvent *eventp,
		     unsigned long context, MenuRoot *mr)
{
  char type = MOTION;
  char c;
  XEvent *ev;
  MenuItem *mi;
  XEvent d;
  Bool Persist = False;
  Bool HaveDoubleClick = False;
  Bool NeedsTarget = False;
  int x, y ;

  if(mr == NULL)
    return;

  /* These built-ins require a selected window 
   * The function code is >= 100 and < 1000
   * F_RESIZE
   * F_MOVE
   * F_ICONIFY
   * F_RAISE
   * F_LOWER
   * F_DESTROY
   * F_DELETE
   * F_STICK
   * F_RAISELOWER
   * F_MAXIMIZE
   * F_FOCUS
   *
   * These do not:
   * The function code is < 100
   * F_NOP
   * F_TITLE
   * F_BEEP
   * F_SCROLL
   * F_MOVECURSOR
   * F_RESTART
   * F_EXEC
   * F_REFRESH
   * F_GOTO_PAGE
   * F_TOGGLE_PAGE
   * F_CIRCULATE_UP
   * F_CIRCULATE_DOWN
   * F_WARP
   * F_DESK
   * F_MODULE
   * F_POPUP
   * F_QUIT
   * F_WINDOWLIST
   * F_FUNCTION
   * F_SEND_WINDOW_LIST
   */

  mi = mr->first;
  while(mi != NULL)
    {
      /* make lower case */
      c = *(mi->item);
      if((mi->func >= 100)&&(mi->func < 1000))
	NeedsTarget = True;
      if(isupper(c))
	c=tolower(c);
      if(c==DOUBLE_CLICK)
	HaveDoubleClick = True;
      else if(c == IMMEDIATE)
	{
	  if(tmp_win)
	    w = tmp_win->frame;
	  else
	    w = None;
	  ExecuteFunction(mi->func, mi->action,w,
			  tmp_win, eventp, context,mi->val1,mi->val2,
			  mi->val1_unit,mi->val2_unit,
			  mi->menu,-1);
	}
      else
	Persist = True;
      mi = mi->next;
    }

  if(!Persist)
    return;

  /* Only defer execution if there is a possibility of needing
   * a window to operate on */
  if(NeedsTarget)
    {
      if (DeferExecution(eventp,&w,&tmp_win,&context, SELECT,ButtonPress))
	{
	  WaitForButtonsUp();
	  return;
	}
    }

  if(!GrabEm(SELECT))
    {
      XBell(dpy,Scr.screen);
      return;
    }
  XQueryPointer( dpy, Scr.Root, &JunkRoot, &JunkChild,
		&x,&y,&JunkX, &JunkY, &JunkMask);

  /* A window has already been selected */
  ev = eventp;

  /* Wait and see if we have a click, or a move */
  /* wait 100 msec, see if the used releases the button */
  if(IsClick(x,y,ButtonReleaseMask,&d))
    {
      type = CLICK;
      ev = &d;
    }

  /* If it was a click, wait to see if its a double click */
  if((HaveDoubleClick) && (type == CLICK) &&
     (IsClick(x,y,ButtonPressMask, &d)))
    {
      type = ONE_AND_A_HALF_CLICKS;
      ev = &d;
    }
  if((HaveDoubleClick) && (type == ONE_AND_A_HALF_CLICKS) &&
     (IsClick(x,y,ButtonReleaseMask, &d)))
    {
      type = DOUBLE_CLICK;
      ev = &d;
    }
  /* some functions operate on button release instead of 
   * presses. These gets really weird for complex functions ... */
  if(eventp->type == ButtonPress)
    eventp->type = ButtonRelease;

  mi = mr->first;
  while(mi != NULL)
    {
      /* make lower case */
      c = *(mi->item);
      if(isupper(c))
	c=tolower(c);
      if(c == type)
	{
	  if(tmp_win)
	    w = tmp_win->frame;
	  else
	    w = None;
	  ExecuteFunction(mi->func, mi->action,w,
			  tmp_win, eventp, context,
			  mi->val1,mi->val2,
			  mi->val1_unit,mi->val2_unit,mi->menu,-2);
	}
      mi = mi->next;
    }
  WaitForButtonsUp();
  UngrabEm();
}


/* For Ultrix 4.2 */
#include <sys/types.h>
#include <sys/time.h>


/**************************************************************************
 * 
 * Move to a new desktop
 *
 *************************************************************************/
void changeDesks(int val1,int val2)
{
  int oldDesk;
  FvwmWindow *t;
  FvwmWindow *FocusWin = 0;
  static FvwmWindow *StickyWin = 0;

  oldDesk = Scr.CurrentDesk;

  if(val1 != 0)
    {
      Scr.CurrentDesk = Scr.CurrentDesk + val1;
    }
  else
    {
      Scr.CurrentDesk = val2;
      if(Scr.CurrentDesk == oldDesk)
	return;
    }

  Broadcast(M_NEW_DESK,1,Scr.CurrentDesk,0,0,0,0,0,0);
  /* Scan the window list, mapping windows on the new Desk,
   * unmapping windows on the old Desk */
  XGrabServer(dpy);
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      /* Only change mapping for non-sticky windows */
      if(!((t->flags & ICONIFIED)&&(Scr.flags & StickyIcons)) &&
	 (!(t->flags & STICKY))&&(!(t->flags & ICON_UNMAPPED)))
	{
	  if(t->Desk == oldDesk) {
	    if (Scr.Focus == t)
		t->FocusDesk = oldDesk;
	    else
		t->FocusDesk = -1;
	    UnmapIt(t);
	  } else if(t->Desk == Scr.CurrentDesk) {
	    MapIt(t);
	    if (t->FocusDesk == Scr.CurrentDesk) {
		FocusWin = t;
	    }
      }
	}
      else
	{
	  /* Window is sticky */
	  t->Desk = Scr.CurrentDesk;
	  if (Scr.Focus == t) {
	    t->FocusDesk = oldDesk;
	    StickyWin = t;
	  }
	}
    }
  XUngrabServer(dpy);
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      /* If its an icon, and its sticking, autoplace it so
       * that it doesn't wind up on top a a stationary
       * icon */
      if(((t->flags & STICKY)||(Scr.flags & StickyIcons))&&
	 (t->flags & ICONIFIED)&&(!(t->flags & ICON_MOVED))&&
	 (!(t->flags & ICON_UNMAPPED)))
	AutoPlace(t);
    }
  /* Better re-draw the pager now */
  RedrawPager();
  
  if(Scr.flags & ClickToFocus) {
#ifndef NO_REMEMBER_FOCUS
    if (FocusWin)
	SetFocus(FocusWin->w, FocusWin);
    else if (StickyWin && (StickyWin->flags && STICKY))
	SetFocus(StickyWin->w, StickyWin);
    else
#endif
	SetFocus(Scr.NoFocusWin,NULL);
  }
}



/**************************************************************************
 * 
 * Move to a new desktop
 *
 *************************************************************************/
void changeWindowsDesk(FvwmWindow *t,int val1)
{
  if(val1 == t->Desk)
    return;

  /* Scan the window list, mapping windows on the new Desk,
   * unmapping windows on the old Desk */
  /* Only change mapping for non-sticky windows */
  if(!((t->flags & ICONIFIED)&&(Scr.flags & StickyIcons)) &&
     (!(t->flags & STICKY))&&(!(t->flags & ICON_UNMAPPED)))
    {
      if(t->Desk == Scr.CurrentDesk)
	{
	  t->Desk = val1;
	  UnmapIt(t);
	}
      else if(val1 == Scr.CurrentDesk)
	{
	  t->Desk = val1;
	  /* If its an icon, auto-place it */
	  if(t->flags & ICONIFIED)
	    AutoPlace(t);
	  MapIt(t);
	}
      else
	t->Desk = val1;
      
    }
  /* Better re-draw the pager now */
  BroadcastConfig(M_CONFIGURE_WINDOW,t);
  RedrawPager();
}


/**************************************************************************
 * 
 * Unmaps a window on transition to a new desktop
 *
 *************************************************************************/
void UnmapIt(FvwmWindow *t)
{
  XWindowAttributes winattrs;
  unsigned long eventMask;
  /*
   * Prevent the receipt of an UnmapNotify, since that would
   * cause a transition to the Withdrawn state.
   */
  XGetWindowAttributes(dpy, t->w, &winattrs);
  eventMask = winattrs.your_event_mask;
  XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
  if(t->flags & ICONIFIED)
    {
      if(t->icon_pixmap_w != None)
	XUnmapWindow(dpy,t->icon_pixmap_w);
      if(t->icon_w != None)
	XUnmapWindow(dpy,t->icon_w);
    }
  else if(t->flags & (MAPPED|MAP_PENDING))
    {
      XUnmapWindow(dpy,t->frame);
    }
  XSelectInput(dpy, t->w, eventMask);
  MoveResizePagerView(t);
}

/**************************************************************************
 * 
 * Maps a window on transition to a new desktop
 *
 *************************************************************************/
void MapIt(FvwmWindow *t)
{
  if(t->flags & ICONIFIED)
    {
      if(t->icon_pixmap_w != None)
	XMapWindow(dpy,t->icon_pixmap_w);
      if(t->icon_w != None)
	XMapWindow(dpy,t->icon_w);
    }
  else if(t->flags & MAPPED)
    {
      XMapWindow(dpy,t->frame);
      t->flags |= MAP_PENDING;
      XMapWindow(dpy, t->Parent);
   }
  MoveResizePagerView(t);  
}



void QuickRestart(void)
{
  extern char *m4_options;
  extern char *display_name;
  int i;
  FvwmWindow *tmp,*next;  
  extern unsigned PopupCount;
  extern int have_the_colors;
  MenuRoot *PopupTable[MAXPOPUPS];
  extern MenuRoot *ActiveMenu;
  extern MenuItem *ActiveItem;
  extern int menu_on;
  XEvent event;

  PopDownMenu();
  PopDownMenu();
  UngrabEm();
  InstallWindowColormaps (&Scr.FvwmRoot);	/* force reinstall */

  Reborder();
  tmp = Scr.FvwmRoot.next;
  while (tmp != NULL)
    {
      next = tmp->next;
      Destroy(tmp);
      tmp = next;
    }
  XSync(dpy,0);
  for(i=0;i<PopupCount;i++)
    {
/*      free(PopupTable[i]);*/
      PopupTable[i] = NULL;
    }

  PopupCount = 0;
  ActiveMenu = NULL;
  ActiveItem = NULL;
  menu_on = 0;
  have_the_colors = 0;

  InitVariables();

#ifdef M4
  MakeMenus(display_name, m4_options);
#else
  MakeMenus(display_name, NULL);
#endif  
#ifndef NON_VIRTUAL
  XUnmapWindow(dpy,Scr.PanFrameLeft.win);
  XUnmapWindow (dpy,Scr.PanFrameRight.win);
  XUnmapWindow (dpy,Scr.PanFrameBottom.win);
  XUnmapWindow(dpy,Scr.PanFrameTop.win);
  Scr.PanFrameBottom.isMapped=
    Scr.PanFrameTop.isMapped=
      Scr.PanFrameLeft.isMapped=
	Scr.PanFrameRight.isMapped=
	  False;
#endif
  while(XPending(dpy))
    XNextEvent(dpy,&event);

  CaptureAllWindows();
#ifndef NON_VIRTUAL
  checkPanFrames();
#endif
  HandleEvents();

}
