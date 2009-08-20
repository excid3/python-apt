// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: sourcelist.cc,v 1.2 2003/12/26 17:04:22 mdz Exp $
/* ######################################################################

   SourcesList - Wrapper for the SourcesList functions

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"
#include "apt_pkgmodule.h"

#include <apt-pkg/sourcelist.h>

#include <Python.h>
									/*}}}*/


    
// PkgsourceList Class							/*{{{*/
// ---------------------------------------------------------------------

static char *doc_PkgSourceListFindIndex = "xxx";
static PyObject *PkgSourceListFindIndex(PyObject *Self,PyObject *Args)
{   
   pkgSourceList *list = GetCpp<pkgSourceList*>(Self);
   PyObject *pyPkgFileIter;
   PyObject *pyPkgIndexFile;

   if (PyArg_ParseTuple(Args, "O!", &PackageFileType,&pyPkgFileIter) == 0) 
      return 0;

   pkgCache::PkgFileIterator &i = GetCpp<pkgCache::PkgFileIterator>(pyPkgFileIter);
   pkgIndexFile *index;
   if(list->FindIndex(i, index))
   {
      pyPkgIndexFile = CppOwnedPyObject_NEW<pkgIndexFile*>(pyPkgFileIter,&PackageIndexFileType,index);
      return pyPkgIndexFile;
   }

   //&PackageIndexFileType,&pyPkgIndexFile)
   
   Py_INCREF(Py_None);
   return Py_None;
}

static char *doc_PkgSourceListReadMainList = "xxx";
static PyObject *PkgSourceListReadMainList(PyObject *Self,PyObject *Args)
{
   pkgSourceList *list = GetCpp<pkgSourceList*>(Self);
   bool res = list->ReadMainList();

   return HandleErrors(Py_BuildValue("b",res));
}

static char *doc_PkgSourceListGetIndexes = "Load the indexes into the fetcher";
static PyObject *PkgSourceListGetIndexes(PyObject *Self,PyObject *Args)
{
   pkgSourceList *list = GetCpp<pkgSourceList*>(Self);

   PyObject *pyFetcher;
   char all = 0;
   if (PyArg_ParseTuple(Args, "O!|b",&PkgAcquireType,&pyFetcher, &all) == 0) 
      return 0;

   pkgAcquire *fetcher = GetCpp<pkgAcquire*>(pyFetcher);
   bool res = list->GetIndexes(fetcher, all);

   return HandleErrors(Py_BuildValue("b",res));
}

static PyMethodDef PkgSourceListMethods[] = 
{
   {"FindIndex",PkgSourceListFindIndex,METH_VARARGS,doc_PkgSourceListFindIndex},
   {"ReadMainList",PkgSourceListReadMainList,METH_VARARGS,doc_PkgSourceListReadMainList},
   {"GetIndexes",PkgSourceListGetIndexes,METH_VARARGS,doc_PkgSourceListReadMainList},
   {}
};

static PyObject *PkgSourceListAttr(PyObject *Self,char *Name)
{
   pkgSourceList *list = GetCpp<pkgSourceList*>(Self);

   if (strcmp("List",Name) == 0)
   {
      PyObject *List = PyList_New(0);
      for (vector<metaIndex *>::const_iterator I = list->begin(); 
	   I != list->end(); I++)
      {
	 PyObject *Obj;
	 Obj = CppPyObject_NEW<metaIndex*>(&MetaIndexType,*I);
	 PyList_Append(List,Obj);
      }      
      return List;
   }
   return Py_FindMethod(PkgSourceListMethods,Self,Name);
}
PyTypeObject PkgSourceListType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgSourceList",                          // tp_name
   sizeof(CppPyObject<pkgSourceList*>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppDealloc<pkgSourceList*>,   // tp_dealloc
   0,                                   // tp_print
   PkgSourceListAttr,                      // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,                                   // tp_as_mapping
   0,                                   // tp_hash
};

PyObject *GetPkgSourceList(PyObject *Self,PyObject *Args)
{
   return CppPyObject_NEW<pkgSourceList*>(&PkgSourceListType,new pkgSourceList());
}

