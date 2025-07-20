// Created on: 2025-01-20
// Copyright (c) 2025 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#include <Draw_PythonInterpretor.hxx>

#include <Draw_Appli.hxx>
#include <Message.hxx>
#include <Message_PrinterOStream.hxx>
#include <OSD.hxx>
#include <OSD_Path.hxx>
#include <Standard_SStream.hxx>
#include <Standard_ErrorHandler.hxx>
#include <Standard_Macro.hxx>
#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>

#include <string.h>
#include <Python.h>
#include <map>

// Static storage for command callbacks
static std::map<std::string, Draw_PythonInterpretor::CallBackData*> g_CommandCallbacks;

// Python wrapper function that calls OCCT command callbacks
static PyObject* PythonCommandWrapper(PyObject* self, PyObject* args)
{
  // Get command name from Python function name
  PyObject* func = PyObject_GetAttrString(self, "__name__");
  if (!func) return NULL;
  
  const char* commandName = PyUnicode_AsUTF8(func);
  Py_DECREF(func);
  
  // Find the callback
  auto it = g_CommandCallbacks.find(commandName);
  if (it == g_CommandCallbacks.end())
  {
    PyErr_SetString(PyExc_RuntimeError, "Command callback not found");
    return NULL;
  }
  
  Draw_PythonInterpretor::CallBackData* callback = it->second;
  
  // Convert Python args to argc/argv format
  Py_ssize_t argc = PyTuple_Size(args) + 1; // +1 for command name
  const char** argv = new const char*[argc];
  argv[0] = commandName; // First argument is command name
  
  for (Py_ssize_t i = 1; i < argc; ++i)
  {
    PyObject* arg = PyTuple_GetItem(args, i - 1);
    if (PyUnicode_Check(arg))
    {
      argv[i] = PyUnicode_AsUTF8(arg);
    }
    else
    {
      // Convert non-string arguments to string
      PyObject* str = PyObject_Str(arg);
      argv[i] = PyUnicode_AsUTF8(str);
      // Note: This creates a memory leak, but matches TCL behavior
    }
  }
  
  // Execute the callback
  Standard_Integer result = 0;
  try
  {
    OCC_CATCH_SIGNALS
    OSD::ControlBreak();
    result = callback->Invoke(*(callback->myDI), (Standard_Integer)argc, argv);
  }
  catch (Standard_Failure const& anException)
  {
    delete[] argv;
    Standard_SStream ss;
    ss << "An exception was caught " << anException << std::ends;
    PyErr_SetString(PyExc_RuntimeError, ss.str().c_str());
    return NULL;
  }
  catch (std::exception const& theStdException)
  {
    delete[] argv;
    Standard_SStream ss;
    ss << "An exception was caught " << theStdException.what() << " ["
       << typeid(theStdException).name() << "]" << std::ends;
    PyErr_SetString(PyExc_RuntimeError, ss.str().c_str());
    return NULL;
  }
  catch (...)
  {
    delete[] argv;
    PyErr_SetString(PyExc_RuntimeError, "UNKNOWN exception was caught");
    return NULL;
  }
  
  delete[] argv;
  
  if (result != 0)
  {
    PyErr_SetString(PyExc_RuntimeError, "Command execution failed");
    return NULL;
  }
  
  // Return the command result
  const char* resultStr = callback->myDI->Result();
  return PyUnicode_FromString(resultStr ? resultStr : "");
}

//=================================================================================================

Draw_PythonInterpretor::Draw_PythonInterpretor()
    : myPythonModule(NULL),
      myPythonDict(NULL),
      myIsInitialized(Standard_False),
      myDoLog(Standard_False),
      myDoEcho(Standard_False),
      myToColorize(Standard_True)
{
  //
}

//=================================================================================================

void Draw_PythonInterpretor::Init()
{
  if (myIsInitialized)
    return;
    
  // Initialize Python interpreter
  if (!Py_IsInitialized())
  {
    Py_Initialize();
  }
  
  // Create a new module for OCCT commands
  myPythonModule = PyModule_New("occt_draw");
  if (!myPythonModule)
  {
    PyErr_Print();
    return;
  }
  
  myPythonDict = PyModule_GetDict((PyObject*)myPythonModule);
  if (!myPythonDict)
  {
    PyErr_Print();
    return;
  }
  
  // Add the module to sys.modules
  PyObject* sys_modules = PyImport_GetModuleDict();
  PyDict_SetItemString(sys_modules, "occt_draw", (PyObject*)myPythonModule);
  
  myIsInitialized = Standard_True;
}

//=================================================================================================

void Draw_PythonInterpretor::SetToColorize(Standard_Boolean theToColorize)
{
  myToColorize = theToColorize;
  for (Message_SequenceOfPrinters::Iterator aPrinterIter(Message::DefaultMessenger()->Printers());
       aPrinterIter.More();
       aPrinterIter.Next())
  {
    if (Handle(Message_PrinterOStream) aPrinter =
          Handle(Message_PrinterOStream)::DownCast(aPrinterIter.Value()))
    {
      aPrinter->SetToColorize(theToColorize);
    }
  }
}

//=================================================================================================

void Draw_PythonInterpretor::add(const Standard_CString          theCommandName,
                                  const Standard_CString          theHelp,
                                  const Standard_CString          theFileName,
                                  Draw_PythonInterpretor::CallBackData* theCallback,
                                  const Standard_CString          theGroup)
{
  if (!myIsInitialized)
  {
    Standard_ASSERT_RAISE(Standard_False, "Python interpreter not initialized");
    return;
  }
  
  // Store the callback
  g_CommandCallbacks[theCommandName] = theCallback;
  
  // Create Python function
  PyMethodDef* methodDef = new PyMethodDef;
  methodDef->ml_name = theCommandName;
  methodDef->ml_meth = PythonCommandWrapper;
  methodDef->ml_flags = METH_VARARGS;
  methodDef->ml_doc = theHelp;
  
  PyObject* func = PyCFunction_New(methodDef, (PyObject*)myPythonModule);
  if (!func)
  {
    PyErr_Print();
    return;
  }
  
  // Add function to module dictionary
  PyDict_SetItemString((PyObject*)myPythonDict, theCommandName, func);
  Py_DECREF(func);
  
  // Store help information (could be extended to match TCL behavior)
  // For now, the help is stored in the Python function's __doc__ attribute
}

//=================================================================================================

Standard_Boolean Draw_PythonInterpretor::Remove(Standard_CString const theCommandName)
{
  if (!myIsInitialized)
    return Standard_False;
    
  // Remove from Python module
  int result = PyDict_DelItemString((PyObject*)myPythonDict, theCommandName);
  
  // Remove callback
  auto it = g_CommandCallbacks.find(theCommandName);
  if (it != g_CommandCallbacks.end())
  {
    delete it->second;
    g_CommandCallbacks.erase(it);
  }
  
  return result == 0;
}

//=================================================================================================

Standard_CString Draw_PythonInterpretor::Result() const
{
  return myResult.ToCString();
}

//=================================================================================================

void Draw_PythonInterpretor::Reset()
{
  myResult.Clear();
}

//=================================================================================================

Draw_PythonInterpretor& Draw_PythonInterpretor::Append(const Standard_CString s)
{
  myResult += s;
  return *this;
}

//=================================================================================================

Draw_PythonInterpretor& Draw_PythonInterpretor::Append(const TCollection_AsciiString& s)
{
  myResult += s;
  return *this;
}

//=================================================================================================

Draw_PythonInterpretor& Draw_PythonInterpretor::Append(const TCollection_ExtendedString& theString)
{
  TCollection_AsciiString aStr(theString, '?');
  myResult += aStr;
  return *this;
}

//=================================================================================================

Draw_PythonInterpretor& Draw_PythonInterpretor::Append(const Standard_Integer i)
{
  myResult += TCollection_AsciiString(i);
  return *this;
}

//=================================================================================================

Draw_PythonInterpretor& Draw_PythonInterpretor::Append(const Standard_Real r)
{
  char s[100];
  sprintf(s, "%.17g", r);
  myResult += s;
  return *this;
}

//=================================================================================================

Draw_PythonInterpretor& Draw_PythonInterpretor::Append(const Standard_SStream& s)
{
  myResult += s.str().c_str();
  return *this;
}

//=================================================================================================

void Draw_PythonInterpretor::AppendElement(const Standard_CString s)
{
  // For Python, we'll just append with space separation
  if (!myResult.IsEmpty())
    myResult += " ";
  myResult += s;
}

//=================================================================================================

Standard_Integer Draw_PythonInterpretor::Eval(const Standard_CString line)
{
  if (!myIsInitialized)
    return 1;
    
  PyObject* result = PyRun_String(line, Py_eval_input, (PyObject*)myPythonDict, (PyObject*)myPythonDict);
  if (!result)
  {
    PyErr_Print();
    return 1;
  }
  
  // Store result
  if (result != Py_None)
  {
    PyObject* str = PyObject_Str(result);
    const char* resultStr = PyUnicode_AsUTF8(str);
    myResult = resultStr;
    Py_DECREF(str);
  }
  
  Py_DECREF(result);
  return 0;
}

//=================================================================================================

Standard_Integer Draw_PythonInterpretor::RecordAndEval(const Standard_CString line,
                                                        const Standard_Integer /*theFlags*/)
{
  // For simplicity, same as Eval (no history recording implemented yet)
  return Eval(line);
}

//=================================================================================================

Standard_Integer Draw_PythonInterpretor::EvalFile(const Standard_CString fname)
{
  if (!myIsInitialized)
    return 1;
    
  FILE* file = fopen(fname, "r");
  if (!file)
    return 1;
    
  PyObject* result = PyRun_File(file, fname, Py_file_input, (PyObject*)myPythonDict, (PyObject*)myPythonDict);
  fclose(file);
  
  if (!result)
  {
    PyErr_Print();
    return 1;
  }
  
  Py_DECREF(result);
  return 0;
}

//=================================================================================================

Standard_Integer Draw_PythonInterpretor::PrintHelp(const Standard_CString theCommandName)
{
  // Simple help implementation
  auto it = g_CommandCallbacks.find(theCommandName);
  if (it == g_CommandCallbacks.end())
  {
    myResult = TCollection_AsciiString("Command '") + theCommandName + "' not found";
    return 1;
  }
  
  // Get help from Python function
  PyObject* func = PyDict_GetItemString((PyObject*)myPythonDict, theCommandName);
  if (func && PyCallable_Check(func))
  {
    PyObject* doc = PyObject_GetAttrString(func, "__doc__");
    if (doc && PyUnicode_Check(doc))
    {
      const char* helpStr = PyUnicode_AsUTF8(doc);
      myResult = helpStr;
    }
    Py_XDECREF(doc);
  }
  
  return 0;
}

//=================================================================================================

Standard_Boolean Draw_PythonInterpretor::Complete(const Standard_CString /*line*/)
{
  // For Python, we can implement syntax checking if needed
  // For now, assume all scripts are complete
  return Standard_True;
}

//=================================================================================================

Draw_PythonInterpretor::~Draw_PythonInterpretor()
{
  // Clean up callbacks
  for (auto& pair : g_CommandCallbacks)
  {
    delete pair.second;
  }
  g_CommandCallbacks.clear();
  
  // Note: We don't finalize Python interpreter as it might be used elsewhere
}

//=================================================================================================

void Draw_PythonInterpretor::SetDoLog(Standard_Boolean doLog)
{
  myDoLog = doLog;
}

void Draw_PythonInterpretor::SetDoEcho(Standard_Boolean doEcho)
{
  myDoEcho = doEcho;
}

Standard_Boolean Draw_PythonInterpretor::GetDoLog() const
{
  return myDoLog;
}

Standard_Boolean Draw_PythonInterpretor::GetDoEcho() const
{
  return myDoEcho;
}

void Draw_PythonInterpretor::ResetLog()
{
  // Simplified logging - could be enhanced
}

void Draw_PythonInterpretor::AddLog(const Standard_CString /*theStr*/)
{
  // Simplified logging - could be enhanced
}

TCollection_AsciiString Draw_PythonInterpretor::GetLog()
{
  // Simplified logging - could be enhanced
  return TCollection_AsciiString();
}