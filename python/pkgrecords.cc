// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: pkgrecords.cc,v 1.3 2002/02/26 01:36:15 mdz Exp $
/* ######################################################################

   Package Records - Wrapper for the package records functions

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"
#include "apt_pkgmodule.h"
#include "pkgrecords.h"


#include <Python.h>
									/*}}}*/

    
// PkgRecords Class							/*{{{*/
// ---------------------------------------------------------------------


static PyObject *PkgRecordsLookup(PyObject *Self,PyObject *Args)
{   
   PkgRecordsStruct &Struct = GetCpp<PkgRecordsStruct>(Self);
   
   PyObject *PkgFObj;
   long int Index;
   if (PyArg_ParseTuple(Args,"(O!l)",&PackageFileType,&PkgFObj,&Index) == 0)
      return 0;
   
   // Get the index and check to make sure it is reasonable
   pkgCache::PkgFileIterator &PkgF = GetCpp<pkgCache::PkgFileIterator>(PkgFObj);
   pkgCache *Cache = PkgF.Cache();
   if (Cache->DataEnd() <= Cache->VerFileP + Index + 1 ||
       Cache->VerFileP[Index].File != PkgF.Index())
   {
      PyErr_SetNone(PyExc_IndexError);
      return 0;
   }
   
   // Do the lookup
   Struct.Last = &Struct.Records.Lookup(pkgCache::VerFileIterator(*Cache,Cache->VerFileP+Index));

   // always return true (to make it consistent with the pkgsrcrecords object
   return Py_BuildValue("i", 1);
}
    
static PyMethodDef PkgRecordsMethods[] = 
{
   {"Lookup",PkgRecordsLookup,METH_VARARGS,"Changes to a new package"},
   {}
};

static PyObject *PkgRecordsAttr(PyObject *Self,char *Name)
{
   PkgRecordsStruct &Struct = GetCpp<PkgRecordsStruct>(Self);

   if (Struct.Last != 0)
   {
      if (strcmp("FileName",Name) == 0)
	 return CppPyString(Struct.Last->FileName());
      else if (strcmp("MD5Hash",Name) == 0)
	 return CppPyString(Struct.Last->MD5Hash());
      else if (strcmp("SHA1Hash",Name) == 0)
	 return CppPyString(Struct.Last->SHA1Hash());
      else if (strcmp("SourcePkg",Name) == 0)
	 return CppPyString(Struct.Last->SourcePkg());
      else if (strcmp("SourceVer",Name) == 0)
	 return CppPyString(Struct.Last->SourceVer());
      else if (strcmp("Maintainer",Name) == 0)
	 return CppPyString(Struct.Last->Maintainer());
      else if (strcmp("ShortDesc",Name) == 0)
	 return CppPyString(Struct.Last->ShortDesc());
      else if (strcmp("LongDesc",Name) == 0)
	 return CppPyString(Struct.Last->LongDesc());
      else if (strcmp("Name",Name) == 0)
	 return CppPyString(Struct.Last->Name());
      else if (strcmp("Homepage",Name) == 0)
	 return CppPyString(Struct.Last->Homepage());
      else if (strcmp("Record", Name) == 0) 
      {
	 const char *start, *stop;
	 Struct.Last->GetRec(start, stop);
	 return PyString_FromStringAndSize(start,stop-start);
      }
   }
   
   return Py_FindMethod(PkgRecordsMethods,Self,Name);
}
PyTypeObject PkgRecordsType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgRecords",                          // tp_name
   sizeof(CppOwnedPyObject<PkgRecordsStruct>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<PkgRecordsStruct>,   // tp_dealloc
   0,                                   // tp_print
   PkgRecordsAttr,                      // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,                                   // tp_as_mapping
   0,                                   // tp_hash
};

									/*}}}*/

PyObject *GetPkgRecords(PyObject *Self,PyObject *Args)
{
   PyObject *Owner;
   if (PyArg_ParseTuple(Args,"O!",&PkgCacheType,&Owner) == 0)
      return 0;

   return HandleErrors(CppOwnedPyObject_NEW<PkgRecordsStruct>(Owner,&PkgRecordsType,
							      GetCpp<pkgCache *>(Owner)));
}

