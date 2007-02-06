/****************************************************************************
 * This module is all original code 
 * by Rob Nation
 * Copyright 1993, Robert Nation
 *     You may use this code for any purpose, as long as the original
 *     copyright remains in the source code and all documentation
 ****************************************************************************/

/***********************************************************************
 *
 * code for launching fvwm modules.
 *
 ***********************************************************************/

#include "../configure.h"

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <X11/keysym.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "fvwm.h"
#include "menus.h"
#include "misc.h"
#include "parse.h"
#include "screen.h"
#include "module.h"


int npipes;
int *readPipes;
int *writePipes;
int *pipeOn;
unsigned long *PipeMask;
struct queue_buff_struct **pipeQueue;

FVWM_INLINE int PositiveWrite(int module, unsigned long *ptr, int size);
void DeleteQueueBuff(int module);
void AddToQueue(int module, unsigned long *ptr, int size, int done);

void initModules(void)
{
  int i;

  npipes = GetFdWidth();

  writePipes = (int *)safemalloc(sizeof(int)*npipes);
  readPipes = (int *)safemalloc(sizeof(int)*npipes);
  pipeOn = (int *)safemalloc(sizeof(int)*npipes);
  PipeMask = (unsigned long *)safemalloc(sizeof(unsigned long)*npipes);
  pipeQueue=(struct queue_buff_struct **)
    safemalloc(sizeof(struct queue_buff_struct *)*npipes);

  for(i=0;i<npipes;i++)
    {
      writePipes[i]= -1;
      readPipes[i]= -1;
      pipeOn[i] = -1;
      PipeMask[i] = MAX_MASK;
      pipeQueue[i] = (struct queue_buff_struct *)NULL;

    }
}

void ClosePipes(void)
{
  int i;
  for(i=0;i<npipes;i++)   
    {
      if(writePipes[i]>0)
	{
	  close(writePipes[i]);
	  close(readPipes[i]);
	}
      while(pipeQueue[i] != NULL)
	{
	  DeleteQueueBuff(i);
	}
    }
}

void executeModule(char *action,FILE *fd, char **win, int *context)
{
  int fvwm_to_app[2],app_to_fvwm[2];
  int i,val;
  char command[256];
  char *cptr;
  char *aptr;
  char *args[10];
  char *arg1 = NULL;
  char arg2[40];
  char arg3[40];
  char arg5[40];
  char arg6[40];
  char *end;
  extern char *ModulePath;
  extern FILE *config_fd;
  extern char *fvwm_file;

  if(action == NULL)
    return;
  strcpy(command,action);

  cptr = command;
  while((isspace(*cptr))&&(*cptr != '\n')&&(*cptr != 0))
    cptr++;

  end = cptr;
  while((!(isspace(*end))&&(*end != '\n'))&&(*end != 0)&&(end <(command+256)))
    end++;

  if((*end == 0)||(end >= command+256))
    aptr = NULL;
  else
    aptr = end+1;
  *end = 0;

  if(aptr)
    {
      while((isspace(*aptr)||(*aptr=='\n'))&&(*aptr!=0)&&(aptr<(command+256)))
	aptr++;
      if((*aptr == 0)||(*aptr == '\n'))
	aptr = NULL;
    }

  arg1 = findIconFile(cptr,ModulePath,X_OK);
  if(arg1 == NULL)
    {
      fprintf(stderr,"Fvwm: No such module %s %s\n",ModulePath,cptr);
      return;
    }

  /* Look for an available pipe slot */
  i=0;
  while((i<npipes) && (writePipes[i] >=0))
    i++;
  if(i>=npipes)
    {
      fprintf(stderr,"fvwm: Too many Accessories!\n");
      return;
    }
  
  /* I want one-ended pipes, so I open two two-ended pipes,
   * and close one end of each. I need one ended pipes so that
   * I can detect when the module crashes/malfunctions */
  if(pipe(fvwm_to_app)!=0)
    {
      fprintf(stderr,"Fvwm: Failed to open pipe\n");
      return;
    }
  if(pipe(app_to_fvwm)!=0)
    {
      fprintf(stderr,"Fvwm: Failed to open pipe2\n");
      close(fvwm_to_app[0]);
      close(fvwm_to_app[1]);
      return;
    }

  
  val = fork();
  if(val > 0)
    {
      /* This fork remains running fvwm */
      /* close appropriate descriptors from each pipe so
       * that fvwm will be able to tell when the app dies */
      close(app_to_fvwm[1]);
      close(fvwm_to_app[0]);

      /* add these pipes to fvwm's active pipe list */
      writePipes[i] = fvwm_to_app[1];
      readPipes[i] = app_to_fvwm[0];
      pipeOn[i] = -1;
      PipeMask[i] = MAX_MASK;
      free(arg1);
      pipeQueue[i] = NULL;

      /* make the PositiveWrite pipe non-blocking. Don't want to jam up
	 fvwm because of an uncooperative module */
      fcntl(writePipes[i],F_SETFL,O_NDELAY);
      /* Mark the pipes close-on exec so other programs
       * won`t inherit them */
      if (fcntl(readPipes[i], F_SETFD, 1) == -1) 
	fvwm_err("module close-on-exec failed",NULL,NULL,NULL);
      if (fcntl(writePipes[i], F_SETFD, 1) == -1) 
	fvwm_err("module close-on-exec failed",NULL,NULL,NULL);
    }
  else if (val ==0)
    {
      /* this is  the child */
      /* need to close config_fd if its still open! */
      if(config_fd != (FILE *)NULL)
	/* Fixes some funny stuff with svr4 and stream IO */
	/* fclose(config_fd) */
	close(fileno(config_fd));

      /* this fork execs the module */
      close(fvwm_to_app[1]);
      close(app_to_fvwm[0]);
      sprintf(arg2,"%d",app_to_fvwm[1]);
      sprintf(arg3,"%d",fvwm_to_app[0]);
      sprintf(arg5,"%lx",(unsigned long)win);
      sprintf(arg6,"%lx",(unsigned long)context);
      args[0]=arg1;
      args[1]=arg2;
      args[2]=arg3;
      args[3]=fvwm_file;
      args[4]=arg5;
      args[5]=arg6;
      if(aptr != NULL)
	{
	  args[6] = aptr;
	  args[7] = 0;
	}
      else
	args[6]= 0;
      execvp(arg1,args);
      fprintf(stderr,"Execution of module failed: %s",arg1);      
      perror("");
      close(app_to_fvwm[1]);
      close(fvwm_to_app[0]);
      exit(1);
    }
  else
    {
      fprintf(stderr,"Fork failed\n");
      free(arg1);
    }
  return;
}

void HandleModuleInput(Window w, int channel)
{
  char text[256];
  int size;
  int cont,n;
  char *newaction = NULL;

  /* Already read a (possibly NULL) window id from the pipe,
   * Now read an fvwm bultin command line */
  n = read(readPipes[channel], &size, sizeof(int));
  if(n < sizeof(int))
    {
      KillModule(channel,1);
      return;
    }

  if(size >255)
    {
      fprintf(stderr,"Module command is too big (%d)\n",size);
      size=255;
    }

  pipeOn[channel] = 1;

  n = read(readPipes[channel],text, size);
  if(n < size)
    {
      KillModule(channel,2);
      return;
    }
  
  text[n]=0;
  n = read(readPipes[channel],&cont, sizeof(int));
  if(n < sizeof(int))
    {
      KillModule(channel,3);
      return;
    }
  if(cont == 0)
    {
      KillModule(channel,4);
    }
  if(strlen(text)>0)
    {
      char function[256],*ptr;
      MenuRoot *mr=0;
      char *item=NULL;
      extern int func_val_1,func_val_2,func,Context;
      extern struct config func_config[];
      extern unsigned PopupCount;
      extern MenuRoot *PopupTable[MAXPOPUPS];      
      FvwmWindow *tmp_win;
      extern char *orig_tline;
      int n,unit_val_1,unit_val_2;
      char unit_1, unit_2;


      orig_tline = text;
      Event.xany.type = ButtonRelease;
      Event.xany.window = w;
	
      func_val_1 = 0;
      func_val_2 = 0;
      unit_1 = 's';
      unit_2 = 's';
      n = sscanf(text,"%s %d %d",function,&func_val_1,&func_val_2);
      if(n != 3)
	n = sscanf(text,"%s %d%c %d%c",function,&func_val_1,&unit_1,&func_val_2,&unit_2);
  
      if(mystrcasecmp(function,"SET_MASK")==0)
	{
	  PipeMask[channel] = func_val_1;
	  return;
	}
      func = F_NOP;
      match_string(func_config,function,"bad mouse function:",NULL);
      if((func == F_POPUP)||(func == F_FUNCTION))
	{
	  unsigned i;
	  ptr = stripcpy2(text,0,True);
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
	}
      else if((func == F_EXEC)||(func == F_RESTART)||
	      (func == F_CIRCULATE_UP)||(func == F_CIRCULATE_DOWN)||
	      (func == F_WARP)||(func == F_MODULE))
	{
	  if((func == F_EXEC)||(func == F_RESTART)||(func== F_MODULE))
	    {
	      item = stripcpy2(text,0,True);
	      newaction = stripcpy3(text,True);
	    }
	  else
	    {
	      item = stripcpy2(text,0,False);
	      newaction = stripcpy3(text,False);
	    }
	}
      if (XFindContext (dpy, w, FvwmContext, (caddr_t *) &tmp_win) == XCNOENT)
	{
	  tmp_win = NULL;
	  w = None;
	}
      if(tmp_win)
	{
	  Event.xbutton.button = 1;
	  Event.xbutton.x_root = tmp_win->frame_x;
	  Event.xbutton.y_root = tmp_win->frame_y;
	  Event.xbutton.x = 0;
	  Event.xbutton.y = 0;
	  Event.xbutton.subwindow = None;
	}
      else
	{
	  Event.xbutton.button = 1;
	  Event.xbutton.x_root = 0;
	  Event.xbutton.y_root = 0;
	  Event.xbutton.x = 0;
	  Event.xbutton.y = 0;
	  Event.xbutton.subwindow = None;
	}
      if(unit_1 == 'p')
	unit_val_1 = 100;
      else
	unit_val_1 = Scr.MyDisplayWidth;
      if(unit_2 == 'p')
	unit_val_2 = 100;
      else
	unit_val_2 = Scr.MyDisplayHeight;

      Context = GetContext(tmp_win,&Event,&w);
      ExecuteFunction(func,newaction, w, tmp_win, &Event, Context,
		      func_val_1,func_val_2,unit_val_1,unit_val_2,mr,channel);
    }
  return;
}


void DeadPipe(int nonsense)
{
  signal(SIGPIPE, DeadPipe);
}


void KillModule(int channel, int place)
{
  close(readPipes[channel]);
  close(writePipes[channel]);
  
  readPipes[channel] = -1;
  writePipes[channel] = -1;
  pipeOn[channel] = -1;
  while(pipeQueue[channel] != NULL)
    {
      DeleteQueueBuff(channel);
    }
  return;
}


void Broadcast(unsigned long event_type, unsigned long num_datum,
	       unsigned long data1, unsigned long data2, unsigned long data3, 
	       unsigned long data4, unsigned long data5, unsigned long data6,
	       unsigned long data7)
{
  int i;
  unsigned long body[10];

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = num_datum+3;
    
  if(num_datum>0)
    body[3] = data1;
  if(num_datum>1)
    body[4] = data2;
  if(num_datum>2)
    body[5] = data3;
  if(num_datum>3)
    body[6] = data4;
  if(num_datum>4)
    body[7] = data5;
  if(num_datum>5)
    body[8] = data6;
  if(num_datum>6)
    body[9] = data7;

  for(i=0;i<npipes;i++)   
    PositiveWrite(i,body, (num_datum+3)*sizeof(unsigned long));
}




void SendPacket(int module, unsigned long event_type, unsigned long num_datum,
	       unsigned long data1, unsigned long data2, unsigned long data3, 
	       unsigned long data4, unsigned long data5, unsigned long data6,
	       unsigned long data7)
{
  unsigned long body[10];

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = num_datum+3;
    
  if(num_datum>0)
    body[3] = data1;
  if(num_datum>1)
    body[4] = data2;
  if(num_datum>2)
    body[5] = data3;
  if(num_datum>3)
    body[6] = data4;
  if(num_datum>4)
    body[7] = data5;
  if(num_datum>5)
    body[8] = data6;
  if(num_datum>6)
    body[9] = data7;
  PositiveWrite(module,body,(num_datum+3)*sizeof(unsigned long));
}

void SendConfig(int module, unsigned long event_type, FvwmWindow *t)
{
  unsigned long body[MAX_BODY_SIZE+HEADER_SIZE];

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = 27;
  body[3] = t->w;
  body[4] = t->frame;
  body[5] = (unsigned long)t;
  body[6] = t->frame_x;
  body[7] = t->frame_y;
  body[8] = t->frame_width;
  body[9] = t->frame_height;
  body[10] = t->Desk;
  body[11] = t->flags;
  body[12] = t->title_height;
  body[13] = t->boundary_width;
  body[14] = t->hints.base_width;
  body[15] = t->hints.base_height;
  body[16] = t->hints.width_inc;
  body[17] = t->hints.height_inc;
  body[18] = t->hints.min_width;
  body[19] = t->hints.min_height;
  body[20] = t->hints.max_width;
  body[21] = t->hints.max_height;
  body[22] = t->icon_w;
  body[23] = t->icon_pixmap_w;
  body[24] = t->hints.win_gravity;
  body[25] = t->TextPixel;
  body[26] = t->BackPixel;
  
  PositiveWrite(module,body,27*sizeof(unsigned long));
}


void BroadcastConfig(unsigned long event_type, FvwmWindow *t)
{
  unsigned long body[MAX_BODY_SIZE+HEADER_SIZE];
  int i;

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = 27;
  body[3] = t->w;
  body[4] = t->frame;
  body[5] = (unsigned long)t;
  body[6] = t->frame_x;
  body[7] = t->frame_y;
  body[8] = t->frame_width;
  body[9] = t->frame_height;
  body[10] = t->Desk;
  body[11] = t->flags;
  body[12] = t->title_height;
  body[13] = t->boundary_width;
  body[14] = t->hints.base_width;
  body[15] = t->hints.base_height;
  body[16] = t->hints.width_inc;
  body[17] = t->hints.height_inc;
  body[18] = t->hints.min_width;
  body[19] = t->hints.min_height;
  body[20] = t->hints.max_width;
  body[21] = t->hints.max_height;
  body[22] = t->icon_w;
  body[23] = t->icon_pixmap_w;
  body[24] = t->hints.win_gravity;
  body[25] = t->TextPixel;
  body[26] = t->BackPixel;

  for(i=0;i<npipes;i++)   
    {  
      PositiveWrite(i,body,27*sizeof(unsigned long));
    }
}

void BroadcastName(unsigned long event_type, unsigned long data1,
		   unsigned long data2, unsigned long data3, char *name)
{
  int l,i;
  unsigned long *body;


  if(name==NULL)
    return;
  l=strlen(name)/(sizeof(unsigned long))+7;
  body = (unsigned long *)safemalloc(l*sizeof(unsigned long));

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = l;

  body[3] = data1;
  body[4] = data2;
  body[5] = data3; 
  strcpy((char *)&body[6],name);


  for(i=0;i<npipes;i++)   
    PositiveWrite(i,(unsigned long *)body, l*sizeof(unsigned long));
      
  free(body);

}


void SendName(int module, unsigned long event_type,
	      unsigned long data1,unsigned long data2, 
	      unsigned long data3, char *name)
{
  int l;
  unsigned long *body;

  if(name == NULL)
    return;
  l=strlen(name)/(sizeof(unsigned long))+7;
  body = (unsigned long *)safemalloc(l*sizeof(unsigned long));

  body[0] = START_FLAG;
  body[1] = event_type;
  body[2] = l;

  body[3] = data1;
  body[4] = data2;
  body[5] = data3; 
  strcpy((char *)&body[6],name);

  PositiveWrite(module,(unsigned long *)body, l*sizeof(unsigned long));

  free(body);
}



#include <sys/errno.h>
FVWM_INLINE int PositiveWrite(int module, unsigned long *ptr, int size)
{
  if((pipeOn[module]<0)||(!((PipeMask[module]) & ptr[1])))
    return -1;

  AddToQueue(module,ptr,size,0);
  return size;
}


void AddToQueue(int module, unsigned long *ptr, int size, int done)
{
  struct queue_buff_struct *c,*e;
  unsigned long *d;

  c = (struct queue_buff_struct *)safemalloc(sizeof(struct queue_buff_struct));
  c->next = NULL;
  c->size = size;
  c->done = done;
  d = (unsigned long *)safemalloc(size);
  c->data = d;
  memcpy(d,ptr,size);

  e = pipeQueue[module];
  if(e == NULL)
    {
      pipeQueue[module] = c;
      return;
    }
  while(e->next != NULL)
    e = e->next;
  e->next = c;
}

void DeleteQueueBuff(int module)
{
  struct queue_buff_struct *a;

  if(pipeQueue[module] == NULL)
     return;
  a = pipeQueue[module];
  pipeQueue[module] = a->next;
  free(a->data);
  free(a);
  return;
}

void FlushQueue(int module)
{
  char *dptr;
  struct queue_buff_struct *d;
  int a;
  extern int errno;

  if((pipeOn[module] <= 0)||(pipeQueue[module] == NULL))
    return;

  while(pipeQueue[module] != NULL)
    {
      d = pipeQueue[module];
      dptr = (char *)d->data;
      while(d->done < d->size)
	{
	  a = write(writePipes[module],&dptr[d->done], d->size - d->done);
	  if(a >=0)
	    d->done += a;
	  /* the write returns EWOULDBLOCK or EAGAIN if the pipe is full.
	   * (This is non-blocking I/O). SunOS returns EWOULDBLOCK, OSF/1
	   * returns EAGAIN under these conditions. Hopefully other OSes
	   * return one of these values too. Solaris 2 doesn't seem to have
	   * a man page for write(2) (!) */
	  else if ((errno == EWOULDBLOCK)||(errno == EAGAIN)||(errno==EINTR))
	    {
	      return;
	    }
	  else
	    {
	      KillModule(module,123);
	      return;
	    }
	}
      DeleteQueueBuff(module);
    }
}
