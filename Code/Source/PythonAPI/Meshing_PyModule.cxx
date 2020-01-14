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

// The functions defined here implement the SV Python API meshing module. 
//
// A Python exception sv.meshing.MeshingError is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.meshing.MeshingError:
//
#include "SimVascular.h"
#include "SimVascular_python.h"
#include "sv_MeshSystem.h"
#include "sv_MeshObject.h"
#include "sv_mesh_init_py.h"
#include "vtkPythonUtil.h"
#include "sv_PyUtils.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
#include "sv_PolyData.h"
#include "sv_arg.h"
#include "sv_VTK.h"
#include "sv_misc_utils.h"
#include "Python.h"

// Needed for Windows.
#ifdef GetObject
#undef GetObject
#endif

#include <iostream>
#include "sv2_globals.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

// Include mesh Kernel class that defines a map between 
// mesh kernel name and enum type.
//
#include "MeshingKernel_PyClass.cxx"

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

////////////////////////////////////////////////////////
//          M o d u l e   D e f i n i t i o n         //
////////////////////////////////////////////////////////

//---------------------
// PyMeshModuleMethods
//---------------------
//
static PyMethodDef PyMeshingModuleMethods[] =
{
/*
  {"logging_off", (PyCFunction)Mesh_logging_off, METH_NOARGS, Mesh_logging_off_doc },

  {"logging_on", (PyCFunction)Mesh_logging_on, METH_VARARGS, Mesh_logging_on_doc },

  {"set_kernel", (PyCFunction)Mesh_set_kernel, METH_VARARGS, Mesh_set_kernel_doc },
*/

  {NULL, NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MESHING_MODULE = "meshing";
static char* MESHING_MODULE_EXCEPTION = "meshing.MeshingError";
static char* MESHING_MODULE_EXCEPTION_OBJECT = "MeshingError";

PyDoc_STRVAR(Meshing_module_doc, "meshing module functions");

// Include mesh.Mesh definition.
#include "MeshGenerator_PyClass.cxx"

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

static struct PyModuleDef PyMeshingModule = {
   m_base,
   MESHING_MODULE, 
   Meshing_module_doc,
   perInterpreterStateSize, 
   PyMeshingModuleMethods
};

//------------------
// PyInit_PyMeshing
//------------------
// The initialization function called by the Python interpreter when the module is loaded.
//
PyMODINIT_FUNC 
PyInit_PyMeshing()
{
  std::cout << "[PyInit_PyMeshing] ========== load meshing module ==========" << std::endl;

  // Initialize the mesh generator class type.
  SetMeshGeneratorTypeFields(PyMeshGeneratorClassType);
  if (PyType_Ready(&PyMeshGeneratorClassType) < 0) {
      std::cout << "Error creating Meshing type" << std::endl;
      return nullptr;
  }

  // Initialize the meshing kernel class type.
  SetMeshingKernelTypeFields(PyMeshingKernelClassType);
  if (PyType_Ready(&PyMeshingKernelClassType) < 0) {
      std::cout << "Error creating MeshingKernel type" << std::endl;
      return nullptr;
  }

  // Create the 'meshing' module. 
  std::cout << "[PyInit_PyMeshing] Create the 'meshing' module. " << std::endl;
  auto module = PyModule_Create(&PyMeshingModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing meshing module.\n");
    return SV_PYTHON_ERROR;
  }

  // Add meshing.MeshingException exception.
  PyRunTimeErr = PyErr_NewException(MESHING_MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, MESHING_MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  // Add the 'meshing.MeshGenerator' class.
  Py_INCREF(&PyMeshGeneratorClassType);
  PyModule_AddObject(module, MESH_GENERATOR_CLASS, (PyObject*)&PyMeshGeneratorClassType);

  // Add the 'mesh.Kernel' class.
  Py_INCREF(&PyMeshingKernelClassType);
  PyModule_AddObject(module, MESHING_KERNEL_CLASS, (PyObject*)&PyMeshingKernelClassType);

  // Set the kernel names in the MeshingKernelType dictionary.
  SetMeshingKernelClassTypes(PyMeshingKernelClassType);

  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

//----------------
//initpyMeshObject
//----------------
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyMeshObject()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from cv_mesh_init\n");
  }
  int (*kernel)(cvMeshObject::KernelType, cvMeshSystem*)=(&cvMeshSystem::RegisterKernel);
  if (Py_BuildValue("i",kernel)==nullptr)
  {
    fprintf(stdout,"Unable to create MeshSystemRegistrar\n");
    return;

  }
  if(PySys_SetObject("MeshSystemRegistrar",Py_BuildValue("i",kernel))<0)
  {
    fprintf(stdout, "Unable to register MeshSystemRegistrar\n");
    return;

  }
  // Initialize
  cvMeshSystem::SetCurrentKernel( cvMeshObject::KERNEL_INVALID );

  pyMeshObjectType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&pyMeshObjectType)<0)
  {
    fprintf(stdout,"Error in pyMeshObjectType\n");
    return;

  }
  PyObject* pythonC;
  pythonC = Py_InitModule("pyMeshObject",pyMeshObjectModule_methods);

  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyMeshObject\n");
    return;

  }
  PyRunTimeErr = PyErr_NewException("pyMeshObject.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  Py_INCREF(&pyMeshObjectType);
  PyModule_AddObject(pythonC,"pyMeshObject",(PyObject*)&pyMeshObjectType);
  return;

}
#endif

