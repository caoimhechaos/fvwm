/****************************************************************************
 * THIS module is based on Twm, but has been siginificantly modified 
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


/**********************************************************************
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 **********************************************************************/
#include <config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fvwm.h"
#include <X11/Xatom.h>
#include "misc.h"
#include "screen.h"
#ifdef SHAPE
#include <X11/extensions/shape.h>
#include <X11/Xresource.h>
#endif /* SHAPE */
#include "module.h"

char NoName[] = "Untitled"; /* name if no name is specified */

/* Used to parse command line of clients for specific desk requests. */
/* Todo: check for multiple desks. */
static XrmDatabase db;
static XrmOptionDescRec table [] = {
  /* Want to accept "-workspace N" or -xrm "fvwm*desk:N" as options
   * to specify the desktop. I have to include dummy options that
   * are meaningless since Xrm seems to allow -w to match -workspace
   * if there would be no ambiguity. */
    {"-workspacf",	"*junk",	XrmoptionSepArg, (caddr_t) NULL},
    {"-workspace",	"*desk",	XrmoptionSepArg, (caddr_t) NULL},
    {"-xrn",		NULL,		XrmoptionResArg, (caddr_t) NULL},
    {"-xrm",		NULL,		XrmoptionResArg, (caddr_t) NULL},
};

/***********************************************************************
 *
 *  Procedure:
 *	AddWindow - add a new window to the fvwm list
 *
 *  Returned Value:
 *	(FvwmWindow *) - pointer to the FvwmWindow structure
 *
 *  Inputs:
 *	w	- the window id of the window to add
 *	iconm	- flag to tell if this is an icon manager window
 *
 ***********************************************************************/
FvwmWindow *AddWindow(Window w)
{
  FvwmWindow *tmp_win;		        /* new fvwm window structure */
  unsigned long valuemask;		/* mask for create windows */
  XSetWindowAttributes attributes;	/* attributes for create windows */
  Atom actual_type;
  int actual_format,i,width,height;
  unsigned long nitems, bytesafter;
  int a,b;
  char *value;
  unsigned long tflag;
  int Desk, border_width, resize_width;
  extern Bool NeedToResizeToo;
  extern FvwmWindow *colormap_win;
  char *forecolor = NULL, *backcolor = NULL;
  int client_argc;
  char **client_argv = NULL, *str_type;
  Bool status;
  XrmValue rm_value;
  unsigned long buttons;
  XTextProperty text_prop;

  NeedToResizeToo = False;
  /* allocate space for the fvwm window */
  tmp_win = (FvwmWindow *)calloc(1, sizeof(FvwmWindow));
  if (tmp_win == (FvwmWindow *)0)
    {
      return NULL;
    }
  tmp_win->flags = 0;
  tmp_win->w = w;

  tmp_win->cmap_windows = (Window *)NULL;

 if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
    {
      free((char *)tmp_win);
      return(NULL);
    }
   if ( XGetWMName(dpy, tmp_win->w, &text_prop) != 0 ) 
     tmp_win->name = (char *)text_prop.value ;
   else
     tmp_win->name = NoName;

  tmp_win->class = NoClass;
  tmp_win->focus_sequence = 0;
  XGetClassHint(dpy, tmp_win->w, &tmp_win->class);
  if (tmp_win->class.res_name == NULL)
    tmp_win->class.res_name = NoName;
  if (tmp_win->class.res_class == NULL)
    tmp_win->class.res_class = NoName;

  FetchWmProtocols (tmp_win);
  FetchWmColormapWindows (tmp_win);
  if(!(XGetWindowAttributes(dpy,tmp_win->w,&(tmp_win->attr))))
    tmp_win->attr.colormap = Scr.FvwmRoot.attr.colormap;

  tmp_win->wmhints = XGetWMHints(dpy, tmp_win->w);

  if(XGetTransientForHint(dpy, tmp_win->w,  &tmp_win->transientfor))
    tmp_win->flags |= TRANSIENT;
  else
    tmp_win->flags &= ~TRANSIENT;

  tmp_win->old_bw = tmp_win->attr.border_width;

#ifdef SHAPE
  {
    int xws, yws, xbs, ybs;
    unsigned wws, hws, wbs, hbs;
    int boundingShaped, clipShaped;
    
    XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
    XShapeQueryExtents (dpy, tmp_win->w,
			&boundingShaped, &xws, &yws, &wws, &hws,
			&clipShaped, &xbs, &ybs, &wbs, &hbs);
    tmp_win->wShaped = boundingShaped;
  }
#endif /* SHAPE */


  /* if the window is in the NoTitle list, or is a transient,
   *  dont decorate it.
   * If its a transient, and DecorateTransients was specified,
   *  decorate anyway
   */
  /*  Assume that we'll decorate */
  tmp_win->flags |= BORDER;
  tmp_win->flags |= TITLE;
  tmp_win->title_height = Scr.TitleHeight + tmp_win->bw;

  tflag = LookInList(Scr.TheList,tmp_win->name,&tmp_win->class, &value, &Desk,
		     &border_width, &resize_width,
                     &forecolor,&backcolor,&tmp_win->buttons); 

  GetMwmHints(tmp_win);

  SelectDecor(tmp_win,tflag,border_width,resize_width); 

  if(tflag & START_ICONIC_FLAG)
    tmp_win->flags |= STARTICONIC;
  if (tflag & STAYSONTOP_FLAG)
    tmp_win->flags |= ONTOP;
  if (tflag&STICKY_FLAG)
    tmp_win->flags |= STICKY;
  if(tflag & LISTSKIP_FLAG)
    tmp_win->flags |= WINDOWLISTSKIP;
  if(tflag & CIRCULATESKIP_FLAG)
    tmp_win->flags |= CIRCULATESKIP;

  if(tflag & SUPPRESSICON_FLAG)
    tmp_win->flags |= SUPPRESSICON;
  if(tflag & NOICON_TITLE_FLAG)
    tmp_win->flags |= NOICON_TITLE;
  if(Scr.flags & SuppressIcons)
    tmp_win->flags |= SUPPRESSICON;

  /* find a suitable icon pixmap */
  if(tflag & ICON_FLAG)
    {
      /* an icon was specified */
      tmp_win->icon_bitmap_file = value;
    }
  else if((tmp_win->wmhints)
	  &&(tmp_win->wmhints->flags & (IconWindowHint|IconPixmapHint)))
    {
      /* window has its own icon */
      tmp_win->icon_bitmap_file = NULL;
    }
  else
    {
      /* use default icon */
      tmp_win->icon_bitmap_file = Scr.DefaultIcon;
    }

  GetWindowSizeHints (tmp_win);


  /* Tentative size estimate */
  tmp_win->frame_width = tmp_win->attr.width+2*tmp_win->boundary_width;
  tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height+
    2*tmp_win->boundary_width;

  ConstrainSize(tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);

  /* Find out if the client requested a specific desk on the command line. */
  if (XGetCommand (dpy, tmp_win->w, &client_argv, &client_argc)) {
      XrmParseCommand (&db, table, 4, "fvwm", &client_argc, client_argv);
      status = XrmGetResource (db, "fvwm.desk", "Fvwm.Desk", &str_type, &rm_value);
      if ((status == True) && (rm_value.size != 0)) {
          Desk = atoi(rm_value.addr);
	  tmp_win->flags |= STAYSONDESK_FLAG;
	  tflag |= STAYSONDESK_FLAG;
      }
      XrmDestroyDatabase (db);
      db = NULL;
  }

  if(!PlaceWindow(tmp_win, tflag, Desk))
    return NULL;

  /*
   * Make sure the client window still exists.  We don't want to leave an
   * orphan frame window if it doesn't.  Since we now have the server
   * grabbed, the window can't disappear later without having been
   * reparented, so we'll get a DestroyNotify for it.  We won't have
   * gotten one for anything up to here, however.
   */
  XGrabServer(dpy);
  if(XGetGeometry(dpy, w, &JunkRoot, &JunkX, &JunkY,
		  &JunkWidth, &JunkHeight,
		  &JunkBW,  &JunkDepth) == 0)
    {
      free((char *)tmp_win);
      XUngrabServer(dpy);
      return(NULL);
    }

  XSetWindowBorderWidth (dpy, tmp_win->w,0);
  XGetWindowProperty (dpy, tmp_win->w, XA_WM_ICON_NAME, 0L, 200L, False,
		      XA_STRING, &actual_type, &actual_format, &nitems,
			  &bytesafter,(unsigned char **)&tmp_win->icon_name);
  if(tmp_win->icon_name==(char *)NULL)
    tmp_win->icon_name = tmp_win->name;

  tmp_win->flags &= ~ICONIFIED;
  tmp_win->flags &= ~ICON_UNMAPPED;
  tmp_win->flags &= ~MAXIMIZED;


  tmp_win->TextPixel = Scr.StdColors.fore;
  tmp_win->ReliefPixel = Scr.StdRelief.fore;
  tmp_win->ShadowPixel = Scr.StdRelief.back;
  tmp_win->BackPixel = Scr.StdColors.back;

  if(tmp_win->flags & STICKY)
    {
      tmp_win->BackPixel = Scr.StickyColors.back;
      tmp_win->ShadowPixel = Scr.StickyRelief.back;
      tmp_win->ReliefPixel = Scr.StickyRelief.fore;
      tmp_win->TextPixel = Scr.StickyColors.fore;
    }
  if(forecolor != NULL)
    {
      XColor color;

      if((XParseColor (dpy, Scr.FvwmRoot.attr.colormap, forecolor, &color))
	 &&(XAllocColor (dpy, Scr.FvwmRoot.attr.colormap, &color)))
	{
	  tmp_win->TextPixel = color.pixel; 
	}
    }
  if(backcolor != NULL)
    {
      XColor color;

      if((XParseColor (dpy, Scr.FvwmRoot.attr.colormap,backcolor, &color))
	 &&(XAllocColor (dpy, Scr.FvwmRoot.attr.colormap, &color)))

	{
	  tmp_win->BackPixel = color.pixel; 
	}
      tmp_win->ShadowPixel = GetShadow(tmp_win->BackPixel);
      tmp_win->ReliefPixel = GetHilite(tmp_win->BackPixel);
    }


  /* add the window into the fvwm list */
  tmp_win->next = Scr.FvwmRoot.next;
  if (Scr.FvwmRoot.next != NULL)
    Scr.FvwmRoot.next->prev = tmp_win;
  tmp_win->prev = &Scr.FvwmRoot;
  Scr.FvwmRoot.next = tmp_win;
  
  /* create windows */
  tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->bw;
  tmp_win->frame_y = tmp_win->attr.y + tmp_win->old_bw - tmp_win->bw;

  tmp_win->frame_width = tmp_win->attr.width+2*tmp_win->boundary_width;
  tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height+
    2*tmp_win->boundary_width;


  valuemask = CWBorderPixel | CWCursor | CWEventMask; 
  if(Scr.d_depth < 2)
    {
      attributes.background_pixmap = Scr.light_gray_pixmap ;
      if(tmp_win->flags & STICKY)
	attributes.background_pixmap = Scr.sticky_gray_pixmap;
      valuemask |= CWBackPixmap;
    }
  else
    {
      attributes.background_pixel = tmp_win->BackPixel;
      valuemask |= CWBackPixel;
    }

  attributes.border_pixel = tmp_win->ShadowPixel;

  attributes.cursor = Scr.FvwmCursors[DEFAULT];
  attributes.event_mask = (SubstructureRedirectMask | ButtonPressMask | 
			   ButtonReleaseMask |EnterWindowMask | 
			   LeaveWindowMask |ExposureMask);
  if(Scr.flags & SaveUnders)
    {
      valuemask |= CWSaveUnder;
      attributes.save_under = TRUE;
    }
  /* What the heck, we'll always reparent everything from now on! */
  tmp_win->frame = 
    XCreateWindow (dpy, Scr.Root, tmp_win->frame_x,tmp_win->frame_y, 
		   tmp_win->frame_width, tmp_win->frame_height,
		   tmp_win->bw,CopyFromParent, InputOutput,
		   CopyFromParent, valuemask, &attributes);

  attributes.save_under = FALSE;

  /* Thats not all, we'll double-reparent the window ! */
  attributes.cursor = Scr.FvwmCursors[DEFAULT];
  tmp_win->Parent = 
    XCreateWindow (dpy, tmp_win->frame,
		   tmp_win->boundary_width, 
		   tmp_win->boundary_width+tmp_win->title_height,
		   (tmp_win->frame_width - 2*tmp_win->boundary_width),
		   (tmp_win->frame_height - 2*tmp_win->boundary_width - 
		    tmp_win->title_height),tmp_win->bw, CopyFromParent,
		   InputOutput,CopyFromParent, valuemask,&attributes);  

  if(Scr.flags & BackingStore)
    {
      valuemask |= CWBackingStore;
      attributes.backing_store = WhenMapped;
    }

#ifndef NO_PAGER
  if(Scr.Pager_w)
    {
      /* Create the pager_view window even if we're sticky, in case the
       * user unsticks the window */
      attributes.event_mask = ExposureMask;
      tmp_win->pager_view= XCreateWindow (dpy, Scr.Pager_w, -10, -10, 2, 2, 1,
					  CopyFromParent, InputOutput,
					  CopyFromParent, valuemask,
					  &attributes);
      XMapRaised(dpy,tmp_win->pager_view);
    }
#endif
  
  attributes.event_mask = (ButtonPressMask|ButtonReleaseMask|ExposureMask|
			   EnterWindowMask|LeaveWindowMask);
  tmp_win->title_x = tmp_win->title_y = 0;
  tmp_win->title_w = 0;
  tmp_win->title_width = tmp_win->frame_width - 2*tmp_win->corner_width
    - 3 + tmp_win->bw;
  if(tmp_win->title_width < 1)
    tmp_win->title_width = 1;
  if(tmp_win->flags & BORDER)
    {
      /* Just dump the windows any old place and left SetupFrame take
       * care of the mess */
      for(i=0;i<4;i++)
	{
	  attributes.cursor = Scr.FvwmCursors[TOP_LEFT+i];	  
	  tmp_win->corners[i] = 
	    XCreateWindow (dpy, tmp_win->frame, 0,0,
			   tmp_win->corner_width, tmp_win->corner_width,
			   0, CopyFromParent,InputOutput,
			   CopyFromParent, valuemask,&attributes);
	}
    }

  if (tmp_win->flags & TITLE)
    {
      tmp_win->title_x = tmp_win->boundary_width +tmp_win->title_height+1; 
      tmp_win->title_y = tmp_win->boundary_width;
      attributes.cursor = Scr.FvwmCursors[TITLE_CURSOR];
      tmp_win->title_w = 
	XCreateWindow (dpy, tmp_win->frame, tmp_win->title_x, tmp_win->title_y,
		       tmp_win->title_width, tmp_win->title_height,0,
		       CopyFromParent, InputOutput, CopyFromParent,
		       valuemask,&attributes);
      attributes.cursor = Scr.FvwmCursors[SYS];
      for(i=4;i>=0;i--)
	{
	  if((i<Scr.nr_left_buttons)&&(tmp_win->left_w[i] > 0))
	    {
	      tmp_win->left_w[i] =
		XCreateWindow (dpy, tmp_win->frame, tmp_win->title_height*i, 0,
			       tmp_win->title_height, tmp_win->title_height, 0,
			       CopyFromParent, InputOutput,
			       CopyFromParent, valuemask, &attributes);
	    }
	  else
	    tmp_win->left_w[i] = None;

	  if((i<Scr.nr_right_buttons)&&(tmp_win->right_w[i] >0))
	    {
	      tmp_win->right_w[i] =
		XCreateWindow (dpy, tmp_win->frame, 
			       tmp_win->title_width-
			       tmp_win->title_height*(i+1),
			       0, tmp_win->title_height,
			       tmp_win->title_height, 
			       0, CopyFromParent, InputOutput,
			       CopyFromParent, valuemask, &attributes);
	    }
	  else
	    tmp_win->right_w[i] = None;
	}
    }

  if(tmp_win->flags & BORDER)
    {
      for(i=0;i<4;i++)
	{
	  attributes.cursor = Scr.FvwmCursors[TOP+i];
	  tmp_win->sides[i] = 
	    XCreateWindow (dpy, tmp_win->frame, 0, 0, tmp_win->boundary_width,
			   tmp_win->boundary_width, 0, CopyFromParent,
			   InputOutput, CopyFromParent,valuemask,
			   &attributes);
	}
    }

  XMapSubwindows (dpy, tmp_win->frame);
  XRaiseWindow(dpy,tmp_win->Parent);
  XReparentWindow(dpy, tmp_win->w, tmp_win->Parent,0,0);

  valuemask = (CWEventMask | CWDontPropagate);
  attributes.event_mask = (StructureNotifyMask | PropertyChangeMask |
			   VisibilityChangeMask |  EnterWindowMask | 
			   LeaveWindowMask | 
			   ColormapChangeMask | FocusChangeMask);

#ifndef NO_PAGER
  if(tmp_win->w == Scr.Pager_w)
    {
      Scr.FvwmPager = tmp_win;
      attributes.event_mask |=ButtonPressMask|ButtonReleaseMask|ExposureMask
	|ButtonMotionMask;
      attributes.do_not_propagate_mask = ButtonPressMask;
    }
  else
#endif
    attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
  if(Scr.flags & AppsBackingStore)
    {
      valuemask |= CWBackingStore;
      attributes.backing_store = WhenMapped;
    }
  XChangeWindowAttributes (dpy, tmp_win->w, valuemask, &attributes);
  if ( XGetWMName(dpy, tmp_win->w, &text_prop) != 0 ) 
    tmp_win->name = (char *)text_prop.value ;
  else
    tmp_win->name = NoName;
  
#ifndef NO_PAGER
  if(tmp_win->w != Scr.Pager_w)
#endif
    XAddToSaveSet(dpy, tmp_win->w);

  /*
   * Reparenting generates an UnmapNotify event, followed by a MapNotify.
   * Set the map state to FALSE to prevent a transition back to
   * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
   * again in HandleMapNotify.
   */
  tmp_win->flags &= ~MAPPED;
  width =  tmp_win->frame_width;
  tmp_win->frame_width = 0;
  height = tmp_win->frame_height;
  tmp_win->frame_height = 0;
  SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y,width,height, True);

  /* wait until the window is iconified and the icon window is mapped
   * before creating the icon window 
   */
  tmp_win->icon_w = None;
  GrabButtons(tmp_win);
  GrabKeys(tmp_win);

  XSaveContext(dpy, tmp_win->w, FvwmContext, (caddr_t) tmp_win);  
  XSaveContext(dpy, tmp_win->frame, FvwmContext, (caddr_t) tmp_win);
  XSaveContext(dpy, tmp_win->Parent, FvwmContext, (caddr_t) tmp_win);
  if (tmp_win->flags & TITLE)
    {
      XSaveContext(dpy, tmp_win->title_w, FvwmContext, (caddr_t) tmp_win);
      for(i=0;i<Scr.nr_left_buttons;i++)
	XSaveContext(dpy, tmp_win->left_w[i], FvwmContext, (caddr_t) tmp_win);
      for(i=0;i<Scr.nr_right_buttons;i++)
	if(tmp_win->right_w[i] != None)
	  XSaveContext(dpy, tmp_win->right_w[i], FvwmContext, 
		       (caddr_t) tmp_win);
    }
  if (tmp_win->flags & BORDER)
    {
      for(i=0;i<4;i++)
	{
	  XSaveContext(dpy, tmp_win->sides[i], FvwmContext, (caddr_t) tmp_win);
	  XSaveContext(dpy,tmp_win->corners[i],FvwmContext, (caddr_t) tmp_win);
	}
    }
  RaiseWindow(tmp_win);
  KeepOnTop();
  XUngrabServer(dpy);

  XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
  XTranslateCoordinates(dpy,tmp_win->frame,Scr.Root,JunkX,JunkY,
			&a,&b,&JunkChild);
  tmp_win->xdiff -= a;
  tmp_win->ydiff -= b;
  if(Scr.flags & ClickToFocus)
    {
      /* need to grab all buttons for window that we are about to
       * unhighlight */
      for(i=0;i<3;i++)
	if(Scr.buttons2grab & (1<<i))
	  {
	    XGrabButton(dpy,(i+1),0,tmp_win->frame,True,
			ButtonPressMask, GrabModeSync,GrabModeAsync,None,
			Scr.FvwmCursors[SYS]);
	    XGrabButton(dpy,(i+1),LockMask,tmp_win->frame,True,
			ButtonPressMask, GrabModeSync,GrabModeAsync,None,
			Scr.FvwmCursors[SYS]);
	  }
    }
  BroadcastConfig(M_ADD_WINDOW,tmp_win);

  BroadcastName(M_WINDOW_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->name);
  BroadcastName(M_ICON_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->icon_name);
  BroadcastName(M_RES_CLASS,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->class.res_class);
  BroadcastName(M_RES_NAME,tmp_win->w,tmp_win->frame,
		(unsigned long)tmp_win,tmp_win->class.res_name);

  FetchWmProtocols (tmp_win);
  FetchWmColormapWindows (tmp_win);
  if(!(XGetWindowAttributes(dpy,tmp_win->w,&(tmp_win->attr))))
    tmp_win->attr.colormap = Scr.FvwmRoot.attr.colormap;
  if(NeedToResizeToo)
    {
      XWarpPointer(dpy, Scr.Root, Scr.Root, 0, 0, Scr.MyDisplayWidth, 
		   Scr.MyDisplayHeight, 
		   tmp_win->frame_x + (tmp_win->frame_width>>1), 
		   tmp_win->frame_y + (tmp_win->frame_height>>1));
      resize_window(tmp_win->w,tmp_win,0,0,0,0);
    }
  InstallWindowColormaps(colormap_win);
  return (tmp_win);
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabButtons - grab needed buttons for the window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabButtons(FvwmWindow *tmp_win)
{
  MouseButton *MouseEntry;

  MouseEntry = Scr.MouseButtonRoot;
  while(MouseEntry != (MouseButton *)0)
    {
      if((MouseEntry->func != (int)0)&&(MouseEntry->Context & C_WINDOW))
	{
	  if(MouseEntry->Button >0)
	    {
	      XGrabButton(dpy, MouseEntry->Button, MouseEntry->Modifier, 
			  tmp_win->w, 
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None, 
			  Scr.FvwmCursors[DEFAULT]);
	      if(MouseEntry->Modifier != AnyModifier)
		{
		  XGrabButton(dpy, MouseEntry->Button, 
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w, 
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None, 
			      Scr.FvwmCursors[DEFAULT]);
		}
	    }
	  else
	    {
	      XGrabButton(dpy, 1, MouseEntry->Modifier, 
			  tmp_win->w, 
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None, 
			  Scr.FvwmCursors[DEFAULT]);
	      XGrabButton(dpy, 2, MouseEntry->Modifier, 
			  tmp_win->w, 
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None, 
			  Scr.FvwmCursors[DEFAULT]);
	      XGrabButton(dpy, 3, MouseEntry->Modifier, 
			  tmp_win->w, 
			  True, ButtonPressMask | ButtonReleaseMask,
			  GrabModeAsync, GrabModeAsync, None, 
			  Scr.FvwmCursors[DEFAULT]);
	      if(MouseEntry->Modifier != AnyModifier)
		{
		  XGrabButton(dpy, 1,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w, 
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None, 
			      Scr.FvwmCursors[DEFAULT]);
		  XGrabButton(dpy, 2,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w, 
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None, 
			      Scr.FvwmCursors[DEFAULT]);
		  XGrabButton(dpy, 3,
			      (MouseEntry->Modifier | LockMask),
			      tmp_win->w, 
			      True, ButtonPressMask | ButtonReleaseMask,
			      GrabModeAsync, GrabModeAsync, None, 
			      Scr.FvwmCursors[DEFAULT]);
		}
	    }
	}
      MouseEntry = MouseEntry->NextButton;
    }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabKeys - grab needed keys for the window
 *
 *  Inputs:
 *	tmp_win - the fvwm window structure to use
 *
 ***********************************************************************/
void GrabKeys(FvwmWindow *tmp_win)
{
  FuncKey *tmp;
  for (tmp = Scr.FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
      if(tmp->cont & (C_WINDOW|C_TITLE|C_RALL|C_LALL|C_SIDEBAR))
	{
	  XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->frame, True,
		   GrabModeAsync, GrabModeAsync);
	  if(tmp->mods != AnyModifier)
	    {
	      XGrabKey(dpy, tmp->keycode, tmp->mods|LockMask, 
		       tmp_win->frame, True,
		       GrabModeAsync, GrabModeAsync);	      
	    }
	}
    }
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	FetchWMProtocols - finds out which protocols the window supports
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void FetchWmProtocols (FvwmWindow *tmp)
{
  unsigned long flags = 0L;
  Atom *protocols = NULL, *ap;
  int i, n;
  Atom atype;
  int aformat;
  unsigned long bytes_remain,nitems;

  if(tmp == NULL) return;
  /* First, try the Xlib function to read the protocols.
   * This is what Twm uses. */
  if (XGetWMProtocols (dpy, tmp->w, &protocols, &n)) 
    {
      for (i = 0, ap = protocols; i < n; i++, ap++) 
	{
	  if (*ap == (Atom)_XA_WM_TAKE_FOCUS) flags |= DoesWmTakeFocus;
	  if (*ap == (Atom)_XA_WM_DELETE_WINDOW) flags |= DoesWmDeleteWindow;
	}
      if (protocols) XFree ((char *) protocols);
    }
  else
    {
      /* Next, read it the hard way. mosaic from Coreldraw needs to 
       * be read in this way. */
      if ((XGetWindowProperty(dpy, tmp->w, _XA_WM_PROTOCOLS, 0L, 10L, False,
			      _XA_WM_PROTOCOLS, &atype, &aformat, &nitems,
			      &bytes_remain,
			      (unsigned char **)&protocols))==Success)
	{
	  for (i = 0, ap = protocols; i < nitems; i++, ap++) 
	    {
	      if (*ap == (Atom)_XA_WM_TAKE_FOCUS) flags |= DoesWmTakeFocus;
	      if (*ap == (Atom)_XA_WM_DELETE_WINDOW) flags |= DoesWmDeleteWindow;
	    }
	  if (protocols) XFree ((char *) protocols);
	}
    }
  tmp->flags |= flags;
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	GetWindowSizeHints - gets application supplied size info
 *
 *  Inputs:
 *	tmp - the fvwm window structure to use
 *
 ***********************************************************************/
void GetWindowSizeHints(FvwmWindow *tmp)
{
  long supplied = 0;

  if (!XGetWMNormalHints (dpy, tmp->w, &tmp->hints, &supplied))
    tmp->hints.flags = 0;

  /* Beat up our copy of the hints, so that all important field are
   * filled in! */
  if (tmp->hints.flags & PResizeInc) 
    {
      if (tmp->hints.width_inc == 0) tmp->hints.width_inc = 1;
      if (tmp->hints.height_inc == 0) tmp->hints.height_inc = 1;
    }
  else
    {
      tmp->hints.width_inc = 1;
      tmp->hints.height_inc = 1;
    }
  
  /*
   * ICCCM says that PMinSize is the default if no PBaseSize is given,
   * and vice-versa.
   */

  if(!(tmp->hints.flags & PBaseSize))
    {
      if(tmp->hints.flags & PMinSize)
	{
	  tmp->hints.base_width = tmp->hints.min_width;
	  tmp->hints.base_height = tmp->hints.min_height;      
	}
      else
	{
	  tmp->hints.base_width = 0;
	  tmp->hints.base_height = 0;
	}
    }
  if(!(tmp->hints.flags & PMinSize))
    {
      tmp->hints.min_width = tmp->hints.base_width;
      tmp->hints.min_height = tmp->hints.base_height;            
    }
  if(!(tmp->hints.flags & PMaxSize))
    {
      tmp->hints.max_width = MAX_WINDOW_WIDTH;
      tmp->hints.max_height = MAX_WINDOW_HEIGHT;
    }
  if(tmp->hints.max_width < tmp->hints.min_width)
    tmp->hints.max_width = MAX_WINDOW_WIDTH;    
  if(tmp->hints.max_height < tmp->hints.min_height)
    tmp->hints.max_height = MAX_WINDOW_HEIGHT;    

  /* Zero width/height windows are bad news! */
  if(tmp->hints.min_height <= 0)
    tmp->hints.min_height = 1;
  if(tmp->hints.min_width <= 0)
    tmp->hints.min_width = 1;
  
  if(!(tmp->hints.flags & PWinGravity))
    {
      tmp->hints.win_gravity = NorthWestGravity;
      tmp->hints.flags |= PWinGravity;
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	LookInList - look through a list for a window name, or class
 *
 *  Returned Value:
 *	the ptr field of the list structure or NULL if the name 
 *	or class was not found in the list
 *
 *  Inputs:
 *	list	- a pointer to the head of a list
 *	name	- a pointer to the name to look for
 *	class	- a pointer to the class to look for
 *
 ***********************************************************************/
unsigned long LookInList(name_list *list, char *name, XClassHint *class, 
			 char **value, int *Desk, int *border_width,
			 int *resize_width, char **forecolor, char **backcolor,
                         unsigned long * buttons)
{
  name_list *nptr;
  unsigned long retval = 0;

  *value = NULL;
  *forecolor = NULL;
  *backcolor = NULL;
  *Desk = 0;
  *buttons = 0;
  /* look for the name first */
  for (nptr = list; nptr != NULL; nptr = nptr->next)
    {
      if (class)
	{
	  /* first look for the res_class  (lowest priority) */
	  if (matchWildcards(nptr->name,class->res_class) == TRUE)
	    {
	      if(nptr->value != NULL)*value = nptr->value;
	      if(nptr->off_flags & STAYSONDESK_FLAG)
		*Desk = nptr->Desk;
	      if(nptr->off_flags & BW_FLAG)
		*border_width = nptr->border_width;
	      if(nptr->off_flags & FORE_COLOR_FLAG)
		*forecolor = nptr->ForeColor;
	      if(nptr->off_flags & BACK_COLOR_FLAG)
		*backcolor = nptr->BackColor;
	      if(nptr->off_flags & NOBW_FLAG)
		*resize_width = nptr->resize_width;
	      retval |= nptr->off_flags;
	      retval &= ~(nptr->on_flags);
              *buttons |= nptr->off_buttons;
              *buttons &= ~(nptr->on_buttons);
	    }

	  /* look for the res_name next */
	  if (matchWildcards(nptr->name,class->res_name) == TRUE)
	    {
	      if(nptr->value != NULL)*value = nptr->value;
	      if(nptr->off_flags & STAYSONDESK_FLAG)
		*Desk = nptr->Desk;
	      if(nptr->off_flags & FORE_COLOR_FLAG)
		*forecolor = nptr->ForeColor;
	      if(nptr->off_flags & BACK_COLOR_FLAG)
		*backcolor = nptr->BackColor;
	      if(nptr->off_flags & BW_FLAG)
		*border_width = nptr->border_width;
	      if(nptr->off_flags & NOBW_FLAG)
		*resize_width = nptr->resize_width;
	      retval |= nptr->off_flags;
	      retval &= ~(nptr->on_flags);
              *buttons |= nptr->off_buttons;
              *buttons &= ~(nptr->on_buttons);
	    }
	}
      /* finally, look for name matches */
      if (matchWildcards(nptr->name,name) == TRUE)
	{
	  if(nptr->value != NULL)*value = nptr->value;
	  if(nptr->off_flags & STAYSONDESK_FLAG)	   
	    *Desk = nptr->Desk;
	  if(nptr->off_flags & FORE_COLOR_FLAG)
	    *forecolor = nptr->ForeColor;
	  if(nptr->off_flags & BACK_COLOR_FLAG)
	    *backcolor = nptr->BackColor;
	  if(nptr->off_flags & BW_FLAG)
	    *border_width = nptr->border_width;
	  if(nptr->off_flags & NOBW_FLAG)
	    *resize_width = nptr->resize_width;
	  retval |= nptr->off_flags;
	  retval &= ~(nptr->on_flags);
          *buttons |= nptr->off_buttons;
          *buttons &= ~(nptr->on_buttons);
	}
    }
  return retval;
}





