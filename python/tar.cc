// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: tar.cc,v 1.4 2004/12/12 17:55:54 mdz Exp $
/* ######################################################################

   Tar Inteface
   
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "generic.h"

#include <apt-pkg/extracttar.h>
#include <apt-pkg/error.h>
#include <apt-pkg/debfile.h>
#include "apt_instmodule.h"

#include <Python.h>
									/*}}}*/

class ProcessTar : public pkgDirStream
{
   public:

   PyObject *Function;
      
   virtual bool DoItem(Item &Itm,int &Fd);
   
   ProcessTar(PyObject *Function) : Function(Function)
   {
      Py_INCREF(Function);
   }
   virtual ~ProcessTar()
   {
      Py_DECREF(Function);
   }
};

// ProcessTar::DoItem - Feed an item to a python function		/*{{{*/
// ---------------------------------------------------------------------
/* The function is called with a tuple that has:
   (FileName,Link,Mode,UID,GID,Size,MTime,Major,Minor) */
bool ProcessTar::DoItem(Item &Itm,int &Fd)
{
   const char *Type;
   switch (Itm.Type)
   {
      case Item::File:
      Type = "FILE";
      break;
      
      case Item::HardLink:
      Type = "HARDLINK";
      break;
      
      case Item::SymbolicLink:
      Type = "SYMLINK";
      break;
      
      case Item::CharDevice:
      Type = "CHARDEV";
      break;
      
      case Item::BlockDevice:
      Type = "BLKDEV";
      break;
      
      case Item::Directory:
      Type = "DIR";
      break;
      
      case Item::FIFO:
      Type = "FIFO";
      break;
   }
  
   if (PyObject_CallFunction(Function,"sssiiiiiii",Type,Itm.Name,
			     Itm.LinkTarget,Itm.Mode,Itm.UID,Itm.GID,Itm.Size,
			     Itm.MTime,Itm.Major,Itm.Minor) == 0)
      return false;
   
   Fd = -1;
   return true;
}
									/*}}}*/
		       
// tarExtract - Examine files from a tar				/*{{{*/
// ---------------------------------------------------------------------
/* */
char *doc_tarExtract =
"tarExtract(File,Func,Comp) -> None\n"
"The tar file referenced by the file object File, Func called for each\n"
"Tar member. Comp must be the string \"gzip\" (gzip is automatically invoked) \n";   
PyObject *tarExtract(PyObject *Self,PyObject *Args)
{
   PyObject *File;
   PyObject *Function;
   char *Comp;
   
   if (PyArg_ParseTuple(Args,"O!Os",&PyFile_Type,&File,
			&Function,&Comp) == 0)
      return 0;

   if (PyCallable_Check(Function) == 0)
   {
      PyErr_SetString(PyExc_TypeError,"argument 2: expected something callable.");
      return 0;
   }
   
   {
      // Open the file and associate the tar
      FileFd Fd(fileno(PyFile_AsFile(File)),false);
      ExtractTar Tar(Fd,0xFFFFFFFF,Comp);
      if (_error->PendingError() == true)
	 return HandleErrors();
      
      ProcessTar Proc(Function);
      if (Tar.Go(Proc) == false)
	 return HandleErrors();
   }  
   
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}
									/*}}}*/

// debExtract - Examine files from a deb				/*{{{*/
// ---------------------------------------------------------------------
/* */
char *doc_debExtract =
"debExtract(File,Func,Chunk) -> None\n"
"The deb referenced by the file object File is examined. The AR member\n"
"given by Chunk is treated as a tar.gz and fed through Func like\n"
"tarExtract\n";
PyObject *debExtract(PyObject *Self,PyObject *Args)
{
   PyObject *File;
   PyObject *Function;
   char *Chunk;
   const char *Comp = "gzip";
   
   if (PyArg_ParseTuple(Args,"O!Os",&PyFile_Type,&File,
			&Function,&Chunk) == 0)
      return 0;
   
   if (PyCallable_Check(Function) == 0)
   {
      PyErr_SetString(PyExc_TypeError,"argument 2: expected something callable.");
      return 0;
   }

   {
      // Open the file and associate the tar
      // Open the file and associate the .deb
      FileFd Fd(fileno(PyFile_AsFile(File)),false);
      debDebFile Deb(Fd);
      if (_error->PendingError() == true)
	 return HandleErrors();
      
      // Get the archive member and positition the file 
      const ARArchive::Member *Member = Deb.GotoMember(Chunk);
      if (Member == 0)
      {
	 _error->Error("Cannot find chunk %s",Chunk);
	 return HandleErrors();
      }
      
      // Extract it.
      if (strcmp(".bz2", &Chunk[strlen(Chunk)-4]) == 0)
	      Comp = "bzip2";
      else if(strcmp(".lzma", &Chunk[strlen(Chunk)-5]) == 0)
	      Comp = "lzma";
      ExtractTar Tar(Deb.GetFile(),Member->Size,Comp);
      ProcessTar Proc(Function);
      if (Tar.Go(Proc) == false)
	 return HandleErrors();
   }  
   
   Py_INCREF(Py_None);
   return HandleErrors(Py_None);
}
									/*}}}*/
