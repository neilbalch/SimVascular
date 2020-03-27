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

// The functions defined here implement the SV Python API 'solid' Module. 
//
// The module name is 'solid'. 
//
// A Python exception sv.solid.SolidModelError is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.solid.SolidModelError: 
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include <functional>
#include <map>
#include <stdio.h>
#include <string.h>

#include "sv_Repository.h"
#include "Solid_PyModule.h"
#include "sv_SolidModel.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"
#include "sv_vtk_utils.h"
#include "sv_PolyData.h"
#include "sv_OCCTSolidModel.h"
#include "sv_PolyDataSolid.h"
#include "sv_sys_geom.h"
#include "sv_PyUtils.h"

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

#include "sv_occt_init_py.h"

// Include solid Kernel class that defines a map between 
// solid model kernel name and enum type.
//
#include "SolidKernel_PyClass.cxx"

//---------------------
// CvSolidModelCtorMap
//---------------------
// Define an object factory for creating cvSolidModel objects.
//
// An entry for SM_KT_PARASOLID is added later in PyAPI_InitParasolid() 
// if the Parasolid interface is defined (by loading the Parasolid plugin).
//
using SolidCtorMapType = std::map<SolidModel_KernelT, std::function<cvSolidModel*()>>;
SolidCtorMapType CvSolidModelCtorMap = {
    {SolidModel_KernelT::SM_KT_OCCT, []() -> cvSolidModel* { return new cvOCCTSolidModel(); } },
    {SolidModel_KernelT::SM_KT_POLYDATA, []() -> cvSolidModel* { return new cvPolyDataSolid(); } },
};

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

static PyObject * CreatePySolidModelObject(SolidModel_KernelT kernel);
static PyObject * CreatePySolidModelObject(cvSolidModel* solidModel);

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
//------------------------------
// PySolidModule_modeler_exists
//------------------------------
//
PyDoc_STRVAR(PySolidModule_modeler_exists_doc,
  "modeler_exists(kernel)  \n\ 
   \n\
   Check if the modeler for the given kernel exists. \n\
   \n\
   Args:\n\
     kernel (str): Name of the solid modeling kernel. Valid names are:  \n\
");

static PyObject *
PySolidModule_modeler_exists(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, "Modeler");
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
static PyMethodDef PySolidModuleMethods[] = {

  { "modeler_exists", (PyCFunction)PySolidModule_modeler_exists, METH_VARARGS | METH_KEYWORDS, PySolidModule_modeler_exists_doc},

  {NULL, NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python interpreter 
// when the module is loaded.

static char* SOLID_MODULE = "solid";
static char* SOLID_MODULE_EXCEPTION = "solid.SolidModelError";
static char* SOLID_MODULE_EXCEPTION_OBJECT = "SolidModelError";

PyDoc_STRVAR(Solid_module_doc, "solid module functions");

// Include derived solid classes.
#include "SolidModel_PyClass.cxx"
#include "SolidOpenCascade_PyClass.cxx"
#include "SolidParasolid_PyClass.cxx"
#include "SolidPolyData_PyClass.cxx"
#include "SolidModeler_PyClass.cxx"

// Include solid.Group definition.
#include "SolidGroup_PyClass.cxx"

//---------------------
// PySolidModelCtorMap
//---------------------
// Define an object factory for creating Python SolidModel derived objects.
//
// An entry for SM_KT_PARASOLID is added later in PyAPI_InitParasolid() 
// if the Parasolid interface is defined (by loading the Parasolid plugin).
//
using PySolidModelCtorMapType = std::map<SolidModel_KernelT, std::function<PyObject*()>>;
PySolidModelCtorMapType PySolidModelCtorMap = {
  {SolidModel_KernelT::SM_KT_OCCT, []()->PyObject* {return PyObject_CallObject((PyObject*)&PyOcctSolidClassType, NULL);}},
  {SolidModel_KernelT::SM_KT_PARASOLID, []()->PyObject* {return PyObject_CallObject((PyObject*)&PyParasolidSolidClassType, NULL);}},
  {SolidModel_KernelT::SM_KT_POLYDATA, []()->PyObject* {return PyObject_CallObject((PyObject*)&PyPolyDataSolidClassType, NULL);}},
};

//--------------------
// CreatePySolidModel
//--------------------
// Create a Python SolidModel object for the given kernel.
//
static PyObject *
CreatePySolidModelObject(SolidModel_KernelT kernel)
{
  //std::cout << "[CreatePySolidModelObject] ========== CreatePySolidModelObject ==========" << std::endl;
  auto cvSolidModel = CreateCvSolidModel(kernel);
  if (cvSolidModel == nullptr) { 
      return nullptr;
  }

  return CreatePySolidModelObject(cvSolidModel);
}

//--------------------------
// CreatePySolidModelObject 
//--------------------------
// Create a Python SolidModel object for the given cvSolidModel object.
//
static PyObject *
CreatePySolidModelObject(cvSolidModel* solidModel)
{
  //std::cout << "[CreatePySolidModelObject] ========== CreatePySolidModelObject ==========" << std::endl;
  //std::cout << "[CreatePySolidModelObject] Copy from given cvSolidModel object" << std::endl;
  PyObject* pySolidModelObj;
  auto kernel = solidModel->GetKernelT();

  try {
      pySolidModelObj = PySolidModelCtorMap[kernel]();
  } catch (const std::bad_function_call& except) {
      std::cout << "[CreatePySolidModelObject] ERROR: Creating pySolidModelObj " << std::endl;
      return nullptr;
  }

  // Set the solidModel object.
  auto pySolidModel = (PySolidModelClass*)pySolidModelObj;
  pySolidModel->solidModel = solidModel->Copy();
  pySolidModel->kernel = kernel;
  return pySolidModelObj;
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

static struct PyModuleDef PySolidModule = {
   m_base,
   SOLID_MODULE, 
   Solid_module_doc, 
   perInterpreterStateSize, 
   PySolidModuleMethods
};

//-----------------
// PyInit_pySolid  
//-----------------
// The initialization function called by the Python interpreter 
// when the 'solid' module is loaded.
//
PyMODINIT_FUNC
PyInit_PySolid(void)
{
  //std::cout << "[PyInit_PySolid] ========== load solid module ==========" << std::endl;

  // Initialize the SolidModeler class type.
  //std::cout << "[PyInit_PySolid] Initialize the SolidModeler class type. " << std::endl;
  SetSolidModelerTypeFields(PySolidModelerClassType);
  if (PyType_Ready(&PySolidModelerClassType) < 0) {
    fprintf(stdout,"Error in PySolidModelerType");
    return SV_PYTHON_ERROR;
  }

  // Initialize the SolidModel class type.
  //std::cout << "[PyInit_PySolid] Initialize the SolidModel class type. " << std::endl;
  SetSolidModelTypeFields(PySolidModelClassType);
  if (PyType_Ready(&PySolidModelClassType) < 0) {
    fprintf(stdout,"Error in PySolidModelType");
    return SV_PYTHON_ERROR;
  }

  // Initialize the group class type.
  SetSolidGroupTypeFields(PySolidGroupClassType);
  if (PyType_Ready(&PySolidGroupClassType) < 0) {
      std::cout << "Error creating SolidGroup type" << std::endl;
      return nullptr;
  }

  // Initialize the OpenCascade class type.
  //std::cout << "[PyInit_PySolid] Initialize the OpenCascade class type. " << std::endl;
  SetOcctSolidTypeFields(PyOcctSolidClassType);
  //std::cout << "[PyInit_PySolid] Set fields done ... " << std::endl;
  if (PyType_Ready(&PyOcctSolidClassType) < 0) {
      std::cout << "Error creating OpenCascade type" << std::endl;
      return nullptr;
  }

  // Initialize the Parasolid class type.
  //std::cout << "[PyInit_PySolid] Initialize the Parasolid class type. " << std::endl;
  SetParasolidSolidTypeFields(PyParasolidSolidClassType);
  //std::cout << "[PyInit_PySolid] Set fields done ... " << std::endl;
  if (PyType_Ready(&PyParasolidSolidClassType) < 0) {
      std::cout << "Error creating PolydataSolid type" << std::endl;
      return nullptr;
  }

  // Initialize the PolyData class type.
  //std::cout << "[PyInit_PySolid] Initialize the PolyData class type. " << std::endl;
  SetPolyDataSolidTypeFields(PyPolyDataSolidClassType);
  //std::cout << "[PyInit_PySolid] Set fields done ... " << std::endl;
  if (PyType_Ready(&PyPolyDataSolidClassType) < 0) {
      std::cout << "Error creating PolydataSolid type" << std::endl;
      return nullptr;
  }

  // Initialize the solid modeling kernel class type.
  SetSolidKernelTypeFields(PySolidKernelClassType);
  if (PyType_Ready(&PySolidKernelClassType) < 0) {
      std::cout << "Error creating SolidKernel type" << std::endl;
      return nullptr;
  }

  // Create the 'solid' module. 
  //std::cout << "[PyInit_PySolid] Create the 'solid' module. " << std::endl;
  auto module = PyModule_Create(&PySolidModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing pySolid");
    return SV_PYTHON_ERROR;
  }

  // Add solid.SolidModelError exception.
  PyRunTimeErr = PyErr_NewException(SOLID_MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, SOLID_MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  // Add the 'SolidModeler' class.
  //std::cout << "[PyInit_PySolid] Add the SolidModeler class type. " << std::endl;
  Py_INCREF(&PySolidModelerClassType);
  PyModule_AddObject(module, SOLID_MODELER_CLASS, (PyObject *)&PySolidModelerClassType);

  // Add the 'SolidModel' class.
  //std::cout << "[PyInit_PySolid] Add the SolidModel class type. " << std::endl;
  Py_INCREF(&PySolidModelClassType);
  PyModule_AddObject(module, SOLID_MODEL_CLASS, (PyObject *)&PySolidModelClassType);

  // Add the 'SolidGroup' class.
  //std::cout << "[PyInit_PySolid] Add the SolidGroup class type. " << std::endl;
  Py_INCREF(&PySolidGroupClassType);
  PyModule_AddObject(module, SOLID_GROUP_CLASS, (PyObject *)&PySolidGroupClassType);

  // Add the 'OpenCascade' class.
  //std::cout << "[PyInit_PySolid] Add the OpenCascade class type. " << std::endl;
  Py_INCREF(&PyOcctSolidClassType);
  PyModule_AddObject(module, SOLID_OCCT_CLASS, (PyObject *)&PyOcctSolidClassType);

  // Add the 'Parasolid' class.
  //std::cout << "[PyInit_PySolid] Add the ParasolidSolid class type. " << std::endl;
  Py_INCREF(&PyParasolidSolidClassType);
  PyModule_AddObject(module, SOLID_PARASOLID_CLASS, (PyObject *)&PyParasolidSolidClassType);

  // Add the 'PolyData' class.
  //std::cout << "[PyInit_PySolid] Add the PolyDataSolid class type. " << std::endl;
  Py_INCREF(&PyPolyDataSolidClassType);
  PyModule_AddObject(module, SOLID_POLYDATA_CLASS, (PyObject *)&PyPolyDataSolidClassType);

  // Add the 'Kernel' class.
  Py_INCREF(&PySolidKernelClassType);
  PyModule_AddObject(module, SOLID_KERNEL_CLASS, (PyObject*)&PySolidKernelClassType);

  // Set the kernel names in the SolidKernelType dictionary.
  SetSolidKernelClassTypes(PySolidKernelClassType);

  // Initialize Open Cascade.
  InitOcct();

  //std::cout << "[PyInit_PySolid] Done. " << std::endl;

  return module;
}

#endif

