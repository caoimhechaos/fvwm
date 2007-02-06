AUTOMAKE_OPTIONS =	foreign
bin_PROGRAMS =		fvwm

fvwm_SOURCES =		fvwm.c configure.c events.c borders.c	\
			menus.c functions.c resize.c		\
			add_window.c pager.c move.c icons.c	\
			windows.c module.c placement.c		\
       			decorations.c colormaps.c misc.c style.c
 
fvwm_LDADD =		../libs/libfvwmlib.a
fvwm_DEPENDENCIES =	../libs/libfvwmlib.a

EXTRA_DIST =		../sample.fvwmrc/system.fvwmrc system.fvwmrc

#ifdef AlphaArchitecture
#LOCAL_LIBRARIES = $(XPMLIB) $(XLIB) -ldnet_stub -L../libs -lfvwmlib
#else 
#ifdef HPArchitecture
#LOCAL_LIBRARIES = $(XPMLIB) $(XLIB) -lV3 -L../libs -lfvwmlib
#else
#LOCAL_LIBRARIES = $(XPMLIB) $(XLIB) -lfvwmlib -L../libs
#endif /* HPArchitecture */
#endif /* AlphaArchitecture */

OBJS = fvwm.o configure.o events.o borders.o menus.o functions.o resize.o\
       add_window.o pager.o move.o icons.o windows.o module.o placement.o \
       decorations.o colormaps.o misc.o style.o
