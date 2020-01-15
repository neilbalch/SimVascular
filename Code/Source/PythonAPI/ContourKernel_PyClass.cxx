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

// Define the Python 'contour.Kernel' class that encapsulates contour kernel types. 
//
#ifndef SV3_CONTOUR_KERNEL_H
#define SV3_CONTOUR_KERNEL_H

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

// Define a map between contour kernel name and enum type.
//
static std::map<std::string,cKernelType> kernelNameEnumMap =
{
    {"CIRCLE", cKERNEL_CIRCLE},
    {"ELLIPSE", cKERNEL_ELLIPSE},
    {"LEVEL_SET", cKERNEL_LEVELSET},
    {"POLYGON", cKERNEL_POLYGON},
    {"SPLINE_POLYGON", cKERNEL_SPLINEPOLYGON},
    {"THRESHOLD", cKERNEL_THRESHOLD}
};

// The list of valid kernel names, used in error messages.
static std::string kernelValidNames = "CIRCLE, ELLIPSE, LEVEL_SET, POLYGON, SPLINE_POLYGON or THRESHOLD";

//---------------------
// ContourKernelObject
//---------------------
// Define the ContourKernelObject class (type).
//
typedef struct {
PyObject_HEAD
} ContourKernelObject;

std::string 
ContourKernel_get_name(cKernelType contourType)
{
  for (auto const& entry : kernelNameEnumMap) {
      if (contourType == entry.second) { 
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
ContourKernel_get_names()
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
// ContourKernelMethods
//----------------------
//
static PyMethodDef ContourKernelMethods[] = {
  { "get_names", (PyCFunction)ContourKernel_get_names, METH_NOARGS, NULL},
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* CONTOUR_KERNEL_CLASS = "Kernel";
static char* CONTOUR_KERNEL_MODULE_CLASS = "contour.Kernel";
// The name of the Kernel class veriable that contains all of the kernel types.
static char* CONTOUR_KERNEL_CLASS_VARIBLE_NAMES = "names";

PyDoc_STRVAR(ContourKernelClass_doc, "contour kernel class functions");

//------------------------------------
// Define the ContourType type object
//------------------------------------
// Define the Python type object that stores contour.kernel types. 
//
static PyTypeObject PyContourKernelClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = CONTOUR_KERNEL_MODULE_CLASS,
  .tp_basicsize = sizeof(ContourKernelObject)
};

//----------------------------
// SetContourKernelClassTypeFields
//----------------------------
// Set the Python type object fields that stores Kernel data. 
//
static void
SetContourKernelTypeFields(PyTypeObject& contourType)
 {
  contourType.tp_doc = ContourKernelClass_doc; 
  contourType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  contourType.tp_methods = ContourKernelMethods;
  contourType.tp_dict = PyDict_New();
};

//-----------------------
// SetContourKernelTypes
//-----------------------
// Set the kernel names in the ContourKernelType dictionary.
//
// The names in the ContourKernelType dictionary are referenced as a string class variable 
// for the Python Kernel class referenced like.
//
//    sv.contour.Kernel.CIRCLE -> "CIRCLE"
//
static void
SetContourKernelClassTypes(PyTypeObject& contourType)
{
  std::cout << "[SetContourKernelClassTypes] " << std::endl;
  std::cout << "[SetContourKernelClassTypes] =============== SetContourKernelClassTypes ==========" << std::endl;

  // Add kernel types to ContourKernelType dictionary.
  for (auto const& entry : kernelNameEnumMap) {
      auto name = entry.first.c_str();
      std::cout << "[SetContourKernelClassTypes] name: " << name << std::endl;
      if (PyDict_SetItemString(contourType.tp_dict, name, PyUnicode_FromString(name))) {
          std::cout << "Error initializing Python API contour kernel types." << std::endl;
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

  if (PyDict_SetItemString(contourType.tp_dict, CONTOUR_KERNEL_CLASS_VARIBLE_NAMES, nameList)) {
      std::cout << "Error initializing Python API contour kernel types." << std::endl;
      return;
  }

};

#endif


