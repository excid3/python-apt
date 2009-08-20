// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: configuration.cc,v 1.4 2003/06/03 03:22:27 mdz Exp $
/* ######################################################################

   Configuration - Binding for the configuration object.

   There are three seperate classes..
     Configuration - A stand alone configuration instance
     ConfigurationPtr - A pointer to a configuration instance, used only
                        for the global instance (_config)
     ConfigurationSub - A subtree - has a reference to its owner.
   
   The wrapping is mostly 1:1 with the C++ code, but there are additions to
   wrap the linked tree walking into nice flat sequence walking.   
   
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"
#include "apt_pkgmodule.h"

#include <apt-pkg/configuration.h>
#include <apt-pkg/cmndline.h>

#include <Python.h>
									/*}}}*/
/* If we create a sub tree then it is of this type, the Owner is used
   to manage reference counting. */
struct SubConfiguration : public CppPyObject<Configuration>
{
   PyObject *Owner;
};

									/*}}}*/
// CnfSubFree - Free a sub configuration				/*{{{*/
// ---------------------------------------------------------------------
/* */
void CnfSubFree(PyObject *Obj)
{
   SubConfiguration *Self = (SubConfiguration *)Obj;
   Py_DECREF(Self->Owner);
   CppDealloc<Configuration>(Obj);
}
									/*}}}*/

// GetSelf - Convert PyObject to Configuration				/*{{{*/
// ---------------------------------------------------------------------
/* */
static inline Configuration &GetSelf(PyObject *Obj)
{
   if (Obj->ob_type == &ConfigurationPtrType)
      return *GetCpp<Configuration *>(Obj);
   return GetCpp<Configuration>(Obj);
}
									/*}}}*/

// Method Wrappers							/*{{{*/
static char *doc_Find = "Find(Name[,default]) -> String/None";
static PyObject *CnfFind(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   char *Default = 0;
   if (PyArg_ParseTuple(Args,"s|s",&Name,&Default) == 0)
      return 0;
   return CppPyString(GetSelf(Self).Find(Name,Default));
}

static char *doc_FindFile = "FindFile(Name[,default]) -> String/None";
static PyObject *CnfFindFile(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   char *Default = 0;
   if (PyArg_ParseTuple(Args,"s|s",&Name,&Default) == 0)
      return 0;
   return CppPyString(GetSelf(Self).FindFile(Name,Default));
}

static char *doc_FindDir = "FindDir(Name[,default]) -> String/None";
static PyObject *CnfFindDir(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   char *Default = 0;
   if (PyArg_ParseTuple(Args,"s|s",&Name,&Default) == 0)
      return 0;
   return CppPyString(GetSelf(Self).FindDir(Name,Default));
}

static char *doc_FindI = "FindI(Name[,default]) -> Integer";
static PyObject *CnfFindI(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   int Default = 0;
   if (PyArg_ParseTuple(Args,"s|i",&Name,&Default) == 0)
      return 0;
   return Py_BuildValue("i",GetSelf(Self).FindI(Name,Default));
}

static char *doc_FindB = "FindB(Name[,default]) -> Integer";
static PyObject *CnfFindB(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   int Default = 0;
   if (PyArg_ParseTuple(Args,"s|i",&Name,&Default) == 0)
      return 0;
   return Py_BuildValue("i",(int)GetSelf(Self).FindB(Name,(Default == 0?false:true)));
}

static char *doc_Set = "Set(Name,Value) -> None";
static PyObject *CnfSet(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   char *Value = 0;
   if (PyArg_ParseTuple(Args,"ss",&Name,&Value) == 0)
      return 0;
   
   GetSelf(Self).Set(Name,Value);
   Py_INCREF(Py_None);
   return Py_None;
}

static char *doc_Exists = "Exists(Name) -> Integer";
static PyObject *CnfExists(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   if (PyArg_ParseTuple(Args,"s",&Name) == 0)
      return 0;
   return Py_BuildValue("i",(int)GetSelf(Self).Exists(Name));
}

static char *doc_Clear = "Clear(Name) -> None";
static PyObject *CnfClear(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   if (PyArg_ParseTuple(Args,"s",&Name) == 0)
      return 0;
   
   GetSelf(Self).Clear(Name);
   
   Py_INCREF(Py_None);
   return Py_None;
}

// The amazing narrowing search ability!
static char *doc_SubTree = "SubTree(Name) -> Configuration";
static PyObject *CnfSubTree(PyObject *Self,PyObject *Args)
{
   char *Name;
   if (PyArg_ParseTuple(Args,"s",&Name) == 0)
      return 0;
   const Configuration::Item *Itm = GetSelf(Self).Tree(Name);
   if (Itm == 0)
   {
      PyErr_SetString(PyExc_KeyError,Name);
      return 0;
   }

   // Create a new sub configuration.
   SubConfiguration *New = PyObject_NEW(SubConfiguration,&ConfigurationSubType);
   new (&New->Object) Configuration(Itm);
   New->Owner = Self;
   Py_INCREF(Self);
   return New;
}

// Return a list of items at a specific level
static char *doc_List = "List([root]) -> List";
static PyObject *CnfList(PyObject *Self,PyObject *Args)
{
   char *RootName = 0;
   if (PyArg_ParseTuple(Args,"|s",&RootName) == 0)
      return 0;
   
   // Convert the whole configuration space into a list
   PyObject *List = PyList_New(0);
   const Configuration::Item *Top = GetSelf(Self).Tree(RootName);
   const Configuration::Item *Root = GetSelf(Self).Tree(0)->Parent;
   if (Top != 0 && RootName != 0)
      Top = Top->Child;
   for (; Top != 0; Top = Top->Next)
   {
      PyObject *Obj;
      PyList_Append(List,Obj = CppPyString(Top->FullTag(Root)));      
      Py_DECREF(Obj);
   }
   
   return List;
}

/* Return a list of values of items at a specific level.. This is used to
   get out value lists */
static char *doc_ValueList = "ValueList([root]) -> List";
static PyObject *CnfValueList(PyObject *Self,PyObject *Args)
{
   char *RootName = 0;
   if (PyArg_ParseTuple(Args,"|s",&RootName) == 0)
      return 0;
   
   // Convert the whole configuration space into a list
   PyObject *List = PyList_New(0);
   const Configuration::Item *Top = GetSelf(Self).Tree(RootName);
   if (Top != 0 && RootName != 0)
      Top = Top->Child;
   for (; Top != 0; Top = Top->Next)
   {
      PyObject *Obj;
      PyList_Append(List,Obj = CppPyString(Top->Value));
      Py_DECREF(Obj);
   }
   
   return List;
}

static char *doc_MyTag = "MyTag() -> String";
static PyObject *CnfMyTag(PyObject *Self,PyObject *Args)
{
   if (PyArg_ParseTuple(Args,"") == 0)
      return 0;
   
   const Configuration::Item *Top = GetSelf(Self).Tree(0);
   if (Top == 0)
      return Py_BuildValue("s","");
   return CppPyString(Top->Parent->Tag);
}

// Look like a mapping
static char *doc_Keys = "keys([root]) -> List";
static PyObject *CnfKeys(PyObject *Self,PyObject *Args)
{
   char *RootName = 0;
   if (PyArg_ParseTuple(Args,"|s",&RootName) == 0)
      return 0;
   
   // Convert the whole configuration space into a list
   PyObject *List = PyList_New(0);
   const Configuration::Item *Top = GetSelf(Self).Tree(RootName);
   const Configuration::Item *Stop = Top;
   const Configuration::Item *Root = 0;
   if (RootName == 0)
      Stop = 0;
   if (Top != 0)
      Root = GetSelf(Self).Tree(0)->Parent;
   for (; Top != 0;)
   {      
      PyObject *Obj;
      PyList_Append(List,Obj = CppPyString(Top->FullTag(Root)));
      Py_DECREF(Obj);
      
      if (Top->Child != 0)
      {
	 Top = Top->Child;
	 continue;
      }
      
      while (Top != 0 && Top->Next == 0 && Top != Root &&
	     Top->Parent != Stop)
	 Top = Top->Parent;
      if (Top != 0)
	 Top = Top->Next;
   }

   return List;
}

// Map access, operator []
static PyObject *CnfMap(PyObject *Self,PyObject *Arg)
{
   if (PyString_Check(Arg) == 0)
   {
      PyErr_SetNone(PyExc_TypeError);
      return 0;
   }
   
   if (GetSelf(Self).Exists(PyString_AsString(Arg)) == false)
   {  
      PyErr_SetString(PyExc_KeyError,PyString_AsString(Arg));
      return 0;
   }
   	 
   return CppPyString(GetSelf(Self).Find(PyString_AsString(Arg)));
}

// Assignment with operator []
static int CnfMapSet(PyObject *Self,PyObject *Arg,PyObject *Val)
{
   if (PyString_Check(Arg) == 0 || PyString_Check(Val) == 0)
   {
      PyErr_SetNone(PyExc_TypeError);
      return -1;
   }
   
   GetSelf(Self).Set(PyString_AsString(Arg),PyString_AsString(Val));
   return 0;
}
									/*}}}*/
// Config file loaders							/*{{{*/
char *doc_LoadConfig = "LoadConfig(Configuration,FileName) -> None";
PyObject *LoadConfig(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   if (PyArg_ParseTuple(Args,"Os",&Self,&Name) == 0)
      return 0;
   if (Configuration_Check(Self)== 0)
   {
      PyErr_SetString(PyExc_TypeError,"argument 1: expected Configuration.");
      return 0;
   }
   
   if (ReadConfigFile(GetSelf(Self),Name,false) == false)
     return HandleErrors();
       
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}
char *doc_LoadConfigISC = "LoadConfigISC(Configuration,FileName) -> None";
PyObject *LoadConfigISC(PyObject *Self,PyObject *Args)
{
   char *Name = 0;
   if (PyArg_ParseTuple(Args,"Os",&Self,&Name) == 0)
      return 0;
   if (Configuration_Check(Self)== 0)
   {
      PyErr_SetString(PyExc_TypeError,"argument 1: expected Configuration.");
      return 0;
   }
   
   if (ReadConfigFile(GetSelf(Self),Name,true) == false)
      return HandleErrors();
       
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}
									/*}}}*/

// ParseCommandLine - Wrapper for the command line interface		/*{{{*/
// ---------------------------------------------------------------------
char *doc_ParseCommandLine =
"ParseCommandLine(Configuration,ListOfOptions,List-argv) -> List\n"
"\n"									   
"This function is like getopt except it manipulates a configuration space.\n"
"output is a list of non-option arguments (filenames, etc).\n"
"ListOfOptions is a list of tuples of the form:\n"
"  ('c',\"long-opt or None\",\"Configuration::Variable\",\"optional type\")\n"
"Where type may be one of HasArg, IntLevel, Boolean, InvBoolean,\n"
"ConfigFile, or ArbItem. The default is Boolean.";
PyObject *ParseCommandLine(PyObject *Self,PyObject *Args)
{
   PyObject *POList;
   PyObject *Pargv;
   if (PyArg_ParseTuple(Args,"OO!O!",&Self,
			&PyList_Type,&POList,&PyList_Type,&Pargv) == 0)
      return 0;
   if (Configuration_Check(Self)== 0)
   {
      PyErr_SetString(PyExc_TypeError,"argument 1: expected Configuration.");
      return 0;
   }
   
   // Convert the option list
   int Length = PySequence_Length(POList);
   CommandLine::Args *OList = new CommandLine::Args[Length+1];
   OList[Length].ShortOpt = 0;
   OList[Length].LongOpt = 0;
   
   for (int I = 0; I != Length; I++)
   {
      char *Type = 0;
      if (PyArg_ParseTuple(PySequence_GetItem(POList,I),"czs|s",
			   &OList[I].ShortOpt,&OList[I].LongOpt,
			   &OList[I].ConfName,&Type) == 0)
      {
	 delete [] OList;
	 return 0;
      }
      OList[I].Flags = 0;
      
      // Convert the type over to flags..
      if (Type != 0)
      {
	 if (strcasecmp(Type,"HasArg") == 0)
	    OList[I].Flags = CommandLine::HasArg;
	 else if (strcasecmp(Type,"IntLevel") == 0)
	    OList[I].Flags = CommandLine::IntLevel;
	 else if (strcasecmp(Type,"Boolean") == 0)
	    OList[I].Flags = CommandLine::Boolean;
	 else if (strcasecmp(Type,"InvBoolean") == 0)
	    OList[I].Flags = CommandLine::InvBoolean;
	 else if (strcasecmp(Type,"ConfigFile") == 0)
	    OList[I].Flags = CommandLine::ConfigFile;
	 else if (strcasecmp(Type,"ArbItem") == 0)
	    OList[I].Flags = CommandLine::ArbItem;
      }     
   }

   // Convert the argument list into a char **
   const char **argv = ListToCharChar(Pargv);
   if (argv == 0)
   {
      delete [] OList;
      return 0;
   }
   
   // Do the command line processing
   PyObject *List = 0;
   {
      CommandLine CmdL(OList,&GetSelf(Self));
      if (CmdL.Parse(PySequence_Length(Pargv),argv) == false)
      {
	 delete [] argv;
	 delete [] OList;
	 return HandleErrors();
      }
      
      // Convert the file listing into a python sequence
      for (Length = 0; CmdL.FileList[Length] != 0; Length++);
      List = PyList_New(Length);   
      for (int I = 0; CmdL.FileList[I] != 0; I++)
      {
	 PyList_SetItem(List,I,PyString_FromString(CmdL.FileList[I]));
      }      
   }
   
   delete [] argv;
   delete [] OList;
   return HandleErrors(List);
}
									/*}}}*/

// Method table for the Configuration object
static PyMethodDef CnfMethods[] = 
{
   // Query
   {"Find",CnfFind,METH_VARARGS,doc_Find},
   {"FindFile",CnfFindFile,METH_VARARGS,doc_FindFile},
   {"FindDir",CnfFindDir,METH_VARARGS,doc_FindDir},
   {"FindI",CnfFindI,METH_VARARGS,doc_FindI},
   {"FindB",CnfFindB,METH_VARARGS,doc_FindB},

   // Others
   {"Set",CnfSet,METH_VARARGS,doc_Set},
   {"Exists",CnfExists,METH_VARARGS,doc_Exists},
   {"SubTree",CnfSubTree,METH_VARARGS,doc_SubTree},
   {"List",CnfList,METH_VARARGS,doc_List},
   {"ValueList",CnfValueList,METH_VARARGS,doc_ValueList},
   {"MyTag",CnfMyTag,METH_VARARGS,doc_MyTag},
   {"Clear",CnfClear,METH_VARARGS,doc_Clear},
   
   // Python Special
   {"keys",CnfKeys,METH_VARARGS,doc_Keys},
   {"has_key",CnfExists,METH_VARARGS,doc_Exists},
   {"get",CnfFind,METH_VARARGS,doc_Find},
   {}
};

// CnfGetAttr - Get an attribute - variable/method			/*{{{*/
// ---------------------------------------------------------------------
/* */
static PyObject *CnfGetAttr(PyObject *Self,char *Name)
{
   return Py_FindMethod(CnfMethods,Self,Name);
}

// Type for a Normal Configuration object
static PyMappingMethods ConfigurationMap = {0,CnfMap,CnfMapSet};
PyTypeObject ConfigurationType = 
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "Configuration",                     // tp_name
   sizeof(CppPyObject<Configuration>),  // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CppDealloc<Configuration>,           // tp_dealloc
   0,                                   // tp_print
   CnfGetAttr,                          // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   &ConfigurationMap,                   // tp_as_mapping
   0,                                   // tp_hash
};
   
PyTypeObject ConfigurationPtrType = 
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "ConfigurationPtr",                  // tp_name
   sizeof(CppPyObject<Configuration *>),  // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   (destructor)PyObject_Free,              // tp_dealloc
   0,                                   // tp_print
   CnfGetAttr,                          // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   &ConfigurationMap,                   // tp_as_mapping
   0,                                   // tp_hash
};
   
PyTypeObject ConfigurationSubType =
{
   PyObject_HEAD_INIT(&PyType_Type)
   0,			                // ob_size
   "ConfigurationSub",                  // tp_name
   sizeof(SubConfiguration),            // tp_basicsize
   0,                                   // tp_itemsize
   // Methods
   CnfSubFree,		                // tp_dealloc
   0,                                   // tp_print
   CnfGetAttr,                          // tp_getattr
   0,                                   // tp_setattr
   0,                                   // tp_compare
   0,                                   // tp_repr
   0,                                   // tp_as_number
   0,                                   // tp_as_sequence
   &ConfigurationMap,                   // tp_as_mapping
   0,                                   // tp_hash
};
   
