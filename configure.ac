dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.13)
AC_INIT([Feline Virtual Window Manager],1.24r-tch1,[Caoimhe Chaos <caoimhechaos@protonmail.com>],fvwm)
AM_CONFIG_HEADER(config.h)
dnl AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE(fvwm, 1.24r-tch1)

AC_CANONICAL_HOST

AC_MSG_CHECKING(for prefix)
if test x$prefix = xNONE
then
	fvwm_cv_prefix=$ac_default_prefix
else
	fvwm_cv_prefix=$prefix
fi
prefix=$fvwm_cv_prefix
AC_MSG_RESULT($prefix)
AC_SUBST(prefix)

AC_MSG_CHECKING(for exec_prefix)
if test x$exec_prefix = xNONE
then
	if test x$prefix = xNONE
	then
		fvwm_cv_exec_prefix=$ac_default_prefix
	else
		fvwm_cv_exec_prefix=$prefix
	fi
else
	fvwm_cv_exec_prefix=$exec_prefix
fi
exec_prefix=$fvwm_cv_exec_prefix
AC_MSG_RESULT($exec_prefix)
AC_SUBST(exec_prefix)

AC_MSG_CHECKING(for libexecdir)
if test x$libexecdir = xNONE
then
	fvwm_cv_libexecdir=$ac_default_libexecdir
else
	fvwm_cv_libexecdir=$libexecdir
fi
dnl HACK HACK !!
libexecdir=`eval "echo ${fvwm_cv_libexecdir}/fvwm"`
AC_MSG_RESULT($libexecdir)
AC_SUBST(libexecdir)

AC_PROG_CC
AC_PROG_CPP
AC_ISC_POSIX
AM_C_PROTOTYPES
if test "x$U" != "x"
then
  AC_MSG_ERROR(Compiler not ANSI compliant)
fi
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_CHECK_TOOL(AR, ar, ar)
AC_PATH_X

if test "x$x_includes" = "xNONE"
then
  AC_MSG_ERROR(I am a window manager. I want X11.)
fi
CFLAGS="$CFLAGS -I$x_includes"
LIBS="$LIBS -L$x_libraries"
for lib in ICE X11 Xext
do
	AC_CHECK_LIB($lib, main, [LIBS="$LIBS -l$lib"])
done

AC_MSG_CHECKING(for config directory)
fvwmconfdir="$x_libraries/X11/fvwm"
AC_MSG_RESULT($fvwmconfdir)
AC_SUBST(fvwmconfdir)

AC_MSG_CHECKING(for pixmap directory)
pixmapsdir="$x_libraries/X11/fvwm/pixmaps"
AC_MSG_RESULT($pixmapsdir)
AC_SUBST(pixmapsdir)

AC_C_CONST
AC_C_INLINE
AC_EXEEXT

AC_CHECK_LIB(Xpm, main, [
	AC_DEFINE(USE_XPM,1,[ We found the XPM library ])
	AC_DEFINE(XPM,1,[ We found the XPM library ])
	USE_XPM=yes
	LIBS="$LIBS -lXpm"
],[
	AC_DEFINE(USE_XPM,[],[ We didn't find the XPM library ])
	AC_DEFINE(XPM,[],[ We didn't find the XPM library ])
	USE_XPM=no
])
AM_CONDITIONAL(USE_XPM, test x$USE_XPM = xyes)

AC_CHECK_HEADER(X11/extensions/shape.h, [AC_DEFINE(SHAPE, 1, [ We have shape.h ])])
AC_CHECK_HEADER(sys/bsdtypes.h, [AC_DEFINE(HAVE_SYS_BSDTYPES_H, 1, [ We have sys/bsdtypes.h ])])

AC_CHECK_FUNC(gethostname, [AC_DEFINE(HAVE_GETHOSTBYNAME,1,[ We have gethostbyname() ])])
AC_CHECK_FUNC(uname, [AC_DEFINE(HAVE_UNAME,1,[ We have uname() ])])
AC_CHECK_FUNC(sysconf, [AC_DEFINE(HAVE_SYSCONF,1,[ We have sysconf() ])])
AC_CHECK_FUNC(getdtablesize, [AC_DEFINE(HAVE_GETDTABLESIZE,1,[ We have getdtablesize() ])])
AC_CHECK_FUNC(strcasecmp, [], [AC_DEFINE(NEEDS_STRCASECMP,1,[ We need strcasecmp() ])])
AC_CHECK_FUNC(setitimer, [], [AC_DEFINE(HAVE_SETITIMER,1,[ We have setitimer() ])])

AC_DEFINE_UNQUOTED(FVWM_ICONDIR, "$pixmapsdir:$x_includes/X11/bitmaps:$x_includes/X11/pixmaps", [Where the bitmaps and pixmaps live])
AC_DEFINE_UNQUOTED(FVWMDIR, "$fvwmconfdir", [Where the fvwm configuration etc. lives])
AC_DEFINE_UNQUOTED(FVWMRC, FVWMDIR "/system.fvwmrc", [Where the fvwm config file lives])
AC_DEFINE_UNQUOTED(FVWMMODDIR, "$libexecdir", [Where the fvwm modules live])

AC_ARG_ENABLE([pager],
  AC_HELP_STRING([--disable-pager],[disables the internal pager (default: on)]),
  [ac_cv_pager=$enableval], [ac_cv_pager=yes])
if test "x$ac_cv_pager" = "xno"
then
  AC_DEFINE(NO_PAGER, 1, [ We dont't want the pager ])
fi
AC_ARG_ENABLE([virtual],
  AC_HELP_STRING([--disable-virtual],[disables the virtual desktops (default: on)]),
  [ac_cv_desktops=$enableval], [ac_cv_desktops=yes])
if test "x$ac_cv_desktops" = "xno"
then
  AC_DEFINE(NON_VIRTUAL, 1, [ We dont't want virtual desktops ])
fi
AC_ARG_ENABLE([saveunder],
  AC_HELP_STRING([--disable-saveunder],[disables the save-under feature (default: on)]),
  [ac_cv_saveunder=$enableval], [ac_cv_saveunder=yes])
if test "x$ac_cv_saveunder" = "xno"
then
  AC_DEFINE(NO_SAVEUNDERS, 1, [ We dont't want save-unders ])
fi
AC_ARG_ENABLE([windowlist],
  AC_HELP_STRING([--disable-windowlist],[disables the built-in window list (default: on)]),
  [ac_cv_winlist=$enableval], [ac_cv_winlist=yes])
if test "x$ac_cv_winlist" = "xno"
then
  AC_DEFINE(NO_WINDOWLIST, 1, [ We dont't want the window list ])
fi
AC_ARG_ENABLE([prune],
  AC_HELP_STRING([--disable-compat],[disables compatibility for old commands (default: on)]),
  [ac_cv_compat=$enableval], [ac_cv_compat=yes])
if test "x$ac_cv_prune" = "xno"
then
  AC_DEFINE(PRUNE, 1, [ We dont't want/like/whatever old commands ])
fi
AC_ARG_ENABLE([remember-focus],
  AC_HELP_STRING([--disable-remember-focus],[disables the focus remembering facilities (default: on)]),
  [ac_cv_focus=$enableval], [ac_cv_focus=yes])
if test "x$ac_cv_focus" = "xno"
then
  AC_DEFINE(NO_REMEMBER_FOCUS, 1, [ We dont't remember the focus ])
fi
AC_ARG_ENABLE([broken-stuff],
  AC_HELP_STRING([--enable-broken-stuff],[enable some broken stuff that never worked and probably never will (default: on)]),
  [ac_cv_broken=$enableval], [ac_cv_broken=no])
if test "x$ac_cv_broken" = "xyes"
then
  AC_DEFINE(BROKEN_STUFF, 1, [ We want it all! ])
fi
AC_ARG_ENABLE([raise-transients],
  AC_HELP_STRING([--disable-raise-transients],[never ever raise transient windows (default: on)]),
  [ac_cv_raise_transients=$enableval], [ac_cv_raise_transients=yes])
if test "x$ac_cv_raise_transients" = "xyes"
then
  AC_DEFINE(DONT_RAISE_TRANSIENTS, 1, [ Never raise transient windows ])
fi

AC_ARG_WITH([rplay],
  AC_HELP_STRING([--with-rplay],[Use rplay for audio support]),
  [ac_cv_user_wants_rplay=$withval],[ac_cv_user_wants_rplay=no])

if test "x$ac_cv_user_wants_rplay" != "xno"
then
  if test -d "$ac_cv_user_wants_rplay"
  then
    OLDCFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -I$ac_cv_user_wants_rplay/include"
    OLDLIBS="$LIBS"
    LIBS="$LIBS -L$ac_cv_user_wants_rplay/lib"
  fi

  AC_CHECK_LIB(rplay, main, [
	AC_DEFINE(HAVE_RPLAY,1,[ We found the rplay library ])
	HAVE_RPLAY=yes
	LIBS="$LIBS -lrplay"
  ],[
	AC_DEFINE(HAVE_RPLAY,0,[ We didn't find the rplay library ])
	HAVE_RPLAY=no
	CFLAGS="$OLDCFLAGS"
	LIBS="$OLDLIBS"
  ])
else
  HAVE_RPLAY=no
fi
AM_CONDITIONAL(HAVE_RPLAY, test x$HAVE_RPLAY = xyes)

AC_HEADER_STDC
AC_TYPE_SIGNAL
AC_C_BIGENDIAN
AC_DECL_SYS_SIGLIST

AC_OUTPUT(Makefile fvwm/Makefile xpmroot/Makefile libs/Makefile
	modules/Makefile modules/GoodStuff/Makefile
	modules/FvwmBanner/Makefile modules/FvwmIdent/Makefile
	modules/FvwmDebug/Makefile modules/FvwmBacker/Makefile
	modules/FvwmClean/Makefile modules/FvwmPager/Makefile
	modules/FvwmAudio/Makefile modules/FvwmScroll/Makefile
	modules/FvwmIconBox/Makefile modules/FvwmSave/Makefile
	modules/FvwmSaveDesk/Makefile modules/FvwmAuto/Makefile
	modules/FvwmWinList/Makefile sample.fvwmrc/Makefile
	fvwm_icons/Makefile)
