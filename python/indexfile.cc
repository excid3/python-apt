// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: indexfile.cc,v 1.2 2003/12/26 17:04:22 mdz Exp $
/* ######################################################################

   pkgIndexFile - Wrapper for the pkgIndexFilefunctions

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"
#include "apt_pkgmodule.h"

#include <apt-pkg/indexfile.h>

#include <Python.h>

static PyObject *PackageIndexFileArchiveURI(PyObject *Self,PyObject *Args)
{   
   pkgIndexFile *File = GetCpp<pkgIndexFile*>(Self);
   char *path;

   if (PyArg_ParseTuple(Args, "s",&path) == 0)
      return 0;

   return HandleErrors(Safe_FromString(File->ArchiveURI(path).c_str()));
}

static PyMethodDef PackageIndexFileMethods[] = 
{
   {"ArchiveURI",PackageIndexFileArchiveURI,METH_VARARGS,"Returns the ArchiveURI"},
   {}
};


static PyObject *PackageIndexFileAttr(PyObject *Self,char *Name)
{
   pkgIndexFile *File = GetCpp<pkgIndexFile*>(Self);
   if (strcmp("Label",Name) == 0)
      return Safe_FromString(File->GetType()->Label);
   else if (strcmp("Describe",Name) == 0)
      return Safe_FromString(File->Describe().c_str());
   else if (strcmp("Exists",Name) == 0)
      return Py_BuildValue("i",(File->Exists()));
   else if (strcmp("HasPackages",Name) == 0)
      return Py_BuildValue("i",(File->HasPackages()));
   else if (strcmp("Size",Name) == 0)
      return Py_BuildValue("i",(File->Size()));
   else if (strcmp("IsTrusted",Name) == 0)
      return Py_BuildValue("i",(File->IsTrusted()));
   
   return Py_FindMethod(PackageIndexFileMethods,Self,Name);
}

static PyObject *PackageIndexFileRepr(PyObject *Self)
{
   pkgIndexFile *File = GetCpp<pkgIndexFile*>(Self);
   
   char S[1024];
   snprintf(S,sizeof(S),"<pkIndexFile object: "
			"Label:'%s' Describe='%s' Exists='%i' "
	                "HasPackages='%i' Size='%i'  "
 	                "IsTrusted='%i' ArchiveURI='%s'>",
	    File->GetType()->Label,  File->Describe().c_str(), File->Exists(), 
	    File->HasPackages(), File->Size(),
            File->IsTrusted(), File->ArchiveURI("").c_str());
   return PyString_FromString(S);
}

PyTypeObject PackageIndexFileType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgIndexFile",         // tp_name
   sizeof(CppOwnedPyObject<pkgIndexFile*>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgIndexFile*>,          // tp_dealloc
   0,                                   // tp_print
   PackageIndexFileAttr,                     // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   PackageIndexFileRepr,                     // tp_repr
   0,                                   // tp_as_number
   0,		                        // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};
   



