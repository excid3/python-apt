// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: cache.cc,v 1.5 2003/06/03 03:03:23 mdz Exp $
/* ######################################################################

   Cache - Wrapper for the cache related functions

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"
#include "apt_pkgmodule.h"

#include <apt-pkg/pkgcache.h>
#include <apt-pkg/cachefile.h>
#include <apt-pkg/sptr.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/error.h>
#include <apt-pkg/packagemanager.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/algorithms.h>

#include <Python.h>
#include "progress.h"

class pkgSourceList;

									/*}}}*/
struct PkgListStruct
{
   pkgCache::PkgIterator Iter;
   unsigned long LastIndex;
   
   PkgListStruct(pkgCache::PkgIterator const &I) : Iter(I), LastIndex(0) {}
   PkgListStruct() {abort();};  // G++ Bug..
};

struct RDepListStruct
{
   pkgCache::DepIterator Iter;
   pkgCache::DepIterator Start;
   unsigned long LastIndex;
   unsigned long Len;
   
   RDepListStruct(pkgCache::DepIterator const &I) : Iter(I), Start(I),
                         LastIndex(0)
   {
      Len = 0;
      pkgCache::DepIterator D = I;
      for (; D.end() == false; D++)
	 Len++;
   }
   RDepListStruct() {abort();};  // G++ Bug..
};

static PyObject *CreateProvides(PyObject *Owner,pkgCache::PrvIterator I)
{
   PyObject *List = PyList_New(0);
   for (; I.end() == false; I++)
   {
      PyObject *Obj;
      PyObject *Ver;
      Ver = CppOwnedPyObject_NEW<pkgCache::VerIterator>(Owner,&VersionType,
							I.OwnerVer());
      Obj = Py_BuildValue("ssN",I.ParentPkg().Name(),I.ProvideVersion(),
			  Ver);
      PyList_Append(List,Obj);
      Py_DECREF(Obj);
   }      
   return List;
}

// Cache Class								/*{{{*/
// ---------------------------------------------------------------------
static PyObject *PkgCacheUpdate(PyObject *Self,PyObject *Args)
{   
   PyObject *CacheFilePy = GetOwner<pkgCache*>(Self);
   pkgCacheFile *Cache = GetCpp<pkgCacheFile*>(CacheFilePy);

   PyObject *pyFetchProgressInst = 0;
   PyObject *pySourcesList = 0;
   if (PyArg_ParseTuple(Args, "OO", &pyFetchProgressInst,&pySourcesList) == 0)
      return 0;

   PyFetchProgress progress;
   progress.setCallbackInst(pyFetchProgressInst);
   pkgSourceList *source = GetCpp<pkgSourceList*>(pySourcesList);
   bool res = ListUpdate(progress, *source);

   PyObject *PyRes = Py_BuildValue("b", res);
   return HandleErrors(PyRes);
}

static PyObject *PkgCacheClose(PyObject *Self,PyObject *Args)
{   
   PyObject *CacheFilePy = GetOwner<pkgCache*>(Self);
   pkgCacheFile *Cache = GetCpp<pkgCacheFile*>(CacheFilePy);
   Cache->Close();

   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}

static PyObject *PkgCacheOpen(PyObject *Self,PyObject *Args)
{   
   PyObject *CacheFilePy = GetOwner<pkgCache*>(Self);
   pkgCacheFile *Cache = GetCpp<pkgCacheFile*>(CacheFilePy);

   PyObject *pyCallbackInst = 0;
   if (PyArg_ParseTuple(Args, "|O", &pyCallbackInst) == 0)
      return 0;

   if(pyCallbackInst != 0) {
      PyOpProgress progress;
      progress.setCallbackInst(pyCallbackInst);
      if (Cache->Open(progress,false) == false)
	 return HandleErrors();
   }  else {
      OpTextProgress Prog;
      if (Cache->Open(Prog,false) == false)
	 return HandleErrors();
   }

   //std::cout << "new cache is " << (pkgCache*)(*Cache) << std::endl;

   // update the cache pointer after the cache was rebuild
   ((CppPyObject<pkgCache*> *)Self)->Object = (pkgCache*)(*Cache);


   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}


static PyMethodDef PkgCacheMethods[] = 
{
   {"Update",PkgCacheUpdate,METH_VARARGS,"Update the cache"},
   {"Open", PkgCacheOpen, METH_VARARGS,"Open the cache"},
   {"Close", PkgCacheClose, METH_VARARGS,"Close the cache"},
   {}
};

static PyObject *CacheAttr(PyObject *Self,char *Name)
{
   pkgCache *Cache = GetCpp<pkgCache *>(Self);
   
   if (strcmp("Packages",Name) == 0)
      return CppOwnedPyObject_NEW<PkgListStruct>(Self,&PkgListType,Cache->PkgBegin());
   else if (strcmp("PackageCount",Name) == 0)
      return Py_BuildValue("i",Cache->HeaderP->PackageCount);
   else if (strcmp("VersionCount",Name) == 0)
      return Py_BuildValue("i",Cache->HeaderP->VersionCount);
   else if (strcmp("DependsCount",Name) == 0)
      return Py_BuildValue("i",Cache->HeaderP->DependsCount);
   else if (strcmp("PackageFileCount",Name) == 0)
      return Py_BuildValue("i",Cache->HeaderP->PackageFileCount);
   else if (strcmp("VerFileCount",Name) == 0)
      return Py_BuildValue("i",Cache->HeaderP->VerFileCount);
   else if (strcmp("ProvidesCount",Name) == 0)
      return Py_BuildValue("i",Cache->HeaderP->ProvidesCount);
   else if (strcmp("FileList",Name) == 0)
   {
      PyObject *List = PyList_New(0);
      for (pkgCache::PkgFileIterator I = Cache->FileBegin(); I.end() == false; I++)
      {
	 PyObject *Obj;
	 Obj = CppOwnedPyObject_NEW<pkgCache::PkgFileIterator>(Self,&PackageFileType,I);
	 PyList_Append(List,Obj);
	 Py_DECREF(Obj);
      }      
      return List;
   }

   return Py_FindMethod(PkgCacheMethods,Self,Name);
}

// Map access, operator []
static PyObject *CacheMapOp(PyObject *Self,PyObject *Arg)
{
   pkgCache *Cache = GetCpp<pkgCache *>(Self);
   
   if (PyString_Check(Arg) == 0)
   {
      PyErr_SetNone(PyExc_TypeError);
      return 0;
   }
   
   // Search for the package
   const char *Name = PyString_AsString(Arg);   
   pkgCache::PkgIterator Pkg = Cache->FindPkg(Name);
   if (Pkg.end() == true)
   {
      PyErr_SetString(PyExc_KeyError,Name);
      return 0;
   }

   return CppOwnedPyObject_NEW<pkgCache::PkgIterator>(Self,&PackageType,Pkg);
}

// we need a special dealloc here to make sure that the CacheFile
// is closed before deallocation the cache (otherwise we have a bad)
// memory leak
void PkgCacheFileDealloc(PyObject *Self)
{
   PyObject *CacheFilePy = GetOwner<pkgCache*>(Self);
   pkgCacheFile *CacheF = GetCpp<pkgCacheFile*>(CacheFilePy);
   CacheF->Close();
   CppOwnedDealloc<pkgCache *>(Self);
}

static PyMappingMethods CacheMap = {0,CacheMapOp,0};
PyTypeObject PkgCacheType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgCache",                          // tp_name
   sizeof(CppOwnedPyObject<pkgCache *>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   PkgCacheFileDealloc,                  // tp_dealloc
   0,                                   // tp_print
   CacheAttr,                           // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   &CacheMap,		                // tp_as_mapping
   0,                                   // tp_hash
};
									/*}}}*/
// PkgCacheFile Class							/*{{{*/
// ---------------------------------------------------------------------
PyTypeObject PkgCacheFileType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgCacheFile",                      // tp_name
   sizeof(CppOwnedPyObject<pkgCacheFile*>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgCacheFile*>,       // tp_dealloc
   0,                                   // tp_print
   0,                                   // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,                                   // tp_as_mapping
   0,                                   // tp_hash
};
									/*}}}*/
// Package List Class							/*{{{*/
// ---------------------------------------------------------------------
static Py_ssize_t PkgListLen(PyObject *Self)
{
   return GetCpp<PkgListStruct>(Self).Iter.Cache()->HeaderP->PackageCount;
}

static PyObject *PkgListItem(PyObject *iSelf,Py_ssize_t Index)
{
   PkgListStruct &Self = GetCpp<PkgListStruct>(iSelf);
   if (Index < 0 || (unsigned)Index >= Self.Iter.Cache()->HeaderP->PackageCount)
   {
      PyErr_SetNone(PyExc_IndexError);
      return 0;
   }

   if ((unsigned)Index < Self.LastIndex)
   {
      Self.LastIndex = 0;
      Self.Iter = Self.Iter.Cache()->PkgBegin();
   }
   
   while ((unsigned)Index > Self.LastIndex)
   {
      Self.LastIndex++;
      Self.Iter++;
      if (Self.Iter.end() == true)
      {
	 PyErr_SetNone(PyExc_IndexError);
	 return 0;
      }
   }
   
   return CppOwnedPyObject_NEW<pkgCache::PkgIterator>(GetOwner<PkgListStruct>(iSelf),&PackageType,
						      Self.Iter);
}

static PySequenceMethods PkgListSeq = 
{
   PkgListLen,
   0,                // concat
   0,                // repeat
   PkgListItem,
   0,                // slice
   0,                // assign item
   0                 // assign slice 
};
   
PyTypeObject PkgListType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgCache::PkgIterator",             // tp_name
   sizeof(CppOwnedPyObject<PkgListStruct>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<PkgListStruct>,      // tp_dealloc
   0,                                   // tp_print
   0,                                   // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   &PkgListSeq,                         // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};
   
									/*}}}*/
// Package Class							/*{{{*/
// ---------------------------------------------------------------------
static PyObject *PackageAttr(PyObject *Self,char *Name)
{
   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(Self);
   PyObject *Owner = GetOwner<pkgCache::PkgIterator>(Self);
   
   if (strcmp("Name",Name) == 0)
      return PyString_FromString(Pkg.Name());
   else if (strcmp("VersionList",Name) == 0)
   {
      PyObject *List = PyList_New(0);
      for (pkgCache::VerIterator I = Pkg.VersionList(); I.end() == false; I++)
      {
	 PyObject *Obj;
	 Obj = CppOwnedPyObject_NEW<pkgCache::VerIterator>(Owner,&VersionType,I);
	 PyList_Append(List,Obj);
	 Py_DECREF(Obj);
      }      
      return List;
   }
   else if (strcmp("CurrentVer",Name) == 0)
   {
      if (Pkg->CurrentVer == 0)
      {
	 Py_INCREF(Py_None);
	 return Py_None;
      }
      
      return CppOwnedPyObject_NEW<pkgCache::VerIterator>(Owner,&VersionType,
							 Pkg.CurrentVer());
   }
   else if (strcmp("Section",Name) == 0)
      return Safe_FromString(Pkg.Section());
   else if (strcmp("RevDependsList",Name) == 0)
      return CppOwnedPyObject_NEW<RDepListStruct>(Owner,&RDepListType,
							 Pkg.RevDependsList());
   else if (strcmp("ProvidesList",Name) == 0)
      return CreateProvides(Owner,Pkg.ProvidesList());
   else if (strcmp("SelectedState",Name) == 0)
      return Py_BuildValue("i",Pkg->SelectedState);
   else if (strcmp("InstState",Name) == 0)
      return Py_BuildValue("i",Pkg->InstState);
   else if (strcmp("CurrentState",Name) == 0)
      return Py_BuildValue("i",Pkg->CurrentState);
   else if (strcmp("ID",Name) == 0)
      return Py_BuildValue("i",Pkg->ID);
   else if (strcmp("Auto",Name) == 0)
      return Py_BuildValue("i",(Pkg->Flags & pkgCache::Flag::Auto) != 0);
   else if (strcmp("Essential",Name) == 0)
      return Py_BuildValue("i",(Pkg->Flags & pkgCache::Flag::Essential) != 0);
   else if (strcmp("Important",Name) == 0)
      return Py_BuildValue("i",(Pkg->Flags & pkgCache::Flag::Important) != 0);
   
   PyErr_SetString(PyExc_AttributeError,Name);
   return 0;
}

static PyObject *PackageRepr(PyObject *Self)
{
   pkgCache::PkgIterator &Pkg = GetCpp<pkgCache::PkgIterator>(Self);
   
   char S[300];
   snprintf(S,sizeof(S),"<pkgCache::Package object: Name:'%s' Section: '%s'"
	                " ID:%u Flags:0x%lX>",
	    Pkg.Name(),Pkg.Section(),Pkg->ID,Pkg->Flags);
   return PyString_FromString(S);
}

PyTypeObject PackageType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgCache::Package",                 // tp_name
   sizeof(CppOwnedPyObject<pkgCache::PkgIterator>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgCache::PkgIterator>,  // tp_dealloc
   0,                                   // tp_print
   PackageAttr,                         // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   PackageRepr,                         // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};
									/*}}}*/
// Description Class							/*{{{*/
// ---------------------------------------------------------------------
static PyObject *DescriptionAttr(PyObject *Self,char *Name)
{
   pkgCache::DescIterator &Desc = GetCpp<pkgCache::DescIterator>(Self);
   PyObject *Owner = GetOwner<pkgCache::DescIterator>(Self);
   
   if (strcmp("LanguageCode",Name) == 0)
      return PyString_FromString(Desc.LanguageCode());
   else if (strcmp("md5",Name) == 0)
      return Safe_FromString(Desc.md5());
   else if (strcmp("FileList",Name) == 0)
   {
      /* The second value in the tuple is the index of the VF item. If the
         user wants to request a lookup then that number will be used. 
         Maybe later it can become an object. */	 
      PyObject *List = PyList_New(0);
      for (pkgCache::DescFileIterator I = Desc.FileList(); I.end() == false; I++)
      {
	 PyObject *DescFile;
	 PyObject *Obj;
	 DescFile = CppOwnedPyObject_NEW<pkgCache::PkgFileIterator>(Owner,&PackageFileType,I.File());
	 Obj = Py_BuildValue("Nl",DescFile,I.Index());
	 PyList_Append(List,Obj);
	 Py_DECREF(Obj);
      }      
      return List;
   }
   PyErr_SetString(PyExc_AttributeError,Name);
   return 0;
}

static PyObject *DescriptionRepr(PyObject *Self)
{
   pkgCache::DescIterator &Desc = GetCpp<pkgCache::DescIterator>(Self);
   
   char S[300];
   snprintf(S,sizeof(S),
	    "<pkgCache::Description object: language_code:'%s' md5:'%s' ",
	    Desc.LanguageCode(), Desc.md5());
   return PyString_FromString(S);
}

PyTypeObject DescriptionType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgCache::DescIterator",             // tp_name
   sizeof(CppOwnedPyObject<pkgCache::DescIterator>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgCache::DescIterator>,          // tp_dealloc
   0,                                   // tp_print
   DescriptionAttr,                         // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   DescriptionRepr,                         // tp_repr
   0,                                   // tp_as_number
   0,		                        // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};
									/*}}}*/
// Version Class							/*{{{*/
// ---------------------------------------------------------------------

/* This is the simple depends result, the elements are split like 
   ParseDepends does */
static PyObject *MakeDepends(PyObject *Owner,pkgCache::VerIterator &Ver,
			     bool AsObj)
{
   PyObject *Dict = PyDict_New();
   PyObject *LastDep = 0;
   unsigned LastDepType = 0;      
   for (pkgCache::DepIterator D = Ver.DependsList(); D.end() == false;)
   {
      pkgCache::DepIterator Start;
      pkgCache::DepIterator End;
      D.GlobOr(Start,End);

      // Switch/create a new dict entry
      if (LastDepType != Start->Type || LastDep != 0)
      {
	 // must be in sync with pkgCache::DepType in libapt
	 // it sucks to have it here duplicated, but we get it
	 // translated from libapt and that is certainly not what
	 // we want in a programing interface
	 const char *Types[] =  
	 {
	    "", "Depends","PreDepends","Suggests",
	    "Recommends","Conflicts","Replaces",
	    "Obsoletes"
	 };
	 PyObject *Dep = PyString_FromString(Types[Start->Type]);
	 LastDepType = Start->Type;
	 LastDep = PyDict_GetItem(Dict,Dep);
	 if (LastDep == 0)
	 {
	    LastDep = PyList_New(0);
	    PyDict_SetItem(Dict,Dep,LastDep);
	    Py_DECREF(LastDep);
	 }	 
	 Py_DECREF(Dep);
      }
      
      PyObject *OrGroup = PyList_New(0);
      while (1)
      {
	 PyObject *Obj;
	 if (AsObj == true)
	    Obj = CppOwnedPyObject_NEW<pkgCache::DepIterator>(Owner,&DependencyType,
							 Start);
	 else
	 {
	    if (Start->Version == 0)
	       Obj = Py_BuildValue("sss",
				   Start.TargetPkg().Name(),
				   "",
				   Start.CompType());
	    else
	       Obj = Py_BuildValue("sss",
				   Start.TargetPkg().Name(),
				   Start.TargetVer(),
				   Start.CompType());
	 }	 
	 PyList_Append(OrGroup,Obj);
	 Py_DECREF(Obj);
	 
	 if (Start == End)
	    break;
	 Start++;
      }
      
      PyList_Append(LastDep,OrGroup);
      Py_DECREF(OrGroup);
   }   
   
   return Dict;
}

static PyObject *VersionAttr(PyObject *Self,char *Name)
{
   pkgCache::VerIterator &Ver = GetCpp<pkgCache::VerIterator>(Self);
   PyObject *Owner = GetOwner<pkgCache::VerIterator>(Self);
   
   if (strcmp("VerStr",Name) == 0)
      return PyString_FromString(Ver.VerStr());
   else if (strcmp("Section",Name) == 0)
      return Safe_FromString(Ver.Section());
   else if (strcmp("Arch",Name) == 0)
      return Safe_FromString(Ver.Arch());
   else if (strcmp("FileList",Name) == 0)
   {
      /* The second value in the tuple is the index of the VF item. If the
         user wants to request a lookup then that number will be used. 
         Maybe later it can become an object. */	 
      PyObject *List = PyList_New(0);
      for (pkgCache::VerFileIterator I = Ver.FileList(); I.end() == false; I++)
      {
	 PyObject *PkgFile;
	 PyObject *Obj;
	 PkgFile = CppOwnedPyObject_NEW<pkgCache::PkgFileIterator>(Owner,&PackageFileType,I.File());
	 Obj = Py_BuildValue("Nl",PkgFile,I.Index());
	 PyList_Append(List,Obj);
	 Py_DECREF(Obj);
      }      
      return List;
   }
   else if (strcmp("DependsListStr",Name) == 0)
      return MakeDepends(Owner,Ver,false);
   else if (strcmp("DependsList",Name) == 0)
      return MakeDepends(Owner,Ver,true);
   else if (strcmp("ParentPkg",Name) == 0)
      return CppOwnedPyObject_NEW<pkgCache::PkgIterator>(Owner,&PackageType,
							 Ver.ParentPkg());
   else if (strcmp("ProvidesList",Name) == 0)
      return CreateProvides(Owner,Ver.ProvidesList());
   else if (strcmp("Size",Name) == 0)
      return Py_BuildValue("i",Ver->Size);
   else if (strcmp("InstalledSize",Name) == 0)
      return Py_BuildValue("i",Ver->InstalledSize);
   else if (strcmp("Hash",Name) == 0)
      return Py_BuildValue("i",Ver->Hash);
   else if (strcmp("ID",Name) == 0)
      return Py_BuildValue("i",Ver->ID);
   else if (strcmp("Priority",Name) == 0)
      return Py_BuildValue("i",Ver->Priority);
   else if (strcmp("PriorityStr",Name) == 0)
      return Safe_FromString(Ver.PriorityType());
   else if (strcmp("Downloadable", Name) == 0)
      return Py_BuildValue("b", Ver.Downloadable());
   else if (strcmp("TranslatedDescription", Name) == 0) {
      return CppOwnedPyObject_NEW<pkgCache::DescIterator>(Owner,
							  &DescriptionType,
							  Ver.TranslatedDescription());
   }
#if 0 // FIXME: enable once pkgSourceList is stored somewhere
   else if (strcmp("IsTrusted", Name) == 0)
   {
      pkgSourceList Sources;
      Sources.ReadMainList();
      for(pkgCache::VerFileIterator i = Ver.FileList(); !i.end(); i++)
      {
	 pkgIndexFile *index;
	 if(Sources.FindIndex(i.File(), index) && !index->IsTrusted())
	    return Py_BuildValue("b", false);
      }
      return Py_BuildValue("b", true);
   }
#endif

   PyErr_SetString(PyExc_AttributeError,Name);
   return 0;
}

static PyObject *VersionRepr(PyObject *Self)
{
   pkgCache::VerIterator &Ver = GetCpp<pkgCache::VerIterator>(Self);
   
   char S[300];
   snprintf(S,sizeof(S),"<pkgCache::Version object: Pkg:'%s' Ver:'%s' "
	                "Section:'%s' Arch:'%s' Size:%lu ISize:%lu Hash:%u "
	                "ID:%u Priority:%u>",
	    Ver.ParentPkg().Name(),Ver.VerStr(),Ver.Section(),Ver.Arch(),
	    (unsigned long)Ver->Size,(unsigned long)Ver->InstalledSize,
	    Ver->Hash,Ver->ID,Ver->Priority);
   return PyString_FromString(S);
}

PyTypeObject VersionType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgCache::VerIterator",             // tp_name
   sizeof(CppOwnedPyObject<pkgCache::VerIterator>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgCache::VerIterator>,          // tp_dealloc
   0,                                   // tp_print
   VersionAttr,                         // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   VersionRepr,                         // tp_repr
   0,                                   // tp_as_number
   0,		                        // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};
   
									/*}}}*/
   
// PackageFile Class							/*{{{*/
// ---------------------------------------------------------------------
static PyObject *PackageFileAttr(PyObject *Self,char *Name)
{
   pkgCache::PkgFileIterator &File = GetCpp<pkgCache::PkgFileIterator>(Self);
//   PyObject *Owner = GetOwner<pkgCache::PkgFileIterator>(Self);
   
   if (strcmp("FileName",Name) == 0)
      return Safe_FromString(File.FileName());
   else if (strcmp("Archive",Name) == 0)
      return Safe_FromString(File.Archive());
   else if (strcmp("Component",Name) == 0)
      return Safe_FromString(File.Component());
   else if (strcmp("Version",Name) == 0)
      return Safe_FromString(File.Version());
   else if (strcmp("Origin",Name) == 0)
      return Safe_FromString(File.Origin());
   else if (strcmp("Label",Name) == 0)
      return Safe_FromString(File.Label());
   else if (strcmp("Architecture",Name) == 0)
      return Safe_FromString(File.Architecture());
   else if (strcmp("Site",Name) == 0)
      return Safe_FromString(File.Site());
   else if (strcmp("IndexType",Name) == 0)
      return Safe_FromString(File.IndexType());
   else if (strcmp("Size",Name) == 0)
      return Py_BuildValue("i",File->Size);
   else if (strcmp("NotSource",Name) == 0)
      return Py_BuildValue("i",(File->Flags & pkgCache::Flag::NotSource) != 0);
   else if (strcmp("NotAutomatic",Name) == 0)
      return Py_BuildValue("i",(File->Flags & pkgCache::Flag::NotAutomatic) != 0);
   else if (strcmp("ID",Name) == 0)
      return Py_BuildValue("i",File->ID);
   
   PyErr_SetString(PyExc_AttributeError,Name);
   return 0;
}

static PyObject *PackageFileRepr(PyObject *Self)
{
   pkgCache::PkgFileIterator &File = GetCpp<pkgCache::PkgFileIterator>(Self);
   
   char S[300];
   snprintf(S,sizeof(S),"<pkgCache::PackageFile object: "
			"File:'%s' a=%s,c=%s,v=%s,o=%s,l=%s "
	                "Arch='%s' Site='%s' IndexType='%s' Size=%lu "
	                "Flags=0x%lX ID:%u>",
	    File.FileName(),File.Archive(),File.Component(),File.Version(),
	    File.Origin(),File.Label(),File.Architecture(),File.Site(),
	    File.IndexType(),File->Size,File->Flags,File->ID);
   return PyString_FromString(S);
}

PyTypeObject PackageFileType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgCache::PkgFileIterator",         // tp_name
   sizeof(CppOwnedPyObject<pkgCache::PkgFileIterator>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgCache::PkgFileIterator>,          // tp_dealloc
   0,                                   // tp_print
   PackageFileAttr,                     // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   PackageFileRepr,                     // tp_repr
   0,                                   // tp_as_number
   0,		                        // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};
   

// depends class
static PyObject *DependencyRepr(PyObject *Self)
{
   pkgCache::DepIterator &Dep = GetCpp<pkgCache::DepIterator>(Self);

   char S[300];
   snprintf(S,sizeof(S),"<pkgCache::Dependency object: "
	                "Pkg:'%s' Ver:'%s' Comp:'%s'>",
	    Dep.TargetPkg().Name(),
	    (Dep.TargetVer() == 0?"":Dep.TargetVer()),
	    Dep.CompType());
   return PyString_FromString(S);
}

static PyObject *DepSmartTargetPkg(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   pkgCache::DepIterator &Dep = GetCpp<pkgCache::DepIterator>(Self);
   PyObject *Owner = GetOwner<pkgCache::DepIterator>(Self);

   pkgCache::PkgIterator P;
   if (Dep.SmartTargetPkg(P) == false)
   {
      Py_INCREF(Py_None);
      return Py_None;
   }
   
   return CppOwnedPyObject_NEW<pkgCache::PkgIterator>(Owner,&PackageType,P);
}

static PyObject *DepAllTargets(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   pkgCache::DepIterator &Dep = GetCpp<pkgCache::DepIterator>(Self);
   PyObject *Owner = GetOwner<pkgCache::DepIterator>(Self);

   SPtr<pkgCache::Version *> Vers = Dep.AllTargets();
   PyObject *List = PyList_New(0);
   for (pkgCache::Version **I = Vers; *I != 0; I++)
   {
      PyObject *Obj;
      Obj = CppOwnedPyObject_NEW<pkgCache::VerIterator>(Owner,&VersionType,
							pkgCache::VerIterator(*Dep.Cache(),*I));
      PyList_Append(List,Obj);
      Py_DECREF(Obj);
   }
   return List;
}

static PyMethodDef DependencyMethods[] = 
{
   {"SmartTargetPkg",DepSmartTargetPkg,METH_VARARGS,"Returns the natural Target or None"},
   {"AllTargets",DepAllTargets,METH_VARARGS,"Returns all possible Versions that match this dependency"},
   {}
};

// Dependency Class							/*{{{*/
// ---------------------------------------------------------------------
   
static PyObject *DependencyAttr(PyObject *Self,char *Name)
{
   pkgCache::DepIterator &Dep = GetCpp<pkgCache::DepIterator>(Self);
   PyObject *Owner = GetOwner<pkgCache::DepIterator>(Self);
   
   if (strcmp("TargetVer",Name) == 0)
   {
      if (Dep->Version == 0)
	 return PyString_FromString("");
      return PyString_FromString(Dep.TargetVer());
   }   
   else if (strcmp("TargetPkg",Name) == 0)
      return CppOwnedPyObject_NEW<pkgCache::PkgIterator>(Owner,&PackageType,
							 Dep.TargetPkg());
   else if (strcmp("ParentVer",Name) == 0)
      return CppOwnedPyObject_NEW<pkgCache::VerIterator>(Owner,&VersionType,
							 Dep.ParentVer());
   else if (strcmp("ParentPkg",Name) == 0)
      return CppOwnedPyObject_NEW<pkgCache::PkgIterator>(Owner,&PackageType,							 Dep.ParentPkg());
   else if (strcmp("CompType",Name) == 0)
      return PyString_FromString(Dep.CompType());
   else if (strcmp("DepType",Name) == 0)
      return PyString_FromString(Dep.DepType());
   else if (strcmp("ID",Name) == 0)
      return Py_BuildValue("i",Dep->ID);
   
   return Py_FindMethod(DependencyMethods,Self,Name);
}

PyTypeObject DependencyType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgCache::DepIterator",             // tp_name
   sizeof(CppOwnedPyObject<pkgCache::DepIterator>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<pkgCache::DepIterator>,          // tp_dealloc
   0,                                   // tp_print
   DependencyAttr,                       // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   DependencyRepr,                       // tp_repr
   0,                                   // tp_as_number
   0,		                        // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};
   
									/*}}}*/
									/*}}}*/
// Reverse Dependency List Class					/*{{{*/
// ---------------------------------------------------------------------
static Py_ssize_t RDepListLen(PyObject *Self)
{
   return GetCpp<RDepListStruct>(Self).Len;
}

static PyObject *RDepListItem(PyObject *iSelf,Py_ssize_t Index)
{
   RDepListStruct &Self = GetCpp<RDepListStruct>(iSelf);
   if (Index < 0 || (unsigned)Index >= Self.Len)
   {
      PyErr_SetNone(PyExc_IndexError);
      return 0;
   }
   
   if ((unsigned)Index < Self.LastIndex)
   {
      Self.LastIndex = 0;
      Self.Iter = Self.Start;
   }
   
   while ((unsigned)Index > Self.LastIndex)
   {
      Self.LastIndex++;
      Self.Iter++;
      if (Self.Iter.end() == true)
      {
	 PyErr_SetNone(PyExc_IndexError);
	 return 0;
      }
   }
   
   return CppOwnedPyObject_NEW<pkgCache::DepIterator>(GetOwner<RDepListStruct>(iSelf),
						      &DependencyType,Self.Iter);
}

static PySequenceMethods RDepListSeq = 
{
   RDepListLen,
   0,                // concat
   0,                // repeat
   RDepListItem,
   0,                // slice
   0,                // assign item
   0                 // assign slice 
};
   
PyTypeObject RDepListType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "pkgCache::DepIterator",             // tp_name
   sizeof(CppOwnedPyObject<RDepListStruct>),   // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppOwnedDealloc<RDepListStruct>,      // tp_dealloc
   0,                                   // tp_print
   0,                                   // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   &RDepListSeq,                         // tp_as_sequence
   0,			                // tp_as_mapping
   0,                                   // tp_hash
};
   
									/*}}}*/



PyObject *TmpGetCache(PyObject *Self,PyObject *Args)
{
   PyObject *pyCallbackInst = 0;
   if (PyArg_ParseTuple(Args, "|O", &pyCallbackInst) == 0)
      return 0;

    if (_system == 0) {
        PyErr_SetString(PyExc_ValueError,"_system not initialized");
        return 0;
    }

   pkgCacheFile *Cache = new pkgCacheFile();

   if(pyCallbackInst != 0) {
      PyOpProgress progress;
      progress.setCallbackInst(pyCallbackInst);
      if (Cache->Open(progress,false) == false)
	 return HandleErrors();
   }  else {
      OpTextProgress Prog;
      if (Cache->Open(Prog,false) == false)
	 return HandleErrors();
   }

   CppOwnedPyObject<pkgCacheFile*> *CacheFileObj =
	   CppOwnedPyObject_NEW<pkgCacheFile*>(0,&PkgCacheFileType, Cache);
   
   CppOwnedPyObject<pkgCache *> *CacheObj =
	   CppOwnedPyObject_NEW<pkgCache *>(CacheFileObj,&PkgCacheType,
					    (pkgCache *)(*Cache));

   //Py_DECREF(CacheFileObj);
   return CacheObj;
}
