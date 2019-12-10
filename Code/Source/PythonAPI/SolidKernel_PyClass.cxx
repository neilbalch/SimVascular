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

// Define the Python 'solid.Kernel' class that encapsulates solid modeling kernel types. 
//
#ifndef PYAPI_SOLID_KERNEL_H
#define PYAPI_SOLID_KERNEL_H

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

// Define a map between solid modeling kernel name and enum type.
//
static std::map<std::string,SolidModel_KernelT> kernelNameEnumMap =
{
    {"DISCRETE",     SM_KT_DISCRETE},
    {"INVALID",      SM_KT_INVALID},
    {"MESHSIMSOLID", SM_KT_MESHSIMSOLID},
    {"OCCT",         SM_KT_OCCT},
    {"PARASOLID",    SM_KT_PARASOLID},
    {"POLYDATA",     SM_KT_POLYDATA},
    {"RESERVED",     SM_KT_RESERVED},
};

// The list of valid kernel names, used in error messages.
static std::string kernelValidNames = "DISCRETE, MESHSIMSOLID, OCCT, PARASOLID or POLYDATA"; 

//---------------------
// SolidKernelObject
//---------------------
// Define the SolidKernelObject class (type).
//
typedef struct {
PyObject_HEAD
} SolidKernelObject;

std::string 
SolidKernel_get_name(SolidModel_KernelT kernelType)
{
  for (auto const& entry : kernelNameEnumMap) {
      if (kernelType == entry.second) { 
          return entry.first;
      }
  }
  return "";
}

////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//
// Python 'Kernel' class methods. 

static PyObject *
SolidKernel_get_names()
{
  PyObject* nameList = PyList_New(kernelNameEnumMap.size());
  int n = 0;
  for (auto const& entry : kernelNameEnumMap) {
      auto name = entry.first.c_str();
      PyList_SetItem(nameList, n, PyUnicode_FromString(name));
      n += 1;
  }
  return nameList; 
}

//----------------------
// SolidKernelMethods
//----------------------
//
static PyMethodDef SolidKernelMethods[] = {
  { "get_names", (PyCFunction)SolidKernel_get_names, METH_NOARGS, NULL},
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* SOLID_KERNEL_CLASS = "Kernel";
static char* SOLID_KERNEL_MODULE_CLASS = "solid.Kernel";
// The name of the Kernel class veriable that contains all of the kernel types.
static char* SOLID_KERNEL_CLASS_VARIBLE_NAMES = "names";

PyDoc_STRVAR(SolidKernelClass_doc, "solid modeling kernel class functions");

//------------------------------------
// Define the SolidType type object
//------------------------------------
// Define the Python type object that stores contour.kernel types. 
//
static PyTypeObject PySolidKernelClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = SOLID_KERNEL_MODULE_CLASS,
  .tp_basicsize = sizeof(SolidKernelObject)
};

//----------------------------
// SetSolidKernelClassTypeFields
//----------------------------
// Set the Python type object fields that stores Kernel data. 
//
static void
SetSolidKernelTypeFields(PyTypeObject& contourType)
 {
  contourType.tp_doc = SolidKernelClass_doc; 
  contourType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  contourType.tp_methods = SolidKernelMethods;
  contourType.tp_dict = PyDict_New();
};

//-----------------------
// SetSolidKernelTypes
//-----------------------
// Set the kernel names in the SolidKernelType dictionary.
//
// The names in the SolidKernelType dictionary are 
// referenced as a string class variable for the Python 
// Kernel class.
//
static void
SetSolidKernelClassTypes(PyTypeObject& solidType)
{
  // Add kernel types to SolidKernelType dictionary.
  for (auto const& entry : kernelNameEnumMap) {
      auto name = entry.first.c_str();
      if (PyDict_SetItemString(solidType.tp_dict, name, PyUnicode_FromString(name))) {
          std::cout << "Error initializing Python API solid kernel types." << std::endl;
          return;
      }
  }

  // Create a string list of kernel types refenced by 'names'..
  //
  PyObject* nameList = PyList_New(kernelNameEnumMap.size());
  int n = 0;
  for (auto const& entry : kernelNameEnumMap) {
      auto name = entry.first.c_str();
      PyList_SetItem(nameList, n, PyUnicode_FromString(name));
      n += 1;
  }

  if (PyDict_SetItemString(solidType.tp_dict, SOLID_KERNEL_CLASS_VARIBLE_NAMES, nameList)) {
      std::cout << "Error initializing Python API solid kernel types." << std::endl;
      return;
  }

};

#endif


