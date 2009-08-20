// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: tag.cc,v 1.3 2002/02/26 01:36:15 mdz Exp $
/* ######################################################################

   Tag - Binding for the RFC 822 tag file parser
   
   Upon reflection I have make the TagSection wrapper look like a map..
   The other option was to use a sequence (which nicely matches the internal
   storage) but really makes no sense to a Python Programmer.. One 
   specialized lookup is provided, the FindFlag lookup - as well as the 
   usual set of duplicate things to match the C++ interface.
   
   The TagFile interface is also slightly different, it has a built in 
   internal TagSection object that is used. Do not hold onto a reference
   to a TagSection and let TagFile go out of scope! The underlying storage
   for the section will go away and it will seg.
   
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "generic.h"
#include "apt_pkgmodule.h"

#include <apt-pkg/tagfile.h>

#include <stdio.h>
#include <iostream>
#include <Python.h>

using namespace std;
									/*}}}*/
/* We need to keep a private copy of the data.. */
struct TagSecData : public CppPyObject<pkgTagSection>
{
   char *Data;
};

struct TagFileData : public PyObject
{
   pkgTagFile Object;
   PyObject *File;
   TagSecData *Section;
   FileFd Fd;
};

									/*}}}*/
// TagSecFree - Free a Tag Section					/*{{{*/
// ---------------------------------------------------------------------
/* */
void TagSecFree(PyObject *Obj)
{
   TagSecData *Self = (TagSecData *)Obj;
   delete [] Self->Data;
   CppDealloc<pkgTagSection>(Obj);
}
									/*}}}*/
// TagFileFree - Free a Tag File					/*{{{*/
// ---------------------------------------------------------------------
/* */
void TagFileFree(PyObject *Obj)
{
   TagFileData *Self = (TagFileData *)Obj;
   Py_DECREF((PyObject *)Self->Section);
   Self->Object.~pkgTagFile();
   Self->Fd.~FileFd();
   Py_DECREF(Self->File);
   PyObject_DEL(Obj);
}
									/*}}}*/

// Tag Section Wrappers							/*{{{*/
static char *doc_Find = "Find(Name) -> String/None";
static PyObject *TagSecFind(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   char *Default = 0;
   if (PyArg_ParseTuple(Args,"s|z",&Name,&Default) == 0)
      return 0;
   
   const char *Start;
   const char *Stop;
   if (GetCpp<pkgTagSection>(Self).Find(Name,Start,Stop) == false)
   {
      if (Default == 0)
	 Py_RETURN_NONE;
      return PyString_FromString(Default);
   }
   return PyString_FromStringAndSize(Start,Stop-Start);
}

static char *doc_FindFlag = "FindFlag(Name) -> integer/none";
static PyObject *TagSecFindFlag(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   if (PyArg_ParseTuple(Args,"s",&Name) == 0)
      return 0;
   
   unsigned long Flag = 0;
   if (GetCpp<pkgTagSection>(Self).FindFlag(Name,Flag,1) == false)
   {
      Py_INCREF(Py_None);
      return Py_None;
   }
   return Py_BuildValue("i",Flag);
}

// Map access, operator []
static PyObject *TagSecMap(PyObject *Self,PyObject *Arg)
{
   if (PyString_Check(Arg) == 0)
   {
      PyErr_SetNone(PyExc_TypeError);
      return 0;
   }
   
   const char *Start;
   const char *Stop;
   if (GetCpp<pkgTagSection>(Self).Find(PyString_AsString(Arg),Start,Stop) == false)
   {  
      PyErr_SetString(PyExc_KeyError,PyString_AsString(Arg));
      return 0;
   }
   	 
   return PyString_FromStringAndSize(Start,Stop-Start);
}

// len() operation
static Py_ssize_t TagSecLength(PyObject *Self)
{
   pkgTagSection &Sec = GetCpp<pkgTagSection>(Self);
   return Sec.Count();
}

// Look like a mapping
static char *doc_Keys = "keys() -> List";
static PyObject *TagSecKeys(PyObject *Self,PyObject *Args)
{
   pkgTagSection &Tags = GetCpp<pkgTagSection>(Self);
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   // Convert the whole configuration space into a list
   PyObject *List = PyList_New(0);
   for (unsigned int I = 0; I != Tags.Count(); I++)
   {  
      const char *Start;
      const char *Stop;
      Tags.Get(Start,Stop,I);
      const char *End = Start;
      for (; End < Stop && *End != ':'; End++);
      
      PyObject *Obj;
      PyList_Append(List,Obj = PyString_FromStringAndSize(Start,End-Start));
      Py_DECREF(Obj);
   }
   return List;
}

static char *doc_Exists = "Exists(Name) -> integer";
static PyObject *TagSecExists(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   if (PyArg_ParseTuple(Args,"s",&Name) == 0)
      return 0;
   
   const char *Start;
   const char *Stop;
   if (GetCpp<pkgTagSection>(Self).Find(Name,Start,Stop) == false)
      return Py_BuildValue("i",0);
   return Py_BuildValue("i",1);
}

static char *doc_Bytes = "Bytes() -> integer";
static PyObject *TagSecBytes(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   return Py_BuildValue("i",GetCpp<pkgTagSection>(Self).size());
}

static PyObject *TagSecStr(PyObject *Self)
{
   const char *Start;
   const char *Stop;
   GetCpp<pkgTagSection>(Self).GetSection(Start,Stop);
   return PyString_FromStringAndSize(Start,Stop-Start);
}
									/*}}}*/
// TagFile Wrappers							/*{{{*/
static char *doc_Step = "Step() -> Integer\n0 means EOF.";
static PyObject *TagFileStep(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   TagFileData &Obj = *(TagFileData *)Self;
   if (Obj.Object.Step(Obj.Section->Object) == false)
      return HandleErrors(Py_BuildValue("i",0));
   
   return HandleErrors(Py_BuildValue("i",1));
}

static char *doc_Offset = "Offset() -> Integer";
static PyObject *TagFileOffset(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   return Py_BuildValue("i",((TagFileData *)Self)->Object.Offset());
}

static char *doc_Jump = "Jump(Offset) -> Integer";
static PyObject *TagFileJump(PyObject *Self,PyObject *Args)
{
   int Offset;
   if (PyArg_ParseTuple(Args,"i",&Offset) == 0)
      return 0;
   
   TagFileData &Obj = *(TagFileData *)Self;
   if (Obj.Object.Jump(Obj.Section->Object,Offset) == false)
      return HandleErrors(Py_BuildValue("i",0));
   
   return HandleErrors(Py_BuildValue("i",1));
}
									/*}}}*/
// ParseSection - Parse a single section from a tag file		/*{{{*/
// ---------------------------------------------------------------------
char *doc_ParseSection ="ParseSection(Text) -> SectionObject";
PyObject *ParseSection(PyObject *self,PyObject *Args)
{
   char *Data;
   if (PyArg_ParseTuple(Args,"s",&Data) == 0)
      return 0;
   
   // Create the object..
   TagSecData *New = PyObject_NEW(TagSecData,&TagSecType);
   new (&New->Object) pkgTagSection();
   New->Data = new char[strlen(Data)+2];
   snprintf(New->Data,strlen(Data)+2,"%s\n",Data);
   
   if (New->Object.Scan(New->Data,strlen(New->Data)) == false)
   {
      cerr << New->Data << endl;
      Py_DECREF((PyObject *)New);
      PyErr_SetString(PyExc_ValueError,"Unable to parse section data");
      return 0;
   }   
   
   New->Object.Trim();
   
   return New;
}
									/*}}}*/
// ParseTagFile - Parse a tagd file					/*{{{*/
// ---------------------------------------------------------------------
/* This constructs the parser state. */
char *doc_ParseTagFile = "ParseTagFile(File) -> TagFile";
PyObject *ParseTagFile(PyObject *self,PyObject *Args)
{
   PyObject *File;
   if (PyArg_ParseTuple(Args,"O!",&PyFile_Type,&File) == 0)
      return 0;
   
   TagFileData *New = PyObject_NEW(TagFileData,&TagFileType);
   new (&New->Fd) FileFd(fileno(PyFile_AsFile(File)),false);
   New->File = File;
   Py_INCREF(New->File);
   new (&New->Object) pkgTagFile(&New->Fd);
   
   // Create the section
   New->Section = PyObject_NEW(TagSecData,&TagSecType);
   new (&New->Section->Object) pkgTagSection();
   New->Section->Data = 0;
   
   return HandleErrors(New);
}
									/*}}}*/								     
// RewriteSection - Rewrite a section..					/*{{{*/
// ---------------------------------------------------------------------
/* An interesting future extension would be to add a user settable 
   order list */
char *doc_RewriteSection = 
"RewriteSection(Section,Order,RewriteList) -> String\n"
"\n"
"The section rewriter allows a section to be taken in, have fields added,\n"
"removed or changed and then put back out. During this process the fields\n"
"within the section are sorted to corrispond to a proper order. Order is a\n"
"list of field names with their proper capitialization.\n"
"apt_pkg.RewritePackageOrder and apt_pkg.RewriteSourceOrder are two predefined\n"
"orders.\n"
"RewriteList is a list of tuples. Each tuple is of the form:\n"
"  (Tag,NewValue[,RenamedTo])\n"
"Tag specifies the tag in the source section. NewValue is the new value of\n"
"that tag and the optional RenamedTo field can cause the tag to be changed.\n"
"If NewValue is None then the tag is removed\n"
"Ex. ('Source','apt','Package') is used for .dsc files.";
PyObject *RewriteSection(PyObject *self,PyObject *Args)
{
   PyObject *Section;
   PyObject *Order;
   PyObject *Rewrite;
   if (PyArg_ParseTuple(Args,"O!O!O!",&TagSecType,&Section,
			&PyList_Type,&Order,&PyList_Type,&Rewrite) == 0)
      return 0;
   
   // Convert the order list
   const char **OrderList = ListToCharChar(Order,true);
   
   // Convert the Rewrite list.
   TFRewriteData *List = new TFRewriteData[PySequence_Length(Rewrite)+1];
   memset(List,0,sizeof(*List)*(PySequence_Length(Rewrite)+1));
   for (int I = 0; I != PySequence_Length(Rewrite); I++)
   {
      List[I].NewTag = 0;
      if (PyArg_ParseTuple(PySequence_GetItem(Rewrite,I),"sz|s",
			  &List[I].Tag,&List[I].Rewrite,&List[I].NewTag) == 0)
      {
	 delete [] OrderList;
	 delete [] List;
	 return 0;
      }
   }
   
   /* This is a glibc extension.. If not running on glibc I'd just take
      this whole function out, it is probably infrequently used */
   char *bp = 0;
   size_t size;
   FILE *F = open_memstream (&bp, &size);

   // Do the rewrite
   bool Res = TFRewrite(F,GetCpp<pkgTagSection>(Section),OrderList,List);
   delete [] OrderList;
   delete [] List;
   fclose(F);
   
   if (Res == false)
   {
      free(bp);
      return HandleErrors();
   }
   
   // Return the string
   PyObject *ResObj = PyString_FromStringAndSize(bp,size);
   free(bp);
   return HandleErrors(ResObj);
}
									/*}}}*/

// Method table for the Tag Section object
static PyMethodDef TagSecMethods[] = 
{
   // Query
   {"Find",TagSecFind,METH_VARARGS,doc_Find},
   {"FindFlag",TagSecFindFlag,METH_VARARGS,doc_FindFlag},
   {"Bytes",TagSecBytes,METH_VARARGS,doc_Bytes},
	 
   // Python Special
   {"keys",TagSecKeys,METH_VARARGS,doc_Keys},
   {"has_key",TagSecExists,METH_VARARGS,doc_Exists},
   {"get",TagSecFind,METH_VARARGS,doc_Find},
   {}
};

// TagSecGetAttr - Get an attribute - variable/method			/*{{{*/
// ---------------------------------------------------------------------
/* */
static PyObject *TagSecGetAttr(PyObject *Self,char *Name)
{
   return Py_FindMethod(TagSecMethods,Self,Name);
}
									/*}}}*/
// Type for a Tag Section
PyMappingMethods TagSecMapMeth = {TagSecLength,TagSecMap,0};
PyTypeObject TagSecType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "TagSection",	                // tp_name
   sizeof(TagSecData),                  // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   TagSecFree,                          // tp_dealloc
   0,		                        // tp_print
   TagSecGetAttr,                       // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   &TagSecMapMeth,                      // tp_as_mapping
   0,                                   // tp_hash
   0,					// tp_call
   TagSecStr,				// tp_str
};

// Method table for the Tag File object
static PyMethodDef TagFileMethods[] = 
{
   // Query
   {"Step",TagFileStep,METH_VARARGS,doc_Step},
   {"Offset",TagFileOffset,METH_VARARGS,doc_Offset},
   {"Jump",TagFileJump,METH_VARARGS,doc_Jump},
	 
   {}
};

// TagFileGetAttr - Get an attribute - variable/method			/*{{{*/
// ---------------------------------------------------------------------
/* */
static PyObject *TagFileGetAttr(PyObject *Self,char *Name)
{
   if (strcmp("Section",Name) == 0)
   {
      PyObject *Obj = ((TagFileData *)Self)->Section;
      Py_INCREF(Obj);
      return Obj;
   }   
   
   return Py_FindMethod(TagFileMethods,Self,Name);
}

// Type for a Tag File
PyTypeObject TagFileType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "TagFile",		                // tp_name
   sizeof(TagFileData),                 // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   TagFileFree,                         // tp_dealloc
   0,                                   // tp_print
   TagFileGetAttr,                      // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   0,		                        // tp_as_mapping
   0,                                   // tp_hash
};
