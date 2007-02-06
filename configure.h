#define FVWMDIR     "/usr/lib/X11/fvwm"
/* #define FVWMDIR        "/local/homes/dsp/nation/modules"*/
#define FVWM_ICONDIR   "/usr/include/X11/bitmaps:/usr/include/X11/pixmaps"
#define FVWMRC         "/usr/lib/X11/fvwm/system.fvwmrc"

/* Imake command needed to put modules in desired target location */
/* Use the second version if it causes grief */
#define TARGET_DIR BINDIR=FVWMDIR 
/* #define TARGET_DIR*/


/* If you want Imake to leave your binary in the standard place where
 * Imake wants to leave binaries, then choose the second line here.
 * If you want to install it in a different directory, uncomment and
 * edit the first line */
/* #define FVWM_BIN_DIR BINDIR=/local/homes/dsp/nation/bin/4.1.3*/
#define FVWM_BIN_DIR BINDIR=/usr/bin/X11
/*#define FVWM_BIN_DIR*/

/* Compiler over-ride for Imakefiles */
/* Leave it as shown to get your default compiler */
#define COMPILER CC=gcc 
/* #define COMPILER */


/***************************************************************************
 *#define SHAPE
 * If you want the Shaped window extensions, specify #define SHAPE
 *   Shaped window extensions seem to increase the window managers RSS
 *   by about 60 Kbytes. They provide for leaving a title-bar on the window
 *   without a border.
 *   If you dont use shaped window extension, you can either make your shaped
 *   windows undecorated, or live with a border and backdrop around all
 *   your shaped windows (oclock, xeyes)
 *
 *   If you normally use a shaped window (xeyes or oclock), you might as
 *   well compile this extension in, since the memory cost is  minimal in
 *   this case (The shaped window shared libs will be loaded anyway. If you
 *   don't normally use a shaped window, you have to decide for yourself
 ***************************************************************************/
#define SHAPE                       
 
/***************************************************************************
 *#define XPM
 *  if you want color icons, specify #define XPM, and get libXpm
 *  from sunsite.unc.edu. The following files are recommended in addition
 *  to the fvwm package (from ftp.x.org)
 *   /pub/R6untarred/contrib/lib/xpm-3.4c
 *   /pub/X11/contrib/xpm3icons.tar.Z,  sample icons
 *   /pub/X11/contrib/ctwm-3.0.tar.Z, pull out the icons. ctwm has really nice
 *                                  color icons.
 *  and this from ftp.x.org:
 *   /contrib/icons.tar.gz, lots of sample icons,
 *
 *   For monochrome, Xpm icons still work, but they're only better than regular
 *   bitmaps because they're shaped (if you specify #define SHAPE).
 ***************************************************************************/
#define XPM                      
/*  linker flags needed to locate and link in the Xpm library, if you use it */
#define XPMLIBRARY -L/usr/lib/X11 -lXpm

/***************************************************************************
 *#define M4
 *   Causes m4 pre-processor patches to be included. Try man m4 for more info.
 *   Warning: m4 defines macros for some simple things like "include"
 *            which might mess up a config like 
 *            IconPath /usr/include/X11/bitmaps, for example, so you
 *            would need to include
 *            undefine(`include') to fix that one. Some version of m4
 *            seem to give good error messages, others don't?
 ***************************************************************************/
/* #define M4                          */

/***************************************************************************
 *#define NO_PAGER 
 *   Omits the code for the built-in pager. The pager module FvwmPager
 *   can be used instead.
 ***************************************************************************/
/* #define NO_PAGER                    */

/***************************************************************************
 *#define NON_VIRTUAL
 *   Omits the virtual desktop - requires NO_PAGER
 ***************************************************************************/
/* #define NON_VIRTUAL                 */

/***************************************************************************
 *#define NO_SAVEUNDERS 
 *   tells thw WM not to request save unders for pop-up
 *   menus. A quick test using monochrome X11 shows that save
 *   unders cost about 4Kbytes RAM, but saves a lot of
 *   window redraws if you have windows that take a while
 *   to refresh. For xcolor, I assume the cost is more like
 *   4Kbytesx8 = 32kbytes (256 color).
 ***************************************************************************/
/* #define NO_SAVEUNDERS               */

/***************************************************************************
 *#define NO_WINDOWLIST 
 *   Caused fvwm built-in window-list to be omitted. The window-list
 *   module FvwmWinList can be used instead 
 ***************************************************************************/
/* #define NO_WINDOWLIST               */

/***************************************************************************
 *#define PRUNE
 *   Removes old configuration commands:
 *       BoundaryWidth, NoBoundaryWidth, Sticky, NoTitle, NoBorder,
 *       StaysOnTop, StartsOnDesk, CirculateSkip, WindowListSkip, Icon,
 *       SuppressIcons, and Module (when used for initial startup
 *       These commands were replaced with Style, except for the Module
 *       command, which is replaced with InitFunction
 *       
 *
 ***************************************************************************/
/* #define PRUNE                       */

/*************************************************************************
 *
 * Really, no one but me should need this 
 *
 ************************************************************************/
#if defined __sun__ && !defined SYSV
#define BROKEN_SUN_HEADERS          
#endif
/***************************************************************************
 *
 * In theory, this stuff can be replaced with GNU Autoconf 
 *
 **************************************************************************/

#if defined _POSIX_SOURCE || defined SYSV || defined __sun__

#define HAVE_WAITPID  1
#define HAVE_GETITIMER 1
#define HAVE_SETITIMER 1
#define HAVE_SYSCONF 1
#define HAVE_UNAME 1
#undef HAVE_GETHOSTNAME 

#else

/**************************************************************************
 *
 * Do it yourself here if you don't like the above!
 *
 **************************************************************************/
/***************************************************************************
 * Define if you have waitpid.  
 **************************************************************************/
#define HAVE_WAITPID  1
/***************************************************************************
 * Define if you have getitimer/setitimer.  
 * undefining this will break auto-raise 
 **************************************************************************/
#define HAVE_GETITIMER 1
#define HAVE_SETITIMER 1

/***************************************************************************
 * Define if you have sysconf
 **************************************************************************/
#define HAVE_SYSCONF 1

/***************************************************************************
 * Define if you have uname. Otherwise, define gethostname
 ***************************************************************************/
#define HAVE_UNAME 1
/* #define HAVE_GETHOSTNAME 1 */

#endif /* End of do-it-yourself OS support section */


/* Please translate the strings into the language which you use for your 
 * pop-up menus */
/* Some decisions about where a function is prohibited (based on 
 * mwm-function-hints) is based on a string comparison between the 
 * menu item and the strings below */
#define MOVE_STRING "move"
#define RESIZE_STRING1 "size"
#define RESIZE_STRING2 "resize"
#define MINIMIZE_STRING "minimize"
#define MINIMIZE_STRING2 "iconify"
#define MAXIMIZE_STRING "maximize"
#define CLOSE_STRING1   "close"
#define CLOSE_STRING2   "delete"
#define CLOSE_STRING3   "destroy"
#define CLOSE_STRING4   "quit"

#ifdef __alpha
#define NEEDS_ALPHA_HEADER
#undef BROKEN_SUN_HEADERS
#endif /* (__alpha) */


/* Allows gcc users to use inline, doesn't cause problems
 * for others. */
#ifndef __GNUC__
#define  FVWM_INLINE  /*nothing*/
#else
#if defined(__GNUC__) && !defined(inline)
#define FVWM_INLINE __inline__
#else
#define FVWM_INLINE inline
#endif
#endif
