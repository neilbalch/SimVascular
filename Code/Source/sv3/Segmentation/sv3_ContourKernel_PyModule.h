
// ContourKernel type. 
//

#ifndef SV3_CONTOUR_KERNEL_H
#define SV3_CONTOUR_KERNEL_H

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

//#include "contour_module.h"

class ContourKernelNames {
    std::string CIRCLE = "CIRCLE"; 
};

// Define a map between contour kernel name and enum type.
static std::map<std::string,cKernelType> kernelNameEnumMap =
{
    {"CIRCLE", cKERNEL_CIRCLE},
    {"ELLIPSE", cKERNEL_ELLIPSE},
    {"LEVEL_SET", cKERNEL_LEVELSET},
    {"POLYGON", cKERNEL_POLYGON},
    {"SPLINE_POLYGON", cKERNEL_SPLINEPOLYGON},
    {"THRESHOLD", cKERNEL_THRESHOLD}
};

static std::string kernelValidNames = "CIRCLE, ELLIPSE, LEVEL_SET, POLYGON, SPLINE_POLYGON, or THRESHOLD";

//---------------------
// ContourKernelObject
//---------------------
// Define the ContourKernelObject class (type).
//
typedef struct {
PyObject_HEAD
} ContourKernelObject;

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

static PyObject *
ContourKernel_get_names()
//ContourKernel_get_names(ContourKernelObject* self)
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

static PyMethodDef ContourKernelMethods[] = {
  { "get_names", (PyCFunction)ContourKernel_get_names, METH_NOARGS, NULL},
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_CONTOUR_KERNEL_CLASS = "Kernel";
static char* MODULE_CONTOUR_KERNEL_CLASS_NAME = "contour.Kernel";

PyDoc_STRVAR(ContourKernelClass_doc, "contour kernel class functions");

//------------------------------------
// Define the ContourType type object
//------------------------------------
// Define the Python type object that stores contour.kernel types. 
//
static PyTypeObject ContourKernelType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = MODULE_CONTOUR_KERNEL_CLASS_NAME,
  //.tp_name = "contour." + std::string(MODULE_CONTOUR_KERNEL_CLASS).c_str(), 
  .tp_basicsize = sizeof(ContourKernelObject)
};

//----------------------------
// SetContourKernelTypeFields
//----------------------------
//
static void
SetContourKernelTypeFields(PyTypeObject& contourType)
 {
  contourType.tp_doc = "contour kernel types.";
  contourType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  contourType.tp_methods = ContourKernelMethods;
  contourType.tp_dict = PyDict_New();
};

//-----------------------
// SetContourKernelTypes
//-----------------------
//
static void
SetContourKernelTypes(PyTypeObject& contourType)
{
  for (auto const& entry : kernelNameEnumMap) {
      auto name = entry.first.c_str();
      if (PyDict_SetItemString(contourType.tp_dict, name, PyUnicode_FromString(name))) {
          std::cout << "Error initializing Python API contour kernel types." << std::endl;
          return;
      }
  }

  PyObject* nameList = PyList_New(kernelNameEnumMap.size());
  int n = 0;
  for (auto const& entry : kernelNameEnumMap) {
      auto name = entry.first.c_str();
      PyList_SetItem(nameList, n, PyUnicode_FromString(name));
      n += 1;
  }

  if (PyDict_SetItemString(contourType.tp_dict, "names", nameList)) {
      std::cout << "Error initializing Python API contour kernel types." << std::endl;
      return;
  }

};

#endif


