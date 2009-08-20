// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: generic.cc,v 1.1.1.1 2001/02/20 06:32:01 jgg Exp $
/* ######################################################################

   generic - Some handy functions to make integration a tad simpler
   
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"

#include <apt-pkg/error.h>
									/*}}}*/

// HandleErrors - This moves errors from _error to Python Exceptions	/*{{{*/
// ---------------------------------------------------------------------
/* We throw away all warnings and only propogate the first error. */
PyObject *HandleErrors(PyObject *Res)
{
   if (_error->PendingError() == false)
   {
      // Throw away warnings
      _error->Discard();
      return Res;
   }
   
   if (Res != 0)
      Py_DECREF(Res);
   
   string Err;
   int errcnt = 0;
   while (_error->empty() == false)
   {
      string Msg;
      bool Type = _error->PopMessage(Msg);
      if (errcnt > 0)
         Err.append(", ");
      Err.append((Type == true ? "E:" : "W:"));
      Err.append(Msg);
      ++errcnt;
   }
   if (errcnt == 0)
      Err = "Internal Error";
   PyErr_SetString(PyExc_SystemError,Err.c_str());
   return 0;
}
									/*}}}*/
// ListToCharChar - Convert a list to an array of char char		/*{{{*/
// ---------------------------------------------------------------------
/* Caller must free the result. 0 on error. */
const char **ListToCharChar(PyObject *List,bool NullTerm)
{
   // Convert the argument list into a char **
   int Length = PySequence_Length(List);
   const char **Res = new const char *[Length + (NullTerm == true?1:0)];
   for (int I = 0; I != Length; I++)
   { 
      PyObject *Itm = PySequence_GetItem(List,I);
      if (PyString_Check(Itm) == 0)
      {
	 PyErr_SetNone(PyExc_TypeError);
	 delete [] Res;
	 return 0;
      }
      Res[I] = PyString_AsString(Itm);
   }
   if (NullTerm == true)
      Res[Length] = 0;
   return Res;
}
									/*}}}*/
// CharCharToList - Inverse of the above				/*{{{*/
// ---------------------------------------------------------------------
/* Zero size indicates the list is Null terminated. */
PyObject *CharCharToList(const char **List,unsigned long Size)
{
   if (Size == 0)
   {
      for (const char **I = List; *I != 0; I++)
	 Size++;
   }
   
   // Convert the whole configuration space into a list
   PyObject *PList = PyList_New(Size);
   for (unsigned long I = 0; I != Size; I++, List++)
      PyList_SetItem(PList,I,PyString_FromString(*List));
   
   return PList;
}
									/*}}}*/
