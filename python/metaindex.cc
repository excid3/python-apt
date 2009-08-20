// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: metaindex.cc,v 1.2 2003/12/26 17:04:22 mdz Exp $
/* ######################################################################

   metaindex - Wrapper for the metaIndex functions

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"
#include "apt_pkgmodule.h"

#include <apt-pkg/metaindex.h>

#include <Python.h>


static PyObject *MetaIndexAttr(PyObject *Self,char *Name)
{
   metaIndex *meta = GetCpp<metaIndex*>(Self);
   if (strcmp("URI",Name) == 0)
      return Safe_FromString(meta->GetURI().c_str());
   else if (strcmp("Dist",Name) == 0)
      return Safe_FromString(meta->GetDist().c_str());
   else if (strcmp("IsTrusted",Name) == 0)
      return Py_BuildValue("i",(meta->IsTrusted()));
   else if (strcmp("IndexFiles",Name) == 0)
   {
      PyObject *List = PyList_New(0);
      vector<pkgIndexFile *> *indexFiles = meta->GetIndexFiles();
      for (vector<pkgIndexFile *>::const_iterator I = indexFiles->begin(); 
	   I != indexFiles->end(); I++)
      {
	 PyObject *Obj;
	 Obj = CppPyObject_NEW<pkgIndexFile*>(&PackageIndexFileType,*I);
	 PyList_Append(List,Obj);
      }      
      return List;
   }

   PyErr_SetString(PyExc_AttributeError,Name);
   return 0;
}

static PyObject *MetaIndexRepr(PyObject *Self)
{
   metaIndex *meta = GetCpp<metaIndex*>(Self);
   
   char S[1024];
   snprintf(S,sizeof(S),"<metaIndex object: "
	                "Type='%s', URI:'%s' Dist='%s' IsTrusted='%i'>",
	    meta->GetType(),  meta->GetURI().c_str(), meta->GetDist().c_str(),
            meta->IsTrusted());

   return PyString_FromString(S);
}

PyTypeObject MetaIndexType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "metaIndex",         // tp_name
   sizeof(CppOwnedPyObject<metaIndex*>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<metaIndex*>,          // tp_dealloc
   0,                                   // tp_print
   MetaIndexAttr,                     // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   MetaIndexRepr,                     // tp_repr
   0,                                   // tp_as_number
   0,		                        // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};
   



