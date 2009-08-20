// Description								/*{{{*/
// $Id: progress.h,v 1.5 2003/06/03 03:03:23 mdz Exp $
/* ######################################################################

   Progress - Wrapper for the progress related functions

   ##################################################################### */

#ifndef PROGRESS_H
#define PROGRESS_H

#include <apt-pkg/progress.h>
#include <apt-pkg/acquire.h>
#include <apt-pkg/packagemanager.h>
#include <apt-pkg/cdrom.h>
#include <Python.h>


class PyCallbackObj {
 protected:
   PyObject *callbackInst;

 public:
   void setCallbackInst(PyObject *o) {
      Py_INCREF(o);
      callbackInst = o;
   }

   bool RunSimpleCallback(const char *method, PyObject *arglist=NULL,
			  PyObject **result=NULL);

   PyCallbackObj() : callbackInst(0) {};
   ~PyCallbackObj()  {Py_DECREF(callbackInst); };
};

struct PyOpProgress : public OpProgress, public PyCallbackObj
{

   virtual void Update();
   virtual void Done();

   PyOpProgress() : OpProgress(), PyCallbackObj() {};
};


struct PyFetchProgress : public pkgAcquireStatus, public PyCallbackObj
{
   enum {
      DLDone, DLQueued, DLFailed, DLHit, DLIgnored
   };

   void UpdateStatus(pkgAcquire::ItemDesc & Itm, int status);

   virtual bool MediaChange(string Media, string Drive);

   /* apt stuff */   
   virtual void IMSHit(pkgAcquire::ItemDesc &Itm);
   virtual void Fetch(pkgAcquire::ItemDesc &Itm);
   virtual void Done(pkgAcquire::ItemDesc &Itm);
   virtual void Fail(pkgAcquire::ItemDesc &Itm);
   virtual void Start();
   virtual void Stop();

   bool Pulse(pkgAcquire * Owner);
   PyFetchProgress() : PyCallbackObj() {};
};

struct PyInstallProgress : public PyCallbackObj
{
   void StartUpdate();
   void UpdateInterface();
   void FinishUpdate();

   pkgPackageManager::OrderResult Run(pkgPackageManager *pm);

   PyInstallProgress() : PyCallbackObj() {};
};

struct PyCdromProgress : public pkgCdromStatus, public PyCallbackObj
{
   // update steps, will be called regularly as a "pulse"
   virtual void Update(string text="", int current=0);
   // ask for cdrom insert
   virtual bool ChangeCdrom();
   // ask for cdrom name
   virtual bool AskCdromName(string &Name);

   PyCdromProgress() : PyCallbackObj() {};
};


#endif
