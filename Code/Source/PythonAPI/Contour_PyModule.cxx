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

// The functions defined here implement the SV Python API 'contour' module.
//
// A Python exception sv.contour.ContourError is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.contour.ContourError:
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include "sv3_Contour.h"
#include "sv3_CircleContour.h"
#include "sv3_LevelSetContour.h"
#include "sv3_PolygonContour.h"
#include "sv3_SplinePolygonContour.h"
#include "sv3_ThresholdContour.h"
#include "Contour_PyModule.h"

#include "sv3_PathElement.h"

#include "sv3_SegmentationUtils.h"
#include "vtkPythonUtil.h"
#include "sv_PyUtils.h"

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
static PyContour * PyCreateContourType();
static PyObject * PyCreateContour(cKernelType contourType); 
static PyObject * PyCreateContour(sv3::Contour* contour);

// Include implementations for the 'ContourKernel' and Contour' classes.
#include "ContourKernel_PyClass.cxx"
#include "Contour_PyClass.cxx"

//////////////////////////////////////////////////////
//          M o d u l e  M e t h o d s              //
//////////////////////////////////////////////////////
//
// Define the 'contour' module methods.

//----------------
// Contour_create
//----------------
//
PyDoc_STRVAR(Contour_create_doc,
  "Contour_create()  \n\ 
   \n\
   Set the control points for the contour. \n\
   \n\
   Args:                                    \n\
     None \n\
");

static PyObject* 
Contour_create(PyContour* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* kernelName = nullptr;

  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  // Get the contour type.
  //
  cKernelType contourType;
  try {
      contourType = kernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." + " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  //std::cout << "[Contour_create] Kernel name: " << kernelName << std::endl;
  auto cont = PyCreateContour(contourType);
  Py_INCREF(cont);
  return cont;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* CONTOUR_MODULE = "contour";

// Contour module exception.
static char* CONTOUR_MODULE_EXCEPTION = "contour.ContourError";
static char* CONTOUR_MODULE_EXCEPTION_OBJECT = "ContourError";

PyDoc_STRVAR(ContourModule_doc, "Contour module functions.");

//------------------------
// PyContourModuleMethods
//------------------------
// Define the methods for the Python 'contour' module.
//
static PyMethodDef PyContourModuleMethods[] =
{
    {"create", (PyCFunction)Contour_create, METH_VARARGS, "Create a Contour object."},

    {NULL,NULL}
};

// Include derived Contour classes.
#include "ContourCircle_PyClass.cxx"
#include "ContourLevelSet_PyClass.cxx"
#include "ContourPolygon_PyClass.cxx"
#include "ContourSplinePolygon_PyClass.cxx"
#include "ContourThreshold_PyClass.cxx"

// Include path.Group definition.
#include "ContourGroup_PyClass.cxx"

//------------------
// PyContourCtorMap
//------------------
// Define an object factory for Python Contour derived classes.
//
using PyContourCtorMapType = std::map<cKernelType, std::function<PyObject*()>>;
PyContourCtorMapType PyContourCtorMap = {
  {cKernelType::cKERNEL_CIRCLE,        []()->PyObject* {return PyObject_CallObject((PyObject*)&PyCircleContourClassType, NULL);}},
  {cKernelType::cKERNEL_ELLIPSE,       []()->PyObject* {return PyObject_CallObject((PyObject*)&PyCircleContourClassType, NULL);}},
  {cKernelType::cKERNEL_LEVELSET,      []()->PyObject* {return PyObject_CallObject((PyObject*)&PyLevelSetContourClassType, NULL);}},
  {cKernelType::cKERNEL_POLYGON,       []()->PyObject* {return PyObject_CallObject((PyObject*)&PyPolygonContourClassType, NULL);}},
  {cKernelType::cKERNEL_SPLINEPOLYGON, []()->PyObject* {return PyObject_CallObject((PyObject*)&PySplinePolygonContourClassType, NULL);}},
  {cKernelType::cKERNEL_THRESHOLD,     []()->PyObject* {return PyObject_CallObject((PyObject*)&PyThresholdContourClassType, NULL);}},
};

//-----------------
// PyCreateContour
//-----------------
// Create a Python contour object for the given kernel type.
// 
static PyObject *
PyCreateContour(cKernelType contourType)
{
  //std::cout << "[PyCreateContour] ========== PyCreateContour ==========" << std::endl;
  PyObject* contourObj;

  try {
      contourObj = PyContourCtorMap[contourType]();
  } catch (const std::bad_function_call& except) {
      return nullptr; 
  }

  return contourObj;
}

//-----------------
// PyCreateContour
//-----------------
// Create a Python contour object for the given sv3::Contour object.
//
// [TODO:DaveP] The concept of contour type and kernel type is
// a bit muddled. Contour type is stored as a string in sv3::Contour.
//
//   Contour types: Circle, Ellipse, Polygon, SplinePolygon, TensionPolygon and Contour
//
//
static PyObject *
PyCreateContour(sv3::Contour* contour)
{
  //std::cout << "[PyCreateContour] ========== PyCreateContour ==========" << std::endl;
  auto kernel = contour->GetKernel();
  auto ctype = contour->GetType();
  PyObject* contourObj;

  if (ctype == "Contour") { 
      contourObj = PyObject_CallObject((PyObject*)&PyContourClassType, NULL);
  } else {
      contourObj = PyCreateContour(kernel);
  }

  // Replace the PyContour->contour with 'contour'.
  //
  auto pyContour = (PyContour*)contourObj;
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
static struct PyModuleDef PyContourModule = {
   m_base,
   CONTOUR_MODULE,   
   ContourModule_doc, 
   perInterpreterStateSize,  
   PyContourModuleMethods
};

//------------------
// PyInit_PyContour 
//------------------
// The initialization function called by the Python interpreter 
// when the module is loaded.
//
PyMODINIT_FUNC PyInit_PyContour()
{
  std::cout << "========== load contour module ==========" << std::endl;

  // Initialize the Contour class type.
  SetContourTypeFields(PyContourClassType);
  if (PyType_Ready(&PyContourClassType) < 0) {
    fprintf(stdout,"Error in PyContourClassType\n");
    return SV_PYTHON_ERROR;
  }

  // Initialize the group class type.
  SetContourGroupTypeFields(PyContourGroupClassType);
  if (PyType_Ready(&PyContourGroupClassType) < 0) {
      std::cout << "Error creating ContourGroup type" << std::endl;
      return nullptr;
  }

  // Initialize the circle class type.
  SetCircleContourTypeFields(PyCircleContourClassType);
  if (PyType_Ready(&PyCircleContourClassType) < 0) {
      std::cout << "Error creating CircleContour type" << std::endl;
      return nullptr;
  }

  // Initialize the level set class type.
  SetLevelSetContourTypeFields(PyLevelSetContourClassType);
  if (PyType_Ready(&PyLevelSetContourClassType) < 0) {
      std::cout << "Error creating LevelSetContour type" << std::endl;
      return nullptr;
  }

  // Initialize the polygon class type.
  SetPolygonContourTypeFields(PyPolygonContourClassType);
  if (PyType_Ready(&PyPolygonContourClassType) < 0) {
      std::cout << "Error creating PolygonContour type" << std::endl;
      return nullptr;
  }

  // Initialize the spline polygon class type.
  SetSplinePolygonContourTypeFields(PySplinePolygonContourClassType);
  if (PyType_Ready(&PySplinePolygonContourClassType) < 0) {
      std::cout << "Error creating SplinePolygonContour type" << std::endl;
      return nullptr;
  }

  // Initialize the threshold class type.
  SetThresholdContourTypeFields(PyThresholdContourClassType);
  if (PyType_Ready(&PyThresholdContourClassType) < 0) {
      std::cout << "Error creating ThresholdContour type" << std::endl;
      return nullptr;
  }

  // Initialize the kernel class type.
  SetContourKernelTypeFields(PyContourKernelClassType);
  if (PyType_Ready(&PyContourKernelClassType) < 0) {
      std::cout << "Error creating ContourKernel type" << std::endl;
      return nullptr;
  }

  // Create the contour module.
  auto module = PyModule_Create(&PyContourModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing PyContour\n");
    return SV_PYTHON_ERROR;
  }

  // Add contour.ContourError exception.
  PyRunTimeErr = PyErr_NewException(CONTOUR_MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, CONTOUR_MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  // Add the 'Contour' object.
  Py_INCREF(&PyContourClassType);
  PyModule_AddObject(module, CONTOUR_CLASS, (PyObject*)&PyContourClassType);

  // Add the 'Group' object.
  Py_INCREF(&PyContourGroupClassType);
  PyModule_AddObject(module, CONTOUR_GROUP_CLASS, (PyObject*)&PyContourGroupClassType);

  // Add the 'Circle' class.
  Py_INCREF(&PyCircleContourClassType);
  PyModule_AddObject(module, CONTOUR_CIRCLE_CLASS, (PyObject*)&PyCircleContourClassType);

  // Add the 'LevelSet' class.
  Py_INCREF(&PyLevelSetContourClassType);
  PyModule_AddObject(module, CONTOUR_LEVELSET_CLASS, (PyObject*)&PyLevelSetContourClassType);

  // Add the 'Polygon' class.
  Py_INCREF(&PyPolygonContourClassType);
  PyModule_AddObject(module, CONTOUR_POLYGON_CLASS, (PyObject*)&PyPolygonContourClassType);

  // Add the 'SplinePolygon' class.
  Py_INCREF(&PySplinePolygonContourClassType);
  PyModule_AddObject(module, CONTOUR_SPLINE_POLYGON_CLASS, (PyObject*)&PySplinePolygonContourClassType);

  // Add the 'Threshold' class.
  Py_INCREF(&PyThresholdContourClassType);
  PyModule_AddObject(module, CONTOUR_THRESHOLD_CLASS, (PyObject*)&PyThresholdContourClassType);

  // Add the 'Kernel' class.
  Py_INCREF(&PyContourKernelClassType);
  PyModule_AddObject(module, CONTOUR_KERNEL_CLASS, (PyObject*)&PyContourKernelClassType);

  // Set the kernel names in the ContourKernelType dictionary.
  SetContourKernelClassTypes(PyContourKernelClassType);

  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

//----------------
//initPyContour
//----------------
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initPyContour()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from sv3_Contour_init\n");
  }

  // Set the global contour kernel.
  //
  // [TODO:DaveP] yuk!
  //
  Contour::gCurrentKernel = cKERNEL_INVALID;

  // Create a Contour class.
  PyContourClassType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyContourClassType)<0) {
    fprintf(stdout,"Error in PyContourClassType\n");
    return;
  }

  auto module = Py_InitModule(CONTOUR_MODULE, PyContourModule_methods);
  if(module == NULL) {
    fprintf(stdout,"Error in initializing PyContour\n");
    return;
  }
  PyRunTimeErr = PyErr_NewException("PyContour.error",NULL,NULL);
  PyModule_AddObject(module,"error",PyRunTimeErr);

  Py_INCREF(&PyContourClassType);
  PyModule_AddObject(pythonC,"PyContour",(PyObject*)&PyContourClassType);

  // Add the 'kernel' class.
  Py_INCREF(&ContourKernelClassType);
  PyModule_AddObject(module, "kernel", (PyObject*)&ContourKernelClassType);

  SetContourKernelClassTypes(ContourKernelClassType);

  
  return module;

}
#endif

