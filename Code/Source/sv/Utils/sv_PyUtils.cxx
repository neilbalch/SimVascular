/* Copyright (c) Stanford University, The Regents of the University of
 *               California, and others.
 *
 * All Rights Reserved.
 *
 * See Copyright-SimVascular.txt for additional details.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sv_PyUtils.h"
#include <string>

SvPyUtilApiFunction::SvPyUtilApiFunction(const std::string& format, PyObject* pyError, const char* function)
{
  std::string functionName = svPyUtilGetFunctionName(function);
  this->msgp = svPyUtilGetMsgPrefix(functionName);
  this->formatString = format + ":" + functionName;
  this->format = this->formatString.c_str();
  this->pyError = pyError;
}

//--------
// error
//--------
// Set the Python exception description.
//
void SvPyUtilApiFunction::error(std::string msg)
{
  auto emsg = this->msgp + msg;
  PyErr_SetString(this->pyError, emsg.c_str());
}

//-----------
// argsError
//-----------
// Set the Python exception for arguments errors.
//
// This function has a pointer return type just so 
// a nullptr can be returned.
//
PyObject * 
SvPyUtilApiFunction::argsError()
{
  return svPyUtilResetException(this->pyError);
}

//--------------------------
// SvPyUtilGetFunctionName
//--------------------------
// Get the function name used to display error messages for the Python API.
//
// Module functions are prefixed with '<MODULE_NAME>_' so replaced the '_'
// with a '.' to make the name look at it would if referenced from Python.
//
std::string svPyUtilGetFunctionName(const char* functionName)
{
    std::string name(functionName);
    std::size_t pos = name.find("_");
    if (pos == std::string::npos) {
        return name;
    }
    return name.replace(pos, 1, ".");
}

//-----------------------
// svPyUtilGetMsgPrefix 
//-----------------------
// Get the string used to prefix an error message for the Python API. 
//
// When an error occurs in the API PyErr_SetString() is called with an error message and 
// the enclosing function returns NULL. The Python API does not automatically print the 
// function name where an exception occurs so add it to the string passed to PyErr_SetString()
// using the prefix created here.
//
std::string svPyUtilGetMsgPrefix(const std::string& functionName)
{
    std::string msgp = functionName + "() "; 
    return msgp;
}

//-------------------------
// svPyUtilResetException
//-------------------------
// Take a Python C API exception and resuse it for the given pyRunTimeErr type.
//
// This is used to take the value of the exceptions generated by PyArg_ParseTuple() 
// (e.g. improper argument types) and use them in SV custom module exception.
//
PyObject * svPyUtilResetException(PyObject * pyRunTimeErr)
{
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    PyErr_Restore(pyRunTimeErr, value, traceback);
    return nullptr;
}

//-------------------------
// svPyUtilCheckPointData
//-------------------------
// Check Python point data.
//
// The point data is a list [x,y,z] of three floats.
//
// If there is a problem with the data then the function returns false and
// a string describing the problem.
//
bool svPyUtilCheckPointData(PyObject *pointData, std::string& msg)
{
  if (!PyList_Check(pointData)) {
      msg = "is not a Python list.";
      return false;
  }

  if (PyList_Size(pointData) != 3) {
    msg = "is not a 3D point (three float values).";
    return false;
  }

  for (int i = 0; i < 3; i++) {
    if (!PyFloat_Check(PyList_GetItem(pointData,i))) {
      msg = "data at " + std::to_string(i) + " in the list is not a float.";
      return false;
      }
  }

  return true;
}

//--------------------------
// svPyUtilConvertPointData
//--------------------------
// Convert a Python object to a double and store it into the given
// position in an array.
//
bool svPyUtilConvertPointData(PyObject* data, int index, std::string& msg, double point[3])
{
  if (!PyFloat_Check(data) && !PyLong_Check(data)) {
      msg = "data at " + std::to_string(index) + " in the list is not a float.";
      return false;
  }
  point[index] = PyFloat_AsDouble(data);
  return true;
}

bool svPyUtilConvertPointData(PyObject* data, int index, std::string& msg, int point[3])
{
  if (!PyLong_Check(data)) {
      msg = "data at " + std::to_string(index) + " in the list is not an integer.";
      return false;
  }
  point[index] = PyLong_AsLong(data);
  return true;
}

//----------------------
// svPyUtilGetPointData 
//----------------------
// Get an array of three float or int valuess. 
//
// The data is a list [x,y,z] of three values.
//
// If there is a problem with the data then the function returns false and
// a string describing the problem.
//
template <typename T>
bool svPyUtilGetPointData(PyObject* pyPoint, std::string& msg, T point[3])
{
  if (!PyList_Check(pyPoint)) {
      msg = "is not a Python list.";
      return false;
  }

  if (PyList_Size(pyPoint) != 3) {
      msg = "is not a 3D point (three float values).";
      return false;
  }

  for (int i = 0; i < 3; i++) {
      auto data = PyList_GetItem(pyPoint,i);
      if (!svPyUtilConvertPointData(data, i, msg, point)) {
          return false;
      }
  }

  return true;
}

// Needed for linking.
template bool svPyUtilGetPointData(PyObject* pyPoint, std::string& msg, double point[3]);
template bool svPyUtilGetPointData(PyObject* pyPoint, std::string& msg, int point[3]);

//-----------------------------
// svPyUtilCheckPointDataList
//-----------------------------
// Check a Python list of point data.
//
// The point data is a list of [x,y,z] (three floats).
//
// If there is a problem with the data then the function returns false and
// a string describing the problem.
//
bool svPyUtilCheckPointDataList(PyObject *pointData, std::string& msg)
{
  if (!PyList_Check(pointData)) {
      msg = "is not a Python list.";
      return false;
  }

  int numPts = PyList_Size(pointData);
  for (int i = 0; i < numPts; i++) {
      PyObject* pt = PyList_GetItem(pointData,i);
      if ((PyList_Size(pt) != 3) || !PyList_Check(pt)) {
          msg = "data at " + std::to_string(i) + " in the list is not a 3D point (three float values).";
          return false;
      }
      for (int j = 0; j < 3; j++) {
          if (!PyFloat_Check(PyList_GetItem(pt,j))) {
              msg = "data at " + std::to_string(i) + " in the list is not a 3D point (three float values).";
              return false;
         }
      }
  }

  return true;
}

//----------------------------
// svPyUtilSetupApiFunction
//----------------------------
// Setup an API function format and message prefix strings.
//
void svPyUtilSetupApiFunction(const char* function, std::string& format, std::string& msg)
{
  std::string functionName = svPyUtilGetFunctionName(function);
  msg = svPyUtilGetMsgPrefix(functionName);
  format = format + ":" + functionName;
}

//----------------------
// svPyUtilSetErrorMsg
//----------------------
// Set the Python API exception message.
//
void svPyUtilSetErrorMsg(PyObject* pyRunTimeErr, std::string& msgp, std::string msg)
{
    auto emsg = msgp + msg;
    PyErr_SetString(pyRunTimeErr, emsg.c_str());
}

