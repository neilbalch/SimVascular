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

// The functions defined here implement the SV Python API path i/o module. 
//
// The module name is 'pathio'. 
//
// [TODO:DavP] is this module ever used?
//
#include "SimVascular.h"
#include "SimVascular_python.h"
#include "Python.h"
#include "sv_PyUtils.h"

#include "sv3_PathIO.h"
#include "sv3_PathIO_init_py.h"

#include <stdio.h>
#include <string.h>
#include <array>
#include <iostream>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif
#include "sv2_globals.h"

using sv3::PathIO;
using sv3::PathGroup;
using sv3::PathElement;

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

//-----------
// GetPathIO 
//-----------
// Get the pathio object from pyPathIO.
//
PathIO * 
GetPathIO(SvPyUtilApiFunction& api, pyPathIO* self) 
{
  auto pathIO = self->pathio;
  if (pathIO == NULL) {
      api.error("The pathIO object has not been created.");
      return nullptr;
  }

  return pathIO;
}

//-------------------
// GetRepositoryData 
//-------------------
// Get repository data of the given type. 
//
static cvRepositoryData *
GetRepositoryData(SvPyUtilApiFunction& api, char* name, RepositoryDataT dataType)
{
  auto data = gRepository->GetObject(name);
  if (data == NULL) {
      api.error("'"+std::string(name)+"' is not in the repository.");
      return nullptr;
  }
  if (gRepository->GetType(name) != dataType) { 
      auto typeStr = std::string(RepositoryDataT_EnumToStr(dataType));
      api.error("'" + std::string(name) + "' does not have type '" + typeStr + "'.");
      return nullptr;
  }

  return data;
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//----------------------
// sv4PathIO_new_object 
//----------------------
//
PyDoc_STRVAR(sv4PathIO_new_object_doc,
  "new_object(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
sv4PathIO_new_object(pyPathIO* self, PyObject* args)
{
  PathIO *pathio = new PathIO();
  Py_INCREF(pathio);
  self->pathio = pathio;
  Py_DECREF(pathio);
  return SV_PYTHON_OK; 
}

//---------------------------
// sv4PathIO_read_path_group 
//---------------------------
//
PyDoc_STRVAR(sv4PathIO_read_path_group_doc,
  "read_path_group(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
sv4PathIO_read_path_group(pyPathIO* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
    char* objName =NULL;
    char* fn=NULL;

    if (!PyArg_ParseTuple(args,api.format, &objName, &fn)) {
      return api.argsError();
    }

    auto pathIO = GetPathIO(api, self);
    if (pathIO == NULL) {
      return nullptr;
    }
    
    // Make sure the specified result object does not exist:
    if (gRepository->Exists(objName)) {
      api.error("The object '"+std::string(objName)+"' is already in the repository.");
      return nullptr;
    }

    // Read a path group file.
    std::string str(fn);
    auto pthGrp = pathIO->ReadFile(str);
    if (pthGrp == NULL) {
      api.error("Error reading the path group file '"+str+"'.");
      return nullptr;
    }

    // Register the object:
    if (!gRepository->Register(objName, pthGrp)) {
      delete pthGrp;
      api.error("Error adding the path group '" + std::string(objName) + "' to the repository.");
      return nullptr;
    }
    
    Py_INCREF(pathIO);
    self->pathio=pathIO;
    Py_DECREF(pathIO);
    return SV_PYTHON_OK; 
}

//----------------------------
// sv4PathIO_write_path_group 
//----------------------------
//
PyDoc_STRVAR(sv4PathIO_write_path_group_doc,
  "write_path_group(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
sv4PathIO_write_path_group(pyPathIO* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
    char* objName=NULL;
    char* fn=NULL;

    if (!PyArg_ParseTuple(args, api.format, &objName, &fn)) {
      return api.argsError();
    }

    auto pathIO = GetPathIO(api, self);
    if (pathIO == NULL) {
      return nullptr;
    }

    auto rd = GetRepositoryData(api, objName, PATHGROUP_T);
    if (rd == nullptr) {
      return nullptr;
    }
    auto pathGrp = dynamic_cast<PathGroup*> (rd);
    
    // Write path group.
    std::string str(fn);
    if(self->pathio->Write(str,pathGrp)==SV_ERROR) {
      api.error("Error writing the path group file '"+str+"'.");
      return nullptr;
    }
    
    return SV_PYTHON_OK; 
}

//----------------------
// sv4PathIO_write_path 
//----------------------
//
PyDoc_STRVAR(sv4PathIO_write_path_doc,
  "write_path(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
sv4PathIO_write_path(pyPathIO* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
    char* objName=NULL;
    char* fn=NULL;

    if (!PyArg_ParseTuple(args,api.format,&objName, &fn)) {
      return api.argsError();
    }

    auto pathIO = GetPathIO(api, self);
    if (pathIO == NULL) {
      return nullptr;
    }

    auto rd = GetRepositoryData(api, objName, PATHGROUP_T);
    if (rd == nullptr) {
      return nullptr;
    }
    auto path = dynamic_cast<PathElement*>(rd);
    
    // Write the path group.
    //
    auto pathGrp = new PathGroup();
    pathGrp->Expand(1);
    pathGrp->SetPathElement(path);
    std::string str(fn);
    if(self->pathio->Write(str,pathGrp)==SV_ERROR) {
      api.error("Error writing the path file '"+str+"'.");
      return nullptr;
    }
    
    return SV_PYTHON_OK; 
} 

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "pathio";
static char* MODULE_PATHIO_CLASS = "PathIO";
static char* MODULE_EXCEPTION = "pathio.PathIOException";
static char* MODULE_EXCEPTION_OBJECT = "PathIOException";

PyDoc_STRVAR(PathIO_doc, "pathio module functions");

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyPathIO();
#elif PYTHON_MAJOR_VERSION == 3
PyMODINIT_FUNC PyInit_pyPathIO();
#endif

int PathIO_pyInit()
{
#if PYTHON_MAJOR_VERSION == 2
    initpyPathIO();
#elif PYTHON_MAJOR_VERSION == 3
    PyInit_pyPathIO();
#endif
  return SV_OK;
}

//----------------------
// PathIO class methods
//----------------------
//
static PyMethodDef pyPathIO_methods[] = {

  {"new_object", (PyCFunction)sv4PathIO_new_object,METH_VARARGS,sv4PathIO_new_object_doc},

  {"read_path_group", (PyCFunction)sv4PathIO_read_path_group, METH_VARARGS, sv4PathIO_read_path_group_doc},

  {"write_path", (PyCFunction)sv4PathIO_write_path, METH_VARARGS, sv4PathIO_write_path_doc},

  {"write_path_group", (PyCFunction)sv4PathIO_write_path_group, METH_VARARGS, sv4PathIO_write_path_group_doc},

  {NULL,NULL}

};

static int 
pyPathIO_init(pyPathIO* self, PyObject* args)
{
  //fprintf(stdout,"pyPathIO initialized.\n");
  return SV_OK;
}

//--------------
// pyPathIOType
//--------------
// Define the PathIO class.
//
static PyTypeObject pyPathIOType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "pathio.PathIO",            /* tp_name */
  sizeof(pyPathIO),             /* tp_basicsize */
  0,                         /* tp_itemsize */
  0,                         /* tp_dealloc */
  0,                         /* tp_print */
  0,                         /* tp_getattr */
  0,                         /* tp_setattr */
  0,                         /* tp_compare */
  0,                         /* tp_repr */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash */
  0,                         /* tp_call */
  0,                         /* tp_str */
  0,                         /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
  "PathIO  objects",           /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  pyPathIO_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)pyPathIO_init,                            /* tp_init */
  0,                         /* tp_alloc */
  0,                  /* tp_new */
};

//-----------------------
// pathio module methods
//-----------------------
//
static PyMethodDef pyPathIOModule_methods[] =
{
    {NULL,NULL}
};

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
static struct PyModuleDef pyPathIOModule = {
   m_base,
   MODULE_NAME, 
   PathIO_doc, 
   perInterpreterStateSize, 
   pyPathIOModule_methods
};

//-----------------
// PyInit_pyPathIO
//-----------------
//
PyMODINIT_FUNC PyInit_pyPathIO()
{
  if (gRepository==NULL) {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from sv3_PathIO_init\n");
  }

  // Initialize
  pyPathIOType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&pyPathIOType)<0) {
    fprintf(stdout,"Error in pyPathIOType\n");
    return SV_PYTHON_ERROR;
  }

  auto module = PyModule_Create(&pyPathIOModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing pyPathIO\n");
    return SV_PYTHON_ERROR;
  }

  // Add an exception for this module.
  PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, MODULE_EXCEPTION_OBJECT,PyRunTimeErr);

  // Add the PathIO class.
  Py_INCREF(&pyPathIOType);
  PyModule_AddObject(module, MODULE_PATHIO_CLASS, (PyObject*)&pyPathIOType);

  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

//----------------
//initpyPathIO
//----------------
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyPathIO()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from cv_mesh_init\n");
  }

  // Initialize
  pyPathIOType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&pyPathIOType)<0)
  {
    fprintf(stdout,"Error in pyPathIOType\n");
    return;
  }
  PyObject* pythonC;
  pythonC = Py_InitModule("pyPathIO",pyPathIOModule_methods);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyPathIO\n");
    return;
  }
  PyRunTimeErr = PyErr_NewException("pyPathIO.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  Py_INCREF(&pyPathIOType);
  PyModule_AddObject(pythonC,"pyPathIO",(PyObject*)&pyPathIOType);
  return ;

}
#endif

