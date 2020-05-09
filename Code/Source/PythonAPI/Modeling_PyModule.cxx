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

// The functions defined here implement the SV Python API modeling module. 
//
// A Python exception sv.modeling.ModelingError is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause.
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include <functional>
#include <map>
#include <stdio.h>
#include <string.h>

#include "sv_Repository.h"
#include "Modeling_PyModule.h"
#include "sv_SolidModel.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"
#include "sv_vtk_utils.h"
#include "sv_PolyData.h"
#include "sv_OCCTSolidModel.h"
#include "sv_PolyDataSolid.h"
#include "sv_sys_geom.h"
#include "PyUtils.h"

#include "sv_FactoryRegistrar.h"

// Needed for Windows.
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"
#include "Python.h"
#include <structmember.h>
#include "vtkPythonUtil.h"

#if PYTHON_MAJOR_VERSION == 3
#include "PyVTKObject.h"
#elif PYTHON_MAJOR_VERSION == 2
#include "PyVTKClass.h"
#endif

// Include solid Kernel class that defines a map between 
// solid model kernel name and enum type.
//
#include "ModelingKernel_PyClass.cxx"

//---------------------
// CvSolidModelCtorMap
//---------------------
// Define an object factory for creating cvSolidModel objects.
//
// An entry for SM_KT_PARASOLID is added later in PyAPI_InitParasolid() 
// if the Parasolid interface is defined (by loading the Parasolid plugin).
//
using ModelingCtorMapType = std::map<SolidModel_KernelT, std::function<cvSolidModel*()>>;
ModelingCtorMapType CvSolidModelCtorMap = {
    {SolidModel_KernelT::SM_KT_OCCT, []() -> cvSolidModel* { return new cvOCCTSolidModel(); } },
    {SolidModel_KernelT::SM_KT_POLYDATA, []() -> cvSolidModel* { return new cvPolyDataSolid(); } },
};

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

static PyObject * CreatePyModelingModelObject(SolidModel_KernelT kernel);
static PyObject * CreatePyModelingModelObject(cvSolidModel* solidModel);

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//--------------------
// CreateCvSolidModel
//--------------------
// Create an cvSolidModel object for the given kernel.
//
static cvSolidModel *
CreateCvSolidModel(SolidModel_KernelT kernel)
{
  //std::cout << "[CreateCvSolidModel] ========== CreateCvSolidModel ==========" << std::endl;
  cvSolidModel* solidModel;

  try {
      solidModel = CvSolidModelCtorMap[kernel]();
  } catch (...) {
      return nullptr;
  }

  return solidModel;
}

////////////////////////////////////////////////////////
//          M o d u l e   M e t h o d s               //
////////////////////////////////////////////////////////
//
//---------------------------------
// PyModelingModule_modeler_exists
//---------------------------------
//
PyDoc_STRVAR(PyModelingModule_modeler_exists_doc,
  "modeler_exists(kernel)  \n\ 
   \n\
   Check if the modeler for the given kernel exists. \n\
   \n\
   Args:\n\
     kernel (str): Name of the solid modeling kernel. Valid names are:  \n\
");

static PyObject *
PyModelingModule_modeler_exists(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  auto api = PyUtilApiFunction("s", PyRunTimeErr, "Modeler");
  char* kernelName = nullptr;
  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  //std::cout << "[PySolidModeler_exists] Kernel: " << kernelName << std::endl;
  SolidModel_KernelT kernel;

  try {
      kernel = kernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." +
          " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  try {
    auto ctore = CvSolidModelCtorMap.at(kernel);
  } catch (const std::out_of_range& except) {
      return Py_False; 
  }

  return Py_True; 
}

////////////////////////////////////////////////////////
//          M o d u l e   D e f i n i t i o n         //
////////////////////////////////////////////////////////

//----------------
// PySolidMethods 
//----------------
// Methods for the 'solid' module.
//
static PyMethodDef PyModelingModuleMethods[] = {

  { "modeler_exists", (PyCFunction)PyModelingModule_modeler_exists, METH_VARARGS | METH_KEYWORDS, PyModelingModule_modeler_exists_doc},

  {NULL, NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python interpreter 
// when the module is loaded.

static char* MODELING_MODULE = "modeling";
static char* MODELING_MODULE_EXCEPTION = "modeling.ModelingError";
static char* MODELING_MODULE_EXCEPTION_OBJECT = "ModelingError";

PyDoc_STRVAR(Solid_module_doc, "Modeling module functions");

// Include derived solid classes.
#include "ModelingModel_PyClass.cxx"
#include "ModelingOpenCascade_PyClass.cxx"
#include "ModelingParasolid_PyClass.cxx"
#include "ModelingPolyData_PyClass.cxx"
#include "ModelingModeler_PyClass.cxx"

// Include solid.Group definition.
#include "ModelingGroup_PyClass.cxx"

//---------------------
// PySolidModelCtorMap
//---------------------
// Define an object factory for creating Python ModelingModel derived objects.
//
// An entry for SM_KT_PARAMODELING is added later in PyAPI_InitParasolid() 
// if the Parasolid interface is defined (by loading the Parasolid plugin).
//
using PyModelingModelCtorMapType = std::map<SolidModel_KernelT, std::function<PyObject*()>>;
PyModelingModelCtorMapType PyModelingModelCtorMap = {
  {SolidModel_KernelT::SM_KT_OCCT, []()->PyObject* {return PyObject_CallObject((PyObject*)&PyOcctSolidType, NULL);}},
  {SolidModel_KernelT::SM_KT_PARASOLID, []()->PyObject* {return PyObject_CallObject((PyObject*)&PyParasolidSolidType, NULL);}},
  {SolidModel_KernelT::SM_KT_POLYDATA, []()->PyObject* {return PyObject_CallObject((PyObject*)&PyPolyDataSolidType, NULL);}},
};

//--------------------
// CreatePyModelingModel
//--------------------
// Create a Python ModelingModel object for the given kernel.
//
static PyObject *
CreatePyModelingModelObject(SolidModel_KernelT kernel)
{
  //std::cout << "[CreatePyModelingModelObject] ========== CreatePyModelingModelObject ==========" << std::endl;
  auto cvSolidModel = CreateCvSolidModel(kernel);
  if (cvSolidModel == nullptr) { 
      return nullptr;
  }

  return CreatePyModelingModelObject(cvSolidModel);
}

//--------------------------
// CreatePyModelingModelObject 
//--------------------------
// Create a Python ModelingModel object for the given cvSolidModel object.
//
static PyObject *
CreatePyModelingModelObject(cvSolidModel* solidModel)
{
  //std::cout << "[CreatePyModelingModelObject] ========== CreatePyModelingModelObject ==========" << std::endl;
  //std::cout << "[CreatePyModelingModelObject] Copy from given cvSolidModel object" << std::endl;
  PyObject* pyModelingModelObj;
  auto kernel = solidModel->GetKernelT();

  try {
      pyModelingModelObj = PyModelingModelCtorMap[kernel]();
  } catch (const std::bad_function_call& except) {
      std::cout << "[CreatePyModelingModelObject] ERROR: Creating pyModelingModelObj " << std::endl;
      return nullptr;
  }

  // Set the solidModel object.
  auto pyModelingModel = (PyModelingModel*)pyModelingModelObj;
  pyModelingModel->solidModel = solidModel->Copy();
  pyModelingModel->kernel = kernel;
  return pyModelingModelObj;
}

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

static struct PyModuleDef PyModelingModule = {
   m_base,
   MODELING_MODULE, 
   Solid_module_doc, 
   perInterpreterStateSize, 
   PyModelingModuleMethods
};

//-------------------
// PyInit_PyModeling
//-------------------
// The initialization function called by the Python interpreter 
// when the 'modeling' module is loaded.
//
PyMODINIT_FUNC
PyInit_PyModeling(void)
{
  //std::cout << "[PyInit_PySolid] ========== load modeling module ==========" << std::endl;

  // Initialize the ModelingModeler class type.
  //std::cout << "[PyInit_PySolid] Initialize the ModelingModeler class type. " << std::endl;
  SetModelingModelerTypeFields(PyModelingModelerType);
  if (PyType_Ready(&PyModelingModelerType) < 0) {
    fprintf(stdout,"Error in PyModelingModelerType");
    return SV_PYTHON_ERROR;
  }

  // Initialize the ModelingModel class type.
  //std::cout << "[PyInit_PySolid] Initialize the ModelingModel class type. " << std::endl;
  SetModelingModelTypeFields(PyModelingModelType);
  if (PyType_Ready(&PyModelingModelType) < 0) {
    fprintf(stdout,"Error in PyModelingModelType");
    return SV_PYTHON_ERROR;
  }

  // Initialize the group class type.
  SetModelingGroupTypeFields(PyModelingGroupType);
  if (PyType_Ready(&PyModelingGroupType) < 0) {
      std::cout << "Error creating SolidGroup type" << std::endl;
      return nullptr;
  }

  // Initialize the OpenCascade class type.
  //std::cout << "[PyInit_PySolid] Initialize the OpenCascade class type. " << std::endl;
  SetOcctSolidTypeFields(PyOcctSolidType);
  //std::cout << "[PyInit_PySolid] Set fields done ... " << std::endl;
  if (PyType_Ready(&PyOcctSolidType) < 0) {
      std::cout << "Error creating OpenCascade type" << std::endl;
      return nullptr;
  }

  // Initialize the Parasolid class type.
  //std::cout << "[PyInit_PySolid] Initialize the Parasolid class type. " << std::endl;
  SetParasolidSolidTypeFields(PyParasolidSolidType);
  //std::cout << "[PyInit_PySolid] Set fields done ... " << std::endl;
  if (PyType_Ready(&PyParasolidSolidType) < 0) {
      std::cout << "Error creating PolydataSolid type" << std::endl;
      return nullptr;
  }

  // Initialize the PolyData class type.
  //std::cout << "[PyInit_PySolid] Initialize the PolyData class type. " << std::endl;
  SetPolyDataSolidTypeFields(PyPolyDataSolidType);
  //std::cout << "[PyInit_PySolid] Set fields done ... " << std::endl;
  if (PyType_Ready(&PyPolyDataSolidType) < 0) {
      std::cout << "Error creating PolydataSolid type" << std::endl;
      return nullptr;
  }

  // Initialize the solid modeling kernel class type.
  SetModelingKernelTypeFields(PyModelingKernelType);
  if (PyType_Ready(&PyModelingKernelType) < 0) {
      std::cout << "Error creating SolidKernel type" << std::endl;
      return nullptr;
  }

  // Create the 'solid' module. 
  //std::cout << "[PyInit_PySolid] Create the 'solid' module. " << std::endl;
  auto module = PyModule_Create(&PyModelingModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing pySolid");
    return SV_PYTHON_ERROR;
  }

  // Add solid.ModelingModelError exception.
  PyRunTimeErr = PyErr_NewException(MODELING_MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, MODELING_MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  // Add the 'ModelingModeler' class.
  //std::cout << "[PyInit_PySolid] Add the ModelingModeler class type. " << std::endl;
  Py_INCREF(&PyModelingModelerType);
  PyModule_AddObject(module, MODELING_MODELER_CLASS, (PyObject *)&PyModelingModelerType);

  // Add the 'ModelingModel' class.
  //std::cout << "[PyInit_PySolid] Add the ModelingModel class type. " << std::endl;
  Py_INCREF(&PyModelingModelType);
  PyModule_AddObject(module, MODELING_MODEL_CLASS, (PyObject *)&PyModelingModelType);

  // Add the 'SolidGroup' class.
  //std::cout << "[PyInit_PySolid] Add the SolidGroup class type. " << std::endl;
  Py_INCREF(&PyModelingGroupType);
  PyModule_AddObject(module, MODELING_GROUP_CLASS, (PyObject *)&PyModelingGroupType);

  // Add the 'OpenCascade' class.
  //std::cout << "[PyInit_PySolid] Add the OpenCascade class type. " << std::endl;
  Py_INCREF(&PyOcctSolidType);
  PyModule_AddObject(module, MODELING_OCCT_CLASS, (PyObject *)&PyOcctSolidType);

  // Add the 'Parasolid' class.
  //std::cout << "[PyInit_PySolid] Add the ParasolidSolid class type. " << std::endl;
  Py_INCREF(&PyParasolidSolidType);
  PyModule_AddObject(module, MODELING_PARAMODELING_CLASS, (PyObject *)&PyParasolidSolidType);

  // Add the 'PolyData' class.
  //std::cout << "[PyInit_PySolid] Add the PolyDataSolid class type. " << std::endl;
  Py_INCREF(&PyPolyDataSolidType);
  PyModule_AddObject(module, MODELING_POLYDATA_CLASS, (PyObject *)&PyPolyDataSolidType);

  // Add the 'Kernel' class.
  Py_INCREF(&PyModelingKernelType);
  PyModule_AddObject(module, MODELING_KERNEL_CLASS, (PyObject*)&PyModelingKernelType);

  // Set the kernel names in the SolidKernelType dictionary.
  SetModelingKernelTypes(PyModelingKernelType);

  // Initialize Open Cascade.
  InitOcct();

  //std::cout << "[PyInit_PySolid] Done. " << std::endl;

  return module;
}

#endif

