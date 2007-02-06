/***************************************************************************
 * FvwmBanner                                                           
 *                                                                           
 *  Show Fvwm Banner
 *                                                                           
 ***************************************************************************/

#include "../../configure.h"

#ifdef ISC
#include <sys/bsdtypes.h> /* Saul */
#endif

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#if defined ___AIX || defined _AIX || defined __QNX__ || defined ___AIXV3 || defined AIXV3 || defined _SEQUENT_
#include <sys/select.h>
#endif

#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>

#include <X11/xpm.h>

#include "fvwm3.xpm"

#ifdef BROKEN_SUN_HEADERS
#include "../../fvwm/sun_headers.h"
#endif

#include "../../libs/fvwmlib.h"     

#ifdef NEEDS_ALPHA_HEADER
#include "../../fvwm/alpha_header.h"
#endif /* NEEDS_ALPHA_HEADER */

typedef struct _XpmIcon {
    Pixmap pixmap;
    Pixmap mask;
    XpmAttributes attributes;
}        XpmIcon;

/**************************************************************************
 * A few function prototypes 
 **************************************************************************/
void RedrawWindow(void);
void GetXPM(void);
Pixel GetColor(char *name);
void change_window_name(char *str);
int flush_expose (Window w);

time_t t0 = -1;

XpmIcon view;
Window win;

Display *dpy;			/* which display are we talking to */
Window Root;
int screen;
int x_fd;
int d_depth;
int ScreenWidth, ScreenHeight;
XSizeHints mysizehints;
Pixel back_pix, fore_pix;
GC NormalGC;
static Atom wm_del_win;

#define MW_EVENTS   (ExposureMask | ButtonReleaseMask)

/****************************************************************************
 *
 * Creates an icon window as needed
 *
 ****************************************************************************/
int main(int argc, char **argv)
{
  char *display_name = NULL;
  int retval;
  XGCValues gcv;
  unsigned long gcm;
  XEvent Event;
  fd_set in_fdset;
  int fd_width ;
  time_t t;
  struct itimerval value;
  int fd[2];

  fd_width = GetFdWidth();

  if(argc>=3)
    {
      /* sever our connection with fvwm, if we have one. */
      fd[0] = atoi(argv[1]);
      fd[1] = atoi(argv[2]);

      if(fd[0]>0)close(fd[0]);
      if(fd[1]>0)close(fd[1]);
    }
  /* Open the display */
  if (!(dpy = XOpenDisplay(display_name))) 
    {
      fprintf(stderr,"FvwmBanner: can't open display %s",
	      XDisplayName(display_name));
      exit (1);
    }
  screen= DefaultScreen(dpy);
  Root = RootWindow(dpy, screen);
  d_depth = DefaultDepth(dpy, screen);
  x_fd = XConnectionNumber(dpy);

  ScreenHeight = DisplayHeight(dpy,screen);
  ScreenWidth = DisplayWidth(dpy,screen);

  /* Get the xpm banner */
  GetXPM();

  /* Create a window to hold the banner */
  mysizehints.flags=
    USSize|USPosition|PWinGravity|PResizeInc|PBaseSize|PMinSize|PMaxSize;
  /* subtract one for the right/bottom border */
  mysizehints.width = view.attributes.width;
  mysizehints.height=view.attributes.height;
  mysizehints.width_inc = 1;
  mysizehints.height_inc = 1;
  mysizehints.base_height = mysizehints.height;
  mysizehints.base_width = mysizehints.width;
  mysizehints.min_height = mysizehints.height;
  mysizehints.min_width = mysizehints.width;
  mysizehints.max_height = mysizehints.height;
  mysizehints.max_width = mysizehints.width;
  mysizehints.win_gravity = NorthWestGravity;

  mysizehints.x = (ScreenWidth - view.attributes.width)/2;
  mysizehints.y = (ScreenHeight - view.attributes.height)/2;

  back_pix = GetColor("white");
  fore_pix = GetColor("black");

  win = XCreateSimpleWindow(dpy,Root,mysizehints.x,mysizehints.y,
				 mysizehints.width,mysizehints.height,
				 1,fore_pix,back_pix);

  /* Set assorted info for the window */
  XSetTransientForHint(dpy,win,Root);
  wm_del_win = XInternAtom(dpy,"WM_DELETE_WINDOW",False);
  XSetWMProtocols(dpy,win,&wm_del_win,1);

  XSetWMNormalHints(dpy,win,&mysizehints);
  XSelectInput(dpy,win,MW_EVENTS);
  change_window_name("FvwmBanner");

  /* Create a GC for drawing */
  gcm = GCForeground|GCBackground;
  gcv.foreground = fore_pix;
  gcv.background = back_pix;
  NormalGC = XCreateGC(dpy, Root, gcm, &gcv);  

#ifdef SHAPE
  if(view.mask != None)
    XShapeCombineMask(dpy, win, ShapeBounding,0,0,view.mask, ShapeSet);
#endif

  XMapWindow(dpy,win);

  /* Display the window */
  while(1)
    {
      FD_ZERO(&in_fdset);

      FD_SET(x_fd,&in_fdset);

      t=time(0);
      if((t0>0)&&(t-t0) >= 5)
	{
	  XDestroyWindow(dpy,win);
	  XSync(dpy,0);
	  exit(0);
	}

      value.it_value.tv_usec = 0;
      if(t0>0)
	value.it_value.tv_sec = t-t0;
      else
	value.it_value.tv_sec = 1;	

      if(!XPending(dpy))
#ifdef __hpux
	retval=select(fd_width,(int *)&in_fdset, 0, 0, &value.it_value);
#else
	retval=select(fd_width,&in_fdset, 0, 0, &value.it_value);
#endif
      if(FD_ISSET(x_fd, &in_fdset))
	{
	  /* read a packet */
	  XNextEvent(dpy,&Event);
	  switch(Event.type)
	    {
	    case Expose:
	      if(Event.xexpose.count == 0)
	    RedrawWindow();
	      break;
	      
	    case ButtonRelease:
	      exit(0);
	    case ClientMessage:
	      if (Event.xclient.format==32 && Event.xclient.data.l[0]==wm_del_win)
		exit(0);
	    default:
	      break;      
	    }
	}
    }
  return 0;
}

/****************************************************************************
 *
 * Draws the icon window
 *
 ****************************************************************************/
void RedrawWindow(void)
{
  flush_expose (win);
  if(t0==-1)
    t0 = time(0);
  XCopyArea(dpy,view.pixmap,win,NormalGC,
	    0,0,view.attributes.width, view.attributes.height,0,0);

}


/****************************************************************************
 *
 * Looks for a color XPM icon file
 *
 ****************************************************************************/
void GetXPM(void)
{
  view.attributes.valuemask |= XpmReturnPixels;
  view.attributes.valuemask |= XpmReturnExtensions;

  if(XpmCreatePixmapFromData(dpy, Root, fvwm2_xpm,
			     &view.pixmap, &view.mask,
			     &view.attributes)!=XpmSuccess)
    exit(1);
}

void nocolor(char *a, char *b)
{
 fprintf(stderr,"FvwmBanner: can't %s %s\n", a,b);
}


/****************************************************************************
 * 
 * Loads a single color
 *
 ****************************************************************************/ 
Pixel GetColor(char *name)
{
  XColor color;
  XWindowAttributes attributes;

  XGetWindowAttributes(dpy,Root,&attributes);
  color.pixel = 0;
   if (!XParseColor (dpy, attributes.colormap, name, &color)) 
     {
       nocolor("parse",name);
     }
   else if(!XAllocColor (dpy, attributes.colormap, &color)) 
     {
       nocolor("alloc",name);
     }
  return color.pixel;
}


/**************************************************************************
 *  Change the window name displayed in the title bar.
 **************************************************************************/
void change_window_name(char *str)
{
  XTextProperty name;
  
  if (XStringListToTextProperty(&str,1,&name) == 0) 
    {
      fprintf(stderr,"FvwmBanner: cannot allocate window name");
      return;
    }
  XSetWMName(dpy,win,&name);
  XSetWMIconName(dpy,win,&name);
  XFree(name.value);
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


