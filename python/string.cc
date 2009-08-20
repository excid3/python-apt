// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: string.cc,v 1.3 2002/01/08 06:53:04 jgg Exp $
/* ######################################################################

   string - Mappings for the string functions that are worthwile for 
            Python users
      
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "apt_pkgmodule.h"
#include "generic.h"
    
#include <apt-pkg/strutl.h>

#include <Python.h>
									/*}}}*/
    
// Templated function							/*{{{*/
/* Macro for the generic string in string out function */    
#define MkStr(Python,CFunc) \
PyObject *Python(PyObject *Self,PyObject *Args) \
{ \
   char *Str = 0; \
   if (PyArg_ParseTuple(Args,"s",&Str) == 0) \
      return 0; \
   return CppPyString(CFunc(Str)); \
}

#define MkInt(Python,CFunc) \
PyObject *Python(PyObject *Self,PyObject *Args) \
{ \
   int Val = 0; \
   if (PyArg_ParseTuple(Args,"i",&Val) == 0) \
      return 0; \
   return CppPyString(CFunc(Val)); \
}
    
MkStr(StrDeQuote,DeQuoteString);
MkStr(StrBase64Encode,Base64Encode);
MkStr(StrURItoFileName,URItoFileName);

//MkFloat(StrSizeToStr,SizeToStr);
MkInt(StrTimeToStr,TimeToStr);
MkInt(StrTimeRFC1123,TimeRFC1123);
									/*}}}*/

// Other String functions						/*{{{*/
PyObject *StrSizeToStr(PyObject *Self,PyObject *Args)
{
   PyObject *Obj;
   if (PyArg_ParseTuple(Args,"O",&Obj) == 0)
      return 0;
   if (PyInt_Check(Obj))
      return CppPyString(SizeToStr(PyInt_AsLong(Obj)));
   if (PyLong_Check(Obj))
      return CppPyString(SizeToStr(PyLong_AsDouble(Obj)));
   if (PyFloat_Check(Obj))
      return CppPyString(SizeToStr(PyFloat_AsDouble(Obj)));
   
   PyErr_SetString(PyExc_TypeError,"Only understand integers and floats");
   return 0;
}

PyObject *StrQuoteString(PyObject *Self,PyObject *Args)
{
   char *Str = 0;
   char *Bad = 0;
   if (PyArg_ParseTuple(Args,"ss",&Str,&Bad) == 0)
      return 0;
   return CppPyString(QuoteString(Str,Bad)); 
}

PyObject *StrStringToBool(PyObject *Self,PyObject *Args)
{
   char *Str = 0;
   if (PyArg_ParseTuple(Args,"s",&Str) == 0)
      return 0;   
   return Py_BuildValue("i",StringToBool(Str));
}

PyObject *StrStrToTime(PyObject *Self,PyObject *Args)
{
   char *Str = 0;
   if (PyArg_ParseTuple(Args,"s",&Str) == 0)
      return 0;
   
   time_t Result;
   if (StrToTime(Str,Result) == false)
   {
      Py_INCREF(Py_None);
      return Py_None;
   }
   
   return Py_BuildValue("i",Result);
}

PyObject *StrCheckDomainList(PyObject *Self,PyObject *Args)
{
   char *Host = 0;
   char *List = 0;
   if (PyArg_ParseTuple(Args,"ss",&Host,&List) == 0)
      return 0;
   return Py_BuildValue("i",CheckDomainList(Host,List));
}

									/*}}}*/
