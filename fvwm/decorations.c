/****************************************************************************
 * 
 * This is all original code by Robert Nation 
 * which reads motif mwm window manager
 * hints from a window, and makes necessary adjustments for fvwm. 
 *
 * Definitions of the hint structure and the constants are courtesy of
 * mitnits@bgumail.bgu.ac.il (Roman Mitnitski ), who sent this note,
 * after conferring with a friend at the OSF:
 * Hi, Rob.
 *
 *I'm happy to announce, that you can use motif public
 *headers in any way you can... I just got the letter from
 *my friend, it says literally:
 *
 *>    Hi.
 *>
 *> Yes, you can use motif public header files, in particular
 *> because there is NO limitation on inclusion of this files
 *> in your programms....Also, no one can put copyright to the NUMBERS
 *> (I mean binary flags for decorations) or DATA STRUCTURES
 *> (I mean little structure used by motif to pass description
 *> of the decorations to the mwm). Call it another name, if you are
 *> THAT MUCH concerned.
 *>
 *> You can even use the little piece of code I've passed to you - 
 *> we are talking about 10M distribution against two pages of code.
 *> Don't be silly.
 *> 
 *> Best wishes. 
 *> Eli.
 *
 *
 ****************************************************************************/

#include <config.h>
#include <fvwm/strings.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "fvwm.h"
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "screen.h"
#include "parse.h"
#include "menus.h"

extern Atom _XA_MwmAtom;

/* Motif  window hints */
typedef struct
{
    CARD32      flags;
    CARD32      functions;
    CARD32      decorations;
    INT32       inputMode;
} PropMotifWmHints;

typedef PropMotifWmHints        PropMwmHints;

PropMwmHints  prop;

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS           (1L << 0)
#define MWM_HINTS_DECORATIONS         (1L << 1)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)       

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

#define PROP_MOTIF_WM_HINTS_ELEMENTS  4
#define PROP_MWM_HINTS_ELEMENTS       PROP_MOTIF_WM_HINTS_ELEMENTS

extern FvwmWindow *Tmp_win;

/****************************************************************************
 * 
 * Reads the property MOTIF_WM_HINTS
 *
 *****************************************************************************/
void GetMwmHints(FvwmWindow *t)
{
  int actual_format;
  Atom actual_type;
  unsigned long nitems, bytesafter;

  if((Scr.flags & (MWMDecorHints|MWMFunctionHints))&&
     (XGetWindowProperty (dpy, t->w, _XA_MwmAtom, 0L, 20L, False,
			_XA_MwmAtom, &actual_type, &actual_format, &nitems,
			&bytesafter,(unsigned char **)&t->mwm_hints)==Success))
    {
      if(nitems >= PROP_MOTIF_WM_HINTS_ELEMENTS)
	{
	  return;
	} 
    }
  
  t->mwm_hints = NULL;
}



/****************************************************************************
 * 
 * Interprets the property MOTIF_WM_HINTS, sets decoration and functions
 * accordingly
 *
 *****************************************************************************/
void SelectDecor(FvwmWindow *t, unsigned long tflags, int border_width, 
		 int resize_width)
{
  int decor,i;
  PropMwmHints *prop;

  if(!(tflags & BW_FLAG))
    border_width = Scr.NoBoundaryWidth;

  if(!(tflags & NOBW_FLAG))
    resize_width = Scr.BoundaryWidth;

  for(i=0;i<5;i++)
    {
      t->left_w[i] = 1;
      t->right_w[i] = 1;
    }

  decor = MWM_DECOR_ALL;
  t->functions = MWM_FUNC_ALL;
  if(t->mwm_hints)
    {
      prop = (PropMwmHints *)t->mwm_hints;
      if(Scr.flags & MWMDecorHints)
	if(prop->flags & MWM_HINTS_DECORATIONS)
	  decor = prop->decorations;
      if(Scr.flags & MWMFunctionHints)
	if(prop->flags & MWM_HINTS_FUNCTIONS)
	  t->functions = prop->functions;
    }

  /* functions affect the decorations! if the user says
   * no iconify function, then the iconify button doesn't show
   * up. */
  if(t->functions & MWM_FUNC_ALL)
    {
      /* If we get ALL + some other things, that means to use
       * ALL except the other things... */
      t->functions &= ~MWM_FUNC_ALL;
      t->functions = (MWM_FUNC_RESIZE | MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE |
		   MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE) & (~(t->functions));
    }
  if((Scr.flags & MWMFunctionHints) && (t->flags & TRANSIENT))
    {
      t->functions &= ~(MWM_FUNC_MAXIMIZE|MWM_FUNC_MINIMIZE);
    }
  
  if(decor & MWM_DECOR_ALL)
    {
      /* If we get ALL + some other things, that means to use
       * ALL except the other things... */
      decor &= ~MWM_DECOR_ALL;
      decor = (MWM_DECOR_BORDER | MWM_DECOR_RESIZEH | MWM_DECOR_TITLE |
	       MWM_DECOR_MENU | MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE) 
	& (~decor);
    }

  /* Now I have the un-altered decor and functions, but with the
   * ALL attribute cleared and interpreted. I need to modify the
   * decorations that are affected by the functions */
  if(!(t->functions & MWM_FUNC_RESIZE))
    decor &= ~MWM_DECOR_RESIZEH;
  /* MWM_FUNC_MOVE has no impact on decorations. */
  if(!(t->functions & MWM_FUNC_MINIMIZE))
    decor &= ~MWM_DECOR_MINIMIZE;
  if(!(t->functions & MWM_FUNC_MAXIMIZE))
    decor &= ~MWM_DECOR_MAXIMIZE;
  /* MWM_FUNC_CLOSE has no impact on decorations. */

  /* This rule is implicit, but its easier to deal with if
   * I take care of it now */
  if(decor & (MWM_DECOR_MENU| MWM_DECOR_MINIMIZE | MWM_DECOR_MAXIMIZE))
    decor |= MWM_DECOR_TITLE;

  /* Selected the mwm-decor field, now trim down, based on
   * .fvwmrc entries */
  if ((tflags & NOTITLE_FLAG)||
      ((!(Scr.flags & DecorateTransients)) && (t->flags & TRANSIENT)))
    decor &= ~MWM_DECOR_TITLE;

  if ((tflags & NOBORDER_FLAG)||
      ((!(Scr.flags&DecorateTransients)) && (t->flags & TRANSIENT)))
    decor &= ~MWM_DECOR_RESIZEH;      

  if((Scr.flags & MWMDecorHints) && (t->flags & TRANSIENT))
    {
      decor &= ~(MWM_DECOR_MAXIMIZE|MWM_DECOR_MINIMIZE);
    }

#ifdef SHAPE
  if(t->wShaped)
      decor &= ~(BORDER|MWM_DECOR_RESIZEH);
#endif
  /* Assume no decorations, and build up */
  t->flags &= ~(BORDER|TITLE);
  t->boundary_width = 0;
  t->corner_width = 0;
  t->title_height = 0;

  if(decor & MWM_DECOR_BORDER)
    {
      /* A narrow border is displayed (5 pixels - 2 relief, 1 top,
       * (2 shadow) */
      t->boundary_width = border_width;
    }
  if(decor & MWM_DECOR_TITLE)
    {
      /*  A title barm with no buttons in it
       * window gets a 1 pixel wide black border. */
      t->flags |= TITLE;
      t->title_height = Scr.TitleHeight;
    }
  if(decor & MWM_DECOR_RESIZEH)
    {
      /* A wide border, with corner tiles is desplayed
       * (10 pixels - 2 relief, 2 shadow) */
      t->flags |= BORDER;
      t->boundary_width = resize_width;
      t->corner_width = Scr.TitleHeight + t->boundary_width; 
    }
  if(!(decor & MWM_DECOR_MENU))
    {
      /*  title-bar menu button omitted 
       * window gets 1 pixel wide black border */
      t->left_w[0] = None;
    }
  if(!(decor & MWM_DECOR_MINIMIZE))
    {
      /* title-bar + iconify button, no menu button.
       * window gets 1 pixel wide black border */
      t->right_w[1] = None;
    }
  if(!(decor & MWM_DECOR_MAXIMIZE))
    {
      /* title-bar + maximize button, no menu button, no iconify.
       * window has 1 pixel wide black border */
      t->right_w[0] = None;
    }
  if (t->buttons & BUTTON1) t->left_w[0]=None;
  if (t->buttons & BUTTON3) t->left_w[1]=None;
  if (t->buttons & BUTTON5) t->left_w[2]=None;
  if (t->buttons & BUTTON7) t->left_w[3]=None;
  if (t->buttons & BUTTON9) t->left_w[4]=None;

  if (t->buttons & BUTTON2) t->right_w[0]=None;
  if (t->buttons & BUTTON4) t->right_w[1]=None;
  if (t->buttons & BUTTON6) t->right_w[2]=None;
  if (t->buttons & BUTTON8) t->right_w[3]=None;
  if (t->buttons & BUTTON10)t->right_w[4]=None;

  t->nr_left_buttons = Scr.nr_left_buttons;
  t->nr_right_buttons = Scr.nr_right_buttons;

  for(i=0;i<Scr.nr_left_buttons;i++)
    if(t->left_w[i] == None)
      t->nr_left_buttons--;

  for(i=0;i<Scr.nr_right_buttons;i++)
    if(t->right_w[i] == None)
      t->nr_right_buttons--;

  if(Scr.flags & MWMBorders)
    t->bw = 0;
  else if(t->boundary_width <= 0)
    {
      t->boundary_width = 0;
      t->bw = 0;
    }
  else
    {
      t->bw = BW;
      t->boundary_width = t->boundary_width - 1;
    }
  if(t->title_height > 0)
    t->title_height += t->bw;
  if(t->boundary_width == 0)
    t->flags &= ~BORDER;
}

/****************************************************************************
 * 
 * Checks the function described in menuItem mi, and sees if it
 * is an allowed function for window Tmp_Win,
 * according to the motif way of life.
 * 
 * This routine is used to determine whether or not to grey out menu items.
 *
 ****************************************************************************/
int check_allowed_function(MenuItem *mi)
{
  /* Complex functions are a little tricky... ignore them for now */

  if ((Tmp_win)&&
      (!(Tmp_win->flags & DoesWmDeleteWindow))&&(mi->func == F_DELETE))
    return 0;

  /* Move is a funny hint. Keeps it out of the menu, but you're still allowed
   * to move. */
  if((mi->func == F_MOVE)&&(Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_MOVE)))
    return 0;

  if((mi->func == F_RESIZE)&&(Tmp_win)&&
     (!(Tmp_win->functions & MWM_FUNC_RESIZE)))
    return 0;

  if((mi->func == F_ICONIFY)&&(Tmp_win)&&
     (!(Tmp_win->flags & ICONIFIED))&&
     (!(Tmp_win->functions & MWM_FUNC_MINIMIZE)))
    return 0;

  if((mi->func == F_MAXIMIZE)&&(Tmp_win)&&
     (!(Tmp_win->functions & MWM_FUNC_MAXIMIZE)))
    return 0;

  if((mi->func == F_DELETE)&&(Tmp_win)&&
     (!(Tmp_win->functions & MWM_FUNC_CLOSE)))
    return 0;

  if((mi->func == F_CLOSE)&&(Tmp_win)&&
     (!(Tmp_win->functions & MWM_FUNC_CLOSE)))
    return 0;

  if((mi->func == F_DESTROY)&&(Tmp_win)&&
     (!(Tmp_win->functions & MWM_FUNC_CLOSE)))
    return 0;

  if(mi->func == F_FUNCTION)
    {
      /* Hard part! What to do now? */
      /* Hate to do it, but for lack of a better idea,
       * check based on the menu entry name */
      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_MOVE))&&
	 (mystrncasecmp(mi->item,MOVE_STRING,strlen(MOVE_STRING))==0))
	return 0;
      
      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_RESIZE))&&
	 (mystrncasecmp(mi->item,RESIZE_STRING1,strlen(RESIZE_STRING1))==0))
	return 0;

      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_RESIZE))&&
	 (mystrncasecmp(mi->item,RESIZE_STRING2,strlen(RESIZE_STRING2))==0))
	return 0;

      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_MINIMIZE))&&
	 (!(Tmp_win->flags & ICONIFIED))&&
	 (mystrncasecmp(mi->item,MINIMIZE_STRING,strlen(MINIMIZE_STRING))==0))
	return 0;

      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_MINIMIZE))&&
	 (mystrncasecmp(mi->item,MINIMIZE_STRING2,strlen(MINIMIZE_STRING2))==0))
	return 0;

      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_MAXIMIZE))&&
	 (mystrncasecmp(mi->item,MAXIMIZE_STRING,strlen(MAXIMIZE_STRING))==0))
	return 0;

      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_CLOSE))&&
	 (mystrncasecmp(mi->item,CLOSE_STRING1,strlen(CLOSE_STRING1))==0))
	return 0;

      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_CLOSE))&&
	 (mystrncasecmp(mi->item,CLOSE_STRING2,strlen(CLOSE_STRING2))==0))
	return 0;

      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_CLOSE))&&
	 (mystrncasecmp(mi->item,CLOSE_STRING3,strlen(CLOSE_STRING3))==0))
	return 0;

      if((Tmp_win)&&(!(Tmp_win->functions & MWM_FUNC_CLOSE))&&
	 (mystrncasecmp(mi->item,CLOSE_STRING4,strlen(CLOSE_STRING4))==0))
	return 0;

    }


  return 1;
}


/****************************************************************************
 * 
 * Checks the function "function", and sees if it
 * is an allowed function for window t,  according to the motif way of life.
 * This routine is used to decide if we should refuse to perform a function.
 *
 ****************************************************************************/
int check_allowed_function2(int function, FvwmWindow *t)
{

  if(Scr.flags & MWMHintOverride)
    return 1;


  if ((t)&&(!(t->flags & DoesWmDeleteWindow))&&(function == F_DELETE))
    return 0;

  if((function == F_RESIZE)&&(t)&&
     (!(t->functions & MWM_FUNC_RESIZE)))
    return 0;

  if((function == F_ICONIFY)&&(t)&&
     (!(t->flags & ICONIFIED))&&
     (!(t->functions & MWM_FUNC_MINIMIZE)))
    return 0;

  if((function == F_MAXIMIZE)&&(t)&&
     (!(t->functions & MWM_FUNC_MAXIMIZE)))
    return 0;

  if((function == F_DELETE)&&(t)&&
     (!(t->functions & MWM_FUNC_CLOSE)))
    return 0;

  if((function == F_DESTROY)&&(t)&&
     (!(t->functions & MWM_FUNC_CLOSE)))
    return 0;

  return 1;
}



