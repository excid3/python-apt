// Description								/*{{{*/
// $Id: cdrom.cc,v 1.1 2003/06/03 03:03:23 mvo Exp $
/* ######################################################################

   Cdrom - Wrapper for the apt-cdrom support

   ##################################################################### */

#include "generic.h"
#include "apt_pkgmodule.h"
#include "progress.h"

#include <apt-pkg/cdrom.h>


struct PkgCdromStruct
{
   pkgCdrom cdrom;
};

static PyObject *PkgCdromAdd(PyObject *Self,PyObject *Args)
{   
   PkgCdromStruct &Struct = GetCpp<PkgCdromStruct>(Self);

   PyObject *pyCdromProgressInst = 0;
   if (PyArg_ParseTuple(Args, "O", &pyCdromProgressInst) == 0) {
      return 0;
   }

   PyCdromProgress progress;
   progress.setCallbackInst(pyCdromProgressInst);

   bool res = Struct.cdrom.Add(&progress);

   return HandleErrors(Py_BuildValue("b", res));   
}

static PyObject *PkgCdromIdent(PyObject *Self,PyObject *Args)
{   
   PkgCdromStruct &Struct = GetCpp<PkgCdromStruct>(Self);

   PyObject *pyCdromProgressInst = 0;
   if (PyArg_ParseTuple(Args, "O", &pyCdromProgressInst) == 0) {
      return 0;
   }

   PyCdromProgress progress;
   progress.setCallbackInst(pyCdromProgressInst);

   string ident;
   bool res = Struct.cdrom.Ident(ident, &progress);

   PyObject *result = Py_BuildValue("(bs)", res, ident.c_str());

   return HandleErrors(result);   
}


static PyMethodDef PkgCdromMethods[] = 
{
   {"Add",PkgCdromAdd,METH_VARARGS,"Add a cdrom"},
   {"Ident",PkgCdromIdent,METH_VARARGS,"Ident a cdrom"},
   {}
};


static PyObject *CdromAttr(PyObject *Self,char *Name)
{
   PkgCdromStruct &Struct = GetCpp<PkgCdromStruct>(Self);

   return Py_FindMethod(PkgCdromMethods,Self,Name);
}




PyTypeObject PkgCdromType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "Cdrom",                          // tp_name
   sizeof(CppOwnedPyObject<PkgCdromStruct>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<PkgCdromStruct>,        // tp_dealloc
   0,                                   // tp_print
   CdromAttr,                           // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,	                                // tp_as_mapping
   0,                                   // tp_hash
};

PyObject *GetCdrom(PyObject *Self,PyObject *Args)
{
   pkgCdrom *cdrom = new pkgCdrom();

   CppOwnedPyObject<pkgCdrom> *CdromObj =
	   CppOwnedPyObject_NEW<pkgCdrom>(0,&PkgCdromType, *cdrom);
   
   return CdromObj;
}




									/*}}}*/
