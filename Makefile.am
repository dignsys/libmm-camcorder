ACLOCAL_AMFLAGS='-I m4'

SUBDIRS = src

if GTESTS
SUBDIRS += unittest
endif

pcfiles = mm-camcorder.pc
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(pcfiles)
EXTRA_DIST = $(pcfiles)
