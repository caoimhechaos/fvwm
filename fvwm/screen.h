/****************************************************************************
 * This module is based on Twm, but has been siginificantly modified 
 * by Rob Nation
 ****************************************************************************/
/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/***********************************************************************
 *
 * fvwm per-screen data include file
 *
 ***********************************************************************/

#ifndef _SCREEN_
#define _SCREEN_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "misc.h"
#include "menus.h"

#define SIZE_HINDENT 5
#define SIZE_VINDENT 3
#define MAX_WINDOW_WIDTH 32767
#define MAX_WINDOW_HEIGHT 32767


/* Cursor types */
#define POSITION 0		/* upper Left corner cursor */
#define TITLE_CURSOR 1          /* title-bar cursor */
#define DEFAULT 2		/* cursor for apps to inherit */
#define SYS 3        		/* sys-menu and iconify boxes cursor */
#define MOVE 4                  /* resize cursor */
#if defined(__alpha)
#ifdef WAIT
#undef WAIT
#endif /*WAIT */
#endif /*alpha */
#define WAIT 5   		/* wait a while cursor */
#define MENU 6  		/* menu cursor */
#define SELECT 7	        /* dot cursor for f.move, etc. from menus */
#define DESTROY 8		/* skull and cross bones, f.destroy */
#define TOP 9
#define RIGHT 10
#define BOTTOM 11
#define LEFT 12
#define TOP_LEFT 13
#define TOP_RIGHT 14
#define BOTTOM_LEFT 15
#define BOTTOM_RIGHT 16
#define MAX_CURSORS 18

/* Maximum number of icon boxes that are allowed */
#define MAX_BOXES 4

#ifndef NON_VIRTUAL
typedef struct 
{
  Window win;
  int isMapped;
} PanFrame;
#endif

typedef struct ScreenInfo
{

  unsigned long screen;
  int d_depth;	        	/* copy of DefaultDepth(dpy, screen) */
  int NumberOfScreens;          /* number of screens on display */
  int MyDisplayWidth;		/* my copy of DisplayWidth(dpy, screen) */
  int MyDisplayHeight;	        /* my copy of DisplayHeight(dpy, screen) */
  
  FvwmWindow FvwmRoot;		/* the head of the fvwm window list */
  Window Root;		        /* the root window */
  Window SizeWindow;		/* the resize dimensions window */
  Window NoFocusWin;            /* Window which will own focus when no other
				 * windows have it */
#ifndef NON_VIRTUAL
  PanFrame PanFrameTop,PanFrameLeft,PanFrameRight,PanFrameBottom;
  int usePanFrames;		/* toggle to disable them */
#endif  

  Pixmap gray_bitmap;           /*dark gray pattern for shaded out menu items*/
  Pixmap gray_pixmap;           /* dark gray pattern for inactive borders */
  Pixmap light_gray_pixmap;     /* light gray pattern for inactive borders */
  Pixmap sticky_gray_pixmap;     /* light gray pattern for sticky borders */

  MouseButton *MouseButtonRoot;
  FuncKey FuncKeyRoot;

  int root_pushes;		/* current push level to install root
				   colormap windows */
  FvwmWindow *pushed_window;	/* saved window to install when pushes drops
				   to zero */
#ifndef NO_PAGER
  FvwmWindow *FvwmPager;
  Window Pager_w;
  Window CPagerWin;
#endif
  Cursor FvwmCursors[MAX_CURSORS];

  name_list *TheList;		/* list of window names with attributes */
  char *DefaultIcon;            /* Icon to use when no other icons are found */

  ColorPair MenuColors;
  ColorPair MenuStippleColors;
  ColorPair MenuRelief;

  ColorPair StdColors; 	/* standard fore/back colors */
  ColorPair StickyColors; 	/* sticky fore/back colors */
  ColorPair StickyRelief; 	/* sticky hilight colors */
  ColorPair HiColors; 	/* standard fore/back colors */
  ColorPair StdRelief;
  ColorPair HiRelief;
  MyFont StdFont;     	/* font structure */
  MyFont WindowFont;   	/* font structure for window titles */
#ifndef NO_PAGER
  MyFont PagerFont;   	/* font struct for window labels in pager (optional)*/
#endif
  MyFont IconFont;      /* for icon labels */
  
  GC NormalGC;		        /* normal GC for menus, pager, resize window */
  GC StippleGC;		        /* normal GC for menus, pager, resize window */
  GC DrawGC;			/* GC to draw lines for move and resize */
  GC HiReliefGC;                /* GC for highlighted window relief */
  GC HiShadowGC;                /* GC for highlighted window shadow */
  GC MenuGC;
  GC MenuStippleGC;
  GC MenuReliefGC;
  GC MenuShadowGC;
  GC StdReliefGC;               /* GC for unselected window relief */
  GC StdShadowGC;               /* GC for unselected window shadow */
  GC ScratchGC1;
  GC ScratchGC2;

  GC StickyReliefGC;               /* GC for unselected sticky window relief */
  GC StickyShadowGC;               /* GC for unselected sticky window shadow */
  GC FontGC;                    /* GC for non-standard fonts */

  int SizeStringWidth;	        /* minimum width of size window */
  int CornerWidth;	        /* corner width for decoratedwindows */
  int BoundaryWidth;	        /* frame width for decorated windows */
  int NoBoundaryWidth;	        /* frame width for decorated windows */
  int TitleHeight;		/* height of the title bar window */
  FvwmWindow *Hilite;		/* the fvwm window that is highlighted 
				 * except for networking delays, this is the
				 * window which REALLY has the focus */
  FvwmWindow *Focus;            /* Last window which Fvwm gave the focus to 
                                 * NOT the window that really has the focus */
  FvwmWindow *Ungrabbed;
  FvwmWindow *PreviousFocus;    /* Window which had focus before fvwm stole it
				 * to do moves/menus/etc. */
  int EntryHeight;		/* menu entry height */
  int EdgeScrollX;              /* #pixels to scroll on screen edge */
  int EdgeScrollY;              /* #pixels to scroll on screen edge */
  unsigned char buttons2grab;   /* buttons to grab in click to focus mode */
  unsigned long flags;
  int IconBoxes[MAX_BOXES][4];
  int NumBoxes;
  int randomx;                  /* values used for randomPlacement */
  int randomy;
  unsigned VScale;              /* Panner scale factor */
  FvwmWindow *LastWindowRaised; /* Last window which was raised. Used for raise
				 * lower func. */
  int VxMax;                    /* Max location for top left of virt desk*/
  int VyMax;
  int Vx;                       /* Current loc for top left of virt desk */
  int Vy;

  int nr_left_buttons;         /* number of left-side title-bar buttons */
  int nr_right_buttons;        /* number of right-side title-bar buttons */

  int left_button_styles[2][5];
  int right_button_styles[2][5];

  int ClickTime;               /*Max button-click delay for Function built-in*/
  int AutoRaiseDelay;          /* Delay between setting focus and raising win*/
  int ScrollResistance;        /* resistance to scrolling in desktop */
  int MoveResistance;          /* res to moving windows over viewport edge */
  int OpaqueSize;
  int CurrentDesk;             /* The current desktop number */
  MenuRoot *InitFunction;
  MenuRoot *RestartFunction;

  /* new style */
  Boolean use_new_style[10];   /* if true then use custom shapes */
  int left_num_coords[5];
  int left_x_coords[5][20];
  int left_y_coords[5][20];
  int left_line_style[5][20];
  int right_num_coords[5];
  int right_x_coords[5][20];
  int right_y_coords[5][20];
  int right_line_style[5][20];

} ScreenInfo;

extern ScreenInfo Scr;

/* for the flags value - these used to be seperate Bool's */
#define ClickToFocus               (1) /* Focus follows mouse, or click to focus?*/
#define DecorateTransients         (2) /* decorate transient windows? */
#define DontMoveOff                (4) /* make sure all windows stay on desktop*/
#define RandomPlacement            (8) /* place windows in random locations? */
#define SuppressIcons             (16) /* prevent generation of icon windows */
#define StickyIcons               (32) /* Icons always sticky? */
#define EdgeWrapX                 (64) /* Should EdgeScroll wrap around? */
#define EdgeWrapY                (128) 
#define CenterOnCirculate        (256) /* center window when circulating? */
#define MWMBorders               (512)
#define MWMMenus                (1024)
#define MWMButtons              (2048)
#define MWMDecorHints           (4096)
#define NoPPosition             (8192)
#define SMART_PLACEMENT        (16384)
#define CirculateSkipIcons     (32768)
#define StubbornIcons          (65536)
#define StubbornPlacement     (131072)
#define StubbornIconPlacement (262144)
#define OpaqueResize          (524288)
#define MWMFunctionHints     (1048576)
#define MWMHintOverride      (2097152)
#define BackingStore         (4194304)
#define AppsBackingStore     (8388608)
#define SaveUnders          (16777216)
#define Lenience            (33554432)
#define SloppyFocus         (67108864)
#endif /* _SCREEN_ */
