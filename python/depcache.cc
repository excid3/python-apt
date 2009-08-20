// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: depcache.cc,v 1.5 2003/06/03 03:03:23 mdz Exp $
/* ######################################################################

   DepCache - Wrapper for the depcache related functions

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"
#include "apt_pkgmodule.h"

#include <apt-pkg/pkgcache.h>
#include <apt-pkg/cachefile.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/policy.h>
#include <apt-pkg/sptr.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/acquire-item.h>
#include <Python.h>

#include <iostream>
#include "progress.h"

#ifndef _
#define _(x) (x)
#endif



// DepCache Class								/*{{{*/
// ---------------------------------------------------------------------



static PyObject *PkgDepCacheInit(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *pyCallbackInst = 0;
   if (PyArg_ParseTuple(Args, "|O", &pyCallbackInst) == 0)
      return 0;

   if(pyCallbackInst != 0) {
      PyOpProgress progress;
      progress.setCallbackInst(pyCallbackInst);
      depcache->Init(&progress);
   } else {
      depcache->Init(0);
   }

   pkgApplyStatus(*depcache);

   Py_INCREF(Py_None);
   return HandleErrors(Py_None);   
}

static PyObject *PkgDepCacheCommit(PyObject *Self,PyObject *Args)
{   
   PyObject *result;

   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *pyInstallProgressInst = 0;
   PyObject *pyFetchProgressInst = 0;
   if (PyArg_ParseTuple(Args, "OO", 
			&pyFetchProgressInst, &pyInstallProgressInst) == 0) {
      return 0;
   }
   FileFd Lock;
   if (_config->FindB("Debug::NoLocking", false) == false) {
      Lock.Fd(GetLock(_config->FindDir("Dir::Cache::Archives") + "lock"));
      if (_error->PendingError() == true)
         return HandleErrors();
   }
   
   pkgRecords Recs(*depcache);
   if (_error->PendingError() == true)
      HandleErrors(Py_None);

   pkgSourceList List;
   if(!List.ReadMainList())
      return HandleErrors(Py_None);

   PyFetchProgress progress;
   progress.setCallbackInst(pyFetchProgressInst);

   pkgAcquire Fetcher(&progress);
   pkgPackageManager *PM;
   PM = _system->CreatePM(depcache);
   if(PM->GetArchives(&Fetcher, &List, &Recs) == false ||
      _error->PendingError() == true) {
      std::cerr << "Error in GetArchives" << std::endl;
      return HandleErrors();
   }

   //std::cout << "PM created" << std::endl;

   PyInstallProgress iprogress;
   iprogress.setCallbackInst(pyInstallProgressInst);

   // Run it
   while (1)
   {
      bool Transient = false;
      
      if (Fetcher.Run() == pkgAcquire::Failed)
	 return false;
      
      // Print out errors
      bool Failed = false;
      for (pkgAcquire::ItemIterator I = Fetcher.ItemsBegin(); I != Fetcher.ItemsEnd(); I++)
      {
	 
	 //std::cout << "looking at: " << (*I)->DestFile 
	 //	   << " status: " << (*I)->Status << std::endl;

	 if ((*I)->Status == pkgAcquire::Item::StatDone &&
	     (*I)->Complete == true)
	    continue;
	 
	 if ((*I)->Status == pkgAcquire::Item::StatIdle)
	 {
	    //std::cout << "transient failure" << std::endl;

	    Transient = true;
	    //Failed = true;
	    continue;
	 }

	 //std::cout << "something is wrong!" << std::endl;

	 _error->Warning(_("Failed to fetch %s  %s\n"),(*I)->DescURI().c_str(),
			 (*I)->ErrorText.c_str());
	 Failed = true;
      }

      if (Transient == true && Failed == true)
      {
	 _error->Error(_("--fix-missing and media swapping is not currently supported"));
	 Py_INCREF(Py_None);
	 return HandleErrors(Py_None);
      }
      
      // Try to deal with missing package files
      if (Failed == true && PM->FixMissing() == false)
      {
	 //std::cerr << "Unable to correct missing packages." << std::endl;
	 _error->Error("Aborting install.");
	 Py_INCREF(Py_None);
	 return HandleErrors(Py_None);
      }

      // fail if something else went wrong 
      //FIXME: make this more flexible, e.g. with a failedDl handler 
      if(Failed) 
	 return Py_BuildValue("b", false);
      _system->UnLock(true);

      pkgPackageManager::OrderResult Res = iprogress.Run(PM);
      //std::cout << "iprogress.Run() returned: " << (int)Res << std::endl;

      if (Res == pkgPackageManager::Failed || _error->PendingError() == true) {
	 return HandleErrors(Py_BuildValue("b", false));
      }
      if (Res == pkgPackageManager::Completed) {
	 //std::cout << "iprogress.Run() returned Completed " << std::endl;
	 return Py_BuildValue("b", true);
      }

      //std::cout << "looping again, install unfinished" << std::endl;
      
      // Reload the fetcher object and loop again for media swapping
      Fetcher.Shutdown();
      if (PM->GetArchives(&Fetcher,&List,&Recs) == false) {
	 return Py_BuildValue("b", false);
      }
      _system->Lock();
   }   
 
   return HandleErrors(Py_None);
}

static PyObject *PkgDepCacheSetCandidateVer(PyObject *Self,PyObject *Args)
{ 
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);
   PyObject *PackageObj;
   PyObject *VersionObj;
   if (PyArg_ParseTuple(Args,"O!O!",
			&PackageType, &PackageObj, 
			&VersionType, &VersionObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgCache::VerIterator &I = GetCpp<pkgCache::VerIterator>(VersionObj);
   if(I.end()) {
      return HandleErrors(Py_BuildValue("b",false));
   }
   depcache->SetCandidateVersion(I);
   
   return HandleErrors(Py_BuildValue("b",true));
}

static PyObject *PkgDepCacheGetCandidateVer(PyObject *Self,PyObject *Args)
{ 
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);
   PyObject *PackageObj;
   PyObject *CandidateObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache & State = (*depcache)[Pkg];
   pkgCache::VerIterator I = State.CandidateVerIter(*depcache);

   if(I.end()) {
      Py_INCREF(Py_None);
      return Py_None;
   }
   CandidateObj = CppOwnedPyObject_NEW<pkgCache::VerIterator>(PackageObj,&VersionType,I);

   return CandidateObj;
}

static PyObject *PkgDepCacheUpgrade(PyObject *Self,PyObject *Args)
{   
   bool res;
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   char distUpgrade=0;
   if (PyArg_ParseTuple(Args,"|b",&distUpgrade) == 0)
      return 0;

   Py_BEGIN_ALLOW_THREADS
   if(distUpgrade)
      res = pkgDistUpgrade(*depcache);
   else
      res = pkgAllUpgrade(*depcache);
   Py_END_ALLOW_THREADS

   Py_INCREF(Py_None);
   return HandleErrors(Py_BuildValue("b",res));   
}

static PyObject *PkgDepCacheMinimizeUpgrade(PyObject *Self,PyObject *Args)
{   
   bool res;
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;

   Py_BEGIN_ALLOW_THREADS
   res = pkgMinimizeUpgrade(*depcache);
   Py_END_ALLOW_THREADS

   Py_INCREF(Py_None);
   return HandleErrors(Py_BuildValue("b",res));   
}


static PyObject *PkgDepCacheReadPinFile(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);
   

   char *file=NULL;
   if (PyArg_ParseTuple(Args,"|s",&file) == 0)
      return 0;

   if(file == NULL) 
      ReadPinFile((pkgPolicy&)depcache->GetPolicy());
   else
      ReadPinFile((pkgPolicy&)depcache->GetPolicy(), file);

   Py_INCREF(Py_None);
   return HandleErrors(Py_None);  
}


static PyObject *PkgDepCacheFixBroken(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);
   
   bool res=true;
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;

   res &=pkgFixBroken(*depcache);
   res &=pkgMinimizeUpgrade(*depcache);

   return HandleErrors(Py_BuildValue("b",res));   
}


static PyObject *PkgDepCacheMarkKeep(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache*>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   depcache->MarkKeep(Pkg);

   Py_INCREF(Py_None);
   return HandleErrors(Py_None);   
}

static PyObject *PkgDepCacheSetReInstall(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache*>(Self);

   PyObject *PackageObj;
   char value = 0;
   if (PyArg_ParseTuple(Args,"O!b",&PackageType,&PackageObj, &value) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   depcache->SetReInstall(Pkg,value);

   Py_INCREF(Py_None);
   return HandleErrors(Py_None);   
}


static PyObject *PkgDepCacheMarkDelete(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   char purge = 0;
   if (PyArg_ParseTuple(Args,"O!|b",&PackageType,&PackageObj, &purge) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   depcache->MarkDelete(Pkg,purge);

   Py_INCREF(Py_None);
   return HandleErrors(Py_None);   
}


static PyObject *PkgDepCacheMarkInstall(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   char autoInst=1;
   char fromUser=1;
   if (PyArg_ParseTuple(Args,"O!|bb",&PackageType,&PackageObj, 
			&autoInst, &fromUser) == 0)
      return 0;

   Py_BEGIN_ALLOW_THREADS
   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   depcache->MarkInstall(Pkg, autoInst, 0, fromUser);
   Py_END_ALLOW_THREADS

   Py_INCREF(Py_None);
   return HandleErrors(Py_None);   
}

static PyObject *PkgDepCacheIsUpgradable(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.Upgradable()));   
}

static PyObject *PkgDepCacheIsGarbage(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.Garbage));   
}

static PyObject *PkgDepCacheIsAutoInstalled(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.Flags & pkgCache::Flag::Auto));   
}

static PyObject *PkgDepCacheIsNowBroken(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.NowBroken()));   
}

static PyObject *PkgDepCacheIsInstBroken(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.InstBroken()));   
}


static PyObject *PkgDepCacheMarkedInstall(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.NewInstall()));   
}


static PyObject *PkgDepCacheMarkedUpgrade(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.Upgrade()));   
}

static PyObject *PkgDepCacheMarkedDelete(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.Delete()));   
}

static PyObject *PkgDepCacheMarkedKeep(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.Keep()));   
}

static PyObject *PkgDepCacheMarkedDowngrade(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   return HandleErrors(Py_BuildValue("b",state.Downgrade()));   
}

static PyObject *PkgDepCacheMarkedReinstall(PyObject *Self,PyObject *Args)
{   
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;

   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   pkgDepCache::StateCache &state = (*depcache)[Pkg];

   bool res = state.Install() && (state.iFlags & pkgDepCache::ReInstall);

   return HandleErrors(Py_BuildValue("b",res));   
}


static PyMethodDef PkgDepCacheMethods[] = 
{
   {"Init",PkgDepCacheInit,METH_VARARGS,"Init the depcache (done on construct automatically)"},
   {"GetCandidateVer",PkgDepCacheGetCandidateVer,METH_VARARGS,"Get candidate version"},
   {"SetCandidateVer",PkgDepCacheSetCandidateVer,METH_VARARGS,"Set candidate version"},

   // global cache operations
   {"Upgrade",PkgDepCacheUpgrade,METH_VARARGS,"Perform Upgrade (optional boolean argument if dist-upgrade should be performed)"},
   {"FixBroken",PkgDepCacheFixBroken,METH_VARARGS,"Fix broken packages"},
   {"ReadPinFile",PkgDepCacheReadPinFile,METH_VARARGS,"Read the pin policy"},
   {"MinimizeUpgrade",PkgDepCacheMinimizeUpgrade, METH_VARARGS,"Go over the entire set of packages and try to keep each package marked for upgrade. If a conflict is generated then the package is restored."},
   // Manipulators
   {"MarkKeep",PkgDepCacheMarkKeep,METH_VARARGS,"Mark package for keep"},
   {"MarkDelete",PkgDepCacheMarkDelete,METH_VARARGS,"Mark package for delete (optional boolean argument if it should be purged)"},
   {"MarkInstall",PkgDepCacheMarkInstall,METH_VARARGS,"Mark package for Install"},
   {"SetReInstall",PkgDepCacheSetReInstall,METH_VARARGS,"Set if the package should be reinstalled"},
   // state information
   {"IsUpgradable",PkgDepCacheIsUpgradable,METH_VARARGS,"Is pkg upgradable"},
   {"IsNowBroken",PkgDepCacheIsNowBroken,METH_VARARGS,"Is pkg is now broken"},
   {"IsInstBroken",PkgDepCacheIsInstBroken,METH_VARARGS,"Is pkg broken on the current install"},
   {"IsGarbage",PkgDepCacheIsGarbage,METH_VARARGS,"Is pkg garbage (mark-n-sweep)"},
   {"IsAutoInstalled",PkgDepCacheIsAutoInstalled,METH_VARARGS,"Is pkg marked as auto installed"},
   {"MarkedInstall",PkgDepCacheMarkedInstall,METH_VARARGS,"Is pkg marked for install"},
   {"MarkedUpgrade",PkgDepCacheMarkedUpgrade,METH_VARARGS,"Is pkg marked for upgrade"},
   {"MarkedDelete",PkgDepCacheMarkedDelete,METH_VARARGS,"Is pkg marked for delete"},
   {"MarkedKeep",PkgDepCacheMarkedKeep,METH_VARARGS,"Is pkg marked for keep"},
   {"MarkedReinstall",PkgDepCacheMarkedReinstall,METH_VARARGS,"Is pkg marked for reinstall"},
   {"MarkedDowngrade",PkgDepCacheMarkedDowngrade,METH_VARARGS,"Is pkg marked for downgrade"},

   // Action
   {"Commit", PkgDepCacheCommit, METH_VARARGS, "Commit pending changes"},
   {}
};


static PyObject *DepCacheAttr(PyObject *Self,char *Name)
{
   pkgDepCache *depcache = GetCpp<pkgDepCache *>(Self);

   // size querries
   if(strcmp("KeepCount",Name) == 0) 
      return Py_BuildValue("l", depcache->KeepCount());
   else if(strcmp("InstCount",Name) == 0) 
      return Py_BuildValue("l", depcache->InstCount());
   else if(strcmp("DelCount",Name) == 0) 
      return Py_BuildValue("l", depcache->DelCount());
   else if(strcmp("BrokenCount",Name) == 0) 
      return Py_BuildValue("l", depcache->BrokenCount());
   else if(strcmp("UsrSize",Name) == 0) 
      return Py_BuildValue("d", depcache->UsrSize());
   else if(strcmp("DebSize",Name) == 0) 
      return Py_BuildValue("d", depcache->DebSize());
   
   
   return Py_FindMethod(PkgDepCacheMethods,Self,Name);
}




PyTypeObject PkgDepCacheType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgDepCache",                          // tp_name
   sizeof(CppOwnedPyObject<pkgDepCache *>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgDepCache *>,        // tp_dealloc
   0,                                   // tp_print
   DepCacheAttr,                           // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,	                                // tp_as_mapping
   0,                                   // tp_hash
};


PyObject *GetDepCache(PyObject *Self,PyObject *Args)
{
   PyObject *Owner;
   if (PyArg_ParseTuple(Args,"O!",&PkgCacheType,&Owner) == 0)
      return 0;


   // the owner of the Python cache object is a cachefile object, get it
   PyObject *CacheFilePy = GetOwner<pkgCache*>(Owner);
   // get the pkgCacheFile from the cachefile
   pkgCacheFile *CacheF = GetCpp<pkgCacheFile*>(CacheFilePy);
   // and now the depcache
   pkgDepCache *depcache = (pkgDepCache *)(*CacheF);

   CppOwnedPyObject<pkgDepCache*> *DepCachePyObj;
   DepCachePyObj = CppOwnedPyObject_NEW<pkgDepCache*>(Owner,
						      &PkgDepCacheType,
						      depcache);
   HandleErrors(DepCachePyObj);

   return DepCachePyObj;
}




									/*}}}*/


// pkgProblemResolver Class						/*{{{*/
// ---------------------------------------------------------------------


PyObject *GetPkgProblemResolver(PyObject *Self,PyObject *Args)
{
   PyObject *Owner;
   if (PyArg_ParseTuple(Args,"O!",&PkgDepCacheType,&Owner) == 0)
      return 0;

   pkgDepCache *depcache = GetCpp<pkgDepCache*>(Owner);
   pkgProblemResolver *fixer = new pkgProblemResolver(depcache);
   CppOwnedPyObject<pkgProblemResolver*> *PkgProblemResolverPyObj;
   PkgProblemResolverPyObj = CppOwnedPyObject_NEW<pkgProblemResolver*>(Owner,
						      &PkgProblemResolverType,
						      fixer);
   HandleErrors(PkgProblemResolverPyObj);

   return PkgProblemResolverPyObj;

}


static PyObject *PkgProblemResolverResolve(PyObject *Self,PyObject *Args)
{   
   bool res;
   pkgProblemResolver *fixer = GetCpp<pkgProblemResolver *>(Self);

   char brokenFix=1;
   if (PyArg_ParseTuple(Args,"|b",&brokenFix) == 0)
      return 0;

   Py_BEGIN_ALLOW_THREADS
   res = fixer->Resolve(brokenFix);
   Py_END_ALLOW_THREADS

   return HandleErrors(Py_BuildValue("b", res));
}

static PyObject *PkgProblemResolverResolveByKeep(PyObject *Self,PyObject *Args)
{  
   bool res;
   pkgProblemResolver *fixer = GetCpp<pkgProblemResolver *>(Self);
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;

   Py_BEGIN_ALLOW_THREADS
   res = fixer->ResolveByKeep();
   Py_END_ALLOW_THREADS

   return HandleErrors(Py_BuildValue("b", res));
}

static PyObject *PkgProblemResolverProtect(PyObject *Self,PyObject *Args)
{   
   pkgProblemResolver *fixer = GetCpp<pkgProblemResolver *>(Self);
   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;
   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   fixer->Protect(Pkg);
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);

}
static PyObject *PkgProblemResolverRemove(PyObject *Self,PyObject *Args)
{
   pkgProblemResolver *fixer = GetCpp<pkgProblemResolver *>(Self);
   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;
   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   fixer->Remove(Pkg);
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);  
}

static PyObject *PkgProblemResolverClear(PyObject *Self,PyObject *Args)
{  
   pkgProblemResolver *fixer = GetCpp<pkgProblemResolver *>(Self);
   PyObject *PackageObj;
   if (PyArg_ParseTuple(Args,"O!",&PackageType,&PackageObj) == 0)
      return 0;
   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(PackageObj);
   fixer->Clear(Pkg);
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);  
} 

static PyObject *PkgProblemResolverInstallProtect(PyObject *Self,PyObject *Args)
{  
   pkgProblemResolver *fixer = GetCpp<pkgProblemResolver *>(Self);
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   fixer->InstallProtect();
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}

static PyMethodDef PkgProblemResolverMethods[] = 
{
   // config
   {"Protect", PkgProblemResolverProtect, METH_VARARGS, "Protect(PkgIterator)"},
   {"Remove", PkgProblemResolverRemove, METH_VARARGS, "Remove(PkgIterator)"},
   {"Clear", PkgProblemResolverClear, METH_VARARGS, "Clear(PkgIterator)"},
   {"InstallProtect", PkgProblemResolverInstallProtect, METH_VARARGS, "ProtectInstalled()"},

   // Actions
   {"Resolve", PkgProblemResolverResolve, METH_VARARGS, "Try to intelligently resolve problems by installing and removing packages"},
   {"ResolveByKeep", PkgProblemResolverResolveByKeep, METH_VARARGS, "Try to resolv problems only by using keep"},
   {}
};


static PyObject *ProblemResolverAttr(PyObject *Self,char *Name)
{
   pkgProblemResolver *fixer = GetCpp<pkgProblemResolver *>(Self);
   
   return Py_FindMethod(PkgProblemResolverMethods,Self,Name);
}


PyTypeObject PkgProblemResolverType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgProblemResolver",                       // tp_name
   sizeof(CppOwnedPyObject<pkgProblemResolver *>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgProblemResolver *>,        // tp_dealloc
   0,                                   // tp_print
   ProblemResolverAttr,                           // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,	                                // tp_as_mapping
   0,                                   // tp_hash
};

									/*}}}*/

// pkgActionGroup Class						        /*{{{*/
// ---------------------------------------------------------------------


static PyObject *PkgActionGroupRelease(PyObject *Self,PyObject *Args)
{  
   pkgDepCache::ActionGroup *ag = GetCpp<pkgDepCache::ActionGroup*>(Self);
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   ag->release();
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}

static PyMethodDef PkgActionGroupMethods[] = 
{
   {"release", PkgActionGroupRelease, METH_VARARGS, "release()"},
   {}
};


static PyObject *ActionGroupAttr(PyObject *Self,char *Name)
{
   pkgDepCache::ActionGroup *ag = GetCpp<pkgDepCache::ActionGroup*>(Self);
   
   return Py_FindMethod(PkgActionGroupMethods,Self,Name);
}


PyTypeObject PkgActionGroupType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgActionGroup",                       // tp_name
   sizeof(CppOwnedPyObject<pkgDepCache::ActionGroup*>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgDepCache::ActionGroup*>,        // tp_dealloc
   0,                                   // tp_print
   ActionGroupAttr,                           // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,	                                // tp_as_mapping
   0,                                   // tp_hash
};

PyObject *GetPkgActionGroup(PyObject *Self,PyObject *Args)
{
   PyObject *Owner;
   if (PyArg_ParseTuple(Args,"O!",&PkgDepCacheType,&Owner) == 0)
      return 0;

   pkgDepCache *depcache = GetCpp<pkgDepCache*>(Owner);
   pkgDepCache::ActionGroup *group = new pkgDepCache::ActionGroup(*depcache);
   CppOwnedPyObject<pkgDepCache::ActionGroup*> *PkgActionGroupPyObj;
   PkgActionGroupPyObj = CppOwnedPyObject_NEW<pkgDepCache::ActionGroup*>(Owner,
						      &PkgActionGroupType,
						      group);
   HandleErrors(PkgActionGroupPyObj);

   return PkgActionGroupPyObj;

}


									/*}}}*/
