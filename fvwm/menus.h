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
 * $XConsortium: menus.h,v 1.24 89/12/10 17:46:26 jim Exp $
 *
 * twm menus include file
 *
 * 17-Nov-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#ifndef _MENUS_
#define _MENUS_

#define MAX_FILE_SIZE 4096	/* max chars to read from file for cut */
#define MAXPOPUPS 50


typedef struct MenuItem
{
    struct MenuItem *next;	/* next menu item */
    struct MenuItem *prev;	/* prev menu item */
    char *item;			/* the character string displayed on left*/
    char *item2;	        /* the character string displayed on right*/
    char *action;		/* action to be performed */
    short item_num;		/* item number of this menu */
    short x;			/* x coordinate for text (item) */
    short x2;			/* x coordinate for text (item2) */
    short y_offset;		/* y coordinate for item */
    short y_height;		/* y height for item */
    short func;			/* twm built in function */
    long val1;                  /* values needed for F_SCROLL */
    long val2;
    long val1_unit;             /* units for val1, val2 */
    long val2_unit;             /* pixels (unit=1) or percent of screen 
				 * (unit = Scr.MyDisplayWidth/Height */
    short state;		/* video state, 0 = normal, 1 = reversed */
    short strlen;		/* strlen(item) */
    short strlen2;		/* strlen(item2) */
    short hotkey;		/* Hot key offset (pete@tecc.co.uk).
				   0 - No hot key
				   +ve - offset to hot key char in item
				   -ve - offset to hot key char in item2
				   (offsets have 1 added, so +1 or -1
				   refer to the *first* character)
				   */
    struct MenuRoot *menu;      /* sub-menu */
} MenuItem;

typedef struct MenuRoot
{
    struct MenuItem *first;	/* first item in menu */
    struct MenuItem *last;	/* last item in menu */
    struct MenuRoot *next;	/* next in list of root menus */
    char *name;			/* name of root */
    Window w;			/* the window of the menu */
    short height;		/* height of the menu */
    short width;		/* width of the menu for 1st col */
    short width2;		/* width of the menu for 2nd col */
    short items;		/* number of items in the menu */
    Bool in_use;
} MenuRoot;

typedef struct MouseButton
{
    int func;			/* the function number */
    MenuRoot *menu;		/* menu if func is F_POPUP */
    MenuItem *item;		/* action to perform if func != F_POPUP */
    int Button;
    int Context;
    int Modifier;
    int val1;
    int val2;
    long val1_unit;             /* units for val1, val2 */
    long val2_unit;             /* pixels (unit=1) or percent of screen 
				 * (unit = Scr.MyDisplayWidth/Height */
    struct MouseButton *NextButton;
} MouseButton;

typedef struct FuncKey
{
    struct FuncKey *next;	/* next in the list of function keys */
    char *name;			/* key name */
    KeyCode keycode;		/* X keycode */
    int cont;			/* context */
    int mods;			/* modifiers */
    int func;			/* function to perform */
    int val1;                   /* values needed for F_SCROLL */
    int val2;
    long val1_unit;             /* units for val1, val2 */
    long val2_unit;             /* pixels (unit=1) or percent of screen  */
    MenuRoot *menu;		/* menu if func is F_POPUP */
    char *action;		/* action string (if any) */
} FuncKey;

#define MENU_ERROR -1
#define MENU_NOP 0
#define MENU_DONE 1
#define SUBMENU_DONE 2


/* Types of events for the FUNCTION builtin */
#define MOTION 'm'
#define IMMEDIATE 'i'
#define CLICK 'c'
#define DOUBLE_CLICK 'd'
#define ONE_AND_A_HALF_CLICKS 'o'

extern MenuRoot *ActiveMenu;
extern MenuItem *ActiveItem;


#endif /* _MENUS_ */
