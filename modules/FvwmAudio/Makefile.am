if HAVE_RPLAY
AUDIO_DEFS =		# -DHAVE_RPLAY
AUDIO_INCLUDES =	@RPLAY_INCLUDES@
AUDIO_LDADD =		@RPLAY_LDADD@ -lrplay
else
AUDIO_DEFS = 
AUDIO_INCLUDES =
AUDIO_LDADD =
endif

FvwmAudio_DEFS =		$(AUDIO_DEFINES)
FvwmAudio_INCLUDES =		$(AUDIO_INCLUDES)
FvwmAudio_LDADD =		$(AUDIO_LDADD) $(XLIB) -L../../libs -lfvwmlib
FvwmAudio_DEPENDENCIES =	../../libs/libfvwmlib.a
FvwmAudio_SOURCES =		FvwmAudio.c

libexec_PROGRAMS =		FvwmAudio
man1_MANS =			FvwmAudio.man
