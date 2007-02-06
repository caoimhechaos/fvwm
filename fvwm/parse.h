/****************************************************************************
 * This module was originally based on the twm module of the same name. 
 * Since its use and contents have changed so dramatically, I have removed
 * the original twm copyright, and inserted my own.
 *
 * by Rob Nation 
 * Copyright 1993 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved
 ****************************************************************************/

/**********************************************************************
 *
 * Codes for fvwm builtins 
 *
 **********************************************************************/

#ifndef _PARSE_
#define _PARSE_

#define F_NOP			0
#define F_BEEP			1
#define F_QUIT			2
#define F_RESTART               3
#define F_REFRESH		4
#define F_TITLE			5
#define F_SCROLL                6      /* scroll the virtual desktop */
#define F_CIRCULATE_UP          7
#define F_CIRCULATE_DOWN        8
#define F_TOGGLE_PAGE           9
#define F_GOTO_PAGE             10
#define F_WINDOWLIST            11
#define F_MOVECURSOR            12
#define F_FUNCTION              13
#define F_WARP                  14
#define F_MODULE                15
#define F_DESK                  16
#define F_CHANGE_WINDOWS_DESK   17
#define F_EXEC			18	/* string */
#define F_POPUP			19	/* string */
#define F_WAIT                  20
#define F_CLOSE                 21
#define F_QUICKIE               22

/* Functions which require a target window */
#define F_RESIZE		100
#define F_RAISE			101
#define F_LOWER			102
#define F_DESTROY		103
#define F_DELETE		104
#define F_MOVE			105
#define F_ICONIFY		106
#define F_STICK                 107
#define F_RAISELOWER            108
#define F_MAXIMIZE              109
#define F_FOCUS                 110


/* Functions for use by modules only! */
#define F_SEND_WINDOW_LIST     1000

/* Functions for internal  only! */
#define F_RAISE_IT              2000

#endif /* _PARSE_ */

