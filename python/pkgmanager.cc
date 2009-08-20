// Description								/*{{{*/
// $Id: acquire.cc,v 1.1 2003/06/03 03:03:23 mvo Exp $
/* ######################################################################

   PkgManager - Wrapper for the pkgPackageManager code

   ##################################################################### */

#include "generic.h"
#include "apt_pkgmodule.h"
#include "pkgrecords.h"

#include <apt-pkg/packagemanager.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/error.h>
#include <apt-pkg/acquire.h>
#include <iostream>


static PyObject *PkgManagerGetArchives(PyObject *Self,PyObject *Args)
{   
   pkgPackageManager *pm = GetCpp<pkgPackageManager*>(Self);
   PyObject *fetcher, *list, *recs;
   
   if (PyArg_ParseTuple(Args, "O!O!O!",
			&PkgAcquireType,&fetcher,
			&PkgSourceListType, &list,
			&PkgRecordsType, &recs) == 0) 
      return 0;

   pkgAcquire *s_fetcher = GetCpp<pkgAcquire*>(fetcher);
   pkgSourceList *s_list = GetCpp<pkgSourceList*>(list);
   PkgRecordsStruct &s_records = GetCpp<PkgRecordsStruct>(recs);

   bool res = pm->GetArchives(s_fetcher, s_list,
			      &s_records.Records);

   return HandleErrors(Py_BuildValue("b",res));
}

static PyObject *PkgManagerDoInstall(PyObject *Self,PyObject *Args)
{
   //PkgManagerStruct &Struct = GetCpp<PkgManagerStruct>(Self);
   pkgPackageManager *pm = GetCpp<pkgPackageManager*>(Self);
   int status_fd = -1;
   
   if (PyArg_ParseTuple(Args, "|i", &status_fd) == 0)
      return 0;

   pkgPackageManager::OrderResult res = pm->DoInstall(status_fd);
  
   return HandleErrors(Py_BuildValue("i",res));
}

static PyObject *PkgManagerFixMissing(PyObject *Self,PyObject *Args)
{
   //PkgManagerStruct &Struct = GetCpp<PkgManagerStruct>(Self);
   pkgPackageManager *pm = GetCpp<pkgPackageManager*>(Self);

   if (PyArg_ParseTuple(Args, "") == 0)
      return 0;

   bool res = pm->FixMissing();
   
   return HandleErrors(Py_BuildValue("b",res));
}

static PyMethodDef PkgManagerMethods[] = 
{
   {"GetArchives",PkgManagerGetArchives,METH_VARARGS,"Load the selected archives into the fetcher"},
   {"DoInstall",PkgManagerDoInstall,METH_VARARGS,"Do the actual install"},
   {"FixMissing",PkgManagerFixMissing,METH_VARARGS,"Fix the install if a pkg couldn't be downloaded"},
   {}
};


static PyObject *PkgManagerAttr(PyObject *Self,char *Name)
{
   //PkgManagerStruct &Struct = GetCpp<PkgManagerStruct>(Self);
   pkgPackageManager *pm = GetCpp<pkgPackageManager*>(Self);

   // some constants
   if(strcmp("ResultCompleted",Name) == 0) 
      return Py_BuildValue("i", pkgPackageManager::Completed);
   if(strcmp("ResultFailed",Name) == 0) 
      return Py_BuildValue("i", pkgPackageManager::Failed);
   if(strcmp("ResultIncomplete",Name) == 0) 
      return Py_BuildValue("i", pkgPackageManager::Incomplete);

   return Py_FindMethod(PkgManagerMethods,Self,Name);
}


PyTypeObject PkgManagerType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "PackageManager",                          // tp_name
   sizeof(CppPyObject<pkgPackageManager*>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppDealloc<pkgPackageManager*>,        // tp_dealloc
   0,                                   // tp_print
   PkgManagerAttr,                           // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,	                                // tp_as_mapping
   0,                                   // tp_hash
};

#include <apt-pkg/init.h>
#include <apt-pkg/configuration.h>

PyObject *GetPkgManager(PyObject *Self,PyObject *Args)
{
   PyObject *Owner;
   if (PyArg_ParseTuple(Args,"O!",&PkgDepCacheType,&Owner) == 0)
      return 0;

   pkgPackageManager *pm = _system->CreatePM(GetCpp<pkgDepCache*>(Owner));

   CppPyObject<pkgPackageManager*> *PkgManagerObj =
	   CppPyObject_NEW<pkgPackageManager*>(&PkgManagerType,pm);
   
   return PkgManagerObj;
}




									/*}}}*/
