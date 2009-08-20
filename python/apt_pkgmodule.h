// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: apt_pkgmodule.h,v 1.4 2003/07/23 02:20:24 mdz Exp $
/* ######################################################################

   Prototypes for the module
   
   ##################################################################### */
									/*}}}*/
#ifndef APT_PKGMODULE_H
#define APT_PKGMODULE_H

#include <Python.h>

// Configuration Stuff
#define Configuration_Check(op) ((op)->ob_type == &ConfigurationType || \
                                 (op)->ob_type == &ConfigurationPtrType || \
                                 (op)->ob_type == &ConfigurationSubType)
extern PyTypeObject ConfigurationType;
extern PyTypeObject ConfigurationPtrType;
extern PyTypeObject ConfigurationSubType;
extern PyTypeObject VersionType;

extern char *doc_LoadConfig;
extern char *doc_LoadConfigISC;
extern char *doc_ParseCommandLine;
PyObject *LoadConfig(PyObject *Self,PyObject *Args);
PyObject *LoadConfigISC(PyObject *Self,PyObject *Args);
PyObject *ParseCommandLine(PyObject *Self,PyObject *Args);

// Tag File Stuff
extern PyTypeObject TagSecType;
extern PyTypeObject TagFileType;
extern char *doc_ParseSection;
extern char *doc_ParseTagFile;
extern char *doc_RewriteSection;
PyObject *ParseSection(PyObject *self,PyObject *Args);
PyObject *ParseTagFile(PyObject *self,PyObject *Args);
PyObject *RewriteSection(PyObject *self,PyObject *Args);

// String Stuff
PyObject *StrQuoteString(PyObject *self,PyObject *Args);
PyObject *StrDeQuote(PyObject *self,PyObject *Args);
PyObject *StrSizeToStr(PyObject *self,PyObject *Args);
PyObject *StrTimeToStr(PyObject *self,PyObject *Args);
PyObject *StrURItoFileName(PyObject *self,PyObject *Args);
PyObject *StrBase64Encode(PyObject *self,PyObject *Args);
PyObject *StrStringToBool(PyObject *self,PyObject *Args);
PyObject *StrTimeRFC1123(PyObject *self,PyObject *Args);
PyObject *StrStrToTime(PyObject *self,PyObject *Args);
PyObject *StrCheckDomainList(PyObject *Self,PyObject *Args);
   
// Cache Stuff
extern PyTypeObject PkgCacheType;
extern PyTypeObject PkgCacheFileType;
extern PyTypeObject PkgListType;
extern PyTypeObject PackageType;
extern PyTypeObject PackageFileType;
extern PyTypeObject DependencyType;
extern PyTypeObject RDepListType;
PyObject *TmpGetCache(PyObject *Self,PyObject *Args);

// DepCache
extern PyTypeObject PkgDepCacheType;
PyObject *GetDepCache(PyObject *Self,PyObject *Args);

// pkgProblemResolver
extern PyTypeObject PkgProblemResolverType;
PyObject *GetPkgProblemResolver(PyObject *Self, PyObject *Args);
PyObject *GetPkgActionGroup(PyObject *Self, PyObject *Args);

// cdrom
extern PyTypeObject PkgCdromType;
PyObject *GetCdrom(PyObject *Self,PyObject *Args);

// acquire
extern PyTypeObject PkgAcquireType;
extern char *doc_GetPkgAcqFile;
PyObject *GetAcquire(PyObject *Self,PyObject *Args);
PyObject *GetPkgAcqFile(PyObject *Self, PyObject *Args, PyObject *kwds);

// packagemanager
extern PyTypeObject PkgManagerType;
PyObject *GetPkgManager(PyObject *Self,PyObject *Args);


// PkgRecords Stuff
extern PyTypeObject PkgRecordsType;
PyObject *GetPkgRecords(PyObject *Self,PyObject *Args);
PyObject *GetPkgSrcRecords(PyObject *Self,PyObject *Args);

// pkgSourceList
extern PyTypeObject PkgSourceListType;
PyObject *GetPkgSourceList(PyObject *Self,PyObject *Args);

// pkgSourceList
extern PyTypeObject PackageIndexFileType;

// metaIndex
extern PyTypeObject MetaIndexType;


#endif
