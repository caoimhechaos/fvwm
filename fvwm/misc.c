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


/**************************************************************************
 *
 * Assorted odds and ends
 *
 **************************************************************************/


#include <config.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "fvwm.h"
#include <X11/Xatom.h>
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

XGCValues Globalgcv;
unsigned long Globalgcm;


/**************************************************************************
 * 
 * Releases dynamically allocated space used to store window/icon names
 *
 **************************************************************************/
void free_window_names (FvwmWindow *tmp, Bool nukename, Bool nukeicon)
{
  if (!tmp)
    return;
  
  if (nukename && nukeicon) 
    {
      if (tmp->name == tmp->icon_name) 
	{
	  if (tmp->name != NoName)
	    XFree (tmp->name);
	  tmp->name = NULL;
	  tmp->icon_name = NULL;
	}
      else
	{
	  if (tmp->name != NoName)
	    XFree (tmp->name);
	  tmp->name = NULL;
	  if (tmp->icon_name != NoName)
	    XFree (tmp->icon_name);
	  tmp->icon_name = NULL;
	}
    }   
  else if (nukename) 
    {
      if (tmp->name != tmp->icon_name && tmp->name != NoName)
	XFree (tmp->name);
      tmp->name = NULL;
    }
  else
    { /* if (nukeicon) */
      if (tmp->icon_name != tmp->name && tmp->icon_name != NoName)
	XFree (tmp->icon_name);
      tmp->icon_name = NULL;
    }
  
  return;
}

/***************************************************************************
 *
 * Handles destruction of a window 
 *
 ****************************************************************************/
void Destroy(FvwmWindow *Tmp_win)
{ 
  int i;
  extern FvwmWindow *ButtonWindow;
  extern FvwmWindow *colormap_win;
  /*
   * Warning, this is also called by HandleUnmapNotify; if it ever needs to
   * look at the event, HandleUnmapNotify will have to mash the UnmapNotify
   * into a DestroyNotify.
   */
  if(!Tmp_win)
    return;

  XUnmapWindow(dpy, Tmp_win->frame);
  XSync(dpy,0);
  
  if(Tmp_win == Scr.Hilite)
    Scr.Hilite = NULL;
  
  Broadcast(M_DESTROY_WINDOW,3,Tmp_win->w,Tmp_win->frame,
	    (unsigned long)Tmp_win,0,0,0,0);

  if(Scr.PreviousFocus == Tmp_win)
    Scr.PreviousFocus = NULL;

  if(ButtonWindow == Tmp_win)
    ButtonWindow = NULL;

  if((Tmp_win == Scr.Focus)&&(Scr.flags & ClickToFocus))
    {
      if(Tmp_win->next)
	SetFocus(Tmp_win->next->w, Tmp_win->next);
      else
	SetFocus(Scr.NoFocusWin, NULL);
    }

  if(Scr.Focus == Tmp_win)
    SetFocus(Scr.NoFocusWin, NULL);

  ClearCirculatedFlag();

  if(Tmp_win == Scr.pushed_window)
    Scr.pushed_window = NULL;

  if(Tmp_win == colormap_win)
    colormap_win = NULL;

  XDestroyWindow(dpy, Tmp_win->frame);
  XDeleteContext(dpy, Tmp_win->frame, FvwmContext);
  
  XDestroyWindow(dpy, Tmp_win->Parent);

  XDeleteContext(dpy, Tmp_win->Parent, FvwmContext);

  XDeleteContext(dpy, Tmp_win->w, FvwmContext);
  
  if ((Tmp_win->icon_w)&&(Tmp_win->flags & PIXMAP_OURS))
    XFreePixmap(dpy, Tmp_win->iconPixmap);
  

#ifndef NO_PAGER
  if ((Scr.Pager_w)&& !(Tmp_win->flags & STICKY))
    XDestroyWindow(dpy, Tmp_win->pager_view);
#endif
    
  if (Tmp_win->icon_w)
    {
      XDestroyWindow(dpy, Tmp_win->icon_w);
      XDeleteContext(dpy, Tmp_win->icon_w, FvwmContext);
    }
  if((Tmp_win->flags &ICON_OURS)&&(Tmp_win->icon_pixmap_w != None))
    XDestroyWindow(dpy, Tmp_win->icon_pixmap_w);
  if(Tmp_win->icon_pixmap_w != None)
    XDeleteContext(dpy, Tmp_win->icon_pixmap_w, FvwmContext);

  if (Tmp_win->flags & TITLE)
    {
      XDeleteContext(dpy, Tmp_win->title_w, FvwmContext);
      for(i=0;i<Scr.nr_left_buttons;i++)
	XDeleteContext(dpy, Tmp_win->left_w[i], FvwmContext);
      for(i=0;i<Scr.nr_right_buttons;i++)
	if(Tmp_win->right_w[i] != None)
	  XDeleteContext(dpy, Tmp_win->right_w[i], FvwmContext);
    }
  if (Tmp_win->flags & BORDER)
    {
      for(i=0;i<4;i++)
	XDeleteContext(dpy, Tmp_win->sides[i], FvwmContext);
      for(i=0;i<4;i++)
	XDeleteContext(dpy, Tmp_win->corners[i], FvwmContext);
    }
  
  Tmp_win->prev->next = Tmp_win->next;
  if (Tmp_win->next != NULL)
    Tmp_win->next->prev = Tmp_win->prev;
  free_window_names (Tmp_win, True, True);		
  if (Tmp_win->wmhints)					
    XFree ((char *)Tmp_win->wmhints);
  if (Tmp_win->class.res_name && Tmp_win->class.res_name != NoName)  
    XFree ((char *)Tmp_win->class.res_name);
  if (Tmp_win->class.res_class && Tmp_win->class.res_class != NoName) 
    XFree ((char *)Tmp_win->class.res_class);
  if(Tmp_win->mwm_hints)
    XFree((char *)Tmp_win->mwm_hints);

  if(Tmp_win->cmap_windows != (Window *)NULL)
    XFree((void *)Tmp_win->cmap_windows);

  free((char *)Tmp_win);

#ifndef NO_PAGER
  RedrawPager();
#endif
  XSync(dpy,0);
  return;
}



/**************************************************************************
 *
 * Removes expose events for a specific window from the queue 
 *
 *************************************************************************/
int flush_expose (Window w)
{
  XEvent dummy;
  int i=0;
  
  while (XCheckTypedWindowEvent (dpy, w, Expose, &dummy))i++;
  return i;
}



/***********************************************************************
 *
 *  Procedure:
 *	RestoreWithdrawnLocation
 * 
 *  Puts windows back where they were before fvwm took over 
 *
 ************************************************************************/
void RestoreWithdrawnLocation (FvwmWindow *tmp,Bool restart)
{
  int a,b,w2,h2;
  unsigned int bw,mask;
  XWindowChanges xwc;
  
  if(!tmp)
    return;
  
  if (XGetGeometry (dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y, 
		    &JunkWidth, &JunkHeight, &bw, &JunkDepth)) 
    {
      XTranslateCoordinates(dpy,tmp->frame,Scr.Root,xwc.x,xwc.y,
			    &a,&b,&JunkChild);
      xwc.x = a + tmp->xdiff;
      xwc.y = b + tmp->ydiff;
      xwc.border_width = tmp->old_bw;
      mask = (CWX | CWY|CWBorderWidth);
      
      /* We can not assume that the window is currently on the screen.
       * Although this is normally the case, it is not always true.  The
       * most common example is when the user does something in an
       * application which will, after some amount of computational delay,
       * cause the window to be unmapped, but then switches screens before
       * this happens.  The XTranslateCoordinates call above will set the
       * window coordinates to either be larger than the screen, or negative.
       * This will result in the window being placed in odd, or even
       * unviewable locations when the window is remapped.  The followin code
       * forces the "relative" location to be within the bounds of the display.
       *
       * gpw -- 11/11/93
       *
       * Unfortunately, this does horrendous things during re-starts, 
       * hence the "if(restart) clause (RN) 
       *
       * Also, fixed so that it only does this stuff if a window is more than
       * half off the screen. (RN)
       */
      
      if(!restart)
	{
	  /* Don't mess with it if its partially on the screen now */
	  if((tmp->frame_x < 0)||(tmp->frame_y<0)||
	     (tmp->frame_x >= Scr.MyDisplayWidth)||
	     (tmp->frame_y >= Scr.MyDisplayHeight))
	    {
	      w2 = (tmp->frame_width>>1);
	      h2 = (tmp->frame_height>>1);
	      if (( xwc.x < -w2) || (xwc.x > (Scr.MyDisplayWidth-w2 )))
		{
		  xwc.x = xwc.x % Scr.MyDisplayWidth;
		  if ( xwc.x < -w2 )
		    xwc.x += Scr.MyDisplayWidth;
		}
	      if ((xwc.y < -h2) || (xwc.y > (Scr.MyDisplayHeight-h2 )))
		{
		  xwc.y = xwc.y % Scr.MyDisplayHeight;
		  if ( xwc.y < -h2 )
		    xwc.y += Scr.MyDisplayHeight;
		}
	    }
	}
      XReparentWindow (dpy, tmp->w,Scr.Root,xwc.x,xwc.y);
      
      if((tmp->flags & ICONIFIED)&&(!(tmp->flags & SUPPRESSICON)))
	{
	  if (tmp->icon_w) 
	    XUnmapWindow(dpy, tmp->icon_w);
	  if (tmp->icon_pixmap_w) 
	    XUnmapWindow(dpy, tmp->icon_pixmap_w);	  
	}
      
      XConfigureWindow (dpy, tmp->w, mask, &xwc);
      XSync(dpy,0);
    }
}


/***************************************************************************
 *
 * Start/Stops the auto-raise timer
 *
 ****************************************************************************/
void SetTimer(int delay)
{
#ifdef HAVE_SETITIMER
  struct itimerval value;
  
  value.it_value.tv_usec = 1000*(delay%1000);
  value.it_value.tv_sec = delay/1000;
  value.it_interval.tv_usec = 0;
  value.it_interval.tv_sec = 0;
  setitimer(ITIMER_REAL,&value,NULL);
#endif
}

/***************************************************************************
 *
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type	ClientMessage
 *     message type	_XA_WM_PROTOCOLS
 *     window		tmp->w
 *     format		32
 *     data[0]		message atom
 *     data[1]		time stamp
 *
 ****************************************************************************/
void send_clientmessage (Window w, Atom a, Time timestamp)
{
  XClientMessageEvent ev;
  
  ev.type = ClientMessage;
  ev.window = w;
  ev.message_type = _XA_WM_PROTOCOLS;
  ev.format = 32;
  ev.data.l[0] = a;
  ev.data.l[1] = timestamp;
  XSendEvent (dpy, w, False, 0L, (XEvent *) &ev);
}





/****************************************************************************
 *
 * Records the time of the last processed event. Used in XSetInputFocus
 *
 ****************************************************************************/
Time lastTimestamp = CurrentTime;	/* until Xlib does this for us */

Bool StashEventTime (XEvent *ev)
{
  Time NewTimestamp = CurrentTime;

  switch (ev->type) 
    {
    case KeyPress:
    case KeyRelease:
      NewTimestamp = ev->xkey.time;
      break;
    case ButtonPress:
    case ButtonRelease:
      NewTimestamp = ev->xbutton.time;
      break;
    case MotionNotify:
      NewTimestamp = ev->xmotion.time;
      break;
    case EnterNotify:
    case LeaveNotify:
      NewTimestamp = ev->xcrossing.time;
      break;
    case PropertyNotify:
      NewTimestamp = ev->xproperty.time;
      break;
    case SelectionClear:
      NewTimestamp = ev->xselectionclear.time;
      break;
    case SelectionRequest:
      NewTimestamp = ev->xselectionrequest.time;
      break;
    case SelectionNotify:
      NewTimestamp = ev->xselection.time;
      break;
    default:
      return False;
    }
  if(NewTimestamp > lastTimestamp)
    lastTimestamp = NewTimestamp;
  return True;
}


/******************************************************************************
 *
 * Cleare the CIRCULATED field of the window flags.
 * 
 *****************************************************************************/
void ClearCirculatedFlag()
{
  FvwmWindow *temp;
  int i=0;

  temp = Scr.FvwmRoot.next;	  
  while(temp != NULL)
    {
      temp->focus_sequence = i++;
      temp = temp->next;
    }
}
