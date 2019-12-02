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

// The functions defined here implement the SV Python API path module calculation method class. 
// The class member data provides string constants representing each of the calculation methods. 
//
// The class name is 'CalculationMethod'. It is referenced from the path module as 'path.CalculationMethod'.
//
#ifndef SV3_PATH_CALC_METHOD_PYMODULE_H
#define SV3_PATH_CALC_METHOD_PYMODULE_H

#include <iostream>
#include <string>
#include <structmember.h>
#include "sv3_PathGroup.h"

// Define a map between method name and enum type.
//
static std::map<std::string, sv3::PathElement::CalculationMethod> calcMethodNameTypeMap =
{
    {"SPACING", sv3::PathElement::CONSTANT_SPACING},
    {"SUBDIVISION", sv3::PathElement::CONSTANT_SUBDIVISION_NUMBER},
    {"TOTAL", sv3::PathElement::CONSTANT_TOTAL_NUMBER}
};

// Define the valid calculation methods, used in error messages.
static std::string calcMethodValidNames = "SPACING, SUBDIVISION or TOTAL";

//-----------------------
// PyPathCalcMethodClass 
//-----------------------
// Define the PyPathCalcMethodClass class (type).
//
typedef struct {
PyObject_HEAD
} PyPathCalcMethodClass;

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//--------------------------
// PathCalcMethod_get_names
//--------------------------
// 
PyDoc_STRVAR(PathCalcMethod_get_names_doc,
  "get_names() \n\ 
   \n\
   Get the calculation method names. \n\
   \n\
   Args: \n\
     None \n\
");

static PyObject *
PathCalcMethod_get_names()
{
  PyObject* nameList = PyList_New(calcMethodNameTypeMap.size());
  int n = 0;
  for (auto const& entry : calcMethodNameTypeMap) {
      auto name = entry.first.c_str();
      PyList_SetItem(nameList, n, PyUnicode_FromString(name));
      n += 1;
  }
  return nameList; 
}

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_PATH_CALC_METHOD_CLASS = "CalculationMethod";

PyDoc_STRVAR(pathcalcmethod_doc, "path calculate method functions");


static PyMethodDef PyPathCalcMethodMethods[] = {
  { "get_names", (PyCFunction)PathCalcMethod_get_names, METH_NOARGS, PathCalcMethod_get_names_doc},
  {NULL, NULL}
};

//------------------------------
// Define the PathCalMethodType
//------------------------------
// Define the Python type object that stores path.CalculationMethod types. 
//
static PyTypeObject PyPathCalcMethodType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = "path.CalculationMethod",
  .tp_basicsize = sizeof(PyPathCalcMethodClass)
};

//-----------------------------
// SetPathCalcMethodTypeFields 
//-----------------------------
//
static void
SetPathCalcMethodTypeFields(PyTypeObject& methodType)
 {
  methodType.tp_doc = "Path calculation method types.";
  methodType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  methodType.tp_methods = PyPathCalcMethodMethods;
  methodType.tp_dict = PyDict_New();
};

//------------------------
// SetPathCalcMethodTypes 
//------------------------
// Set the calculate method names in the PyPathCalcMethodType dictionary.
//
static void
SetPathCalcMethodTypes(PyTypeObject& methodType)
{
  for (auto const& entry : calcMethodNameTypeMap) {
      auto name = entry.first.c_str();
      if (PyDict_SetItemString(methodType.tp_dict, name, PyUnicode_FromString(name))) {
          std::cout << "Error initializing Python API path calculation method types." << std::endl;
          return;
      }
  }

  PyObject* nameList = PyList_New(calcMethodNameTypeMap.size());
  int n = 0;
  for (auto const& entry : calcMethodNameTypeMap) {
      auto name = entry.first.c_str();
      PyList_SetItem(nameList, n, PyUnicode_FromString(name));
      n += 1;
  }

  if (PyDict_SetItemString(methodType.tp_dict, "names", nameList)) {
      std::cout << "Error initializing Python API path calculation method types." << std::endl;
      return;
  }

};

#endif


