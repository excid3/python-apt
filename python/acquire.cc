// Description								/*{{{*/
// $Id: acquire.cc,v 1.1 2003/06/03 03:03:23 mvo Exp $
/* ######################################################################

   Acquire - Wrapper for the acquire code

   ##################################################################### */

#include "generic.h"
#include "apt_pkgmodule.h"
#include "progress.h"

#include <apt-pkg/acquire-item.h>

// pkgAcquire::Item
static PyObject *AcquireItemAttr(PyObject *Self,char *Name)
{
   pkgAcquire::ItemIterator &I = GetCpp<pkgAcquire::ItemIterator>(Self);
   
   if (strcmp("ID",Name) == 0)
      return Py_BuildValue("i",(*I)->ID);
   else if (strcmp("Status",Name) == 0)
      return Py_BuildValue("i",(*I)->Status);
   else if (strcmp("Complete",Name) == 0)
      return Py_BuildValue("i",(*I)->Complete);
   else if (strcmp("Local",Name) == 0)
      return Py_BuildValue("i",(*I)->Local);
   else if (strcmp("IsTrusted",Name) == 0)
      return Py_BuildValue("i",(*I)->IsTrusted());
   else if (strcmp("FileSize",Name) == 0)
      return Py_BuildValue("i",(*I)->FileSize);
   else if (strcmp("ErrorText",Name) == 0)
      return Safe_FromString((*I)->ErrorText.c_str());
   else if (strcmp("DestFile",Name) == 0)
      return Safe_FromString((*I)->DestFile.c_str());
   else if (strcmp("DescURI",Name) == 0)
      return Safe_FromString((*I)->DescURI().c_str());
   // constants
   else if (strcmp("StatIdle",Name) == 0)
      return Py_BuildValue("i", pkgAcquire::Item::StatIdle);
   else if (strcmp("StatFetching",Name) == 0)
      return Py_BuildValue("i", pkgAcquire::Item::StatFetching);
   else if (strcmp("StatDone",Name) == 0)
      return Py_BuildValue("i", pkgAcquire::Item::StatDone);
   else if (strcmp("StatError",Name) == 0)
      return Py_BuildValue("i", pkgAcquire::Item::StatError);
   else if (strcmp("StatAuthError",Name) == 0)
      return Py_BuildValue("i", pkgAcquire::Item::StatAuthError);


   PyErr_SetString(PyExc_AttributeError,Name);
   return 0;
}


static PyObject *AcquireItemRepr(PyObject *Self)
{
   pkgAcquire::ItemIterator &I = GetCpp<pkgAcquire::ItemIterator>(Self);
   
   char S[300];
   snprintf(S,sizeof(S),"<pkgAcquire::ItemIterator object: "
			"Status: %i Complete: %i Local: %i IsTrusted: %i "
	                "FileSize: %i DestFile:'%s' "
                        "DescURI: '%s' ID:%i ErrorText: '%s'>",
	    (*I)->Status, (*I)->Complete, (*I)->Local, (*I)->IsTrusted(),
	    (*I)->FileSize, (*I)->DestFile.c_str(), (*I)->DescURI().c_str(),
	    (*I)->ID,(*I)->ErrorText.c_str());
   return PyString_FromString(S);
}


PyTypeObject AcquireItemType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgAcquire::ItemIterator",         // tp_name
   sizeof(CppOwnedPyObject<pkgAcquire::ItemIterator>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgAcquire::ItemIterator>,          // tp_dealloc
   0,                                   // tp_print
   AcquireItemAttr,                     // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   AcquireItemRepr,                     // tp_repr
   0,                                   // tp_as_number
   0,		                        // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};


static PyObject *PkgAcquireRun(PyObject *Self,PyObject *Args)
{   
   pkgAcquire *fetcher = GetCpp<pkgAcquire*>(Self);

   int pulseInterval = 500000;
   if (PyArg_ParseTuple(Args, "|i", &pulseInterval) == 0) 
      return 0;

   pkgAcquire::RunResult run = fetcher->Run(pulseInterval);

   return HandleErrors(Py_BuildValue("i",run));
}

static PyObject *PkgAcquireShutdown(PyObject *Self,PyObject *Args)
{   
   pkgAcquire *fetcher = GetCpp<pkgAcquire*>(Self);

   if (PyArg_ParseTuple(Args, "") == 0) 
      return 0;

   fetcher->Shutdown();

   Py_INCREF(Py_None);
   return HandleErrors(Py_None);   
}

static PyMethodDef PkgAcquireMethods[] = 
{
   {"Run",PkgAcquireRun,METH_VARARGS,"Run the fetcher"},
   {"Shutdown",PkgAcquireShutdown, METH_VARARGS,"Shutdown the fetcher"},
   {}
};


static PyObject *AcquireAttr(PyObject *Self,char *Name)
{
   pkgAcquire *fetcher = GetCpp<pkgAcquire*>(Self);

   if(strcmp("TotalNeeded",Name) == 0) 
      return Py_BuildValue("d", fetcher->TotalNeeded());
   if(strcmp("FetchNeeded",Name) == 0) 
      return Py_BuildValue("d", fetcher->FetchNeeded());
   if(strcmp("PartialPresent",Name) == 0) 
      return Py_BuildValue("d", fetcher->PartialPresent());
   if(strcmp("Items",Name) == 0) 
   {
      PyObject *List = PyList_New(0);
      for (pkgAcquire::ItemIterator I = fetcher->ItemsBegin(); 
	   I != fetcher->ItemsEnd(); I++)
      {
	 PyObject *Obj;
	 Obj = CppOwnedPyObject_NEW<pkgAcquire::ItemIterator>(Self,&AcquireItemType,I);
	 PyList_Append(List,Obj);
	 Py_DECREF(Obj);

      }
      return List;
   }
   // some constants
   if(strcmp("ResultContinue",Name) == 0) 
      return Py_BuildValue("i", pkgAcquire::Continue);
   if(strcmp("ResultFailed",Name) == 0) 
      return Py_BuildValue("i", pkgAcquire::Failed);
   if(strcmp("ResultCancelled",Name) == 0) 
      return Py_BuildValue("i", pkgAcquire::Cancelled);

   return Py_FindMethod(PkgAcquireMethods,Self,Name);
}




PyTypeObject PkgAcquireType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "Acquire",                          // tp_name
   sizeof(CppPyObject<pkgAcquire*>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppDealloc<pkgAcquire*>,        // tp_dealloc
   0,                                   // tp_print
   AcquireAttr,                           // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,	                                // tp_as_mapping
   0,                                   // tp_hash
};

PyObject *GetAcquire(PyObject *Self,PyObject *Args)
{
   pkgAcquire *fetcher;

   PyObject *pyFetchProgressInst = NULL;
   if (PyArg_ParseTuple(Args,"|O",&pyFetchProgressInst) == 0)
      return 0;

   if (pyFetchProgressInst != NULL) {
      // FIXME: memleak?
      PyFetchProgress *progress = new PyFetchProgress();
      progress->setCallbackInst(pyFetchProgressInst);
      fetcher = new pkgAcquire(progress);
   } else {
      fetcher = new pkgAcquire();
   }

   CppPyObject<pkgAcquire*> *FetcherObj =
	   CppPyObject_NEW<pkgAcquire*>(&PkgAcquireType, fetcher);
   
   return FetcherObj;
}





// pkgAcquireFile

static PyObject *AcquireFileAttr(PyObject *Self,char *Name)
{
   pkgAcqFile *acqFile = GetCpp<pkgAcqFile*>(Self);

   PyErr_SetString(PyExc_AttributeError,Name);
   return 0;
}




PyTypeObject PkgAcquireFileType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgAcquireFile",                   // tp_name
   sizeof(CppPyObject<pkgAcqFile*>),// tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppDealloc<pkgAcqFile*>,        // tp_dealloc
   0,                                   // tp_print
   AcquireFileAttr,                           // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,	                                // tp_as_mapping
   0,                                   // tp_hash
};

char *doc_GetPkgAcqFile =
"GetPkgAcqFile(pkgAquire, uri[, md5, size, descr, shortDescr, destDir, destFile]) -> PkgAcqFile\n";
PyObject *GetPkgAcqFile(PyObject *Self, PyObject *Args, PyObject * kwds)
{
   PyObject *pyfetcher;
   char *uri, *md5, *descr, *shortDescr, *destDir, *destFile;
   int size = 0;
   uri = md5 = descr = shortDescr = destDir = destFile = "";

   char * kwlist[] = {"owner","uri", "md5", "size", "descr", "shortDescr",
                      "destDir", "destFile", NULL};

   if (PyArg_ParseTupleAndKeywords(Args, kwds, "O!s|sissss", kwlist,
				   &PkgAcquireType, &pyfetcher, &uri, &md5, 
				   &size, &descr, &shortDescr, &destDir, &destFile) == 0) 
      return 0;

   pkgAcquire *fetcher = GetCpp<pkgAcquire*>(pyfetcher);
   pkgAcqFile *af = new pkgAcqFile(fetcher,  // owner
				   uri, // uri
				   md5,  // md5
				   size,   // size
				   descr, // descr
				   shortDescr,
				   destDir,
				   destFile); // short-desc
   CppPyObject<pkgAcqFile*> *AcqFileObj =   CppPyObject_NEW<pkgAcqFile*>(&PkgAcquireFileType);
   AcqFileObj->Object = af;

   return AcqFileObj;
}


									/*}}}*/
