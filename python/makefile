# -*- make -*-
BASE=..
SUBDIR=python

# Bring in the default rules
include ../buildlib/defaults.mak

# The apt_pkg module
MODULE=apt_pkg
SLIBS = -lapt-pkg 
LIB_MAKES = apt-pkg/makefile
APT_PKG_SRC = apt_pkgmodule.cc configuration.cc generic.cc tag.cc string.cc \
              cache.cc pkgrecords.cc pkgsrcrecords.cc sourcelist.cc \
	      depcache.cc progress.cc cdrom.cc acquire.cc pkgmanager.cc \
	      indexfile.cc metaindex.cc
SOURCE := $(APT_PKG_SRC)	      
include $(PYTHON_H) progress.h

# The apt_int module.. 
MODULE=apt_inst
SLIBS = -lapt-inst -lapt-pkg
LIB_MAKES = apt-inst/makefile
APT_INST_SRC = apt_instmodule.cc tar.cc generic.cc
SOURCE := $(APT_INST_SRC)	      
include $(PYTHON_H)

