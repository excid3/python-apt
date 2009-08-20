// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: apt_instmodule.h,v 1.2 2002/01/08 06:53:04 jgg Exp $
/* ######################################################################

   Prototypes for the module
   
   ##################################################################### */
									/*}}}*/
#ifndef APT_INSTMODULE_H
#define APT_INSTMODULE_H

#include <Python.h>

PyObject *debExtract(PyObject *Self,PyObject *Args);
extern char *doc_debExtract;
PyObject *tarExtract(PyObject *Self,PyObject *Args);
extern char *doc_tarExtract;

#endif
