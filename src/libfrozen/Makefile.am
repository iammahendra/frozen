SUBDIRS = data core backends

libfrozen_libs  = core/libfrozen_core.la
libfrozen_libs += backends/libfrozen_backends.la
libfrozen_libs += data/libfrozen_data.la

lib_LTLIBRARIES           = libfrozen.la
libfrozen_la_SOURCES      =
libfrozen_la_LDFLAGS      = -module -avoid-version
libfrozen_la_CPPFLAGS     = -fvisibility=hidden
libfrozen_la_LIBADD       = $(libfrozen_libs)
libfrozen_la_DEPENDENCIES = $(libfrozen_libs)

