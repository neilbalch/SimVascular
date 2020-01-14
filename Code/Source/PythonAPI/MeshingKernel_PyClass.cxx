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

// Define the Python 'meshing.Kernel' class that encapsulates meshing kernel types. 
//
#ifndef PYAPI_MESHING_KERNEL_H
#define PYAPI_MESHING_KERNEL_H

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

// Define a map between mesh kernel name and enum type.
//
static std::map<std::string,cvMeshObject::KernelType> kernelNameEnumMap =
{
    {"GMSH",     cvMeshObject::KERNEL_GMSH},
    {"INVALID",  cvMeshObject::KERNEL_INVALID},
    {"MESHSIM",  cvMeshObject::KERNEL_MESHSIM},
    {"TETGEN",   cvMeshObject::KERNEL_TETGEN}
};

// The list of valid kernel names, used in error messages.
static std::string kernelValidNames = "GMSH, MESHSIM, or TETGEN"; 

//--------------------
// MeshingKernelClass
//--------------------
// Define the MeshingKernelClass.
//
typedef struct {
PyObject_HEAD
} MeshingKernelClass;

std::string 
MeshingKernel_get_name(cvMeshObject::KernelType kernelType)
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
MeshingKernel_get_names()
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

//-------------------
// MeshingKernelMethods
//-------------------
//
static PyMethodDef MeshingKernelMethods[] = {
  { "get_names", (PyCFunction)MeshingKernel_get_names, METH_NOARGS, NULL},
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MESHING_KERNEL_CLASS = "Kernel";
static char* MESHING_KERNEL_MODULE_CLASS = "mesh.Kernel";
// The name of the Kernel class veriable that contains all of the kernel types.
static char* MESHING_KERNEL_CLASS_VARIBLE_NAMES = "names";

PyDoc_STRVAR(MeshingKernelClass_doc, "mesh kernel class functions");

//------------------------------------
// Define the MeshingType type object
//------------------------------------
// Define the Python type object that stores contour.kernel types. 
//
static PyTypeObject PyMeshingKernelClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = MESHING_KERNEL_MODULE_CLASS,
  .tp_basicsize = sizeof(MeshingKernelClass)
};

//----------------------------
// SetMeshingKernelClassTypeFields
//----------------------------
// Set the Python type object fields that stores Kernel data. 
//
static void
SetMeshingKernelTypeFields(PyTypeObject& contourType)
 {
  contourType.tp_doc = MeshingKernelClass_doc; 
  contourType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  contourType.tp_methods = MeshingKernelMethods;
  contourType.tp_dict = PyDict_New();
};

//-----------------------
// SetMeshingKernelTypes
//-----------------------
// Set the kernel names in the MeshingKernelType dictionary.
//
// The names in the MeshingKernelType dictionary are 
// referenced as a string class variable for the Python 
// Kernel class.
//
static void
SetMeshingKernelClassTypes(PyTypeObject& meshType)
{
  // Add kernel types to MeshingKernelType dictionary.
  for (auto const& entry : kernelNameEnumMap) {
      auto name = entry.first.c_str();
      if (PyDict_SetItemString(meshType.tp_dict, name, PyUnicode_FromString(name))) {
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

  if (PyDict_SetItemString(meshType.tp_dict, MESHING_KERNEL_CLASS_VARIBLE_NAMES, nameList)) {
      std::cout << "Error initializing Python API solid kernel types." << std::endl;
      return;
  }

};

#endif


