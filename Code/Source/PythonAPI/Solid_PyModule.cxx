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
#include <map>

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

//--------------
// SolidCtorMap
//--------------
// Define an object factory for creating objects for Solid derived classes.
//
using SolidCtorMapType = std::map<SolidModel_KernelT, std::function<cvSolidModel*()>>;
SolidCtorMapType SolidCtorMap = {
    {SolidModel_KernelT::SM_KT_OCCT, []() -> cvSolidModel* { return new cvOCCTSolidModel(); } },
    {SolidModel_KernelT::SM_KT_POLYDATA, []() -> cvSolidModel* { return new cvPolyDataSolid(); } },
};

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

// createSolidModelType() references PySolidModelType and is used before 
// it is defined so we need to define its prototype here.
static PySolidModel * CreateSolidModelType();

////////////////////////////////////////////////////////
//          M o d u l e   D e f i n i t i o n         //
////////////////////////////////////////////////////////

//----------------
// PySolidMethods 
//----------------
// Methods for the 'solid' module.
//
static PyMethodDef PySolidModuleMethods[] = {
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
#include "Solid_PyClass.cxx"
#include "SolidPolyData_PyClass.cxx"

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
  std::cout << "[PyInit_PySolid] ========== load solid module ==========" << std::endl;

  // Initialize the SolidModel class type.
  std::cout << "[PyInit_PySolid] Initialize the SolidModel class type. " << std::endl;
  SetSolidModelTypeFields(PySolidModelClassType);
  if (PyType_Ready(&PySolidModelClassType) < 0) {
    fprintf(stdout,"Error in PySolidModelType");
    return SV_PYTHON_ERROR;
  }

  // Initialize the PolyData class type.
  std::cout << "[PyInit_PySolid] Initialize the PolyData class type. " << std::endl;
  SetPolyDataSolidTypeFields(PyPolyDataSolidClassType);
  std::cout << "[PyInit_PySolid] Set fields done ... " << std::endl;
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
  std::cout << "[PyInit_PySolid] Create the 'solid' module. " << std::endl;
  auto module = PyModule_Create(&PySolidModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing pySolid");
    return SV_PYTHON_ERROR;
  }

  // Add solid.SolidModelError exception.
  PyRunTimeErr = PyErr_NewException(SOLID_MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, SOLID_MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  // Add the 'SolidModel' class.
  std::cout << "[PyInit_PySolid] Add the SolidModel class type. " << std::endl;
  Py_INCREF(&PySolidModelClassType);
  PyModule_AddObject(module, SOLID_MODEL_CLASS, (PyObject *)&PySolidModelClassType);

  // Add the 'PolyData' class.
  std::cout << "[PyInit_PySolid] Add the PolyDataSolid class type. " << std::endl;
  Py_INCREF(&PyPolyDataSolidClassType);
  PyModule_AddObject(module, SOLID_POLYDATA_CLASS, (PyObject *)&PyPolyDataSolidClassType);

  // Add the 'Kernel' class.
  Py_INCREF(&PySolidKernelClassType);
  PyModule_AddObject(module, SOLID_KERNEL_CLASS, (PyObject*)&PySolidKernelClassType);

  // Set the kernel names in the SolidKernelType dictionary.
  SetSolidKernelClassTypes(PySolidKernelClassType);

  std::cout << "[PyInit_PySolid] Done. " << std::endl;

  return module;
}

#endif


//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC initpySolid(void);
int Solid_pyInit()
{ 
  initpySolid();
  return SV_OK;
}

PyMODINIT_FUNC
initpySolid(void)
{
    // Initialize-gRepository
  if (gRepository ==NULL)
  {
    gRepository=new cvRepository();
    fprintf(stdout,"New gRepository created from cv_solid_init\n");
  }
  //Initialize-gCurrentKernel
  cvSolidModel::gCurrentKernel = SM_KT_INVALID;
  #ifdef SV_USE_PARASOLID
  cvSolidModel::gCurrentKernel = SM_KT_PARASOLID;
  #endif

  PySolidModelType.tp_new=PyType_GenericNew;
  pycvFactoryRegistrarType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PySolidModelType)<0)
  {
    fprintf(stdout,"Error in PySolidModelType");
    return;
  }
  if (PyType_Ready(&pycvFactoryRegistrarType)<0)
  {
    fprintf(stdout,"Error in PySolidModelType");
    return;
  }
  //Init our defined functions
  PyObject *pythonC;
  pythonC = Py_InitModule("pySolid", pySolid_methods);
  if (pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pySolid");
    return;
  }

  PyRunTimeErr=PyErr_NewException("pySolid.error",NULL,NULL);
  PyModule_AddObject(pythonC, "error",PyRunTimeErr);
  Py_INCREF(&PySolidModelType);
  Py_INCREF(&pycvFactoryRegistrarType);
  PyModule_AddObject(pythonC, "PySolidModel", (PyObject *)&PySolidModelType);
  PyModule_AddObject(pythonC, "pyCvFactoryRegistrar", (PyObject *)&pycvFactoryRegistrarType);

  pycvFactoryRegistrar* tmp = PyObject_New(pycvFactoryRegistrar, &pycvFactoryRegistrarType);
  tmp->registrar = (cvFactoryRegistrar *)&cvSolidModel::gRegistrar;
  PySys_SetObject("solidModelRegistrar", (PyObject *)tmp);


}
#endif

