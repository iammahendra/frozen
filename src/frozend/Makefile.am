if HAVE_DL
OPTIONAL = -ldl
endif

bin_PROGRAMS    = frozend
frozend_SOURCES = frozend.c paths.h paths.h.in
frozend_CFLAGS  = -I$(top_srcdir)/src/libfrozen/core/ -I$(top_srcdir)/src/libfrozen/data/
frozend_LDADD   = $(top_srcdir)/src/libfrozen/libfrozen.la $(OPTIONAL)

frozend.c: paths.h
paths.h: paths.h.in
	sed "s#%modulesdir%#$(modulesdir)#g" paths.h.in > $@

CLEANFILES=paths.h
