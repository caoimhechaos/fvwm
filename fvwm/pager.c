/****************************************************************************
 * This module is all new
 * by Rob Nation 
 ****************************************************************************/
/***********************************************************************
 *
 * fvwm pager handling code
 *
 ***********************************************************************/

#include <config.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"

extern XEvent Event;

XGCValues Globalgcv;
unsigned long Globalgcm;

void DrawPartitionLines(void);
FvwmWindow *FindCounterpart(Window target);
Bool pagerOn = True;
Bool EnablePagerRedraw = True;
Bool DoHandlePageing = True;

/****************************************************************************
 * Cause the pager window to be re-drawn
 *
 * Trying to get clever - re-draw the pager by causing an expose event,
 * the really redraw only when an expose arrives. Advantage is that the
 * number of re-draws will be minimized
 ***************************************************************************/
void RedrawPager()
{
#ifndef NO_PAGER
  if((Scr.FvwmPager)&&(EnablePagerRedraw))
    XClearArea(dpy,Scr.Pager_w, 0, 0, Scr.FvwmPager->frame_width,
	       Scr.FvwmPager->frame_height,True);
#endif
}

void ReallyRedrawPager()
{
#ifndef NO_PAGER
  FvwmWindow *t;
  Pixel TextColor,BackColor;

  if(!Scr.FvwmPager)
    return;

  flush_expose (Scr.Pager_w);
  flush_expose (Scr.CPagerWin);

  if(Scr.PagerFont.height > 0)
    {
     if(Scr.Hilite != NULL)
	{
	  if(!(Scr.Hilite->flags & STICKY)&& (!(Scr.Hilite->flags&ICONIFIED)||(!(Scr.Hilite->flags & SUPPRESSICON)))&&
	     (!(Scr.Hilite->flags&ICONIFIED)||(!(Scr.flags & StickyIcons)))&&
	     (Scr.Hilite->icon_name != NULL))
	    {
	      TextColor = Scr.HiColors.fore;
	      BackColor = Scr.HiColors.back;
	      NewFontAndColor(Scr.PagerFont.font->fid,TextColor,BackColor);
	      flush_expose(Scr.Hilite->pager_view);
	      XDrawImageString (dpy, Scr.Hilite->pager_view, Scr.FontGC,
			   2,Scr.PagerFont.y+2, 
			   Scr.Hilite->icon_name, strlen(Scr.Hilite->icon_name));
	    }
	}

      TextColor = Scr.StdColors.fore;
      BackColor = Scr.StdColors.back;
      NewFontAndColor(Scr.PagerFont.font->fid,TextColor,BackColor);
	
	
      for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
	{
	  if(t != Scr.Hilite)
	    {
	      if(!(t->flags & STICKY)&&
		 (!(t->flags & ICONIFIED)||(!(t->flags & SUPPRESSICON)))&&
		 (!(t->flags & ICONIFIED)||(!(Scr.flags & StickyIcons)))&&
		 (t->icon_name != NULL))
		{
		  flush_expose(t->pager_view);		  
		  XDrawImageString (dpy, t->pager_view, Scr.FontGC,
				    2,Scr.PagerFont.y+2, 
				    t->icon_name, strlen(t->icon_name));
		}
	    }
	}
    }
  DrawPartitionLines();
#endif
}

#ifndef NO_PAGER
void DrawPartitionLines(void)
{
  int y, y1, y2, x, x1, x2;
  int MaxW,MaxH,width,height;

  MaxW = Scr.VxMax + Scr.MyDisplayWidth;
  MaxH = Scr.VyMax + Scr.MyDisplayHeight;

  width = Scr.FvwmPager->frame_width - 2*Scr.FvwmPager->boundary_width;
  height = Scr.FvwmPager->frame_height - Scr.FvwmPager->title_height 
    - 2*Scr.FvwmPager->boundary_width;

  x = Scr.MyDisplayWidth;
  y1 = 0;
  y2 = height;
  while(x < MaxW)
    {
      x1 = x*width/MaxW;
      XDrawLine(dpy,Scr.Pager_w,Scr.NormalGC,x1,y1,x1,y2);
      x += Scr.MyDisplayWidth;
    }

  y = Scr.MyDisplayHeight;
  x1 = 0;
  x2 = width;
  while(y < MaxH)
    {
      y1 = y*height/MaxH;
      XDrawLine(dpy,Scr.Pager_w,Scr.NormalGC,x1,y1,x2,y1);
      y += Scr.MyDisplayHeight;
    }
}
#endif

void SwitchPages(Bool align, Bool ChangeFocus)
{
#ifndef NO_PAGER
  int x,y;
  unsigned int width,height;
  FvwmWindow *tmp_win;
  Window dumwin;

  if(!Scr.FvwmPager)
    return;

  XTranslateCoordinates (dpy, Event.xbutton.window, Scr.Pager_w,
			 Event.xbutton.x, Event.xbutton.y, &x, &y, &dumwin);

  width = Scr.FvwmPager->frame_width - 2*Scr.FvwmPager->boundary_width;
  height = Scr.FvwmPager->frame_height - Scr.FvwmPager->title_height 
    - 2*Scr.FvwmPager->boundary_width;

  if(x<0)x=0;
  if(y<0)y=0;
  x = x * (Scr.VxMax+Scr.MyDisplayWidth)/width;
  y = y * (Scr.VyMax+Scr.MyDisplayHeight)/height;
  if(align)
    {
      x = (x/Scr.MyDisplayWidth)*Scr.MyDisplayWidth;
      y = (y/Scr.MyDisplayHeight)*Scr.MyDisplayHeight;
    }
  if(x<0)x=0;
  if(y<0)y=0;
#ifndef NON_VIRTUAL
  MoveViewport(x,y,True);
#endif
  if((ChangeFocus)&&(Scr.flags & ClickToFocus))
    {
      tmp_win = FindCounterpart(Event.xbutton.subwindow);
      if(tmp_win)
	{
	  RaiseWindow(tmp_win);
	  SetFocus(tmp_win->w,tmp_win);
	  ClearCirculatedFlag();
	}
    }
#endif
}


void RaiseWindow(FvwmWindow *t)
{
  FvwmWindow *t2;
  int count, i;
  Window *wins;
  SetTimer(0);

  /* raise the target, at least */
  count = 1;
  Broadcast(M_RAISE_WINDOW,3,t->w,t->frame,(unsigned long)t,0,0,0,0);
  
  for (t2 = Scr.FvwmRoot.next; t2 != NULL; t2 = t2->next)
    {
      if(t2->flags & ONTOP)
	count++;
      if((t2->flags & TRANSIENT) &&(t2->transientfor == t->w)&&
	 (t2 != t))
	{
#ifndef NO_PAGER
	  if((Scr.Pager_w)&& !(t2->flags & STICKY))
	    XRaiseWindow(dpy,t2->pager_view);
#endif
	  count++;
	  Broadcast(M_RAISE_WINDOW,3,t2->w,t2->frame,(unsigned long) t2,
		    0,0,0,0);
	  if ((t2->flags & ICONIFIED)&&(!(t2->flags & SUPPRESSICON)))
	    {
	      count += 2;
	    }	  
	}
    }
  if ((t->flags & ICONIFIED)&&(!(t->flags & SUPPRESSICON)))
    {
      count += 2;
    }

#ifndef NO_PAGER
  if((Scr.Pager_w)&& !(t->flags & STICKY))
    {
      XRaiseWindow(dpy,t->pager_view);
    }
#endif

  wins = (Window *)safemalloc(count*sizeof(Window));

  i=0;

  /* ONTOP windows on top */
  for (t2 = Scr.FvwmRoot.next; t2 != NULL; t2 = t2->next)
    {
      if(t2->flags & ONTOP)
	{
	  Broadcast(M_RAISE_WINDOW,3,t2->w,t2->frame,(unsigned long) t2,
		    0,0,0,0);
	  wins[i++] = t2->frame;
	}
    }
  
  /* now raise transients */
#ifndef DONT_RAISE_TRANSIENTS
  for (t2 = Scr.FvwmRoot.next; t2 != NULL; t2 = t2->next)
    {
      if((t2->flags & TRANSIENT) &&(t2->transientfor == t->w)&&
	 (t2 != t)&&(!(t2->flags & ONTOP)))
	{
	  wins[i++] = t2->frame;
	  if ((t2->flags & ICONIFIED)&&(!(t2->flags & SUPPRESSICON)))
	    {
	      if(!(t2->flags & NOICON_TITLE))wins[i++] = t2->icon_w;
	      wins[i++] = t2->icon_pixmap_w;
	    }
	}
    }
#endif


  if ((t->flags & ICONIFIED)&&(!(t->flags & SUPPRESSICON)))
    {
      if(!(t->flags & NOICON_TITLE))wins[i++] = t->icon_w;
      wins[i++] = t->icon_pixmap_w;
    }
  if(!(t->flags & ONTOP))
    wins[i++] = t->frame;
  if(!(t->flags & ONTOP))
    Scr.LastWindowRaised = t;

  if(i > 0)
    XRaiseWindow(dpy,wins[0]);

  XRestackWindows(dpy,wins,i);
  free(wins);

  RedrawPager();
#ifndef NON_VIRTUAL
  raisePanFrames();
#endif
}


void LowerWindow(FvwmWindow *t)
{
  XLowerWindow(dpy,t->frame);

  SetTimer(0);

  Broadcast(M_LOWER_WINDOW,3,t->w,t->frame,(unsigned long)t,0,0,0,0);
#ifndef NO_PAGER
  if((Scr.Pager_w)&& !(t->flags & STICKY))
    XLowerWindow(dpy,t->pager_view);
#endif

  if((t->flags & ICONIFIED)&&(!(t->flags & SUPPRESSICON)))
    {
      XLowerWindow(dpy, t->icon_w);
      XLowerWindow(dpy, t->icon_pixmap_w);
    }
  Scr.LastWindowRaised = (FvwmWindow *)0;
#ifndef NO_PAGER
  if(Scr.CPagerWin)
    XLowerWindow(dpy,Scr.CPagerWin);
#endif
  RedrawPager();
}


void PagerMoveWindow()
{
#ifndef NO_PAGER
  FvwmWindow *tmp_win;
  unsigned int width, height;
  int Xoff,Yoff,x,y,MaxW,MaxH,xl,yt,xl1,yt1;
  Window target;
  Bool done,finished = False;
  /* I tried to implement a feature so that when windows which could be
   * opaque moved in a normal move would get the full size windows moved in
   * conjunction with the pager version of the window, but it seems to
   * be buggy on some machines */
#ifdef BROKEN_STUFF
  Bool OpaqueMove = False;
  int dwidth,dheight;
#endif
  if(!Scr.FvwmPager)
    return;


  EnablePagerRedraw = False;
  target = Event.xbutton.subwindow;
  tmp_win = FindCounterpart(target);

  if(tmp_win == NULL)
    return;


  MaxW = Scr.VxMax + Scr.MyDisplayWidth;
  MaxH = Scr.VyMax + Scr.MyDisplayHeight;

  width = Scr.FvwmPager->frame_width - 2*Scr.FvwmPager->boundary_width;
  height = Scr.FvwmPager->frame_height - Scr.FvwmPager->title_height 
    - 2*Scr.FvwmPager->boundary_width;

  XQueryPointer(dpy, target, &JunkRoot, &JunkChild,
		&JunkX, &JunkY,	&Xoff, &Yoff, &JunkMask);

  XQueryPointer(dpy, Scr.Pager_w, &JunkRoot, &JunkChild,
		&JunkX, &JunkY, &xl, &yt, &JunkMask);
  if(xl<0)xl=0;
  if(yt<0)yt=0;
  if(xl>width)xl=width;
  if(yt>height)yt=height;
  xl -= Xoff;
  yt -= Yoff;
  xl1=xl;
  yt1=yt;
  
  while (!finished)
    {
      /* block until there is an interesting event */
      XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask | KeyPressMask |
		 PointerMotionMask | ButtonMotionMask | ExposureMask |
		 VisibilityChangeMask, &Event);
      StashEventTime(&Event);

      if (Event.type == MotionNotify) 
	/* discard any extra motion events before a logical release */
	while(XCheckMaskEvent(dpy, PointerMotionMask | ButtonMotionMask |
			      ButtonRelease, &Event))
	 {
	   StashEventTime(&Event);
	   if(Event.type == ButtonRelease) break;
	 }

      done = FALSE;

      /* Handle a limited number of key press events to allow mouseless
       * operation */
      if(Event.type == KeyPress)
	Keyboard_shortcuts(&Event,ButtonRelease);
      switch(Event.type)
	{
	case ButtonPress:
	case KeyPress:
	  /* throw away enter and leave events until release */
	  done = TRUE;
	  break;
	case ButtonRelease:
	  XQueryPointer(dpy, Scr.Pager_w, &JunkRoot, &JunkChild,
			&JunkX, &JunkY, &xl, &yt, &JunkMask);
	  if(xl<0)xl=0;
	  if(yt<0)yt=0;
	  if(xl>width)xl=width;
	  if(yt>height)yt=height;
	  xl -= Xoff;
	  yt -= Yoff;
	  done = TRUE;
	  finished = TRUE;
	  break;

	case MotionNotify:
	  XQueryPointer(dpy, Scr.Pager_w, &JunkRoot, &JunkChild,
			&JunkX, &JunkY, &xl, &yt, &JunkMask);
	  if(xl<0)xl=0;
	  if(yt<0)yt=0;
	  if(xl>width)xl=width;
	  if(yt>height)yt=height;

	  /* redraw the rubberband */
	  xl -= Xoff;
	  yt -= Yoff;

	  done = TRUE;
	  break;

	default:
	  break;
	}
      if(!done)
	{
	  DispatchEvent();
	}
      XMoveWindow(dpy, target,xl, yt);
      DrawPartitionLines();

    }

  x = xl*MaxW/(int)width - Scr.Vx;
  y = yt*MaxH/(int)height - Scr.Vy;

  MoveResizePagerView(tmp_win);
  if((xl1!=xl)||(yt1 != yt))
    {
      if((tmp_win->flags & ICONIFIED)&&(!(tmp_win->flags & SUPPRESSICON)))
	{
	  tmp_win->icon_x_loc = x;
	  tmp_win->icon_xl_loc = x - 
	    (tmp_win->icon_w_width - tmp_win->icon_p_width)/2;
	  tmp_win->icon_y_loc = y;
	  XMoveWindow (dpy, tmp_win->icon_w, tmp_win->icon_xl_loc, y+tmp_win->icon_p_height);

	  if(tmp_win->icon_pixmap_w != None)
	    XMoveWindow (dpy, tmp_win->icon_pixmap_w, x,y);

	  tmp_win->flags |= ICON_MOVED;

	}
      else 
	{
	  /* show the actual window */
	  SetupFrame(tmp_win,x,y, tmp_win->frame_width,tmp_win->frame_height,FALSE);
	}  
    }
  EnablePagerRedraw = True;
  RedrawPager();
#endif      
}



#ifndef NO_PAGER
FvwmWindow *FindCounterpart(Window target)
{
  FvwmWindow *t,*tmp_win=0;

  tmp_win = NULL;
  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      if(t->pager_view == target)
	{
	  tmp_win = t;
	}
    }
  return tmp_win;
}
#endif

/***************************************************************************
 * 
 * Check to see if the pointer is on the edge of the screen, and scroll/page
 * if needed 
 ***************************************************************************/
void HandlePaging(int HorWarpSize, int VertWarpSize, int *xl, int *yt, 
		  int *delta_x, int *delta_y,Bool Grab)
{
#ifndef NON_VIRTUAL	  
  int x,y,total;
#endif

  *delta_x = 0;
  *delta_y = 0;
  
#ifndef NON_VIRTUAL	  
  if (DoHandlePageing) 
    {
      if((Scr.ScrollResistance >= 10000)||
	 ((HorWarpSize ==0)&&(VertWarpSize==0)))
	return;
      
      /* need to move the viewport */
      if(( *xl >= SCROLL_REGION)&&( *xl < Scr.MyDisplayWidth-SCROLL_REGION)&&
	 ( *yt >= SCROLL_REGION)&&( *yt < Scr.MyDisplayHeight-SCROLL_REGION))
	return;

      total = 0;
      while(total < Scr.ScrollResistance)
	{
	  sleep_a_little(10000);
	  total+=10;
	  if(XCheckWindowEvent(dpy,Scr.PanFrameTop.win,
			       LeaveWindowMask,&Event))
	    {
	      StashEventTime(&Event);
	      return;
	    }
	  if(XCheckWindowEvent(dpy,Scr.PanFrameBottom.win,
			       LeaveWindowMask,&Event))
	    {
	      StashEventTime(&Event);
	      return;
	    }	
	  if(XCheckWindowEvent(dpy,Scr.PanFrameLeft.win,
			       LeaveWindowMask,&Event))
	    {
	      StashEventTime(&Event);
	      return;
	    }      
	  if(XCheckWindowEvent(dpy,Scr.PanFrameRight.win,
			       LeaveWindowMask,&Event))
	    {
	      StashEventTime(&Event);
	      return;
	    }
	}
      
      XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
		    &x, &y, &JunkX, &JunkY, &JunkMask);   

      /* Turn off the rubberband if its on */
      MoveOutline(Scr.Root,0,0,0,0);
      
      /* Move the viewport */
      /* and/or move the cursor back to the approximate correct location */
      /* that is, the same place on the virtual desktop that it */
      /* started at */
      if( x<SCROLL_REGION)
	*delta_x = -HorWarpSize;
      else if ( x >= Scr.MyDisplayWidth-SCROLL_REGION)
	*delta_x = HorWarpSize;
      else
	*delta_x = 0;
      if( y<SCROLL_REGION)
	*delta_y = -VertWarpSize;
      else if ( y >= Scr.MyDisplayHeight-SCROLL_REGION)
	*delta_y = VertWarpSize;
      else
	*delta_y = 0;
      
      /* Ouch! lots of bounds checking */
      if(Scr.Vx + *delta_x < 0) 
	{
	  if (!(Scr.flags & EdgeWrapX ))
	    {
	      *delta_x = -Scr.Vx;
	      *xl = x - *delta_x;
	    }
	  else 
	    {
	      *delta_x += Scr.VxMax + Scr.MyDisplayWidth;
	      *xl = x + *delta_x % Scr.MyDisplayWidth + HorWarpSize;
	    }
	}
      else if(Scr.Vx + *delta_x > Scr.VxMax)
	{
	  if (!(Scr.flags & EdgeWrapX))
	    {
	      *delta_x = Scr.VxMax - Scr.Vx;
	      *xl = x - *delta_x;
	    }
	  else 
	    {
	      *delta_x -= Scr.VxMax +Scr.MyDisplayWidth;
	      *xl = x + *delta_x % Scr.MyDisplayWidth - HorWarpSize;
	    }
	}
      else
	*xl = x - *delta_x;
      
      if(Scr.Vy + *delta_y < 0) 
	{
	  if (!(Scr.flags & EdgeWrapY))
	    {
	      *delta_y = -Scr.Vy;
	      *yt = y - *delta_y;
	    }
	  else
	    {
	      *delta_y += Scr.VyMax + Scr.MyDisplayHeight;
	      *yt = y + *delta_y % Scr.MyDisplayHeight + VertWarpSize;
	    }
	}
      else if(Scr.Vy + *delta_y > Scr.VyMax) 
	{
	  if (!(Scr.flags & EdgeWrapY))
	    {
	      *delta_y = Scr.VyMax - Scr.Vy;
	      *yt = y - *delta_y;
	    }
	  else
	    {
	      *delta_y -= Scr.VyMax + Scr.MyDisplayHeight;
	      *yt = y + *delta_y % Scr.MyDisplayHeight - VertWarpSize;
	    }
	}
      else
	*yt = y - *delta_y;
      
      if(*xl <= SCROLL_REGION) *xl = SCROLL_REGION+1;
      if(*yt <= SCROLL_REGION) *yt = SCROLL_REGION+1;
      if(*xl >= Scr.MyDisplayWidth - SCROLL_REGION)
	*xl = Scr.MyDisplayWidth - SCROLL_REGION -1;
      if(*yt >= Scr.MyDisplayHeight - SCROLL_REGION)
	*yt = Scr.MyDisplayHeight - SCROLL_REGION -1;
      
      if((*delta_x != 0)||(*delta_y!=0))
	{
	  if(Grab)
	    XGrabServer(dpy);
	  XWarpPointer(dpy,None,Scr.Root,0,0,0,0,*xl,*yt);
	  MoveViewport(Scr.Vx + *delta_x,Scr.Vy + *delta_y,False);
	  XQueryPointer(dpy, Scr.Root, &JunkRoot, &JunkChild,
			xl, yt, &JunkX, &JunkY, &JunkMask);
	  if(Grab)
	    XUngrabServer(dpy);
	}
    }
#endif	  
}



void MoveResizeViewPortIndicator(void)
{
#ifndef NO_PAGER
  int width,height,x1,x2,y1,y2;

  if((Scr.CPagerWin)&&(Scr.FvwmPager))
    {
      width = Scr.FvwmPager->frame_width - 2*Scr.FvwmPager->boundary_width;
      height = Scr.FvwmPager->frame_height - Scr.FvwmPager->title_height 
	- 2*Scr.FvwmPager->boundary_width;
      x1 = Scr.Vx * width/(Scr.VxMax+Scr.MyDisplayWidth)+1;
      y1 = Scr.Vy * height/(Scr.VyMax+Scr.MyDisplayHeight)+1;
      x2 = (Scr.MyDisplayWidth) * width/(Scr.VxMax+Scr.MyDisplayWidth)-1;
      y2 = (Scr.MyDisplayHeight) * height/(Scr.VyMax+Scr.MyDisplayHeight)-1;
      if(x1==1)
	{
	  x1--;
	  x2++;
	}
      if(y1==1)
	{
	  y1--;
	  y2++;
	}
      XMoveResizeWindow(dpy,Scr.CPagerWin,x1,y1,x2,y2);
    }
#endif
}


void MoveResizePagerView(FvwmWindow *t)
{
#ifndef NO_PAGER
  unsigned int width,height;
  int ww,wh;
  int wx,wy;
  int MaxH,MaxW;

  if((!Scr.FvwmPager)||(!pagerOn))
    return;

  width = Scr.FvwmPager->frame_width - 2*Scr.FvwmPager->boundary_width;
  height = Scr.FvwmPager->frame_height - Scr.FvwmPager->title_height 
    - 2*Scr.FvwmPager->boundary_width;

  MaxW = Scr.VxMax + Scr.MyDisplayWidth;
  MaxH = Scr.VyMax + Scr.MyDisplayHeight;

  if((!(t->flags & STICKY)&&
     (!(t->flags & ICONIFIED)||(!(t->flags & SUPPRESSICON)))&&
     (!(t->flags & ICONIFIED)||(!(t->flags & SUPPRESSICON))))&&
     (!((t->flags & ICONIFIED)&&(t->flags &ICON_UNMAPPED)))&&
     (t->Desk == Scr.CurrentDesk))
    {
      if(t->flags & ICONIFIED)
	{	      
	  /* show the icon loc */
	  wx = (t->icon_x_loc + Scr.Vx)*(int)width/MaxW;;
	  wy = (t->icon_y_loc + Scr.Vy)*(int)height/MaxH;
	  ww = t->icon_w_width*(int)width/MaxW;
	  wh = (t->icon_w_height+t->icon_p_height)*(int)height/MaxH;
	}
      else 
	{
	  /* show the actual window */
	  wx = (t->frame_x + Scr.Vx)*(int)width/MaxW;
	  wy = (t->frame_y + Scr.Vy)*(int)height/MaxH;
	  ww = t->frame_width*(int)width/MaxW;
	  wh = t->frame_height*(int)height/MaxH;
	}
      if(ww<2)ww=2;
      if(wh<2)wh=2;
      XMoveResizeWindow(dpy, t->pager_view, wx, wy, ww, wh);
    }
  else
    {
      /* window is sticky - make sure that the pager_view window is not 
       * visible */
      XMoveResizeWindow(dpy, t->pager_view, -10, -10, 5, 5);
    }
  RedrawPager();
#endif
}


/***********************************************************************
 *
 *  Procedure:
 *	Initialize_pager - creates the pager window, if needed
 *
 *  Inputs:
 *	x,y location of the window
 *
 ***********************************************************************/
#ifndef NO_PAGER
char *pager_name = "Fvwm Pager";
XSizeHints sizehints =
{
  (PMinSize | PResizeInc | PBaseSize | PWinGravity),
  0, 0, 100, 100,	                /* x, y, width and height */
  1, 1,		                        /* Min width and height */
  0, 0,		                        /* Max width and height */
  1, 1,	                         	/* Width and height increments */
  {0, 0}, {0, 0},                       /* Aspect ratio - not used */
  1, 1,                 		/* base size */
  (NorthWestGravity)                    /* gravity */
};

void initialize_pager(int x, int y)
{
  XTextProperty name;
  int width,height,window_x,window_y;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  extern Pixel PagerBackColor;
  extern Pixel PagerForeColor;

  width = (Scr.VxMax + Scr.MyDisplayWidth)/Scr.VScale;
  height = (Scr.VyMax + Scr.MyDisplayHeight)/Scr.VScale;
  
  if(x >=0)
    window_x = x;
  else
    {
      sizehints.win_gravity = NorthEastGravity;
      window_x = Scr.MyDisplayWidth - width + x -2;
    }

  if(y >=0)
    window_y = y;
  else
    {
      window_y = Scr.MyDisplayHeight - height + y -2;
      if(x<0)
	sizehints.win_gravity = SouthEastGravity;
      else
	sizehints.win_gravity = SouthWestGravity;
    }
  valuemask = (CWBackPixel | CWBorderPixel | CWEventMask|CWCursor);
  attributes.background_pixel = PagerBackColor;
  attributes.border_pixel = Scr.StdColors.fore;
  attributes.cursor = Scr.FvwmCursors[DEFAULT];
  attributes.event_mask = (ExposureMask | EnterWindowMask|ButtonReleaseMask|
			   ButtonMotionMask);
  sizehints.width = width;
  sizehints.height = height;
  sizehints.x = window_x;
  sizehints.y = window_y;

  Scr.Pager_w = XCreateWindow (dpy, Scr.Root, window_x, window_y, width,
			       height, (unsigned int) 1,
			       CopyFromParent, InputOutput,
			       (Visual *) CopyFromParent,
			       valuemask, &attributes);
  XSetWMNormalHints(dpy,Scr.Pager_w,&sizehints);
  XStringListToTextProperty(&pager_name,1,&name);
  XSetWMName(dpy,Scr.Pager_w,&name);
  XSetWMIconName(dpy,Scr.Pager_w,&name);
  XFree((char *)name.value);

  attributes.event_mask = KeyPressMask | ExposureMask;
  attributes.background_pixel = PagerForeColor;
  attributes.cursor = Scr.FvwmCursors[DEFAULT];
  Scr.CPagerWin=XCreateWindow(dpy,Scr.Pager_w,-10, -10, 10, 10,0,
			      CopyFromParent,
			      InputOutput,CopyFromParent,
			      CWEventMask|CWBackPixel|CWCursor,
			      &attributes);
  XMapRaised(dpy,Scr.CPagerWin);
}
#endif
