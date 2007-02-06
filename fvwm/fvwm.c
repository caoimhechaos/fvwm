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
 * fvwm - "F? Virtual Window Manager"
 ***********************************************************************/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "screen.h"
#include "parse.h"
#include <X11/Xproto.h>
#include <X11/Xatom.h>
/* need to get prototype for XrmUniqueQuark for XUniqueContext call */
#include <X11/Xresource.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif /* SHAPE */

#if defined (sparc) && defined (SVR4)
/* Solaris has sysinfo instead of gethostname.  */
#include <sys/systeminfo.h>
#endif

#define MAXHOSTNAME 255

ScreenInfo Scr;		        /* structures for the screen */
Display *dpy;			/* which display are we talking to */
extern char *config_file;

XErrorHandler CatchRedirectError(Display *, XErrorEvent *);
XErrorHandler FvwmErrorHandler(Display *, XErrorEvent *);
void newhandler(int sig);
void CreateCursors(void);
void NoisyExit(int);
void ChildDied(int nonsense);
void SaveDesktopState(void);

XContext FvwmContext;		/* context for fvwm windows */
XContext MenuContext;		/* context for fvwm menus */

XClassHint NoClass;		/* for applications with no class */

int JunkX = 0, JunkY = 0;
Window JunkRoot, JunkChild;		/* junk window */
unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

/* assorted gray bitmaps for decorative borders */
#define g_width 2
#define g_height 2
static char g_bits[] = {0x02, 0x01};

#define l_g_width 4
#define l_g_height 2
static char l_g_bits[] = {0x08, 0x02};
Bool debugging = False,PPosOverride;

#define s_g_width 4
#define s_g_height 4
static char s_g_bits[] = {0x01, 0x02, 0x04, 0x08};
char **g_argv;

#ifdef M4
int  m4_enable;			/* use m4? */
int  m4_prefix;			/* Do GNU m4 prefixing (-P) */
char m4_options[BUFSIZ];	/* Command line options to m4 */
char m4_prog[BUFSIZ];		/* Name of the m4 program */
int  m4_default_quotes;		/* Use default m4 quotes */
char m4_startquote[16];		/* Left quote characters for m4 */
char m4_endquote[16];		/* Right quote characters for m4 */
#endif

#ifndef NO_PAGER
extern Pixel PagerForeColor;
#endif

#ifdef SHAPE
int ShapeEventBase, ShapeErrorBase;
#endif

long isIconicState = 0;
extern XEvent Event;
Bool Restarting = False;
int fd_width, x_fd;
char *display_name = NULL;

/***********************************************************************
 *
 *  Procedure:
 *	main - start of fvwm
 *
 ***********************************************************************
 */
void main(int argc, char **argv)
{
    unsigned long valuemask;	/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    void InternUsefulAtoms (void);
    void InitVariables(void);
    void enterAlarm(int);
    int i;
    extern int x_fd;
    int len;
    char *display_string;
    char message[255];
    char num[10];

    Bool single = False;
    Bool option_error = FALSE;

#ifdef M4
    /* Set the defaults for m4 processing */
     
    m4_enable = TRUE;
    m4_prefix = FALSE;
    strcpy(m4_prog, "m4");
    *m4_options = '\0';
    m4_default_quotes = 1;
    strcpy(m4_startquote, "`");
    strcpy(m4_endquote, "'");
#endif
    
    for (i = 1; i < argc; i++) 
      {
	if (mystrncasecmp(argv[i],"-debug",6)==0)
	  debugging = True;
	else if (mystrncasecmp(argv[i],"-s",2)==0)
	  {
	    single = True;
	  }
	else if (mystrncasecmp(argv[i],"-d",2)==0)
	  {
	    if (++i >= argc)
	      usage();
	    display_name = argv[i];
	  }
	else if (mystrncasecmp(argv[i],"-f",2)==0)
	 {
	    if (++i >= argc)
	      usage();
	    config_file = argv[i];
	  }
#ifdef M4
	else if (mystrncasecmp(argv[i], "-no-m4", 6) == 0)
	  {
	    m4_enable = FALSE;
	  }
	else if (mystrncasecmp(argv[i], "-m4-prefix", 10) == 0)
	  {	
	    m4_prefix = TRUE;
	  }
	else if (mystrncasecmp(argv[i],"-m4opt", 6) == 0)
	  {
	    if (++i < argc) 
	      {
		strcat(m4_options, argv[i]);
		strcat(m4_options, " ");	    
	      }
	  }
	else if (mystrncasecmp(argv[i], "-m4-squote", 6) == 0)
	  {
	    if (++i < argc)
	      {
		strcpy(m4_startquote, argv[i]);
		m4_default_quotes = 0;
	      }
	  }
	else if (mystrncasecmp(argv[i], "-m4-equote", 6) == 0)
	  {
	    if (++i < argc)
	      {
		strcpy(m4_endquote, argv[i]);
		m4_default_quotes = 0;
	      }
	  }
	else if (mystrncasecmp(argv[i], "-m4prog", 7) == 0)
	  {
	    if (++i < argc)
	      {
		strcpy(m4_prog, argv[i]);
	      }
	  }
#endif
	else if (mystrncasecmp(argv[i], "-version", 8) == 0)
	  {
	    fprintf(stderr, "Fvwm Version %s\n", PACKAGE_VERSION);
	  }
	else
	  {
	    fprintf(stderr, "fvwm:  Unknown option:  `%s'\n", argv[i]);
	    option_error = TRUE;
	  }
      }

    if (option_error)
      {
	usage();
      }
    
    g_argv = argv;

    newhandler (SIGINT);
    newhandler (SIGHUP);
    newhandler (SIGQUIT);
    newhandler (SIGTERM);
    signal (SIGUSR1, Restart);

    signal (SIGPIPE, DeadPipe);
    signal(SIGALRM,enterAlarm);

    ReapChildren();

    if (!(dpy = XOpenDisplay(display_name))) 
      {
	fvwm_err("can't open display %s", XDisplayName(display_name),
		 NULL,NULL);
	exit (1);
      }
    x_fd = XConnectionNumber(dpy);
    
    if (fcntl(x_fd, F_SETFD, 1) == -1) 
      {
	fvwm_err("close-on-exec failed",NULL,NULL,NULL);
	exit (1);
      }
    Scr.screen= DefaultScreen(dpy);
    Scr.NumberOfScreens = ScreenCount(dpy);

    if(!single)
      {
	for(i=0;i<Scr.NumberOfScreens;i++)
	  {
	    if(i!= Scr.screen)
	      {
		sprintf(message,"%s -d %s",argv[0],XDisplayString(dpy));
		len = strlen(message);
		message[len-1] = 0;
		sprintf(num,"%d",i);
		strcat(message,num);
		strcat(message," -s ");
		if(debugging)
		  strcat(message," -debug");
		strcat(message," -f ");
		strcat(message,config_file);
		strcat(message," &\n");
		system(message);
	      }
	  }
      }
	    

    /*  Add a DISPLAY entry to the environment, incase we were started
     * with fvwm -display term:0.0
     */
    len = strlen(XDisplayString(dpy));
    display_string = safemalloc(len+10);
    sprintf(display_string,"DISPLAY=%s",XDisplayString(dpy));
    putenv(display_string);
    /* Add a HOSTDISPLAY environment variable, which is the same as
     * DISPLAY, unless display = :0.0 or unix:0.0, in which case the full
     * host name will be used for ease in networking . */
    if(strncmp(display_string,"DISPLAY=:",9)==0)
      {
	char client[MAXHOSTNAME], *rdisplay_string;
	mygethostname(client,MAXHOSTNAME);
	rdisplay_string = safemalloc(len+14 + strlen(client));
	sprintf(rdisplay_string,"HOSTDISPLAY=%s:%s",client,&display_string[9]);
	putenv(rdisplay_string);
      }
    else if(strncmp(display_string,"DISPLAY=unix:",13)==0)
      {
	char client[MAXHOSTNAME], *rdisplay_string;
	mygethostname(client,MAXHOSTNAME);
	rdisplay_string = safemalloc(len+14 + strlen(client));
	sprintf(rdisplay_string,"HOSTDISPLAY=%s:%s",client,
		&display_string[13]);
	putenv(rdisplay_string);
      }
    else
      {
	char *rdisplay_string;
	rdisplay_string = safemalloc(len+14);
	sprintf(rdisplay_string,"HOSTDISPLAY=%s",XDisplayString(dpy));
	putenv(rdisplay_string);
      }

    Scr.Root = RootWindow(dpy, Scr.screen);
    if(Scr.Root == None) 
      {
	fvwm_err("Screen %d is not a valid screen",(char *)Scr.screen,
		 NULL,NULL);
	exit(1);
      }

#ifdef SHAPE
    XShapeQueryExtension(dpy, &ShapeEventBase, &ShapeErrorBase);
#endif /* SHAPE */

    InternUsefulAtoms ();

    /* Make sure property priority colors is empty */
    XChangeProperty (dpy, Scr.Root, _XA_MIT_PRIORITY_COLORS,
		     XA_CARDINAL, 32, PropModeReplace, NULL, 0);

    XSetErrorHandler((XErrorHandler)CatchRedirectError);
    XSelectInput(dpy, Scr.Root,
		 LeaveWindowMask| EnterWindowMask | PropertyChangeMask | 
		 SubstructureRedirectMask | KeyPressMask |
		 ButtonPressMask | ButtonReleaseMask  );
    XSync(dpy, 0);

    XSetErrorHandler((XErrorHandler)FvwmErrorHandler);
    CreateCursors();
    InitVariables();
    initModules();

    XGrabServer(dpy);

    Scr.gray_bitmap = 
      XCreateBitmapFromData(dpy,Scr.Root,g_bits, g_width,g_height);

    /* read config file, set up menus, colors, fonts */
#ifdef M4
    MakeMenus(display_name, m4_options);
#else
    MakeMenus(display_name, NULL);
#endif
    
    if(Scr.d_depth<2)
      {
	Scr.gray_pixmap = 
	XCreatePixmapFromBitmapData(dpy,Scr.Root,g_bits, g_width,g_height,
				    Scr.StdColors.fore,Scr.StdColors.back,
				    Scr.d_depth);	
	Scr.light_gray_pixmap = 
	XCreatePixmapFromBitmapData(dpy,Scr.Root,l_g_bits,l_g_width,l_g_height,
				    Scr.StdColors.fore,Scr.StdColors.back,
				    Scr.d_depth);
	Scr.sticky_gray_pixmap = 
	XCreatePixmapFromBitmapData(dpy,Scr.Root,s_g_bits,s_g_width,s_g_height,
				   Scr.StickyColors.fore,Scr.StickyColors.back,
				   Scr.d_depth);
      }

    /* create a window which will accept the keyboard focus when no other 
       windows have it */
    attributes.event_mask = KeyPressMask|FocusChangeMask;
    attributes.override_redirect = True;
    Scr.NoFocusWin=XCreateWindow(dpy,Scr.Root,-10, -10, 10, 10, 0, 0,
				 InputOnly,CopyFromParent,
				 CWEventMask|CWOverrideRedirect,
				 &attributes);
    XMapWindow(dpy, Scr.NoFocusWin);

    XSetInputFocus (dpy, Scr.NoFocusWin, RevertToParent, CurrentTime);
    Scr.TitleHeight=Scr.WindowFont.font->ascent+Scr.WindowFont.font->descent+3;

    XSync(dpy, 0);
    if(debugging)
      XSynchronize(dpy,1);

    Scr.SizeStringWidth = XTextWidth (Scr.StdFont.font,
				      " +8888 x +8888 ", 15);
    attributes.border_pixel = Scr.StdColors.fore;
    attributes.background_pixel = Scr.StdColors.back;
    attributes.bit_gravity = NorthWestGravity;
    valuemask = (CWBorderPixel | CWBackPixel | CWBitGravity);
    if(!(Scr.flags & MWMMenus))
      {
	Scr.SizeWindow = XCreateWindow (dpy, Scr.Root,
					0, 0, 
					(unsigned int)(Scr.SizeStringWidth +
						       SIZE_HINDENT*2),
					(unsigned int) (Scr.StdFont.height +
							SIZE_VINDENT*2),
					(unsigned int) 0, 0,
					(unsigned int) CopyFromParent,
					(Visual *) CopyFromParent,
					valuemask, &attributes);
      }
    else
      {
	Scr.SizeWindow = XCreateWindow (dpy, Scr.Root,
					Scr.MyDisplayWidth/2 - 
					(Scr.SizeStringWidth +
					 SIZE_HINDENT*2)/2,
					Scr.MyDisplayHeight/2 -
					(Scr.StdFont.height + 
					 SIZE_VINDENT*2)/2, 
					(unsigned int)(Scr.SizeStringWidth +
						       SIZE_HINDENT*2),
					(unsigned int) (Scr.StdFont.height +
							SIZE_VINDENT*2),
					(unsigned int) 0, 0,
					(unsigned int) CopyFromParent,
					(Visual *) CopyFromParent,
					valuemask, &attributes);
      }
#ifndef NON_VIRTUAL
    initPanFrames();
#endif
    CaptureAllWindows();
#ifndef NON_VIRTUAL
    checkPanFrames();
#endif
    XUngrabServer(dpy);
    MoveResizeViewPortIndicator();
    fd_width = GetFdWidth();

  
    if(Restarting)
      {
	if(Scr.RestartFunction != NULL)
	  ExecuteFunction(F_FUNCTION,NULL,None,NULL,&Event,C_ROOT,0,0,
			  0,0,Scr.RestartFunction,-1);
      }
    else
      {
	if(Scr.InitFunction != NULL)
	  ExecuteFunction(F_FUNCTION,NULL,None,NULL,&Event,C_ROOT,0,0,
			  0,0,Scr.InitFunction,-1);
      }

    HandleEvents();
    return;
}

/***********************************************************************
 *
 *  Procedure:
 *      CaptureAllWindows
 *
 *   Decorates all windows at start-up
 *
 ***********************************************************************/

void CaptureAllWindows(void)
{
  int i,j;
  unsigned int nchildren;
  Window root, parent, *children;

  PPosOverride = TRUE;

  if(!XQueryTree(dpy, Scr.Root, &root, &parent, &children, &nchildren))
    return;

  /*
   * weed out icon windows
   */
  for (i = 0; i < nchildren; i++) 
    {
      if (children[i]) 
	{
	  XWMHints *wmhintsp = XGetWMHints (dpy, children[i]);
	  if (wmhintsp) 
	    {
	      if (wmhintsp->flags & IconWindowHint) 
		{
		  for (j = 0; j < nchildren; j++) 
		    {
		      if (children[j] == wmhintsp->icon_window) 
			{
			  children[j] = None;
			  break;
			}
		    }
		}
	      XFree ((char *) wmhintsp);
	    }
	}
    }
  /*
   * map all of the non-override windows
   */
  
  for (i = 0; i < nchildren; i++)
    {
      if (children[i] && MappedNotOverride(children[i]))
	{
	  XUnmapWindow(dpy, children[i]);
	  Event.xmaprequest.window = children[i];
	  HandleMapRequest ();
	}
    }
  
  isIconicState = DontCareState;

  if(nchildren > 0)
    XFree((char *)children);

  /* after the windows already on the screen are in place,
   * don't use PPosition */
  PPosOverride = FALSE;
}

/***********************************************************************
 *
 *  Procedure:
 *	MappedNotOverride - checks to see if we should really
 *		put a fvwm frame on the window
 *
 *  Returned Value:
 *	TRUE	- go ahead and frame the window
 *	FALSE	- don't frame the window
 *
 *  Inputs:
 *	w	- the window to check
 *
 ***********************************************************************/

int MappedNotOverride(Window w)
{
  XWindowAttributes wa;
  Atom atype;
  int aformat;
  unsigned long nitems, bytes_remain;
  unsigned char *prop;

  isIconicState = DontCareState;

  if(!XGetWindowAttributes(dpy, w, &wa))
    return False;

  if(XGetWindowProperty(dpy,w,_XA_WM_STATE,0L,3L,False,_XA_WM_STATE,
			&atype,&aformat,&nitems,&bytes_remain,&prop)==Success)
    {
      if(prop != NULL)
	{
	  isIconicState = *(long *)prop;
	  XFree(prop);
	}
    }
#ifndef NO_PAGER
  if(w == Scr.Pager_w)
    return True;
#endif
  return (((isIconicState == IconicState)||(wa.map_state != IsUnmapped)) &&
	  (wa.override_redirect != True));
}


/***********************************************************************
 *
 *  Procedure:
 *	InternUsefulAtoms:
 *            Dont really know what it does
 *
 ***********************************************************************
 */
Atom _XA_MIT_PRIORITY_COLORS;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_STATE;
Atom _XA_WM_COLORMAP_WINDOWS;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_DELETE_WINDOW;
Atom _XA_WM_DESKTOP;
Atom _XA_MwmAtom;

void InternUsefulAtoms (void)
{
  /* 
   * Create priority colors if necessary.
   */
  _XA_MIT_PRIORITY_COLORS = XInternAtom(dpy, "_MIT_PRIORITY_COLORS", False);   
  _XA_WM_CHANGE_STATE = XInternAtom (dpy, "WM_CHANGE_STATE", False);
  _XA_WM_STATE = XInternAtom (dpy, "WM_STATE", False);
  _XA_WM_COLORMAP_WINDOWS = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
  _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);
  _XA_WM_TAKE_FOCUS = XInternAtom (dpy, "WM_TAKE_FOCUS", False);
  _XA_WM_DELETE_WINDOW = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
  _XA_WM_DESKTOP = XInternAtom (dpy, "WM_DESKTOP", False);
  _XA_MwmAtom=XInternAtom(dpy,"_MOTIF_WM_HINTS",False);
  return;
}

/***********************************************************************
 *
 *  Procedure:
 *	newhandler: Installs new signal handler
 *
 ************************************************************************/
void newhandler(int sig)
{
  if (signal (sig, SIG_IGN) != SIG_IGN)
    signal (sig, SigDone);
}


/*************************************************************************
 * Restart on a signal
 ************************************************************************/
void Restart(int nonsense)
{
  Done(1, *g_argv);
  SIGNAL_RETURN;
}

/***********************************************************************
 *
 *  Procedure:
 *	CreateCursors - Loads fvwm cursors
 *
 ***********************************************************************
 */
void CreateCursors(void)
{
  /* define cursors */
  Scr.FvwmCursors[POSITION] = XCreateFontCursor(dpy,XC_top_left_corner);
  Scr.FvwmCursors[DEFAULT] = XCreateFontCursor(dpy, XC_top_left_arrow);
  Scr.FvwmCursors[SYS] = XCreateFontCursor(dpy, XC_hand2);
  Scr.FvwmCursors[TITLE_CURSOR] = XCreateFontCursor(dpy, XC_top_left_arrow);
  Scr.FvwmCursors[MOVE] = XCreateFontCursor(dpy, XC_fleur);
  Scr.FvwmCursors[MENU] = XCreateFontCursor(dpy, XC_sb_left_arrow);
  Scr.FvwmCursors[WAIT] = XCreateFontCursor(dpy, XC_watch);
  Scr.FvwmCursors[SELECT] = XCreateFontCursor(dpy, XC_dot);
  Scr.FvwmCursors[DESTROY] = XCreateFontCursor(dpy, XC_pirate);
  Scr.FvwmCursors[LEFT] = XCreateFontCursor(dpy, XC_left_side);
  Scr.FvwmCursors[RIGHT] = XCreateFontCursor(dpy, XC_right_side);
  Scr.FvwmCursors[TOP] = XCreateFontCursor(dpy, XC_top_side);
  Scr.FvwmCursors[BOTTOM] = XCreateFontCursor(dpy, XC_bottom_side);
  Scr.FvwmCursors[TOP_LEFT] = XCreateFontCursor(dpy,XC_top_left_corner);
  Scr.FvwmCursors[TOP_RIGHT] = XCreateFontCursor(dpy,XC_top_right_corner);
  Scr.FvwmCursors[BOTTOM_LEFT] = XCreateFontCursor(dpy,XC_bottom_left_corner);
  Scr.FvwmCursors[BOTTOM_RIGHT] =XCreateFontCursor(dpy,XC_bottom_right_corner);
}

/***********************************************************************
 *
 *  Procedure:
 *	InitVariables - initialize fvwm variables
 *
 ************************************************************************/
void InitVariables(void)
{
  FvwmContext = XUniqueContext();
  MenuContext = XUniqueContext();
  NoClass.res_name = NoName;
  NoClass.res_class = NoName;

  Scr.d_depth = DefaultDepth(dpy, Scr.screen);
  Scr.FvwmRoot.w = Scr.Root;
  Scr.FvwmRoot.next = 0;
  XGetWindowAttributes(dpy,Scr.Root,&(Scr.FvwmRoot.attr));
  Scr.root_pushes = 0;
  Scr.pushed_window = &Scr.FvwmRoot;
  Scr.FvwmRoot.number_cmap_windows = 0;
  

  Scr.MyDisplayWidth = DisplayWidth(dpy, Scr.screen);
  Scr.MyDisplayHeight = DisplayHeight(dpy, Scr.screen);
    
  Scr.NoBoundaryWidth = 1;
  Scr.BoundaryWidth = BOUNDARY_WIDTH;
  Scr.CornerWidth = CORNER_WIDTH;
  Scr.Hilite = NULL;
  Scr.Focus = NULL;
  Scr.Ungrabbed = NULL;
  
  Scr.StdFont.font = NULL;
  Scr.StdFont.name = "fixed";
  Scr.WindowFont.name = "fixed";

  Scr.VScale = 32;
#ifndef NON_VIRTUAL  
  Scr.VxMax = 3;
  Scr.VyMax = 3;
#else
  Scr.VxMax = 1;
  Scr.VyMax = 1;
#endif
  Scr.Vx = Scr.Vy = 0;

  /* Sets the current desktop number to zero */
  /* Multiple desks are available even in non-virtual
   * compilations */
  {
    Atom atype;
    int aformat;
    unsigned long nitems, bytes_remain;
    unsigned char *prop;
    
    Scr.CurrentDesk = 0;
    if ((XGetWindowProperty(dpy, Scr.Root, _XA_WM_DESKTOP, 0L, 1L, True,
			    _XA_WM_DESKTOP, &atype, &aformat, &nitems,
			    &bytes_remain, &prop))==Success)
      {
	if(prop != NULL)
	  {
	    Restarting = True;
	    Scr.CurrentDesk = *(unsigned long *)prop;
	  }
      }
   }

  Scr.EdgeScrollX = Scr.EdgeScrollY = -100000;
  Scr.ScrollResistance = Scr.MoveResistance = 0;
  Scr.OpaqueSize = 5;
  Scr.ClickTime = 150;
  Scr.AutoRaiseDelay = 0;

  /* set major operating modes */
  Scr.flags = 0;
  Scr.NumBoxes = 0;

  Scr.randomx = Scr.randomy = 0;
  Scr.buttons2grab = 7;

#ifndef NO_PAGER
  Scr.PagerFont.name = NULL;
  Scr.PagerFont.height = 0;
  Scr.PagerFont.y = 0;
  Scr.FvwmPager = (FvwmWindow *)0;
  Scr.Pager_w = None;
  Scr.CPagerWin = None;
#endif
  Scr.InitFunction = NULL;
  Scr.RestartFunction = NULL;
  Scr.left_button_styles[0][0] = 55;
  Scr.left_button_styles[1][0] = 22;
  Scr.left_button_styles[0][1] = -35;
  Scr.left_button_styles[1][1] = -10;
  Scr.left_button_styles[0][2] = 1;
  Scr.left_button_styles[1][2] = 1;
  Scr.left_button_styles[0][3] = 35;
  Scr.left_button_styles[1][3] = 10;
  Scr.left_button_styles[0][4] = 56;
  Scr.left_button_styles[1][4] = 25;

  Scr.right_button_styles[0][0] = 55;
  Scr.right_button_styles[1][0] = 55;
  Scr.right_button_styles[0][1] = 22;
  Scr.right_button_styles[1][1] = 22;
  Scr.right_button_styles[0][2] = 1;
  Scr.right_button_styles[1][2] = 1;
  Scr.right_button_styles[0][3] = 50;
  Scr.right_button_styles[1][3] = 50;
  Scr.right_button_styles[0][4] = 28;
  Scr.right_button_styles[1][4] = 28;
  return;
}



/***********************************************************************
 *
 *  Procedure:
 *	Reborder - Removes fvwm border windows
 *
 ************************************************************************/
void Reborder(void)
{
  FvwmWindow *tmp;			/* temp fvwm window structure */
  int i;
  extern unsigned PopupCount;
  extern MenuRoot *PopupTable[MAXPOPUPS];      

  /* put a border back around all windows */
  XGrabServer (dpy);

#ifndef NO_PAGER
  if(Scr.Pager_w != None)
    XDestroyWindow(dpy,Scr.Pager_w);
#endif
  
  InstallWindowColormaps (&Scr.FvwmRoot);	/* force reinstall */
  for (tmp = Scr.FvwmRoot.next; tmp != NULL; tmp = tmp->next)
    {
      XUnmapWindow(dpy,tmp->frame);
      RestoreWithdrawnLocation (tmp,True); 
      XDestroyWindow(dpy,tmp->frame);
    }
  
  for(i=0;i<PopupCount;i++)
    if(PopupTable[i]->w != None)
      XDestroyWindow(dpy,PopupTable[i]->w);
  
  XUngrabServer (dpy);
  XSetInputFocus (dpy, PointerRoot, RevertToPointerRoot,CurrentTime);
  XSync(dpy,0);

}

/***********************************************************************
 *
 *  Procedure: NoisyExit
 *	Print error messages and die. (segmentation violation)
 *
 **********************************************************************/
void NoisyExit(int nonsense)
{
  XErrorEvent event;
  
  fvwm_err("Seg Fault",NULL,NULL,NULL);
  event.error_code = 0;
  event.request_code = 0;
  FvwmErrorHandler(dpy, &event);

  /* Attempt to do a re-start of fvwm */
  Done(0,NULL);
}





/***********************************************************************
 *
 *  Procedure:
 *	Done - cleanup and exit fvwm
 *
 ***********************************************************************
 */
void SigDone(int nonsense)
{
  Done(0, NULL);
  SIGNAL_RETURN;
}

void Done(int restart, char *command)
{
#ifndef NON_VIRTUAL
  MoveViewport(0,0,False);
#endif

  /* Close all my pipes */
  ClosePipes();

  Reborder ();

#ifdef M4  
  if (m4_enable)
    {
      extern char *fvwm_file;

      /* With m4 processing, a temporary file was created to hold the
         processed file.  Delete the file now because we don't need it
	 any more.  It will be created again during restart. */ 
      unlink(fvwm_file);
    }
#endif

  if(restart)
    {
      SaveDesktopState();		/* I wonder why ... */

      /* Really make sure that the connection is closed and cleared! */
      XSelectInput(dpy, Scr.Root, 0 );
      XSync(dpy, 0);
      XCloseDisplay(dpy);

      {
	char *my_argv[10];
	int i,done,j;

	i=0;
	j=0;
	done = 0;
	while((g_argv[j] != NULL)&&(i<8))
	  {
	    if(strcmp(g_argv[j],"-s")!=0)
	      {
		my_argv[i] = g_argv[j];
		i++;
		j++;
	      }
	    else
	      j++;
	  }
	if(strstr(command,"fvwm")!= NULL)
	   my_argv[i++] = "-s";
	while(i<10)
	  my_argv[i++] = NULL;
	
	/* really need to destroy all windows, explicitly,
	 * not sleep, but this is adequate for now */
	sleep(1);
	ReapChildren();
	execvp(command,my_argv);
      }
      fprintf(stderr, "FVWM: Call of '%s' failed!!!!\n",command);
      execvp(g_argv[0], g_argv);    /* that _should_ work */
      fprintf(stderr, "FVWM: Call of '%s' failed!!!!\n", g_argv[0]); 
    }
  else
    {
      XCloseDisplay(dpy);
      exit(0);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	CatchRedirectError - Figures out if there's another WM running
 *
 ************************************************************************/
XErrorHandler CatchRedirectError(Display *dpy, XErrorEvent *event)
{
  fvwm_err("another WM is running",NULL,NULL,NULL);
  exit(1);
}


/***********************************************************************
 *
 *  Procedure:
 *	FvwmErrorHandler - displays info on internal errors
 *
 ************************************************************************/
XErrorHandler FvwmErrorHandler(Display *dpy, XErrorEvent *event)
{
  extern int last_event_type;

  /* some errors are acceptable, mostly they're caused by 
   * trying to update a lost  window */
  if((event->error_code == BadWindow)||(event->request_code == X_GetGeometry)||
     (event->error_code==BadDrawable)||(event->request_code==X_SetInputFocus)||
     (event->request_code==X_GrabButton)||
     (event->request_code==X_ChangeWindowAttributes)||
     (event->request_code == X_InstallColormap))
    return 0 ;


  fvwm_err("internal error",NULL,NULL,NULL);
  fprintf(stderr,"      Request %d, Error %d\n", event->request_code,
	  event->error_code);
  fprintf(stderr,"      EventType: %d",last_event_type);
  fprintf(stderr,"\n");
  return 0;
}

void fvwm_err(char *message, char *arg1, char *arg2, char *arg3)
{
  fprintf(stderr,"fvwm: ");
  fprintf(stderr,message,arg1,arg2,arg3);
  fprintf(stderr,"\n");
}

void usage(void)
{
#ifdef M4
#define USAGE "Fvwm Ver %s\n\nusage: fvwm [-d dpy] [-debug] [-f config_file] [-s] [-no-m4] [-m4-prefix] [-m4opt option] [-m4-squote squote] [-m4-equote equote] [-m4prog m4prog]\n"
#else
#define USAGE "Fvwm Ver %s\n\nusage: fvwm [-d dpy] [-debug] [-f config_file] [-s]\n"
#endif

  fprintf(stderr,USAGE,PACKAGE_VERSION);

}




#ifndef NON_VIRTUAL
/* the root window is surrounded by four window slices, which are InputOnly.
 * So you can see 'through' them, but they eat the input. An EnterEvent in
 * one of these windows causes a Paging. The windows have the according cursor
 * pointing in the pan direction or are hidden if there is no more panning
 * in that direction. This is mostly intended to get a panning even atop
 * of Motif applictions, which does not work yet. It seems Motif windows
 * eat all mouse events.
 *
 * Hermann Dunkel, HEDU, dunkel@cul-ipn.uni-kiel.de 1/94
*/

/***************************************************************************
 * checkPanFrames hides PanFrames if they are on the very border of the
 * VIRTUELL screen and EdgeWrap for that direction is off. 
 * (A special cursor for the EdgeWrap border could be nice) HEDU
 ****************************************************************************/
void checkPanFrames(void)
{
  extern Bool DoHandlePageing;
  int wrapX = (Scr.flags & EdgeWrapX);
  int wrapY = (Scr.flags & EdgeWrapY);

  /* Remove Pan frames if paging by edge-scroll is permanently or
   * temporarily disabled */
  if((Scr.EdgeScrollY == 0)||(!DoHandlePageing))
    {
      XUnmapWindow(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=False;   
      XUnmapWindow (dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=False;
    }   
  if((Scr.EdgeScrollX == 0)||(!DoHandlePageing))
    {
      XUnmapWindow(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=False;
      XUnmapWindow (dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=False;
    }   
  if(((Scr.EdgeScrollX == 0)&&(Scr.EdgeScrollY == 0))||(!DoHandlePageing))
    return;

  /* LEFT, hide only if EdgeWrap is off */
  if (Scr.Vx==0 && Scr.PanFrameLeft.isMapped && (!wrapX)) 
    {
      XUnmapWindow(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=False;
    }
  else if (Scr.Vx > 0 && Scr.PanFrameLeft.isMapped==False) 
    {
      XMapRaised(dpy,Scr.PanFrameLeft.win);
      Scr.PanFrameLeft.isMapped=True;
    }
  /* RIGHT, hide only if EdgeWrap is off */
  if (Scr.Vx == Scr.VxMax && Scr.PanFrameRight.isMapped && (!wrapX))
    {
      XUnmapWindow (dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=False;
    }
  else if (Scr.Vx < Scr.VxMax && Scr.PanFrameRight.isMapped==False) 
    {
      XMapRaised(dpy,Scr.PanFrameRight.win);
      Scr.PanFrameRight.isMapped=True;
    }
  /* TOP, hide only if EdgeWrap is off */
  if (Scr.Vy==0 && Scr.PanFrameTop.isMapped && (!wrapY)) 
    {
      XUnmapWindow(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=False;
    }
  else if (Scr.Vy > 0 && Scr.PanFrameTop.isMapped==False) 
    {
      XMapRaised(dpy,Scr.PanFrameTop.win);
      Scr.PanFrameTop.isMapped=True;
    }
  /* BOTTOM, hide only if EdgeWrap is off */
  if (Scr.Vy == Scr.VyMax && Scr.PanFrameBottom.isMapped && (!wrapY)) 
    {
      XUnmapWindow (dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=False;
    }
  else if (Scr.Vy < Scr.VyMax && Scr.PanFrameBottom.isMapped==False) 
    {
      XMapRaised(dpy,Scr.PanFrameBottom.win);
      Scr.PanFrameBottom.isMapped=True;
    }
}

/****************************************************************************
 *
 * Gotta make sure these things are on top of everything else, or they
 * don't work!
 *
 ***************************************************************************/
void raisePanFrames(void)
{
  if (Scr.PanFrameTop.isMapped) XRaiseWindow(dpy,Scr.PanFrameTop.win);
  if (Scr.PanFrameLeft.isMapped) XRaiseWindow(dpy,Scr.PanFrameLeft.win);
  if (Scr.PanFrameRight.isMapped) XRaiseWindow(dpy,Scr.PanFrameRight.win);
  if (Scr.PanFrameBottom.isMapped) XRaiseWindow(dpy,Scr.PanFrameBottom.win);
}

/****************************************************************************
 *
 * Creates the windows for edge-scrolling 
 *
 ****************************************************************************/
void initPanFrames()
{
  XSetWindowAttributes attributes;    /* attributes for create */
  unsigned long valuemask; 
  
  attributes.event_mask =  (EnterWindowMask | LeaveWindowMask | 
			    VisibilityChangeMask);
  valuemask=  (CWEventMask | CWCursor );
  
  attributes.cursor = Scr.FvwmCursors[TOP];
  Scr.PanFrameTop.win = 
    XCreateWindow (dpy, Scr.Root, 
		   0,0,
		   Scr.MyDisplayWidth,PAN_FRAME_THICKNESS,
		   0,	/* no border */
		   CopyFromParent, InputOnly,
		   CopyFromParent, 
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[LEFT];
  Scr.PanFrameLeft.win = 
    XCreateWindow (dpy, Scr.Root, 
		   0,PAN_FRAME_THICKNESS,
		   PAN_FRAME_THICKNESS,
		   Scr.MyDisplayHeight-2*PAN_FRAME_THICKNESS,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent, 
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[RIGHT];
  Scr.PanFrameRight.win = 
    XCreateWindow (dpy, Scr.Root, 
		   Scr.MyDisplayWidth-PAN_FRAME_THICKNESS,PAN_FRAME_THICKNESS,
		   PAN_FRAME_THICKNESS,
		   Scr.MyDisplayHeight-2*PAN_FRAME_THICKNESS,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent, 
		   valuemask, &attributes);
  attributes.cursor = Scr.FvwmCursors[BOTTOM];
  Scr.PanFrameBottom.win = 
    XCreateWindow (dpy, Scr.Root, 
		   0,Scr.MyDisplayHeight-PAN_FRAME_THICKNESS,
		   Scr.MyDisplayWidth,PAN_FRAME_THICKNESS,
		   0,	/* no border */
		   CopyFromParent, InputOnly, CopyFromParent, 
		   valuemask, &attributes);
  Scr.PanFrameTop.isMapped=Scr.PanFrameLeft.isMapped=
    Scr.PanFrameRight.isMapped= Scr.PanFrameBottom.isMapped=False;
  
  Scr.usePanFrames=True;
}
#endif /* NON_VIRTUAL */

/****************************************************************************
 *
 * Save Desktop State
 *
 ****************************************************************************/
void SaveDesktopState()
{
  FvwmWindow *t;
  unsigned long data[1];

  for (t = Scr.FvwmRoot.next; t != NULL; t = t->next)
    {
      data[0] = (unsigned long) t->Desk;
      XChangeProperty (dpy, t->w, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
		       PropModeReplace, (unsigned char *) data, 1);
    }

  data[0] = (unsigned long) Scr.CurrentDesk;
  XChangeProperty (dpy, Scr.Root, _XA_WM_DESKTOP, _XA_WM_DESKTOP, 32,
		   PropModeReplace, (unsigned char *) data, 1);

  XSync(dpy, 0);
}


