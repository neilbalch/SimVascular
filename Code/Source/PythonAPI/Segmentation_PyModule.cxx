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

// The functions defined here implement the SV Python API 'segmentation' module.
//
// A Python exception sv.segmentation.SegmentationError is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause.
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include "sv3_Contour.h"
#include "sv3_CircleContour.h"
#include "sv3_LevelSetContour.h"
#include "sv3_PolygonContour.h"
#include "sv3_SplinePolygonContour.h"
#include "sv3_ThresholdContour.h"
#include "Segmentation_PyModule.h"

#include "sv3_PathElement.h"

#include "sv3_SegmentationUtils.h"
#include "vtkPythonUtil.h"
#include "PyUtils.h"

#include <stdio.h>
#include <string.h>
#include <array>
#include <map>
#include <functional> 
#include <iostream>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
#include "sv_PolyData.h"
#include "vtkSmartPointer.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

// Prototypes for creating SV and Python contour objects. 
static PySegmentation * PyCreateSegmentationType();
static PyObject * PyCreateSegmentation(cKernelType contourType); 
static PyObject * PyCreateSegmentation(sv3::Contour* contour);

// Include implementations for the 'ContourKernel' and Contour' classes.
#include "SegmentationMethod_PyClass.cxx"
#include "Segmentation_PyClass.cxx"

//////////////////////////////////////////////////////
//          M o d u l e  M e t h o d s              //
//////////////////////////////////////////////////////
//
// Define the 'segmentation' module methods.

//---------------------
// Segmentation_create 
//---------------------
// [TODO:DaveP] DO we need this?
//
PyDoc_STRVAR(Segmentation_create_doc,
  "Segmentation_create()  \n\ 
   \n\
   Set the control points for the contour. \n\
   \n\
   Args:                                    \n\
     None \n\
");

static PyObject* 
Segmentation_create(PySegmentation* self, PyObject* args)
{
  auto api = PyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* kernelName = nullptr;

  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }


  //
  cKernelType contourType;
  try {
      contourType = kernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." + " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  //std::cout << "[Segmentation_create] Kernel name: " << kernelName << std::endl;
  auto cont = PyCreateSegmentation(contourType);
  Py_INCREF(cont);
  return cont;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* SEGMENTATION_MODULE = "segmentation";

// Contour module exception.
static char* SEGMENTATION_MODULE_EXCEPTION = "segmentation.SegmentationError";
static char* SEGMENTATION_MODULE_EXCEPTION_OBJECT = "SegmentationError";

PyDoc_STRVAR(SegmentationModule_doc, "Segmentation module functions.");

//------------------------
// PySegmentationModuleMethods
//------------------------
// Define the methods for the Python 'contour' module.
//
static PyMethodDef PySegmentationModuleMethods[] =
{
    {"create", (PyCFunction)Segmentation_create, METH_VARARGS, "Create a Contour object."},

    {NULL,NULL}
};

// Include derived Segmentation classes.
#include "SegmentationCircle_PyClass.cxx"
#include "SegmentationLevelSet_PyClass.cxx"
#include "SegmentationPolygon_PyClass.cxx"
#include "SegmentationSplinePolygon_PyClass.cxx"
#include "SegmentationThreshold_PyClass.cxx"

// Include segmentation.Group definition.
#include "SegmentationGroup_PyClass.cxx"

//------------------
// PySegmentationCtorMap
//------------------
// Define an object factory for Python Contour derived classes.
//
using PySegmentationCtorMapType = std::map<cKernelType, std::function<PyObject*()>>;
PySegmentationCtorMapType PySegmentationCtorMap = {
  {cKernelType::cKERNEL_CIRCLE,        []()->PyObject* {return PyObject_CallObject((PyObject*)&PyCircleSegmentationClassType, NULL);}},
  {cKernelType::cKERNEL_ELLIPSE,       []()->PyObject* {return PyObject_CallObject((PyObject*)&PyCircleSegmentationClassType, NULL);}},
  {cKernelType::cKERNEL_LEVELSET,      []()->PyObject* {return PyObject_CallObject((PyObject*)&PyLevelSetSegmentationClassType, NULL);}},
  {cKernelType::cKERNEL_POLYGON,       []()->PyObject* {return PyObject_CallObject((PyObject*)&PyPolygonSegmentationClassType, NULL);}},
  {cKernelType::cKERNEL_SPLINEPOLYGON, []()->PyObject* {return PyObject_CallObject((PyObject*)&PySplinePolygonSegmentationClassType, NULL);}},
  {cKernelType::cKERNEL_THRESHOLD,     []()->PyObject* {return PyObject_CallObject((PyObject*)&PyThresholdSegmentationClassType, NULL);}},
};

//----------------------
// PyCreateSegmentation
//----------------------
// Create a Python Segmentation object for the given kernel type.
// 
static PyObject *
PyCreateSegmentation(cKernelType contourType)
{
  //std::cout << "[PyCreateSegmentation] ========== PyCreateSegmentation ==========" << std::endl;
  PyObject* contourObj;

  try {
      contourObj = PySegmentationCtorMap[contourType]();
  } catch (const std::bad_function_call& except) {
      return nullptr; 
  }

  return contourObj;
}

//----------------------
// PyCreateSegmentation
//----------------------
// Create a Python Segmentation object for the given sv3::Contour object.
//
// [TODO:DaveP] The concept of contour type and kernel type is
// a bit muddled. Contour type is stored as a string in sv3::Contour.
//
//   Contour types: Circle, Ellipse, Polygon, SplinePolygon, TensionPolygon and Contour
//
static PyObject *
PyCreateSegmentation(sv3::Contour* contour)
{
  //std::cout << "[PyCreateSegmentation] ========== PyCreateSegmentation ==========" << std::endl;
  auto kernel = contour->GetKernel();
  auto ctype = contour->GetType();
  PyObject* contourObj;

  if (ctype == "Contour") { 
      contourObj = PyObject_CallObject((PyObject*)&PySegmentationClassType, NULL);
  } else {
      contourObj = PyCreateSegmentation(kernel);
  }

  // Replace the PySegmentation->contour with 'contour'.
  //
  auto pyContour = (PySegmentation*)contourObj;
  delete pyContour->contour;
  pyContour->contour = contour;

  return contourObj;
}

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3

// Size of per-interpreter state of the module.
// Set to -1 if the module keeps state in global variables. 
static int perInterpreterStateSize = -1;

// Always initialize this to PyModuleDef_HEAD_INIT.
static PyModuleDef_Base m_base = PyModuleDef_HEAD_INIT;

// Define the module definition struct which holds all information 
// needed to create a module object. 
static struct PyModuleDef PySegmentationModule = {
   m_base,
   SEGMENTATION_MODULE,   
   SegmentationModule_doc, 
   perInterpreterStateSize,  
   PySegmentationModuleMethods
};

//-----------------------
// PyInit_PySegmentation 
//-----------------------
// The initialization function called by the Python interpreter 
// when the module is loaded.
//
PyMODINIT_FUNC 
PyInit_PySegmentation()
{
  std::cout << "========== load segmentation module ==========" << std::endl;

  // Initialize the Contour class type.
  SetSegmentationTypeFields(PySegmentationClassType);
  if (PyType_Ready(&PySegmentationClassType) < 0) {
    fprintf(stdout,"Error in PySegmentationClassType\n");
    return SV_PYTHON_ERROR;
  }

  // Initialize the group class type.
  SetSegmentationGroupTypeFields(PySegmentationGroupClassType);
  if (PyType_Ready(&PySegmentationGroupClassType) < 0) {
      std::cout << "Error creating SegmentationGroup type" << std::endl;
      return nullptr;
  }

  // Initialize the circle class type.
  SetCircleSegmentationTypeFields(PyCircleSegmentationClassType);
  if (PyType_Ready(&PyCircleSegmentationClassType) < 0) {
      std::cout << "Error creating CircleSegmentation type" << std::endl;
      return nullptr;
  }

  // Initialize the level set class type.
  SetLevelSetSegmentationTypeFields(PyLevelSetSegmentationClassType);
  if (PyType_Ready(&PyLevelSetSegmentationClassType) < 0) {
      std::cout << "Error creating LevelSetSegmentation type" << std::endl;
      return nullptr;
  }

  // Initialize the polygon class type.
  SetPolygonSegmentationTypeFields(PyPolygonSegmentationClassType);
  if (PyType_Ready(&PyPolygonSegmentationClassType) < 0) {
      std::cout << "Error creating PolygonSegmentation type" << std::endl;
      return nullptr;
  }

  // Initialize the spline polygon class type.
  SetSplinePolygonSegmentationTypeFields(PySplinePolygonSegmentationClassType);
  if (PyType_Ready(&PySplinePolygonSegmentationClassType) < 0) {
      std::cout << "Error creating SplinePolygonSegmentation type" << std::endl;
      return nullptr;
  }

  // Initialize the threshold class type.
  SetThresholdSegmentationTypeFields(PyThresholdSegmentationClassType);
  if (PyType_Ready(&PyThresholdSegmentationClassType) < 0) {
      std::cout << "Error creating ThresholdSegmentation type" << std::endl;
      return nullptr;
  }

  // Initialize the kernel class type.
  SetSegmentationMethodTypeFields(PySegmentationMethodClassType);
  if (PyType_Ready(&PySegmentationMethodClassType) < 0) {
      std::cout << "Error creating SegmentationMethod type" << std::endl;
      return nullptr;
  }

  // Create the contour module.
  auto module = PyModule_Create(&PySegmentationModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing PySegmentation\n");
    return SV_PYTHON_ERROR;
  }

  // Add contour.SegmentationError exception.
  PyRunTimeErr = PyErr_NewException(SEGMENTATION_MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, SEGMENTATION_MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  // Add the 'Segmentation' object.
  Py_INCREF(&PySegmentationClassType);
  PyModule_AddObject(module, SEGMENTATION_CLASS, (PyObject*)&PySegmentationClassType);

  // Add the 'Group' object.
  Py_INCREF(&PySegmentationGroupClassType);
  PyModule_AddObject(module, SEGMENTATION_GROUP_CLASS, (PyObject*)&PySegmentationGroupClassType);

  // Add the 'Circle' class.
  Py_INCREF(&PyCircleSegmentationClassType);
  PyModule_AddObject(module, SEGMENTATION_CIRCLE_CLASS, (PyObject*)&PyCircleSegmentationClassType);

  // Add the 'LevelSet' class.
  Py_INCREF(&PyLevelSetSegmentationClassType);
  PyModule_AddObject(module, SEGMENTATION_LEVELSET_CLASS, (PyObject*)&PyLevelSetSegmentationClassType);

  // Add the 'Polygon' class.
  Py_INCREF(&PyPolygonSegmentationClassType);
  PyModule_AddObject(module, SEGMENTATION_POLYGON_CLASS, (PyObject*)&PyPolygonSegmentationClassType);

  // Add the 'SplinePolygon' class.
  Py_INCREF(&PySplinePolygonSegmentationClassType);
  PyModule_AddObject(module, SEGMENTATION_SPLINE_POLYGON_CLASS, (PyObject*)&PySplinePolygonSegmentationClassType);

  // Add the 'Threshold' class.
  Py_INCREF(&PyThresholdSegmentationClassType);
  PyModule_AddObject(module, SEGMENTATION_THRESHOLD_CLASS, (PyObject*)&PyThresholdSegmentationClassType);

  // Add the 'Method' class.
  Py_INCREF(&PySegmentationMethodClassType);
  PyModule_AddObject(module, SEGMENTATION_METHOD_CLASS, (PyObject*)&PySegmentationMethodClassType);

  // Set the kernel names in the SegmentationMethodType dictionary.
  SetSegmentationMethodClassTypes(PySegmentationMethodClassType);

  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

//----------------
//initPySegmentation
//----------------
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initPySegmentation()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from sv3_Segmentation_init\n");
  }

  // Set the global contour kernel.
  //
  // [TODO:DaveP] yuk!
  //
  Segmentation::gCurrentKernel = cKERNEL_INVALID;

  // Create a Segmentation class.
  PySegmentationClassType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PySegmentationClassType)<0) {
    fprintf(stdout,"Error in PySegmentationClassType\n");
    return;
  }

  auto module = Py_InitModule(SEGMENTATION_MODULE, PySegmentationModule_methods);
  if(module == NULL) {
    fprintf(stdout,"Error in initializing PySegmentation\n");
    return;
  }
  PyRunTimeErr = PyErr_NewException("PySegmentation.error",NULL,NULL);
  PyModule_AddObject(module,"error",PyRunTimeErr);

  Py_INCREF(&PySegmentationClassType);
  PyModule_AddObject(pythonC,"PySegmentation",(PyObject*)&PySegmentationClassType);

  // Add the 'kernel' class.
  Py_INCREF(&SegmentationMethodClassType);
  PyModule_AddObject(module, "kernel", (PyObject*)&SegmentationMethodClassType);

  SetSegmentationMethodClassTypes(SegmentationMethodClassType);

  
  return module;

}
#endif

