/* This module, and the entire NoClutter program, and the concept for
 * interfacing this module to the Window Manager, are all original work
 * by Robert Nation
 *
 * Copyright 1994, Robert Nation. No guarantees or warantees or anything
 * are provided or implied in any way whatsoever. Use this program at your
 * own risk. Permission to use this program for any purpose is given,
 * as long as the copyright is kept intact. */

#define TRUE 1
#define FALSE 

#include <config.h>
#ifdef HAVE_SYS_BSDTYPES_H
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
#include <fvwm/module.h>

#include "FvwmClean.h"

char *MyName;
int fd_width;
int fd[2];
struct list *list_root = NULL;
unsigned long current_focus = 0;

long period[3] = {-1,-1,-1};
long maxperiod = -1;
char command[3][256];
int num_commands = 0;

unsigned long next_event_time = 0;
unsigned long t0;

/***********************************************************************
 *
 *  Procedure:
 *	main - start of module
 *
 ***********************************************************************/
void main(int argc, char **argv)
{
  char *temp, *s;
  FILE *file;

  /* Save the program name - its used for error messages and option parsing */
  temp = argv[0];

  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName,"*");
  strcat(MyName, temp);

  if((argc != 6)&&(argc != 7))
    {
      fprintf(stderr,"%s Version %s should only be executed by fvwm!\n",MyName,
	      PACKAGE_VERSION);
      exit(1);
    }

  /* Save the program name - its used for error messages and option parsing */
  temp = argv[0];

  s=strrchr(argv[0], '/');
  if (s != NULL)
    temp = s + 1;

  MyName = safemalloc(strlen(temp)+2);
  strcpy(MyName,"*");
  strcat(MyName, temp);

  /* Dead pipes mean fvwm died */
  signal (SIGPIPE, DeadPipe);  
  
  fd[0] = atoi(argv[1]);
  fd[1] = atoi(argv[2]);

  /* scan config file for set-up parameters */
  file = fopen(argv[3],"r");
  if(file != (FILE *)NULL)
    {
      char line[256];
      char *tline;

      tline = fgets(line,(sizeof line)-1,file);
      while((tline != (char *)0)&&(num_commands < 3))
	{
	  while(isspace(*tline))tline++;
	  if((strlen(&tline[0])>1)&&
	     (mystrncasecmp(tline, MyName,strlen(MyName))==0))
	    {
	      sscanf(&tline[strlen(MyName)],"%ld",&period[num_commands]);
	      if(period[num_commands]>maxperiod)
		maxperiod = period[num_commands];
	      while(!isspace(*tline))tline++;
	      while(isspace(*tline))tline++; /* points to "time" field */
	      while(!isspace(*tline))tline++;
	      while(isspace(*tline))tline++; /* points to "command" field */
	      strcpy(command[num_commands],tline);
	      num_commands++;
	    }
	  tline = fgets(line,(sizeof line)-1,file);
	}
    }

  if(num_commands == 0)
    {
      fprintf(stderr,"%s: Nothing to do!\n",MyName);
      exit(0);
    }

  fd_width = GetFdWidth();

  /* Create a list of all windows */
  /* Request a list of all windows,
   * wait for ConfigureWindow packets */
  SendInfo(fd,"Send_WindowList",0);

  Loop(fd);
}


/***********************************************************************
 *
 *  Procedure:
 *	Loop - wait for data to process
 *
 ***********************************************************************/
void Loop(int *fd)
{
  unsigned long header[3], *body;
  fd_set in_fdset;
  struct itimerval value;
  int retval, count;

  while(1)
    {
      FD_ZERO(&in_fdset);
      FD_SET(fd[1],&in_fdset);

      /* Find out when we have to iconify or whatever a window */
      find_next_event_time();
      
      /* set up a time-out, in case no packets arrive before we have to
       * iconify something */
      value.it_value.tv_usec = 0;
      value.it_value.tv_sec = next_event_time - t0;

#ifdef __hpux
      if(value.it_value.tv_sec > 0)
	retval=select(fd_width,(int *)&in_fdset, 0, 0, &value.it_value);
      else
	retval=select(fd_width,(int *)&in_fdset, 0, 0, NULL);  
#else      
      if(value.it_value.tv_sec > 0)
	retval=select(fd_width,&in_fdset, 0, 0, &value.it_value);
      else
	retval=select(fd_width,&in_fdset, 0, 0, NULL);  
#endif
      if(FD_ISSET(fd[1], &in_fdset))
	{
	  /* read a packet */
	  if(ReadFvwmPacket(fd[1],header, &body) > 0)
	    {
	      /* dispense with the new packet */
	      process_message(header[1],body);
	      free(body);
	    }
	}
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	Process message - examines packet types, and takes appropriate action
 *
 ***********************************************************************/
void process_message(unsigned long type,unsigned long *body)
{
  struct list *l;

  switch(type)
    {
    case M_ADD_WINDOW:
      if(!find_window(body[0]))
	add_window(body[0]);
      break;
    case M_CONFIGURE_WINDOW:
      if(!find_window(body[0]))
	add_window(body[0]);
      break;
    case M_DESTROY_WINDOW:
      remove_window(body[0]);
      break;
    case M_FOCUS_CHANGE:
      l = find_window(body[0]);
      if(l == 0)
	{
	  add_window(body[0]);
	  l = find_window(body[0]);	  
	  update_focus(l,body[0]);
	}
      else
	update_focus(l,body[0]);
      break;
    default:
      break;
    }
}




/***********************************************************************
 *
 *  Procedure:
 *	safemalloc - mallocs specified space or exits if there's a 
 *		     problem
 *
 ***********************************************************************/
char *safemalloc(int length)
{
  char *ptr;
  
  if(length <=0)
    length = 1;

  ptr = malloc(length);
  if(ptr == (char *)0)
    {
      fprintf(stderr,"%s:malloc failed",MyName);
      exit(1);
    }
  return ptr;
}

/***********************************************************************
 *
 *  Procedure:
 *	find_window - find a window in the current window list 
 *
 ***********************************************************************/
struct list *find_window(unsigned long id)
{
  struct list *l;

  if(list_root == NULL)
    return NULL;

  for(l = list_root; l!= NULL; l= l->next)
    {
      if(l->id == id)
	return l;
    }
  return NULL;
}


/***********************************************************************
 *
 *  Procedure:
 *	remove_window - remove a window in the current window list 
 *
 ***********************************************************************/
void remove_window(unsigned long id)
{
  struct list *l, *prev;

  if(list_root == NULL)
    return;

  prev = NULL;
  for(l = list_root; l!= NULL; l= l->next)
    {
      if(l->id == id)
	{
	  if(prev != NULL)
	    prev->next = l->next;
	  else
	    list_root = l->next;
	  free(l);
	  return;
	}
      prev = l;
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	add_window - add a new window in the current window list 
 *
 ***********************************************************************/
void add_window(unsigned long new_win)
{
  struct list *t;
  
  if(new_win == 0)
    return;

  t = (struct list *)safemalloc(sizeof(struct list));
  t->id = new_win;
  t->last_focus_time = time(0);
  t->actions = 0;
  t->next = list_root;
  list_root = t;
}

				
/***********************************************************************
 *
 *  Procedure:
 *	mark the window which currently has focus, so we don't iconify it
 *
 ***********************************************************************/
void update_focus(struct list *l, unsigned long win)
{
  struct list *t;

  t=find_window(current_focus);
  if(t!= NULL)
    {
      t->last_focus_time = time(0);
      t->actions = 0;
    }
    
  if(l != NULL)
    {
      l->last_focus_time = time(0);
      l->actions = 0;
    }
current_focus = win;
}

/***********************************************************************
 *
 *  Procedure:
 *	SIGPIPE handler - SIGPIPE means fvwm is dying
 *
 ***********************************************************************/
void DeadPipe(int nonsense)
{
  exit(0);
}

/***********************************************************************
 *
 *  Procedure:
 *	checks to see if we are supposed to take some action now,
 *      finds time for next action to be performed.
 *
 ***********************************************************************/
void find_next_event_time(void)
{
  struct list *t;

  t0 = time(0);
  next_event_time = t0;
  
  if(list_root == NULL)
    return;

  next_event_time = t0 + maxperiod;

  for(t = list_root; t!= NULL; t= t->next)  
    {
      if(t->id != current_focus)
	{
	  if((period[2] >0)&&(t->last_focus_time + period[2] <= t0))
	    {
	      if(!(t->actions & ACTION1))
		{
		  SendInfo(fd,command[2],t->id);
		  t->actions |= ACTION1;
		}
	    }
	  else if((period[1]>0)&&(t->last_focus_time + period[1] <= t0))
	    {
	      if(!(t->actions & ACTION2))
		{
		  SendInfo(fd,command[1],t->id);
		  t->actions |= ACTION2;
		}		
	    }
	  else if((period[0]>0)&&(t->last_focus_time + period[0] <= t0))
	    {
	      if(!(t->actions & ACTION3))
		{
		  SendInfo(fd,command[0],t->id);
		  t->actions |= ACTION3;
		}		
	    }

	}
      else
	t->last_focus_time = t0;

      if((period[0] > 0)&&(t->last_focus_time + period[0] > t0)&&
	 (t->last_focus_time + period[0] <next_event_time))
	next_event_time = t->last_focus_time + period[0];
      else if((period[1]>0)&&(t->last_focus_time + period[1] > t0)&
	      (t->last_focus_time + period[1] <next_event_time))
	next_event_time = t->last_focus_time + period[1];
      else if((period[2]>0)&&(t->last_focus_time + period[2] > t0)&&
	      (t->last_focus_time + period[2] <next_event_time))
	next_event_time = t->last_focus_time + period[2];
    }
}


