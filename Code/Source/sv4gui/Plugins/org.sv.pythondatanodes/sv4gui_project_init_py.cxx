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

// The functions defined here implement the SV Python API project Module. 
//
// The module name is 'project'. The module defines a 'Path' class used
// to store path data. The 'Path' class cannot be imported and must
// be used prefixed by the module name. For example
//
//     path = path.Path()
//
// A Python exception sv.path.PathError is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    try:
//    except sv.path.PathError:
//
#include "SimVascular.h"
#include "SimVascular_python.h"
#include "Python.h"

#include "sv4gui_project_init_py.h"
#include "sv_PyUtils.h"

#include <stdio.h>
#include <string.h>
#include <array>
#include <iostream>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
#include "sv_PolyData.h"
#include "vtkSmartPointer.h"
#include "sv2_globals.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject* PyRunTimeErr;

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////


//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//-----------------
// sv4Project_open  
//-----------------
//
PyDoc_STRVAR(sv4Project_open_doc,
  "sv4Project_open(point) \n\ 
   \n\
   Add a control point to a path. \n\
   \n\
   Args: \n\
     point (list[x,y,z]): A list of three floats represent the 3D coordinates of the control point. \n\
");

static PyObject * 
sv4Project_open(PyProject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* fileNameArg;

  if (!PyArg_ParseTuple(args, api.format, &fileNameArg)) {
      return api.argsError();
  }

  return SV_PYTHON_OK; 
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "project";
static char* MODULE_PATH_CLASS = "Project";
static char* MODULE_EXCEPTION = "project.ProjectError";
static char* MODULE_EXCEPTION_OBJECT = "ProjectError";

PyDoc_STRVAR(Project_doc, "project module functions");

//------------------
// PyProjectMethods 
//------------------
// Project class methods.
//
static PyMethodDef PyProjectMethods[] = {

  {"open", (PyCFunction)sv4Project_open, METH_VARARGS, sv4Project_open_doc },

  {NULL,NULL}
};

//-----------------------------------
// Define the PyProjectType type object
//-----------------------------------
// Define the Python type object that stores Project data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
PyTypeObject PyProjectType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  "project.Project", 
  sizeof(PyProject)
};

//------------
// PyProjectInit
//------------
// This is the __init__() method for the Project class. 
//
// This function is used to initialize an object after it is created.
//
static int
PyProjectInit(PyProject* self, PyObject* args, PyObject *kwds)
{
  static int numObjs = 1;
  std::cout << "[PyProjectInit] New Project object: " << numObjs << std::endl;
/*
  self->project = new ProjectElement();
  self->id = numObjs;
  numObjs += 1;
*/
  return 0;
}

//-----------
// PyProjectNew 
//-----------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyProjectNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyProjectNew] PyProjectNew " << std::endl;
  auto self = (PyProject*)type->tp_alloc(type, 0);
  if (self != NULL) {
      self->id = 1;
  }

  return (PyObject *) self;
}

//--------
// PyProject
//--------
//
static void
PyProjectDealloc(PyProject* self)
{
  std::cout << "[PyProjectDealloc] Free PyProject" << std::endl;
/*
  delete self->project;
  Py_TYPE(self)->tp_free(self);
*/
}

//-------------------
// SetProjectTypeFields 
//-------------------
// Set the Python type object fields that stores Project data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetPyProjectTypeFields(PyTypeObject& projectType)
{
  // Doc string for this type.
  projectType.tp_doc = "Project  objects";
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  projectType.tp_new = PyProjectNew;
  projectType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  projectType.tp_init = (initproc)PyProjectInit;
  projectType.tp_dealloc = (destructor)PyProjectDealloc;
  projectType.tp_methods = PyProjectMethods;
}

//--------------
// CreatePyProject
//--------------
// 
PyObject *
CreatePyProject()
{
/*
  std::cout << "[CreatePyProject] Create Project object ... " << std::endl;
  auto projectObj = PyObject_CallObject((PyObject*)&PyProjectType, NULL);
  auto pyProject = (PyProject*)projectObj;

  if (project != nullptr) {
      delete pyProject->project; 
      pyProject->project = project; 
  }
  std::cout << "[CreatePyProject] pyProject id: " << pyProject->id << std::endl;
  return projectObj;
*/
}

//----------------------
// PyProjectModule_methods
//----------------------
//
static PyMethodDef PyProjectModuleMethods[] =
{
    {NULL,NULL}
};

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
//
static struct PyModuleDef PyProjectModule = {
   m_base,
   MODULE_NAME,
   Project_doc, 
   perInterpreterStateSize,
   PyProjectModuleMethods
};

//---------------
// PyInit_PyProject
//---------------
// The initialization function called by the Python interpreter when the module is loaded.
//
PyMODINIT_FUNC PyInit_PyProject()
{
  std::cout << "========== PyInit_PyProject ==========" << std::endl;

  // Setup the Project object type.
  //
  SetPyProjectTypeFields(PyProjectType);
  if (PyType_Ready(&PyProjectType) < 0) {
    fprintf(stdout, "Error initilizing ProjectType \n");
    return SV_PYTHON_ERROR;
  }

  // Create the project module.
  auto module = PyModule_Create(&PyProjectModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing 'project' module \n");
    return SV_PYTHON_ERROR;
  }

  // Add project.ProjectException exception.
  //
  PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  // Add Project class.
  Py_INCREF(&PyProjectType);
  PyModule_AddObject(module, MODULE_PATH_CLASS, (PyObject*)&PyProjectType);
  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyProject()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from cv_mesh_init\n");
  }

  // Initialize
  pyProjectType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&pyProjectType)<0)
  {
    fprintf(stdout,"Error in pyProjectType\n");
    return;
  }
  PyObject* pythonC;
  pythonC = Py_InitModule("pyProject",pyProjectModule_methods);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyProject\n");
    return;
  }
  PyRunTimeErr = PyErr_NewException("pyProject.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  Py_INCREF(&pyProjectType);
  PyModule_AddObject(pythonC,"pyProject",(PyObject*)&pyProjectType);
  return ;

}
#endif

