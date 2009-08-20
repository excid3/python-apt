// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: apt_pkgmodule.cc,v 1.5 2003/07/23 02:20:24 mdz Exp $
/* ######################################################################

   apt_pkgmodule - Top level for the python module. Create the internal
                   structures for the module in the interpriter.
      
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "apt_pkgmodule.h"
#include "generic.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/version.h>
#include <apt-pkg/deblistparser.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/tagfile.h>
#include <apt-pkg/md5.h>
#include <apt-pkg/sha1.h>
#include <apt-pkg/sha256.h>
#include <apt-pkg/init.h>
#include <apt-pkg/pkgsystem.h>
    
#include <sys/stat.h>
#include <unistd.h>
#include <Python.h>
									/*}}}*/

// newConfiguration - Build a new configuration class			/*{{{*/
// ---------------------------------------------------------------------
static char *doc_newConfiguration = "Construct a configuration instance";
static PyObject *newConfiguration(PyObject *self,PyObject *args)
{
   return CppPyObject_NEW<Configuration>(&ConfigurationType);
}
									/*}}}*/

// Version Wrappers							/*{{{*/
// These are kind of legacy..
static char *doc_VersionCompare = "VersionCompare(a,b) -> int";
static PyObject *VersionCompare(PyObject *Self,PyObject *Args)
{
   char *A;
   char *B;
   int LenA;
   int LenB;
   
   if (PyArg_ParseTuple(Args,"s#s#",&A,&LenA,&B,&LenB) == 0)
      return 0;
   
   if (_system == 0)
   {
      PyErr_SetString(PyExc_ValueError,"_system not initialized");
      return 0;
   }
   
   return Py_BuildValue("i",_system->VS->DoCmpVersion(A,A+LenA,B,B+LenB));
}

static char *doc_CheckDep = "CheckDep(PkgVer,DepOp,DepVer) -> int";
static PyObject *CheckDep(PyObject *Self,PyObject *Args)
{
   char *A;
   char *B;
   char *OpStr;
   unsigned int Op = 0;
   
   if (PyArg_ParseTuple(Args,"sss",&A,&OpStr,&B) == 0)
      return 0;
   if (*debListParser::ConvertRelation(OpStr,Op) != 0)
   {
      PyErr_SetString(PyExc_ValueError,"Bad comparision operation");
      return 0;
   }

   if (_system == 0)
   {
      PyErr_SetString(PyExc_ValueError,"_system not initialized");
      return 0;
   }
   
   return Py_BuildValue("i",_system->VS->CheckDep(A,Op,B));
//   return Py_BuildValue("i",pkgCheckDep(B,A,Op));
}

static char *doc_UpstreamVersion = "UpstreamVersion(a) -> string";
static PyObject *UpstreamVersion(PyObject *Self,PyObject *Args)
{
   char *Ver;
   if (PyArg_ParseTuple(Args,"s",&Ver) == 0)
      return 0;
   return CppPyString(_system->VS->UpstreamVersion(Ver));
}

static char *doc_ParseDepends = 
"ParseDepends(s) -> list of tuples\n"
"\n"
"The resulting tuples are (Pkg,Ver,Operation). Each anded dependency is a\n"
"list of or'd dependencies\n"
"Source depends are evaluated against the curernt arch and only those that\n"
"Match are returned.";
static PyObject *RealParseDepends(PyObject *Self,PyObject *Args,
				  bool ParseArchFlags)
{
   string Package;
   string Version;
   unsigned int Op;
   
   const char *Start;
   const char *Stop;
   int Len;
   
   if (PyArg_ParseTuple(Args,"s#",&Start,&Len) == 0)
      return 0;
   Stop = Start + Len;
   PyObject *List = PyList_New(0);
   PyObject *LastRow = 0;
   while (1)
   {
      if (Start == Stop)
	 break;
      
      Start = debListParser::ParseDepends(Start,Stop,Package,Version,Op,
					  ParseArchFlags);
      if (Start == 0)
      {
	 PyErr_SetString(PyExc_ValueError,"Problem Parsing Dependency");
	 Py_DECREF(List);
	 return 0;
      }
      
      if (LastRow == 0)
	 LastRow = PyList_New(0);
      
      if (Package.empty() == false)
      {
	 PyObject *Obj;
	 PyList_Append(LastRow,Obj = Py_BuildValue("sss",Package.c_str(),
						   Version.c_str(),
						pkgCache::CompTypeDeb(Op)));
	 Py_DECREF(Obj);
      }
      
      // Group ORd deps into a single row..
      if ((Op & pkgCache::Dep::Or) != pkgCache::Dep::Or)
      {
	 if (PyList_Size(LastRow) != 0)
	    PyList_Append(List,LastRow);
	 Py_DECREF(LastRow);
	 LastRow = 0;
      }      
   }
   return List;
}
static PyObject *ParseDepends(PyObject *Self,PyObject *Args)
{
   return RealParseDepends(Self,Args,false);
}
static PyObject *ParseSrcDepends(PyObject *Self,PyObject *Args)
{
   return RealParseDepends(Self,Args,true);
}
									/*}}}*/
// md5sum - Compute the md5sum of a file or string			/*{{{*/
// ---------------------------------------------------------------------
static char *doc_md5sum = "md5sum(String) -> String or md5sum(File) -> String";
static PyObject *md5sum(PyObject *Self,PyObject *Args)
{
   PyObject *Obj;
   if (PyArg_ParseTuple(Args,"O",&Obj) == 0)
      return 0;
   
   // Digest of a string.
   if (PyString_Check(Obj) != 0)
   {
      char *s;
      Py_ssize_t len;
      MD5Summation Sum;
      PyString_AsStringAndSize(Obj, &s, &len);
      Sum.Add((const unsigned char*)s, len);
      return CppPyString(Sum.Result().Value());
   }   
   
   // Digest of a file
   if (PyFile_Check(Obj) != 0)
   {
      MD5Summation Sum;
      int Fd = fileno(PyFile_AsFile(Obj));
      struct stat St;
      if (fstat(Fd,&St) != 0 ||
	  Sum.AddFD(Fd,St.st_size) == false)
      {
	 PyErr_SetFromErrno(PyExc_SystemError);
	 return 0;
      }
      
      return CppPyString(Sum.Result().Value());
   }
   
   PyErr_SetString(PyExc_TypeError,"Only understand strings and files");
   return 0;
}
									/*}}}*/
// sha1sum - Compute the sha1sum of a file or string			/*{{{*/
// ---------------------------------------------------------------------
static char *doc_sha1sum = "sha1sum(String) -> String or sha1sum(File) -> String";
static PyObject *sha1sum(PyObject *Self,PyObject *Args)
{
   PyObject *Obj;
   if (PyArg_ParseTuple(Args,"O",&Obj) == 0)
      return 0;
   
   // Digest of a string.
   if (PyString_Check(Obj) != 0)
   {
      char *s;
      Py_ssize_t len;
      SHA1Summation Sum;
      PyString_AsStringAndSize(Obj, &s, &len);
      Sum.Add((const unsigned char*)s, len);
      return CppPyString(Sum.Result().Value());
   }   
   
   // Digest of a file
   if (PyFile_Check(Obj) != 0)
   {
      SHA1Summation Sum;
      int Fd = fileno(PyFile_AsFile(Obj));
      struct stat St;
      if (fstat(Fd,&St) != 0 ||
	  Sum.AddFD(Fd,St.st_size) == false)
      {
	 PyErr_SetFromErrno(PyExc_SystemError);
	 return 0;
      }
      
      return CppPyString(Sum.Result().Value());
   }
   
   PyErr_SetString(PyExc_TypeError,"Only understand strings and files");
   return 0;
}
									/*}}}*/
// sha256sum - Compute the sha1sum of a file or string			/*{{{*/
// ---------------------------------------------------------------------
static char *doc_sha256sum = "sha256sum(String) -> String or sha256sum(File) -> String";
static PyObject *sha256sum(PyObject *Self,PyObject *Args)
{
   PyObject *Obj;
   if (PyArg_ParseTuple(Args,"O",&Obj) == 0)
      return 0;
   
   // Digest of a string.
   if (PyString_Check(Obj) != 0)
   {
      char *s;
      Py_ssize_t len;
      SHA256Summation Sum;
      PyString_AsStringAndSize(Obj, &s, &len);
      Sum.Add((const unsigned char*)s, len);
      return CppPyString(Sum.Result().Value());
   }   
   
   // Digest of a file
   if (PyFile_Check(Obj) != 0)
   {
      SHA256Summation Sum;
      int Fd = fileno(PyFile_AsFile(Obj));
      struct stat St;
      if (fstat(Fd,&St) != 0 ||
	  Sum.AddFD(Fd,St.st_size) == false)
      {
	 PyErr_SetFromErrno(PyExc_SystemError);
	 return 0;
      }
      
      return CppPyString(Sum.Result().Value());
   }
   
   PyErr_SetString(PyExc_TypeError,"Only understand strings and files");
   return 0;
}
									/*}}}*/
// init - 3 init functions						/*{{{*/
// ---------------------------------------------------------------------
static char *doc_Init = 
"init() -> None\n"
"Legacy. Do InitConfig then parse the command line then do InitSystem\n";
static PyObject *Init(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   pkgInitConfig(*_config);   
   pkgInitSystem(*_config,_system);
   
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}

static char *doc_InitConfig =
"initconfig() -> None\n"
"Load the default configuration and the config file\n";
static PyObject *InitConfig(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   pkgInitConfig(*_config);   
   
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}

static char *doc_InitSystem =
"initsystem() -> None\n"
"Construct the underlying system\n";
static PyObject *InitSystem(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   pkgInitSystem(*_config,_system);
   
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}
									/*}}}*/

// fileutils.cc: GetLock						/*{{{*/
// ---------------------------------------------------------------------
static char *doc_GetLock = 
"GetLock(string) -> int\n"
"This will create an empty file of the given name and lock it. Once this"
" is done all other calls to GetLock in any other process will fail with"
" -1. The return result is the fd of the file, the call should call"
" close at some time\n";
static PyObject *GetLock(PyObject *Self,PyObject *Args)
{
   const char *file;
   char errors = false;
   if (PyArg_ParseTuple(Args,"s|b",&file,&errors) == 0)
      return 0;
   
   int fd = GetLock(file, errors);

   return  HandleErrors(Py_BuildValue("i", fd));
}

static char *doc_PkgSystemLock =
"PkgSystemLock() -> boolean\n"
"Get the global pkgsystem lock\n";
static PyObject *PkgSystemLock(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   bool res = _system->Lock();
   
   Py_INCREF(Py_None);
   return HandleErrors(Py_BuildValue("b", res));
}

static char *doc_PkgSystemUnLock =
"PkgSystemUnLock() -> boolean\n"
"Unset the global pkgsystem lock\n";
static PyObject *PkgSystemUnLock(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   bool res = _system->UnLock();
   
   Py_INCREF(Py_None);
   return HandleErrors(Py_BuildValue("b", res));
}

									/*}}}*/

// initapt_pkg - Core Module Initialization				/*{{{*/
// ---------------------------------------------------------------------
/* */
static PyMethodDef methods[] = 
{
   // Constructors
   {"newConfiguration",newConfiguration,METH_VARARGS,doc_newConfiguration},
   {"init",Init,METH_VARARGS,doc_Init},
   {"InitConfig",InitConfig,METH_VARARGS,doc_InitConfig},
   {"InitSystem",InitSystem,METH_VARARGS,doc_InitSystem},

   // Tag File
   {"ParseSection",ParseSection,METH_VARARGS,doc_ParseSection},
   {"ParseTagFile",ParseTagFile,METH_VARARGS,doc_ParseTagFile},
   {"RewriteSection",RewriteSection,METH_VARARGS,doc_RewriteSection},

   // Locking
   {"GetLock",GetLock,METH_VARARGS,doc_GetLock},
   {"PkgSystemLock",PkgSystemLock,METH_VARARGS,doc_PkgSystemLock},
   {"PkgSystemUnLock",PkgSystemUnLock,METH_VARARGS,doc_PkgSystemUnLock},

   // Command line
   {"ReadConfigFile",LoadConfig,METH_VARARGS,doc_LoadConfig},
   {"ReadConfigFileISC",LoadConfigISC,METH_VARARGS,doc_LoadConfig},
   {"ParseCommandLine",ParseCommandLine,METH_VARARGS,doc_ParseCommandLine},
   
   // Versioning
   {"VersionCompare",VersionCompare,METH_VARARGS,doc_VersionCompare},
   {"CheckDep",CheckDep,METH_VARARGS,doc_CheckDep},
   {"UpstreamVersion",UpstreamVersion,METH_VARARGS,doc_UpstreamVersion},
 
   // Depends
   {"ParseDepends",ParseDepends,METH_VARARGS,doc_ParseDepends},
   {"ParseSrcDepends",ParseSrcDepends,METH_VARARGS,doc_ParseDepends},
   
   // Stuff
   {"md5sum",md5sum,METH_VARARGS,doc_md5sum},
   {"sha1sum",sha1sum,METH_VARARGS,doc_sha1sum},
   {"sha256sum",sha256sum,METH_VARARGS,doc_sha256sum},

   // Strings
   {"CheckDomainList",StrCheckDomainList,METH_VARARGS,"CheckDomainList(String,String) -> Bool"},
   {"QuoteString",StrQuoteString,METH_VARARGS,"QuoteString(String,String) -> String"},
   {"DeQuoteString",StrDeQuote,METH_VARARGS,"DeQuoteString(String) -> String"},
   {"SizeToStr",StrSizeToStr,METH_VARARGS,"SizeToStr(int) -> String"},
   {"TimeToStr",StrTimeToStr,METH_VARARGS,"TimeToStr(int) -> String"},
   {"URItoFileName",StrURItoFileName,METH_VARARGS,"URItoFileName(String) -> String"},
   {"Base64Encode",StrBase64Encode,METH_VARARGS,"Base64Encode(String) -> String"},
   {"StringToBool",StrStringToBool,METH_VARARGS,"StringToBool(String) -> int"},
   {"TimeRFC1123",StrTimeRFC1123,METH_VARARGS,"TimeRFC1123(int) -> String"},
   {"StrToTime",StrStrToTime,METH_VARARGS,"StrToTime(String) -> Int"},

   // Cache
   {"GetCache",TmpGetCache,METH_VARARGS,"GetCache() -> PkgCache"},
   {"GetDepCache",GetDepCache,METH_VARARGS,"GetDepCache(Cache) -> DepCache"},
   {"GetPkgRecords",GetPkgRecords,METH_VARARGS,"GetPkgRecords(Cache) -> PkgRecords"},
   {"GetPkgSrcRecords",GetPkgSrcRecords,METH_VARARGS,"GetPkgSrcRecords() -> PkgSrcRecords"},
   {"GetPkgSourceList",GetPkgSourceList,METH_VARARGS,"GetPkgSourceList() -> PkgSourceList"},

   // misc
   {"GetPkgProblemResolver",GetPkgProblemResolver,METH_VARARGS,"GetDepProblemResolver(DepCache) -> PkgProblemResolver"},
   {"GetPkgActionGroup",GetPkgActionGroup,METH_VARARGS,"GetPkgActionGroup(DepCache) -> PkgActionGroup"},

   // Cdrom
   {"GetCdrom",GetCdrom,METH_VARARGS,"GetCdrom() -> Cdrom"},

   // Acquire
   {"GetAcquire",GetAcquire,METH_VARARGS,"GetAcquire() -> Acquire"},
   {"GetPkgAcqFile",(PyCFunction)GetPkgAcqFile,METH_KEYWORDS|METH_VARARGS, doc_GetPkgAcqFile},

   // PkgManager
   {"GetPackageManager",GetPkgManager,METH_VARARGS,"GetPackageManager(DepCache) -> PackageManager"},

   {}
};

static void AddStr(PyObject *Dict,const char *Itm,const char *Str)
{
   PyObject *Obj = PyString_FromString(Str);
   PyDict_SetItemString(Dict,(char *)Itm,Obj);
   Py_DECREF(Obj);
}

static void AddInt(PyObject *Dict,const char *Itm,unsigned long I)
{
   PyObject *Obj = Py_BuildValue("i",I);
   PyDict_SetItemString(Dict,(char *)Itm,Obj);
   Py_DECREF(Obj);
}

extern "C" void initapt_pkg()
{
   PyObject *Module = Py_InitModule("apt_pkg",methods);
   PyObject *Dict = PyModule_GetDict(Module);
   
   // Global variable linked to the global configuration class
   CppPyObject<Configuration *> *Config = CppPyObject_NEW<Configuration *>(&ConfigurationPtrType);
   Config->Object = _config;
   PyDict_SetItemString(Dict,"Config",Config);
   Py_DECREF(Config);
   
   // Tag file constants
   PyObject *Obj;
   PyDict_SetItemString(Dict,"RewritePackageOrder",
			Obj = CharCharToList(TFRewritePackageOrder));
   Py_DECREF(Obj);
   PyDict_SetItemString(Dict,"RewriteSourceOrder",
			Obj = CharCharToList(TFRewriteSourceOrder));
   Py_DECREF(Obj);
   
   // Version..
   AddStr(Dict,"Version",pkgVersion);
   AddStr(Dict,"LibVersion",pkgLibVersion);
   AddStr(Dict,"Date",__DATE__);
   AddStr(Dict,"Time",__TIME__);

   // My constants
   AddInt(Dict,"DepDepends",pkgCache::Dep::Depends);
   AddInt(Dict,"DepPreDepends",pkgCache::Dep::PreDepends);
   AddInt(Dict,"DepSuggests",pkgCache::Dep::Suggests);
   AddInt(Dict,"DepRecommends",pkgCache::Dep::Recommends);
   AddInt(Dict,"DepConflicts",pkgCache::Dep::Conflicts);
   AddInt(Dict,"DepReplaces",pkgCache::Dep::Replaces);
   AddInt(Dict,"DepObsoletes",pkgCache::Dep::Obsoletes);

   AddInt(Dict,"PriImportant",pkgCache::State::Important);
   AddInt(Dict,"PriRequired",pkgCache::State::Required);
   AddInt(Dict,"PriStandard",pkgCache::State::Standard);
   AddInt(Dict,"PriOptional",pkgCache::State::Optional);
   AddInt(Dict,"PriExtra",pkgCache::State::Extra);

   AddInt(Dict,"CurStateNotInstalled",pkgCache::State::NotInstalled);
   AddInt(Dict,"CurStateUnPacked",pkgCache::State::UnPacked);
   AddInt(Dict,"CurStateHalfConfigured",pkgCache::State::HalfConfigured);
   AddInt(Dict,"CurStateHalfInstalled",pkgCache::State::HalfInstalled);
   AddInt(Dict,"CurStateConfigFiles",pkgCache::State::ConfigFiles);
   AddInt(Dict,"CurStateInstalled",pkgCache::State::Installed);

   AddInt(Dict,"SelStateUnknown",pkgCache::State::Unknown);
   AddInt(Dict,"SelStateInstall",pkgCache::State::Install);
   AddInt(Dict,"SelStateHold",pkgCache::State::Hold);
   AddInt(Dict,"SelStateDeInstall",pkgCache::State::DeInstall);
   AddInt(Dict,"SelStatePurge",pkgCache::State::Purge);

   AddInt(Dict,"InstStateOk",pkgCache::State::Ok);
   AddInt(Dict,"InstStateReInstReq",pkgCache::State::ReInstReq);
   AddInt(Dict,"InstStateHold",pkgCache::State::Hold);
   AddInt(Dict,"InstStateHoldReInstReq",pkgCache::State::HoldReInstReq);
}
									/*}}}*/
									  
