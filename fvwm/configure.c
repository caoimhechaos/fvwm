/****************************************************************************
 * This module is mostly all new
 * by Rob Nation 
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/


/***************************************************************************
 * 
 * Configure.c: reads the .fvwmrc or system.fvwmrc file, interprets it,
 * and sets up menus, bindings, colors, and fonts as specified
 *
 ***************************************************************************/

#include <config.h>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>

#include <X11/Xproto.h>
#include <X11/Xatom.h>
#ifdef M4
#include <X11/Xmu/SysUtil.h>
#endif


#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"

char *IconPath = FVWM_ICONDIR;
#ifdef XPM
char *PixmapPath=FVWM_ICONDIR;
#endif
char *ModulePath = FVWMDIR;

char *white = "white";
char *black = "black";
char *grey = "SlateGrey";
char *Menuback;
char *Menufore;
char *Menustipple;
char *Stdback;
char *Stdfore;
char *Stickyback;
char *Stickyfore;
char *Hiback;
char *Hifore;
#ifndef NO_PAGER
char *Pagerback;
char *Pagerfore;
#endif

void     GetColors(void);
Pixel    GetColor(char *);
MenuRoot *NewMenuRoot(char *name);
char     *stripcpy(char *);
char     *stripcpy2(char *,int, Bool);
char     *stripcpy3(char *, Bool);
#ifndef NO_PAGER
void     initialize_pager(int x, int y);
#endif
void     bad_binding(int num);
void     nofont(char *name);
void nocolor(char *note, char *name);
int contexts;
int mods,func,func_val_1,func_val_2;

#ifndef NO_PAGER
int pager_x=10000,pager_y=10000;
#endif

unsigned PopupCount = 0;
MenuRoot *PopupTable[MAXPOPUPS];
int dummy;
char *config_file = ".fvwmrc";

extern XContext MenuContext;		/* context for fvwm menus */
extern Bool DoHandlePageing;

/* value for the rubberband XORing */
unsigned long XORvalue;
int have_the_colors = 0;

/*
 * Order is important here! if one keyword is the same as the first part of
 * another keyword, the shorter one must come first!
 */
struct config main_config[] =
{
  {"OpaqueResize",      SetFlag,        (char **)OpaqueResize,(int *)0},
  {"StubbornIcons",     SetFlag,        (char **)StubbornIcons,(int *)0},
  {"StubbornPlacement", SetFlag,        (char **)StubbornPlacement,(int *)0},
  {"StubbornIconPlacement", SetFlag,    (char **)StubbornIconPlacement,
                                                 (int *)0},
  {"Font",              assign_string,  &Scr.StdFont.name, (int *)0},
  {"WindowFont",        assign_string,  &Scr.WindowFont.name, (int *)0},
  {"MenuForeColor",     assign_string,  &Menufore, (int *)0},
  {"MenuBackColor",     assign_string,  &Menuback, (int *)0},
  {"MenuStippleColor",  assign_string,  &Menustipple, (int *)0},
  {"StdForeColor",      assign_string,  &Stdfore, (int *)0},
  {"StdBackColor",      assign_string,  &Stdback, (int *)0},
#ifdef XPM
  {"PixmapPath",        assign_string,  &PixmapPath, (int *)0},
#endif
  {"StickyForeColor",   assign_string,  &Stickyfore, (int *)0},
  {"StickyBackColor",   assign_string,  &Stickyback, (int *)0},
  {"HiForeColor",       assign_string,  &Hifore, (int *)0},
  {"HiBackColor",       assign_string,  &Hiback, (int *)0},
  {"IconPath",          assign_string,  &IconPath, (int *)0},
  {"IconBox",           SetBox,         (char **)0, (int *)0},
  {"StickyIcons",       SetFlag,        (char **)StickyIcons, (int *)0},
  {"IconFont",          assign_string,  &Scr.IconFont.name, (int *)0},
#ifndef PRUNE
  {"Icon",              SetOneStyle,    (char **)&Scr.TheList,(int *)ICON_FLAG},
  {"NoIconTitle",       SetOneStyle,    (char **)&Scr.TheList, (int *)NOICON_TITLE_FLAG},
#endif
  {"MWMBorders",        SetFlag,        (char **)MWMBorders, (int *)0},
  {"MWMMenus",          SetFlag,        (char **)MWMMenus, (int *)0},
  {"MWMButtons",        SetFlag,        (char **)MWMButtons, (int *)0},
  {"MWMDecorHints",     SetFlag,        (char **)MWMDecorHints, (int *)0},
  {"MWMFunctionHints",  SetFlag,        (char **)MWMFunctionHints, (int *)0},
  {"MWMHintOverride",   SetFlag,        (char **)MWMHintOverride, (int *)0},
  {"Lenience",          SetFlag,        (char **)Lenience, (int *)0},
  {"NoPPosition",       SetFlag,        (char **)NoPPosition, (int *)0},
  {"CirculateSkipIcons",SetFlag,        (char **)CirculateSkipIcons, (int *)0},

#ifndef PRUNE
  {"NoTitle",           SetOneStyle,    (char **)&Scr.TheList, 
                                        (int *)NOTITLE_FLAG},
  {"NoBorder",          SetOneStyle,    (char **)&Scr.TheList, 
                                        (int *)NOBORDER_FLAG},
  {"Sticky",            SetOneStyle,    (char **)&Scr.TheList, 
                                        (int *)STICKY_FLAG},
  {"StaysOnTop",        SetOneStyle,    (char **)&Scr.TheList,
                                        (int *)STAYSONTOP_FLAG},
  {"StartsOnDesk",      SetOneStyle,    (char **)&Scr.TheList,
                                        (int *)STAYSONDESK_FLAG},
  {"CirculateSkip",     SetOneStyle,    (char **)&Scr.TheList,
                                        (int *)CIRCULATESKIP_FLAG},
  {"WindowListSkip",    SetOneStyle,    (char **)&Scr.TheList,
                                        (int *)LISTSKIP_FLAG},
#endif
  {"Style",             ParseStyle,     (char **)0, (int *)0},
  {"EdgeScroll",        SetInts,        (char **)&Scr.EdgeScrollX,
                                                 &Scr.EdgeScrollY},
  {"ButtonStyle",       ButtonStyle,    (char **)0, (int *)0},
  {"RandomPlacement",   SetFlag,        (char **)RandomPlacement, (int *)0},
  {"SmartPlacement",    SetFlag,        (char **)SMART_PLACEMENT, (int *)0},
#ifndef PRUNE
  {"SuppressIcons",     SetFlag,        (char **)SuppressIcons, (int *)0},
#endif
  {"DontMoveOff",       SetFlag,        (char **)DontMoveOff, (int *)0},
  {"DecorateTransients",SetFlag,        (char **)DecorateTransients, (int *)0},
  {"CenterOnCirculate", SetFlag,        (char **)CenterOnCirculate, (int *)0},
  {"AutoRaise",         SetInts,        (char **)&Scr.AutoRaiseDelay,&dummy},
#ifndef NO_PAGER
  {"PagerBackColor",    assign_string,  &Pagerback, (int *)0},
  {"PagerForeColor",    assign_string,  &Pagerfore, (int *)0},
  {"PagerFont",         assign_string,  &Scr.PagerFont.name, (int *)0},
  {"Pager",             SetInts,        (char **)&pager_x, &pager_y},
#endif
  {"DeskTopScale",      SetInts,        (char **)&Scr.VScale, &dummy},
  {"DeskTopSize",       SetInts,        (char **)&Scr.VxMax, &Scr.VyMax},
  {"ClickTime",         SetInts,        (char **)&Scr.ClickTime, &dummy},
  {"OpaqueMove",        SetInts,        (char **)&Scr.OpaqueSize, &dummy},
#ifndef PRUNE
  {"BoundaryWidth",     SetInts,        (char **)&Scr.BoundaryWidth,&dummy},
  {"NoBoundaryWidth",   SetInts,        (char **)&Scr.NoBoundaryWidth,&dummy},
#endif
  {"XorValue",          SetInts,        (char **)&XORvalue, &dummy},
  {"Mouse",             ParseMouseEntry,(char **)1, (int *)0},
  {"Popup",             ParsePopupEntry,(char **)1, (int *)0},
  {"Function",          ParsePopupEntry,(char **)1, (int *)0},
  {"Key",               ParseKeyEntry,  (char **)1, (int *)0},
  {"ClickToFocus",      SetFlag,        (char **)ClickToFocus,(int *)0},
  {"SloppyFocus",       SetFlag,        (char **)SloppyFocus,(int *)0},
  {"Cursor",            SetCursor,      (char **)0, (int *)0},
  {"PagingDefault",     SetInts,        (char **)&DoHandlePageing, &dummy},
  {"EdgeResistance",    SetInts,        (char **)&Scr.ScrollResistance,
                                                 &Scr.MoveResistance},
  {"BackingStore",      SetFlag,        (char **)BackingStore, (int *)0},
  {"AppsBackingStore",  SetFlag,        (char **)AppsBackingStore, (int *)0},
  {"SaveUnders",        SetFlag,        (char **)SaveUnders, (int *)0},
  
  {"ModulePath",        assign_string,  &ModulePath, (int *)0},
#ifndef PRUNE
  {"Module",            executeModule,  (char **)0, (int *)0},
#endif
  {"",                  0,              (char **)0, (int *)0}
};

struct config func_config[] =
{
  {"Nop",          set_func,(char **)F_NOP},
  {"Title",        set_func,(char **)F_TITLE},
  {"Beep",         set_func,(char **)F_BEEP},
  {"Quit",         set_func,(char **)F_QUIT},
  {"Refresh",      set_func,(char **)F_REFRESH},
  {"Move",         set_func,(char **)F_MOVE},
  {"Iconify",      set_func,(char **)F_ICONIFY},
  {"Maximize",     set_func,(char **)F_MAXIMIZE},
  {"Resize",       set_func,(char **)F_RESIZE},
  {"RaiseLower",   set_func,(char **)F_RAISELOWER},
  {"Raise",        set_func,(char **)F_RAISE},
  {"Lower",        set_func,(char **)F_LOWER},
  {"Delete",       set_func,(char **)F_DELETE},
  {"Close",        set_func,(char **)F_CLOSE},
  {"Destroy",      set_func,(char **)F_DESTROY},
  {"PopUp",        set_func,(char **)F_POPUP},
  {"Function",     set_func,(char **)F_FUNCTION},
  {"CursorMove",   set_func,(char **)F_MOVECURSOR},
  {"Stick",        set_func,(char **)F_STICK},
  {"CirculateUp",  set_func,(char **)F_CIRCULATE_UP},
  {"CirculateDown",set_func,(char **)F_CIRCULATE_DOWN},
  {"Wait",         set_func,(char **)F_WAIT},
  {"Warp",         set_func,(char **)F_WARP},
  {"Desk",         set_func,(char **)F_DESK},
  {"WindowsDesk",  set_func,(char **)F_CHANGE_WINDOWS_DESK},
  {"Focus",        set_func,(char **)F_FOCUS},
  {"Module",       set_func,(char **)F_MODULE},
  {"Quickie",       set_func,(char **)F_QUICKIE},
  {"Send_WindowList",set_func, (char **)F_SEND_WINDOW_LIST},

#ifndef NON_VIRTUAL
  {"Scroll",       set_func,(char **)F_SCROLL},
  {"GotoPage",     set_func,(char **)F_GOTO_PAGE},
  {"TogglePage",   set_func,(char **)F_TOGGLE_PAGE},
#endif
  {"Exec",         set_func,(char **)F_EXEC},
  {"Restart",      set_func,(char **)F_RESTART},
#ifndef NO_WINDOWLIST
  {"WindowList",   set_func,(char **)F_WINDOWLIST},
#endif
  {"",                    0,(char **)0}
};
  
struct charstring 
{
  char key;
  int  value;
};


/* The keys musat be in lower case! */
struct charstring win_contexts[]=
{
  {'w',C_WINDOW},
  {'t',C_TITLE},
  {'i',C_ICON},
  {'r',C_ROOT},
  {'f',C_FRAME},
  {'s',C_SIDEBAR},
  {'1',C_L1},
  {'2',C_R1},
  {'3',C_L2},
  {'4',C_R2},
  {'5',C_L3},
  {'6',C_R3},
  {'7',C_L4},
  {'8',C_R4},
  {'9',C_L5},
  {'0',C_R5},
  {'a',C_WINDOW|C_TITLE|C_ICON|C_ROOT|C_FRAME|C_SIDEBAR|
     C_L1|C_L2|C_L3|C_L4|C_L5|C_R1|C_R2|C_R3|C_R4|C_R5},
  {0,0}
};

/* The keys musat be in lower case! */
struct charstring key_modifiers[]=
{
  {'s',ShiftMask},
  {'c',ControlMask},
  {'m',Mod1Mask},
  {'1',Mod1Mask},
  {'2',Mod2Mask},
  {'3',Mod3Mask},
  {'4',Mod4Mask},
  {'5',Mod5Mask},
  {'a',AnyModifier},
  {'n',0},
  {0,0}
};

void     find_context(char *, int *, struct charstring *);
char *orig_tline;
FILE *config_fd = (FILE *)0;
/* we'll let modules get the fvwm_file that configuration was based on
 * for consistency */
char *fvwm_file;

#ifdef M4
static char *m4_defs(Display*, const char*, char*, char*);
#endif

/*****************************************************************************
 * 
 * This routine is responsible for reading and parsing the config file
 *
 ****************************************************************************/
void MakeMenus(const char *display_name, char *m4_options)
{
  char *system_file = FVWMRC;
  char *home_file;
  char line[256],*tline;
  char *Home;			/* the HOME environment variable */
  int HomeLen;			/* length of Home */
#ifdef M4
  extern int m4_enable;
#endif

  XORvalue = (((unsigned long) 1) << Scr.d_depth) - 1;

  Menuback = white;
  Menufore = black;
  Menustipple = grey;
  Stdback = white;
  Stdfore = black;
  Stickyback = NULL;
  Stickyfore = NULL;
  Hiback = white;
  Hifore = black;
#ifndef NO_PAGER
  Pagerback = white;
  Pagerfore = black;
#endif

  /* initialize some lists */
  Scr.MouseButtonRoot = NULL;
  Scr.FuncKeyRoot.next = NULL;
  Scr.TheList = NULL;
  
  Scr.DefaultIcon = NULL;

  /* find the home directory to look in */
  Home = getenv("HOME");
  if (Home == NULL)
    Home = "./";
  HomeLen = strlen(Home);

  if(config_file[0] == '/')
    { 
      home_file = safemalloc(strlen(config_file)+1);
      strcpy(home_file,config_file);
    }
  else if(strncmp(config_file,"~/",2)==0)
    {
      home_file = safemalloc(HomeLen + strlen(&config_file[2])+3);
      strcpy(home_file,Home);
      strcat(home_file,"/");
      strcat(home_file,&config_file[2]);
    }
  else
    {
      home_file = safemalloc(HomeLen+strlen(config_file)+3);
      strcpy(home_file,Home);
      strcat(home_file,"/");
      strcat(home_file,config_file);
    }
  fvwm_file = home_file;
  config_fd = fopen(home_file,"r");
  if(config_fd == (FILE *)NULL)
    {
      fvwm_file = system_file;
      config_fd = fopen(system_file,"r");
    }
  if(config_fd == (FILE *)NULL)
    {
      fvwm_err("can't open %s or %s",system_file,home_file,NULL);
      exit(1);
    }
  if(fvwm_file != home_file)
    free(home_file);

#ifdef M4
  if (m4_enable)
    {
      /*
       * Process the config file through m4 and save the
       * results in a temp file.
       */
  
      fvwm_file = m4_defs(dpy, display_name, m4_options, fvwm_file);
      fclose(config_fd);
  
      config_fd = fopen(fvwm_file, "r"); 

      if (config_fd == (FILE *) 0) 
	{
	  perror("Cannot open m4-processed config file\n");
	  exit(1);
	}
    }
#endif	/* M4 */

  tline = fgets(line,(sizeof line)-1,config_fd);
  orig_tline = tline;
  while(tline != (char *)0)
    {
      while(isspace(*tline))tline++;
      if((strlen(&tline[0])>1)&&(tline[0]!='#')&&(tline[0]!='*'))
	match_string(main_config,tline,"error in config:",config_fd);
      tline = fgets(line,(sizeof line)-1,config_fd);
      orig_tline = tline;
    }

  fclose(config_fd);
  config_fd = (FILE *)NULL;

  /* If no edge scroll line is provided in the setup file, use
   * a default */
  if(Scr.EdgeScrollX == -100000)
    Scr.EdgeScrollX = 25;
  if(Scr.EdgeScrollY == -100000)
    Scr.EdgeScrollY = Scr.EdgeScrollX;

  /* if edgescroll >1000 and < 100000m
   * wrap at edges of desktop (a "spherical" desktop) */
   if (Scr.EdgeScrollX >= 1000) 
     {
       Scr.EdgeScrollX /= 1000;
       Scr.flags |= EdgeWrapX;
     }
   if (Scr.EdgeScrollY >= 1000) 
     {
       Scr.EdgeScrollY /= 1000;
       Scr.flags |= EdgeWrapY;
     }

  Scr.EdgeScrollX=Scr.EdgeScrollX*Scr.MyDisplayWidth/100;
  Scr.EdgeScrollY=Scr.EdgeScrollY*Scr.MyDisplayHeight/100;

  Scr.VxMax = Scr.VxMax*Scr.MyDisplayWidth - Scr.MyDisplayWidth;
  Scr.VyMax = Scr.VyMax*Scr.MyDisplayHeight - Scr.MyDisplayHeight;
  if(Scr.VxMax <0)
    Scr.VxMax = 0;
  if(Scr.VyMax <0)
    Scr.VyMax = 0;

  if (Scr.VxMax == 0)
    Scr.flags &= ~EdgeWrapX;
  if (Scr.VyMax == 0)
    Scr.flags &= ~EdgeWrapY;

  GetColors();

#ifndef NO_PAGER
  if(pager_x < 10000)initialize_pager(pager_x,pager_y);
#endif

  return;
}


/*****************************************************************************
 * 
 * Copies a text string from the config file to a specified location
 *
 ****************************************************************************/
void assign_string(char *text, FILE *fd, char **arg, int *junk)
{
  *arg = stripcpy(text);
}

/*****************************************************************************
 * 
 * Changes a button decoration style
 *
 ****************************************************************************/
void ButtonStyle(char *text, FILE *fd, char **arg,int *junk)
{
  int button,w,h,n,num;
  char * letter=text;
  int offset,i;
  int num_coords;
  int *x_coords;
  int *y_coords;
  int *line_style;

  while(isspace(*letter))letter++;
  if (*letter == ':') /* signal for new data format */
  {
    /* get button number */
    num = sscanf(++letter,"%d%n",&button,&offset);
    if((num != 1)||(button>10)||(button<0))
    {
      fvwm_err("Bad button style (1) in line %s",orig_tline,NULL,NULL);    
      return;
    }
    if(button==10)button=0;
    /* get number of points */
    text=offset+letter;
    num = sscanf(text,"%d%n",&num_coords,&offset);
    if((num != 1)||(num_coords>20)||(num_coords<2))
    {
      fvwm_err("Bad button style (2) in line %s",orig_tline,NULL,NULL);    
      return;
    }
    /* which arrays to use? */
    n=button/2;
    if((n*2) == button)
    {
      /* right */
      n = n - 1;
      if(n<0)n=4;
      Scr.right_num_coords[n] = num_coords;
      x_coords = Scr.right_x_coords[n];
      y_coords = Scr.right_y_coords[n];
      line_style = Scr.right_line_style[n];
    }
    else
    {
      /* left */
      Scr.left_num_coords[n] = num_coords;
      x_coords = Scr.left_x_coords[n];
      y_coords = Scr.left_y_coords[n];
      line_style = Scr.left_line_style[n];
    }

    /* get the points */
    for(i=0;i<num_coords;i++)
    {
      text+=offset;
      /* X x Y @ line_style */
      num = sscanf(text,"%dx%d@%d%n",&x_coords[i],&y_coords[i],&line_style[i],&offset);
      if(num != 3)
      {
        fvwm_err("Bad button style (3) in line %s",orig_tline,NULL,NULL);    
        return;
      }
    }
    
    /* use the coordinate list instead of a box shape */
    Scr.use_new_style[button] = True;
    return;
  }
  /* drop through into old style processing */
  num = sscanf(text," %d %dx%d",&button, &w, &h);
  if(!((num != 3)||(button>10)||(button<0)))
  {
    if(button==10)button=0;
    Scr.use_new_style[button] = False;
  }


  num = sscanf(text," %d %dx%d",&button, &w, &h);
  if((num != 3)||(button>10)||(button<0))
    {
      fvwm_err("Bad button style in line %s",orig_tline,NULL,NULL);    
      return;
    }
  /* allow button 10 as well as button 0 as definition for 10 */
  if(button==10)button=0;
  n=button/2;
  if((n*2) == button)
    {
      n = n - 1;
      if(n<0)n=4;
      Scr.right_button_styles[0][n] = w;
      Scr.right_button_styles[1][n] = h;
    }
  else
    {
      Scr.left_button_styles[0][n] = w;
      Scr.left_button_styles[1][n] = h;
    }
}

/*****************************************************************************
 * 
 * Changes a cursor def.
 *
 ****************************************************************************/
void SetCursor(char *text, FILE *fd, char **arg,int *junk)
{
  int num,cursor_num,cursor_style;

  num = sscanf(text,"%d %d",&cursor_num, &cursor_style);
  if((num != 2)||(cursor_num >= MAX_CURSORS)||(cursor_num<0))
    {
      fvwm_err("Bad cursor in line %s",orig_tline,NULL,NULL);    
      return;
    }
  Scr.FvwmCursors[cursor_num] = XCreateFontCursor(dpy,cursor_style);
}

/*****************************************************************************
 * 
 * Sets a boolean flag to true
 *
 ****************************************************************************/
void SetFlag(char *text, FILE *fd, char **arg,int *junk)
{
  Scr.flags |= (unsigned long)arg;
}

/*****************************************************************************
 * 
 * Reads in one or two integer values
 *
 ****************************************************************************/
void SetInts(char *text, FILE *fd, char **arg1, int *arg2)
{
  sscanf(text,"%d%*c%d",(int *)arg1,(int *)arg2);
}


/*****************************************************************************
 * 
 * Reads Dimensions for an icon box from the config file
 *
 ****************************************************************************/
void SetBox(char *text, FILE *fd, char **arg,int *junk)
{
  int num;

  if(Scr.NumBoxes < MAX_BOXES)
    {
      /* Standard X11 geometry string */
      num = sscanf(text,"%d%d%d%d",&Scr.IconBoxes[Scr.NumBoxes][0],
	     &Scr.IconBoxes[Scr.NumBoxes][1],
	     &Scr.IconBoxes[Scr.NumBoxes][2],
	     &Scr.IconBoxes[Scr.NumBoxes][3]);

      /* check for negative locations */
      if(Scr.IconBoxes[Scr.NumBoxes][0] < 0)
	Scr.IconBoxes[Scr.NumBoxes][0] += Scr.MyDisplayWidth;
      if(Scr.IconBoxes[Scr.NumBoxes][1] < 0)
	Scr.IconBoxes[Scr.NumBoxes][1] += Scr.MyDisplayHeight;

      if(Scr.IconBoxes[Scr.NumBoxes][2] < 0)
	Scr.IconBoxes[Scr.NumBoxes][2] += Scr.MyDisplayWidth;
      if(Scr.IconBoxes[Scr.NumBoxes][3] < 0)
	Scr.IconBoxes[Scr.NumBoxes][3] += Scr.MyDisplayHeight;

      if(num == 4)
	Scr.NumBoxes++;
    }
}


/****************************************************************************
 *
 * This routine computes the shadow color from the background color
 *
 ****************************************************************************/
Pixel GetShadow(Pixel background) 
{
  XColor bg_color;
  XWindowAttributes attributes;
  unsigned int r,g,b;
  
  XGetWindowAttributes(dpy,Scr.Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);
  
  r = bg_color.red % 0xffff;
  g = bg_color.green % 0xffff;
  b = bg_color.blue % 0xffff;
  
  r = r >>1;
  g = g >>1;
  b = b >>1;
  
  bg_color.red = r;
  bg_color.green = g;
  bg_color.blue = b;

  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    {
      nocolor("alloc shadow","");
      bg_color.pixel = background;
    }
  
  return bg_color.pixel;
}

/****************************************************************************
 *
 * This routine computes the hilight color from the background color
 *
 ****************************************************************************/
Pixel GetHilite(Pixel background) 
{
  XColor bg_color, white_p;
  XWindowAttributes attributes;
  
  XGetWindowAttributes(dpy,Scr.Root,&attributes);
  
  bg_color.pixel = background;
  XQueryColor(dpy,attributes.colormap,&bg_color);

  white_p.pixel = GetColor(white);
  XQueryColor(dpy,attributes.colormap,&white_p);
  
#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif

  bg_color.red = max((white_p.red/5), bg_color.red);
  bg_color.green = max((white_p.green/5), bg_color.green);
  bg_color.blue = max((white_p.blue/5), bg_color.blue);
  
  bg_color.red = min(white_p.red, (bg_color.red*140)/100);
  bg_color.green = min(white_p.green, (bg_color.green*140)/100);
  bg_color.blue = min(white_p.blue, (bg_color.blue*140)/100);

#undef min
#ifdef max
#undef max
#endif
  
  if(!XAllocColor(dpy,attributes.colormap,&bg_color))
    {
      nocolor("alloc hilight","");
      bg_color.pixel = background;
    }
  return bg_color.pixel;
}

/****************************************************************************
 *
 * This routine loads all needed colors, and fonts,
 * and creates the GC's
 *
 ***************************************************************************/
#ifndef NO_PAGER
Pixel PagerBackColor;
Pixel PagerForeColor;
#endif
void GetColors(void)
{
  extern MyFont *IconFont;

  if(have_the_colors) return;

  if(Stickyback == NULL)
    Stickyback = Stdback;
  if(Stickyfore == NULL)
    Stickyfore = Stdfore;
  have_the_colors = 1;

  /* setup default colors */
  if(Scr.d_depth < 2)
    {
      /* black and white - override user choices */

      Scr.MenuColors.back = GetColor(white);
      Scr.MenuColors.fore = GetColor(black); 
      Scr.MenuStippleColors.back = GetColor(white);
      Scr.MenuStippleColors.fore = GetColor(black); 
      Scr.MenuRelief.back = GetColor(black);
      Scr.MenuRelief.fore = GetColor(white);
      Scr.StdColors.back = GetColor(white);
      Scr.StdColors.fore = GetColor(black); 
      Scr.StickyColors.back = GetColor(white);
      Scr.StickyColors.fore = GetColor(black); 
      Scr.HiColors.back  = GetColor(white);
      Scr.HiColors.fore  = GetColor(black); 
      Scr.StdRelief.back = GetColor(black);
      Scr.StdRelief.fore = GetColor(white);
      Scr.StickyRelief.back = GetColor(black);
      Scr.StickyRelief.fore = GetColor(white);
      Scr.HiRelief.back  = GetColor(black);
      Scr.HiRelief.fore  = GetColor(white);
#ifndef NO_PAGER
      PagerBackColor     = GetColor(white);
      PagerForeColor     = GetColor(black);
#endif
    }
  else
    {
      /* color - accept user choices */

      Scr.MenuColors.back = GetColor(Menuback);
      Scr.MenuColors.fore = GetColor(Menufore); 
      Scr.MenuStippleColors.back = GetColor(Menuback);
      Scr.MenuStippleColors.fore = GetColor(Menustipple); 
      Scr.MenuRelief.back = GetShadow(Scr.MenuColors.back);
      Scr.MenuRelief.fore = GetHilite(Scr.MenuColors.back);
      Scr.StdColors.back = GetColor(Stdback);
      Scr.StdColors.fore = GetColor(Stdfore); 
      Scr.StickyColors.back = GetColor(Stickyback);
      Scr.StickyColors.fore = GetColor(Stickyfore); 
      Scr.HiColors.back  =  GetColor(Hiback);
      Scr.HiColors.fore  = GetColor(Hifore); 
      Scr.StdRelief.back = GetShadow(Scr.StdColors.back);
      Scr.StdRelief.fore = GetHilite(Scr.StdColors.back);
      Scr.StickyRelief.back = GetShadow(Scr.StickyColors.back);
      Scr.StickyRelief.fore = GetHilite(Scr.StickyColors.back);
      Scr.HiRelief.back  = GetShadow(Scr.HiColors.back);
      Scr.HiRelief.fore  = GetHilite(Scr.HiColors.back);
#ifndef NO_PAGER
      PagerBackColor     = GetColor(Pagerback);
      PagerForeColor     = GetColor(Pagerfore);
#endif
    }

  /* load the font */
  if ((Scr.StdFont.font = XLoadQueryFont(dpy, Scr.StdFont.name)) == NULL)
    {
      nofont(Scr.StdFont.name);
      if ((Scr.StdFont.font = XLoadQueryFont(dpy, "fixed")) == NULL)
	exit(1);
    }
  Scr.StdFont.height = Scr.StdFont.font->ascent + Scr.StdFont.font->descent;
  Scr.StdFont.y = Scr.StdFont.font->ascent;
  Scr.EntryHeight = Scr.StdFont.height + HEIGHT_EXTRA;

  /* load the window-title font */
  if ((Scr.WindowFont.font = XLoadQueryFont(dpy, Scr.WindowFont.name)) == NULL)
    {
      nofont(Scr.WindowFont.name);
      if ((Scr.WindowFont.font = XLoadQueryFont(dpy, "fixed")) == NULL)
	exit(1);
    }

  Scr.WindowFont.height=
    Scr.WindowFont.font->ascent+Scr.WindowFont.font->descent;
  Scr.WindowFont.y = Scr.WindowFont.font->ascent;

  /* load the pager-label font */
#ifndef NO_PAGER
  if(Scr.PagerFont.name != NULL)
    {
      if ((Scr.PagerFont.font = XLoadQueryFont(dpy, Scr.PagerFont.name))!=NULL)
	{
	  Scr.PagerFont.height=
	    Scr.PagerFont.font->ascent+Scr.PagerFont.font->descent;
	  Scr.PagerFont.y = Scr.PagerFont.font->ascent;
	}
      else
	nofont(Scr.PagerFont.name);
    }
#endif

  IconFont = &Scr.StdFont;
  if(Scr.IconFont.name != NULL)
    {
      if ((Scr.IconFont.font = XLoadQueryFont(dpy, Scr.IconFont.name))!=NULL)
	{
	  Scr.IconFont.height=
	    Scr.IconFont.font->ascent+Scr.IconFont.font->descent;
	  Scr.IconFont.y = Scr.IconFont.font->ascent;
	  IconFont = &Scr.IconFont;
	}
      else
	nofont(Scr.IconFont.name);
    }

  /* create graphics contexts */
  CreateGCs();
  XSync(dpy,0);
  return;
}

/****************************************************************************
 * 
 *  Prints an error message for font loading
 *
 ****************************************************************************/ 
void nofont(char *name)
{
  fvwm_err("can't get font %s", name,NULL,NULL);
}

/****************************************************************************
 * 
 *  Processes a menu body definition
 *
 ****************************************************************************/ 
MenuRoot *ParseMenuBody(char *name,FILE *fd)
{
  MenuRoot *mr;
  char newline[256];
  register char *pline;
  char unit_1,unit_2;
  int n;

  pline = fgets(newline,(sizeof newline)-1,fd);
  orig_tline = pline;
  if (pline == NULL)
    return 0;

  mr = NewMenuRoot(name);
  GetColors();

  while(isspace(*pline))pline++;
  while((pline != (char *)0)
      &&(mystrncasecmp("End",pline,3)!=0))
    {
      if((*pline!='#')&&(*pline != 0)&&(*pline!='*'))
	{
	  char *ptr2 = 0;
	  match_string(func_config,pline, "bad menu body function:",fd);
	  if((func == F_EXEC)||(func == F_POPUP)||(func == F_RESTART)
	     ||(func == F_FUNCTION)||(func == F_MODULE))
	    ptr2=stripcpy3(pline,True);
	  else
	    ptr2=stripcpy3(pline,False);

	  func_val_1 = 0;
	  func_val_2 = 0;
	  unit_1 = 's';
	  unit_2 = 's';
	  if(ptr2 != NULL)
	    {
	      n = sscanf(ptr2,"%d %d",&func_val_1,&func_val_2);
	      if( n < 2)
		n = sscanf(ptr2,"%d%c %d%c",&func_val_1,&unit_1,&func_val_2,&unit_2);
	    }
	  AddToMenu(mr, stripcpy2(pline,1,True), stripcpy2(pline,2,True),
		    ptr2, func,func_val_1,func_val_2, unit_1, unit_2);
	}
      
      pline = fgets(newline,(sizeof newline)-1,fd);
      if(pline == (char *)0)return NULL;

      orig_tline = pline;

      while(isspace(*pline))pline++;
    }
  MakeMenu(mr);

  return mr;
}

/****************************************************************************
 * 
 *  Parses a popup definition 
 *
 ****************************************************************************/ 
void ParsePopupEntry(char *tline,FILE *fd, char **junk,int *junk2)
{
  MenuRoot *mr=0;
  mr = ParseMenuBody(stripcpy2(tline,0,True),fd);
  if (PopupCount < MAXPOPUPS)
    {
      PopupTable[PopupCount] = mr;
      PopupCount++;
      if(strcmp(mr->name,"InitFunction")==0)
	{
	  Scr.InitFunction = mr;
	}
      else if(strcmp(mr->name,"RestartFunction")==0)
	{
	  Scr.RestartFunction = mr;
	}
    }
  else
    {
      fprintf(stderr,"Popup/Function %s ignored, you have more than %u\n",
	      mr->name,MAXPOPUPS);
      free(mr);
    }
}

/****************************************************************************
 * 
 *  Parses a mouse binding
 *
 ****************************************************************************/ 
void ParseMouseEntry(char *tline,FILE *fd, char **junk,int *junk2)
{
  char context[256],modifiers[256],function[256],*ptr;
  MenuRoot *mr=0;
  MenuItem *mi=0;
  MouseButton *temp;
  int button,i,j;
  int n;
  char unit_1, unit_2;

  unit_1 = 's';
  unit_2 = 's';
  func_val_1 = 0;
  func_val_2 = 0;

  n = sscanf(tline,"%d %s %s %s %d %d",&button,context,modifiers,function,
	 &func_val_1,&func_val_2);
  if(n < 6)
    n = sscanf(tline,"%d %s %s %s %d%c %d%c",&button,context,modifiers,function,
	   &func_val_1,&unit_1,&func_val_2,&unit_2);

  find_context(context,&contexts,win_contexts);
  if((contexts != C_ALL) && (contexts & C_LALL))
    {
      /* check for nr_left_buttons */
      i=0;
      j=(contexts &C_LALL)/C_L1;
      while(j>0)
	{
	  i++;
	  j=j>>1;
	}
      if(Scr.nr_left_buttons <i)
	Scr.nr_left_buttons = i;
    }
  if((contexts != C_ALL) && (contexts & C_RALL))
    {
      /* check for nr_right_buttons */
      i=0;
      j=(contexts&C_RALL)/C_R1;
      while(j>0)
	{
	  i++;
	  j=j>>1;
	}
      if(Scr.nr_right_buttons <i)
	Scr.nr_right_buttons = i;
    }
  find_context(modifiers,&mods,key_modifiers);
  if((contexts & C_WINDOW)&&(((mods==0)||mods == AnyModifier)))
    {
      Scr.buttons2grab &= ~(1<<(button-1));
    }

  func = F_NOP;
  match_string(func_config,function,"bad mouse function:",fd);

  if((func == F_POPUP)||(func == F_FUNCTION))
    {
      unsigned i;
      ptr = stripcpy2(tline,0,True);
      if(ptr != NULL)
	for (i = 0; i < PopupCount; i++)
	  if (mystrcasecmp(PopupTable[i]->name,ptr) == 0)
	    {
	      mr = PopupTable[i];
	      break;
	    }
      if (!mr)
	{
	  no_popup(ptr);
	  func = F_NOP;
	}
      if(ptr != NULL)
	free(ptr);
    }
  else if((func == F_EXEC)||(func == F_RESTART)||
	  (func == F_CIRCULATE_UP)||(func == F_CIRCULATE_DOWN)||
	  (func == F_WARP)||(func == F_MODULE))
    {
      mi = (MenuItem *)safemalloc(sizeof(MenuItem));
      
      mi->next = (MenuItem *)NULL;
      mi->prev = (MenuItem *)NULL;
      mi->item_num = 0;
      if((func == F_EXEC)||(func == F_RESTART)||(func== F_MODULE))
	{
	  mi->item = stripcpy2(tline,0,True);
	  mi->action = stripcpy3(tline,True);
	}
      else
	{
	  mi->item = stripcpy2(tline,0,False);
	  mi->action = stripcpy3(tline,False);
	}
      mi->state = 0;
      mi->func = func;
      mi->strlen = strlen(mi->item);
      mi->val1 = 0;
      mi->val2 = 0;
      mi->val1_unit = 1;
      mi->val2_unit = 1;
    }
  
  temp = Scr.MouseButtonRoot;
  Scr.MouseButtonRoot = (MouseButton *)safemalloc(sizeof(MouseButton));
  Scr.MouseButtonRoot->func = func;
  Scr.MouseButtonRoot->menu = mr;
  Scr.MouseButtonRoot->item = mi;
  Scr.MouseButtonRoot->Button = button;
  Scr.MouseButtonRoot->Context = contexts;
  Scr.MouseButtonRoot->Modifier = mods;
  Scr.MouseButtonRoot->NextButton = temp;
  Scr.MouseButtonRoot->val1 = func_val_1;
  Scr.MouseButtonRoot->val2 = func_val_2;
  if((unit_1 == 'p')||(unit_1 == 'P'))
    Scr.MouseButtonRoot->val1_unit = 100;
  else
    Scr.MouseButtonRoot->val1_unit = Scr.MyDisplayWidth;
  if((unit_2 == 'p')||(unit_2 == 'P'))
    Scr.MouseButtonRoot->val2_unit = 100;
  else
    Scr.MouseButtonRoot->val2_unit = Scr.MyDisplayHeight;

  return;
}

void no_popup(char *ptr)
{
  if((ptr)&&(orig_tline))
    fprintf(stderr,"Popup '%s' not defined in line %s",ptr,orig_tline);
}


/****************************************************************************
 * 
 *  Processes a line with a key binding
 *
 ****************************************************************************/ 
void ParseKeyEntry(char *tline, FILE *fd,char **junk,int *junk2)
{
  char context[256],modifiers[256],function[256],*ptr;
  char name[256];
  MenuRoot *mr = 0;
  char unit_1, unit_2;
  int n;


  ptr = NULL;
  func_val_1 = 0;
  func_val_2 = 0;
  unit_1 = 's';
  unit_2 = 's';
  n = sscanf(tline,"%s %s %s %s %d %d",name,context,modifiers,function,
	 &func_val_1,&func_val_2);
  if(n < 6)
    n = sscanf(tline,"%s %s %s %s %d%c %d%c",name,context,modifiers,function,
	       &func_val_1,&unit_1,&func_val_2,&unit_2);
  find_context(context,&contexts,win_contexts);
  find_context(modifiers,&mods,key_modifiers);
  match_string(func_config,function,"bad key function:",fd);

  /* Make CirculateUp and CirculateDown take args. by Y.NOMURA */
 
  if ((func == F_CIRCULATE_UP) || (func == F_CIRCULATE_DOWN)||
      (func == F_WARP))
    ptr = stripcpy3(tline,False);
  
  /* End of addition */
 
  if((func == F_EXEC)||(func == F_RESTART)||(func == F_MODULE))
    {
      ptr = stripcpy3(tline,True);
    }
  else if((func == F_POPUP)||(func == F_FUNCTION))
    {
      unsigned i;
      ptr = stripcpy2(tline,0,True);
      if(ptr != NULL)
	{
	  for (i = 0; i < PopupCount; i++)
	    if (mystrcasecmp(PopupTable[i]->name,ptr) == 0)
	      {
		mr = PopupTable[i];
		break;
	      }
	}
      if (!mr)
	{
	  no_popup(ptr);
	  func = F_NOP;
	}
    }

  AddFuncKey(name,contexts,mods,func,ptr,func_val_1,func_val_2,mr, unit_1,unit_2);
}

/****************************************************************************
 * 
 * Sets menu/keybinding/mousebinding function to specified value
 *
 ****************************************************************************/ 
void set_func(char *text, FILE *fd, char **value,int *junk)
{
  func = (unsigned long)value;
}

/****************************************************************************
 * 
 * Turns a  string context of context or modifier values into an array of 
 * true/false values (bits)
 *
 ****************************************************************************/ 
void find_context(char *string, int *output, struct charstring *table)
{
  int i=0,j=0;
  Bool matched;
  char tmp1;

  *output=0;
  i=0;
  while(i<strlen(string))
    {
      j=0;
      matched = FALSE;
      while((!matched)&&(table[j].key != 0))
	{
	  /* in some BSD implementations, tolower(c) is not defined
	   * unless isupper(c) is true */
	  tmp1=string[i];
	  if(isupper(tmp1))
	    tmp1=tolower(tmp1);
	  /* end of ugly BSD patch */

	  if(tmp1 == table[j].key)
	    {
	      *output |= table[j].value;
	      matched = TRUE;
	    }
	  j++;
	}
      if(!matched)
	{
	  fprintf(stderr,"fvwm: bad entry %c in line %s",
		  string[i],orig_tline);
	}
      i++;
    }
  return;
}

/****************************************************************************
 * 
 * Matches text from config to a table of strings, calls routine
 * indicated in table.
 *
 ****************************************************************************/ 
void match_string(struct config *table, char *text, char *error_msg, FILE *fd)
{
  int j;
  Bool matched;

  j=0;
  matched = FALSE;
  while((!matched)&&(strlen(table[j].keyword)>0))
    {
      if(mystrncasecmp(text,table[j].keyword,strlen(table[j].keyword))==0)
	{
	  matched=TRUE;
	  /* found key word */
	  table[j].action(&text[strlen(table[j].keyword)],
				fd,table[j].arg,table[j].arg2);
	}
      else
	j++;
    }
  if(!matched)
    {
      fvwm_err("%s %s in line %s",error_msg,text,orig_tline);
    }
}

  
  

/****************************************************************************
 * 
 * Generates the window for a menu
 *
 ****************************************************************************/ 
void MakeMenu(MenuRoot *mr)
{
  MenuItem *cur;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  int y;
  
  /* lets first size the window accordingly */
  mr->width += 10;
  if(mr->width2 > 0)
    mr->width += 5;
  
  /* allow two pixels for top border */
  for (y=2, cur = mr->first; cur != NULL; cur = cur->next)
  {
    cur->y_offset = y;
    cur->x = 5;
    if(cur->func==F_TITLE)
      {
	/* Title */
	if(cur->strlen2  == 0)
	  cur->x = (mr->width - XTextWidth(Scr.StdFont.font, cur->item,
					   cur->strlen))>>1;
	
	if(Scr.flags & MWMMenus)
	  cur->y_height = Scr.EntryHeight + HEIGHT_EXTRA_TITLE;
	else
	  {
	    if((cur == mr->first)||(cur->next == NULL))
	      cur->y_height=Scr.EntryHeight-HEIGHT_EXTRA+1+
		(HEIGHT_EXTRA_TITLE>>1);
	    else
	      cur->y_height = Scr.EntryHeight -HEIGHT_EXTRA +1+
		HEIGHT_EXTRA_TITLE;
	  }
      }
    else if(cur->func==F_NOP && *cur->item==0)
      /* Separator */
      cur->y_height = HEIGHT_SEPARATOR;
    else
      /* Normal text entry */
      cur->y_height = Scr.EntryHeight;
    y += cur->y_height;
    if(mr->width2 == 0)
      {
	cur->x2 = cur->x;
      }
    else
      {
	cur->x2 = mr->width -5;
      }
  }
  mr->in_use = 0;
  mr->height = y+2;

#ifndef NO_SAVEUNDERS   
  valuemask = (CWBackPixel | CWEventMask | CWCursor | CWSaveUnder);
#else
  valuemask = (CWBackPixel | CWEventMask | CWCursor);
#endif
  attributes.background_pixel = Scr.MenuColors.back;
  attributes.event_mask = (ExposureMask | EnterWindowMask);
  attributes.cursor = Scr.FvwmCursors[MENU];
#ifndef NO_SAVEUNDERS   
  attributes.save_under = TRUE;
#endif
  mr->width = mr->width + mr->width2;
  mr->w = XCreateWindow (dpy, Scr.Root, 0, 0, (unsigned int) (mr->width),
			 (unsigned int) mr->height, (unsigned int) 0,
			 CopyFromParent, (unsigned int) InputOutput,
			 (Visual *) CopyFromParent,
			 valuemask, &attributes);
  XSaveContext(dpy,mr->w,MenuContext,(caddr_t)mr);
  
  return;
}

/***********************************************************************
 * Procedure:
 *	scanForHotkeys - Look for hotkey markers in a MenuItem
 * 							(pete@tecc.co.uk)
 * 
 * Inputs:
 *	it	- MenuItem to scan
 * 	which 	- +1 to look in it->item1 and -1 to look in it->item2.
 *
 ***********************************************************************/

void scanForHotkeys(MenuItem *it, int which) 
{
  char *start, *txt;

  start = (which > 0) ? it->item : it->item2;	/* Get start of string	*/
  for (txt = start; *txt != '\0'; txt++) 
    {	/* Scan whole string	*/
      if (*txt == '&') 
	{		/* A hotkey marker?			*/
      if (txt[1] == '&') 
	{	/* Just an escaped &			*/
	  char *tmp;		/* Copy the string down over it		*/
	  for (tmp = txt; *tmp != '\0'; tmp++) tmp[0] = tmp[1];
	  continue;		/* ...And skip to the key char		*/
	}
      /* It's a hot key marker - work out the offset value		*/
      it->hotkey = (1 + (txt - start)) * which;
      for (; *txt != '\0'; txt++) txt[0] = txt[1];	/* Copy down..	*/
      return;			/* Only one hotkey per item...		*/
    }
  }
  it->hotkey = 0;		/* No hotkey found.  Set offset to zero	*/
}



/***********************************************************************
 *
 *  Procedure:
 *	AddToMenu - add an item to a root menu
 *
 *  Returned Value:
 *	(MenuItem *)
 *
 *  Inputs:
 *	menu	- pointer to the root menu to add the item
 *	item	- the text to appear in the menu
 *	action	- the string to possibly execute
 *	func	- the numeric function
 *
 ***********************************************************************/
void AddToMenu(MenuRoot *menu, char *item, char *item2, char *action,int func, 
	       long func_val_1,long func_val_2, char unit_1, char unit_2)
{
  MenuItem *tmp;
  int width;


  if(item == NULL)
     return;
  tmp = (MenuItem *)safemalloc(sizeof(MenuItem));
  if (menu->first == NULL)
    {
      menu->first = tmp;
      tmp->prev = NULL;
    }
  else
    {
      menu->last->next = tmp;
      tmp->prev = menu->last;
    }
  menu->last = tmp;
  
  tmp->item = item;
  if (item != (char *)0)
    {
      scanForHotkeys(tmp, 1);				/* pete@tecc.co.uk */
      tmp->strlen = strlen(item);
    }
  else
    tmp->strlen = 0;

  tmp->item2 = item2;
  if (item2 != (char *)0)
    {
      if (tmp->hotkey == 0) scanForHotkeys(tmp, -1);	/* pete@tecc.co.uk */
      tmp->strlen2 = strlen(item2);
    }
  else
    tmp->strlen2 = 0;
  tmp->menu = 0;

  if((func == F_POPUP)||(func == F_FUNCTION))
    {
      unsigned i;
      if(action != (char *)0)
	{
	  for (i = 0; i < PopupCount; i++)
	    if (mystrcasecmp(PopupTable[i]->name,action) == 0)
	      {
		tmp->menu = PopupTable[i];
		break;
	      }
	}
      if(tmp->menu == (MenuRoot *)0)
	{
	  no_popup(action);
	  func = F_NOP;
	}
    }
  tmp->action = action;
  tmp->next = NULL;
  tmp->state = 0;
  tmp->func = func;
  tmp->val1 = func_val_1;
  tmp->val2 = func_val_2;
  if((unit_1 == 'p')||(unit_1 == 'P'))
    tmp->val1_unit = 100;
  else
    tmp->val1_unit = Scr.MyDisplayWidth;
  if((unit_2 == 'p')||(unit_2 == 'P'))
    tmp->val2_unit = 100;
  else
    tmp->val2_unit = Scr.MyDisplayHeight;

  width = XTextWidth(Scr.StdFont.font, item, tmp->strlen);
  if(tmp->func == F_POPUP)
    width += 15;
  if (width <= 0)
    width = 1;
  if (width > menu->width)
    menu->width = width;
  width = XTextWidth(Scr.StdFont.font, item2, tmp->strlen2);
  if (width < 0)
    width = 0;
  if (width > menu->width2)
    menu->width2 = width;
  if((width==0)&&(tmp->strlen2>0))
    menu->width2 = 1;
  
  tmp->item_num = menu->items++;
}

/***********************************************************************
 *
 *  Procedure:
 *	NewMenuRoot - create a new menu root
 *
 *  Returned Value:
 *	(MenuRoot *)
 *
 *  Inputs:
 *	name	- the name of the menu root
 *
 ***********************************************************************/
MenuRoot *NewMenuRoot(char *name)
{
  MenuRoot *tmp;
  
  tmp = (MenuRoot *) safemalloc(sizeof(MenuRoot));
  tmp->name = name;
  tmp->first = NULL;
  tmp->last = NULL;
  tmp->items = 0;
  tmp->width = 0;
  tmp->width2 = 0;
  tmp->w = None;
  return (tmp);
}



/***********************************************************************
 *
 *  Procedure:
 *	AddFuncKey - add a function key to the list
 *
 *  Inputs:
 *	name	- the name of the key
 *	cont	- the context to look for the key press in
 *	mods	- modifier keys that need to be pressed
 *	func	- the function to perform
 *	action	- the action string associated with the function (if any)
 *
 ***********************************************************************/
void AddFuncKey (char *name, int cont, int mods, int func,  char *action,
		 int val1, int val2,MenuRoot *mr, char unit_1, char unit_2)
{
  FuncKey *tmp;
  KeySym keysym;
  KeyCode keycode;
  int i, min, max;

  /*
   * Don't let a 0 keycode go through, since that means AnyKey to the
   * XGrabKey call in GrabKeys().
   */
  if ((keysym = XStringToKeysym(name)) == NoSymbol ||
      (keycode = XKeysymToKeycode(dpy, keysym)) == 0)
    return;
  
 
  XDisplayKeycodes(dpy, &min, &max);
  for (i=min; i<=max; i++)
    if (XKeycodeToKeysym(dpy, i, 0) == keysym)
      {
	tmp = (FuncKey *) safemalloc(sizeof(FuncKey));
	tmp->next = Scr.FuncKeyRoot.next;
	Scr.FuncKeyRoot.next = tmp;
	
	tmp->name = name;
	tmp->keycode = i;
	tmp->cont = cont;
	tmp->mods = mods;
	tmp->func = func;
	tmp->action = action;
	tmp->val1 = val1;
	tmp->val2 = val2;
	if((unit_1 == 'p')||(unit_1 == 'P'))
	  tmp->val1_unit = 100;
	else
	  tmp->val1_unit = Scr.MyDisplayWidth;
	if((unit_2 == 'p')||(unit_2 == 'P'))
	  tmp->val2_unit = 100;
	else
	  tmp->val2_unit = Scr.MyDisplayHeight;
	    
	tmp->menu = mr;
      }
  return;
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

  XGetWindowAttributes(dpy,Scr.Root,&attributes);
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

void nocolor(char *note, char *name)
{
  fvwm_err("can't %s color %s", note,name,NULL);
}
/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips leading spaces and trailing spaces and new lines
 *
 ****************************************************************************/ 
char *stripcpy(char *source)
{
  char *tmp,*ptr;
  int len;

  while(isspace(*source))
    source++;
  len = strlen(source);
  tmp = source + len -1;
  while(((isspace(*tmp))||(*tmp == '\n'))&&(tmp >=source))
    {
      tmp--;
      len--;
    }
  ptr = safemalloc(len+1);
  strncpy(ptr,source,len);
  ptr[len]=0;
  return ptr;
}
  

/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips all data before the first quote and after the second
 *
 ****************************************************************************/
char *stripcpy2(char *source, int tab_sensitive, Bool error)
{
  char *ptr;
  int count;
  while((*source != '"')&&(*source != 0))
    source++;
  if(*source == 0)
    {
      if(error)
	bad_binding(2);
      return 0;
    }
  source++;
  ptr = source;
  count = 0;
  if(!tab_sensitive)
    while((*ptr!='"')&&(*ptr != 0))
      {
	ptr++;  
	count++;
      }
  else if(tab_sensitive==1)
    while((*ptr!='"')&&(*ptr != 0)&&(*ptr!='\t'))
      {
	ptr++;  
	count++;
      }
  else if(tab_sensitive==2)
    {
      while((*ptr!='"')&&(*ptr != 0)&&(*ptr!='\t'))
	{
	  source++;
	  ptr++;
	}
      if((*ptr!='"')&&(*ptr != 0))
	{
	  ptr++;
	  source++;
	}
      while((*ptr!='"')&&(*ptr != 0))
	{
	  ptr++;
	  count++;
	}
    }
  ptr = safemalloc(count+1);
  strncpy(ptr,source,count);
  ptr[count]=0;
  return ptr;
}


/****************************************************************************
 * 
 * Copies a string into a new, malloc'ed string
 * Strips all data before the second quote. and strips trailing spaces and
 * new lines
 *
 ****************************************************************************/
char *stripcpy3(char *source,Bool Warn)
{
  while((*source != '"')&&(*source != 0))
    source++;
  if(*source != 0)
    source++;
  while((*source != '"')&&(*source != 0))
    source++;
  if(*source == 0)
    {
      if(Warn)bad_binding(3);
      return 0;
    }
  source++;
  return stripcpy(source);
}
  
void bad_binding(int num)
{
  fvwm_err("bad binding in line %s",orig_tline,NULL,NULL);
  return;
}


/***********************************************************************
 *
 *  Procedure:
 *	CreateGCs - open fonts and create all the needed GC's.  I only
 *		    want to do this once, hence the first_time flag.
 *
 ***********************************************************************/
void CreateGCs(void)
{
  XGCValues gcv;
  unsigned long gcm;
  
  /* create GC's */
  gcm = GCFunction|GCLineWidth|GCForeground|GCSubwindowMode; 
  gcv.function = GXxor;
  gcv.line_width = 0;
  gcv.foreground = XORvalue;
  gcv.subwindow_mode = IncludeInferiors;
  Scr.DrawGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground|GCFont;
  gcv.line_width = 0;
  gcv.function = GXcopy;
  gcv.plane_mask = AllPlanes;
  gcv.foreground = Scr.StdColors.fore;
  gcv.background = Scr.StdColors.back;
  gcv.font =  Scr.StdFont.font->fid;
  /*
   * Prevent GraphicsExpose and NoExpose events.  We'd only get NoExpose
   * events anyway;  they cause BadWindow errors from XGetWindowAttributes
   * call in FindScreenInfo (events.c) (since drawable is a pixmap).
   */
  gcv.graphics_exposures = False;
  
  Scr.NormalGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  /* GC for pager labels */
  Scr.FontGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.fill_style = FillStippled;
  gcv.stipple = Scr.gray_bitmap;
  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground|GCFont|GCStipple|GCFillStyle;

  Scr.StippleGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcm = GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
    GCBackground|GCFont;
  gcv.foreground = Scr.HiRelief.fore;
  gcv.background = Scr.HiRelief.back;
  gcv.fill_style = FillSolid;
  Scr.HiReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.HiRelief.back;
  gcv.background = Scr.HiRelief.fore;
  Scr.HiShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.MenuColors.fore;
  gcv.background = Scr.MenuColors.back;
  Scr.MenuGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
  if(Scr.d_depth < 2)
    {
      gcv.fill_style = FillStippled;
      gcv.stipple = Scr.gray_bitmap;
      gcm=GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
	GCBackground|GCFont|GCStipple|GCFillStyle;
      Scr.MenuStippleGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
      
      gcm=GCFunction|GCPlaneMask|GCGraphicsExposures|GCLineWidth|GCForeground|
	GCBackground|GCFont;
      gcv.fill_style = FillSolid;
    }
  else
    {
      gcv.foreground = Scr.MenuStippleColors.fore;
      gcv.background = Scr.MenuStippleColors.back;
      Scr.MenuStippleGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);
    }
  gcv.foreground = Scr.MenuRelief.fore;
  gcv.background = Scr.MenuRelief.back;
  Scr.MenuReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.MenuRelief.back;
  gcv.background = Scr.MenuRelief.fore;
  Scr.MenuShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.StdRelief.fore;
  gcv.background = Scr.StdRelief.back;
  Scr.StdReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  
  Scr.ScratchGC1 = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = Scr.StdRelief.back;
  gcv.background = Scr.StdRelief.fore;
  Scr.StdShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  
  Scr.ScratchGC2 = XCreateGC(dpy, Scr.Root, gcm, &gcv);

  gcv.foreground = Scr.StickyRelief.fore;
  gcv.background = Scr.StickyRelief.back;
  Scr.StickyReliefGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  

  gcv.foreground = Scr.StickyRelief.back;
  gcv.background = Scr.StickyRelief.fore;
  Scr.StickyShadowGC = XCreateGC(dpy, Scr.Root, gcm, &gcv);  
}

/***********************************************************************
 *
 *  Procedure:
 *	SetOneStyle - add a window name to the no title list
 *
 *  Inputs:
 *	name	- a pointer to the name of the window 
 *
 ***********************************************************************/
#ifndef PRUNE
void SetOneStyle(char *text, FILE *fd, char **list, int *junk)
{
  char *name;
  unsigned long new_flags;
  char *icon_name = NULL;
  int desknumber = 0;

  new_flags = (unsigned long)junk;

  /* first, see if an entry for this name exists */
  if((new_flags & ICON_FLAG) || (new_flags & STAYSONDESK_FLAG))
    name = stripcpy2(text,FALSE,TRUE);
  else
    name = stripcpy(text);

  /* in case there was no argument! */
  if(name == NULL)
    return;

  /* capture default icons */
  if(strlen(name) == 0)
    {
      if(new_flags & ICON_FLAG)
	Scr.DefaultIcon = stripcpy3(text,TRUE);
      free(name);
      return;
    }
  if(new_flags & ICON_FLAG)
    {
      icon_name = stripcpy3(text,TRUE);
    }
  else if (new_flags & STAYSONDESK_FLAG)
    {
      char *p = stripcpy3(text,TRUE);
      desknumber = atoi (p);
      free (p);
    }
  AddToList(name,icon_name,new_flags,0,desknumber,0,0,NULL,NULL,0,0);
}    
#endif 

#ifdef	M4

/* For m4... */
#include <X11/Xmu/CharSet.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/* Code taken and munged from xrdb.c */
#define MAXHOSTNAME 255
#define Resolution(pixels, mm)  ((((pixels) * 100000 / (mm)) + 50) / 100)

/* EXTRA should be the length of the strings
   "define(" + ", ``" + "'')dnl\n". */

#define EXTRA   50

extern int m4_prefix;
extern char m4_prog[];
extern int m4_default_quotes;
extern char m4_startquote[];
extern char m4_endquote[];

static char *MkDef(char *name, char *def)
{
    static char *cp = NULL;
    static int maxsize = 0;
    int n;
    
    /* The char * storage only lasts for 1 call... */

    /* Get space to hold everything, if needed */
    
    n = EXTRA + strlen(name) + strlen(def);
    if (n > maxsize) {
	maxsize = n;
	if (cp == NULL) {
	    cp = malloc(n);
	} else {
	    cp = realloc(cp, n);
	}
    }

    if (cp == NULL) {
	perror("MkDef can't allocate enough space for a macro definition");
	exit(0377);
    }

    /* Create the macro definition, using the appropriate prefix, if any */

    if (m4_prefix)
      {
	strcpy(cp, "m4_define(");
      }
    else
      strcpy(cp, "define(");
    
    strcat(cp, name);

    /* Tack on "," and 2 sets of starting quotes */
    strcat(cp, ",");
    strcat(cp, m4_startquote);
    strcat(cp, m4_startquote);

    /* The definition itself */
    strcat(cp, def);

    /* Add 2 sets of closing quotes */
    strcat(cp, m4_endquote);
    strcat(cp, m4_endquote);

    /* End the definition, appropriately */
    strcat(cp, ")");
    if (m4_prefix)
      {
	strcat(cp, "m4_");
      }

    strcat(cp, "dnl\n");
    
    return(cp);
}

static char *MkNum(char *name,int def)
{
    char num[20];
    
    sprintf(num, "%d", def);
    return(MkDef(name, num));
}

static char *m4_defs(Display *display, const char *host, char *m4_options, char *config_file)
{
    Screen *screen;
    Visual *visual;
    char client[MAXHOSTNAME], server[MAXHOSTNAME], *colon;
    char ostype[BUFSIZ];
    char options[BUFSIZ];
    static char tmp_name[BUFSIZ];
    struct hostent *hostname;
    char *vc;			/* Visual Class */
    FILE *tmpf;
    struct passwd *pwent;
    /* Generate a temporary filename.  Honor the TMPDIR environment variable,
       if set. Hope nobody deletes this file! */

    if ((vc=getenv("TMPDIR"))) {
      strcpy(tmp_name, vc);
    } else {
      strcpy(tmp_name, "/tmp");
    }
    strcat(tmp_name, "/fvwmrcXXXXX");
    mktemp(tmp_name);
    
    if (*tmp_name == '\0')
      {
	perror("mktemp failed in m4_defs");
	exit(0377);
      }

    /*
     * Create the appropriate command line to run m4, and
     * open a pipe to the command.
     */

    sprintf(options, "%s %s %s > %s\n",
	    m4_prog,
	    ((m4_prefix == 0) ? "" : "--prefix-builtins"),
	    m4_options, tmp_name);

    tmpf = popen(options, "w");
    if (tmpf == NULL) {
	perror("Cannot open pipe to m4");
	exit(0377);
    }
    
    mygethostname(client,MAXHOSTNAME);

    mygetostype  (ostype, sizeof ostype);

    /* Change the quoting characters, if specified */

    if (!m4_default_quotes)
      {
	fprintf(tmpf, "changequote(%s, %s)dnl\n", m4_startquote, m4_endquote);
      }

    hostname = gethostbyname(client);
    strcpy(server, XDisplayName(host));
    colon = strchr(server, ':');
    if (colon != NULL) *colon = '\0';
    if ((server[0] == '\0') || (!strcmp(server, "unix")))
      strcpy(server, client);	/* must be connected to :0 or unix:0 */

    /* TWM_TYPE is fvwm, for completeness */

    fputs(MkDef("TWM_TYPE", "fvwm"), tmpf);
    
    /* The machine running the X server */
    fputs(MkDef("SERVERHOST", server), tmpf);
    /* The machine running the window manager process */
    fputs(MkDef("CLIENTHOST", client), tmpf);
    if (hostname)
      fputs(MkDef("HOSTNAME", (char *)hostname->h_name), tmpf);
    else
      fputs(MkDef("HOSTNAME", (char *)client), tmpf);

    fputs(MkDef("OSTYPE", ostype), tmpf);

    pwent=getpwuid(geteuid());
    fputs(MkDef("USER", pwent->pw_name), tmpf);

    fputs(MkDef("HOME", getenv("HOME")), tmpf);
    fputs(MkNum("VERSION", ProtocolVersion(display)), tmpf);
    fputs(MkNum("REVISION", ProtocolRevision(display)), tmpf);
    fputs(MkDef("VENDOR", ServerVendor(display)), tmpf);
    fputs(MkNum("RELEASE", VendorRelease(display)), tmpf);
    screen = ScreenOfDisplay(display, Scr.screen);
    visual = DefaultVisualOfScreen(screen);
    fputs(MkNum("WIDTH", Scr.MyDisplayWidth), tmpf);
    fputs(MkNum("HEIGHT", Scr.MyDisplayHeight), tmpf);

    fputs(MkNum("X_RESOLUTION",Resolution(screen->width,screen->mwidth)),tmpf);
    fputs(MkNum("Y_RESOLUTION",Resolution(screen->height,screen->mheight)),tmpf);
    fputs(MkNum("PLANES",DisplayPlanes(display, Scr.screen)), tmpf);

    fputs(MkNum("BITS_PER_RGB", visual->bits_per_rgb), tmpf);

    switch(visual->class) 
      {
	case(StaticGray):
	  vc = "StaticGray";
	break;
	case(GrayScale):
	  vc = "GrayScale";
	break;
	case(StaticColor):
	  vc = "StaticColor";
	break;
	case(PseudoColor):
	  vc = "PseudoColor";
	break;
	case(TrueColor):
	  vc = "TrueColor";
	break;
	case(DirectColor):
	  vc = "DirectColor";
	break;
      default:
	vc = "NonStandard";
	break;
      }
    
    fputs(MkDef("CLASS", vc), tmpf);
    if (visual->class != StaticGray && visual->class != GrayScale) 
      fputs(MkDef("COLOR", "Yes"), tmpf);
    else 
      fputs(MkDef("COLOR", "No"), tmpf);
    fputs(MkDef("FVWM_VERSION", PACKAGE_VERSION), tmpf);

    /* Add options together */
    *options = '\0';
#ifdef	SHAPE
    strcat(options, "SHAPE ");
#endif
#ifdef	XPM
    strcat(options, "XPM ");
#endif

    strcat(options, "M4 ");

#ifdef	NO_PAGER
    strcat(options, "NO_PAGER ");
#endif
#ifdef	NON_VIRTUAL
    strcat(options, "NON_VIRTUAL ");
#endif
#ifdef	NO_SAVEUNDERS
    strcat(options, "NO_SAVEUNDERS ");
#endif
#ifdef	NO_WINDOWLIST
    strcat(options, "NO_WINDOWLIST ");
#endif
#ifdef	PRUNE
    strcat(options, "PRUNE ");
#endif
    fputs(MkDef("OPTIONS", options), tmpf);

    fputs(MkDef("FVWMDIR", FVWMDIR), tmpf);
    
    /*
     * At this point, we've sent the definitions to m4.  Just include
     * the fvwmrc file now.
     */
    
    fprintf(tmpf, "%sinclude(%s%s%s)\n",
	    (m4_prefix) ? "m4_": "",
	    m4_startquote,
	    config_file,
	    m4_endquote);
    
    pclose(tmpf);
    return(tmp_name);
}
#endif /* M4 */
