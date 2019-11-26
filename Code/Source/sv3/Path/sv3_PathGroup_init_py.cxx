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

// The functions defined here implement the SV Python API path group module. 
//
// The module name is 'path_group'. The module defines a 'PathGroup' class used
// to store path group data. The 'PathGroup' class cannot be imported and must
// be used prefixed by the module name. For example
//
//     aorta_path_group = path_group.PathGroup()
//
// A Python exception sv.path_group.PathGroupError is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    try:
//    except sv.path_group.PathGroupError:
//
#include "SimVascular.h"
#include "SimVascular_python.h"
#include "Python.h"
#include "sv_PyUtils.h"

#include "sv3_PathGroup.h"
#include "sv3_PathGroup_init_py.h"
#include "sv3_PathElement.h"
#include "sv3_PathElement_init_py.h"
#include "sv3_PathIO.h"

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
using sv3::PathGroup;
using sv3::PathElement;

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

static PyObject * CreatePyPathGroup(PathGroup* pathGroup);

// Define a map between method name and enum type.
static std::map<std::string, PathElement::CalculationMethod> methodNameTypeMap =
{
    {"Spacing", PathElement::CONSTANT_SPACING},
    {"Subdivision", PathElement::CONSTANT_SUBDIVISION_NUMBER},
    {"Total", PathElement::CONSTANT_TOTAL_NUMBER}
};


//-----------------------
// PathGroup_set_path 
//-----------------------
//
PyDoc_STRVAR(PathGroup_set_path_doc,
  "set_path(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_path(PyPathGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("si", PyRunTimeErr, __func__);
  char* objName;
  int index=-2;
  cvRepositoryData *rd;
  RepositoryDataT type;

  if (!PyArg_ParseTuple(args, api.format, &objName, &index)) {
     return api.argsError();
  }

/*
    
        // Retrieve source object:
    rd = gRepository->GetObject( objName );
    char r[2048];
    if ( rd == NULL )
    {
        r[0] = '\0';
        sprintf(r, "couldn't find object %s", objName);
        PyErr_SetString(PyRunTimeErr,r);
        
    }
    
    type = rd->GetType();
    
    if ( type != PATH_T )
    {
        r[0] = '\0';
        sprintf(r, "%s not a path object", objName);
        PyErr_SetString(PyRunTimeErr,r);
        
    }
    
    PathElement* path = static_cast<PathElement*> (rd);
    if (path==NULL)
    {
        PyErr_SetString(PyRunTimeErr,"Path does not exist.");
        
    }
    
    int timestepSize = self->pathGroup->GetTimeSize();
    if (index+1>=timestepSize)
    {
        self->pathGroup->Expand(index);
        self->pathGroup->SetPathElement(path,index);
    }
    else
        self->pathGroup->SetPathElement(path, index);
            
*/
    return Py_None; 
}

//----------------------------
// PathGroup_get_time_size 
//----------------------------
//
// [TODO:DaveP] bad method name: get_number_time_steps() ?
//
PyDoc_STRVAR(PathGroup_get_time_size_doc,
  "set_path(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_time_size(PyPathGroup* self, PyObject* args)
{
  int timestepSize = self->pathGroup->GetTimeSize();
  return Py_BuildValue("i",timestepSize); 
}

//-----------------------
// PathGroup_get_path 
//-----------------------
PyDoc_STRVAR(PathGroup_get_path_doc,
  "get_path(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_path(PyPathGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int index;
  char* pathName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &index)) {
     return api.argsError();
  }
        
  auto pathGroup = self->pathGroup;
  int numPaths = pathGroup->GetTimeSize();

  if (index > numPaths-1) {
      api.error("The index argument '" + std::to_string(index) + "' is must be between 0 and " +
        std::to_string(numPaths-1));
      return nullptr;
  }

  auto path = pathGroup->GetPathElement(index);
  return CreatePyPath(path);
}

//-----------------------------
// PathGroup_get_path_group_id 
//-----------------------------
//
PyDoc_STRVAR(PathGroup_get_path_group_id_doc,
  "get_path_group_id(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_path_group_id(PyPathGroup* self, PyObject* args)
{
  int id = self->pathGroup->GetPathID();
  return Py_BuildValue("i",id); 
}
    
//--------------------------------
// PathGroup_set_path_group_id
//--------------------------------
//
PyDoc_STRVAR(PathGroup_set_path_group_id_doc,
  "set_path_group_id(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_path_group_id(PyPathGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int id;

  if (!PyArg_ParseTuple(args, api.format, &id)) {
      return api.argsError();
  }
    
  self->pathGroup->SetPathID(id);
  return Py_None;
}

//--------------------------
// PathGroup_set_spacing 
//--------------------------
//
PyDoc_STRVAR(PathGroup_set_spacing_doc,
  "set_spacing(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_spacing(PyPathGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__);
  double spacing;

  if (!PyArg_ParseTuple(args, api.format, &spacing)) {
     return api.argsError();
  }
    
  self->pathGroup->SetSpacing(spacing);
  return Py_None;
}

//--------------------------
// PathGroup_get_spacing 
//--------------------------
//
PyDoc_STRVAR(PathGroup_get_spacing_doc,
  "get_spacing(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_spacing(PyPathGroup* self, PyObject* args)
{
  double spacing = self->pathGroup->GetSpacing();
  return Py_BuildValue("d",spacing); 
}

//-------------------------
// PathGroup_set_method 
//-------------------------
//
PyDoc_STRVAR(PathGroup_set_method_doc,
  "set_method(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_method(PyPathGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* methodName;

  if (!PyArg_ParseTuple(args, api.format, &methodName)) {
     return api.argsError();
  }

  PathElement::CalculationMethod method;

  try {
      method = methodNameTypeMap.at(std::string(methodName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown method name '" + std::string(methodName) + "'." +
          " Valid names are: Spacing, Subdivision or Total.";
      api.error(msg);
      return nullptr;
  }

  self->pathGroup->SetMethod(method);
  return Py_None;
}

//----------------------
// PathGroup_get_method 
//----------------------
//
PyDoc_STRVAR(PathGroup_get_method_doc,
  "get_method(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_method(PyPathGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  PathElement::CalculationMethod method = self->pathGroup->GetMethod();
  std::string methodName;

  for (auto const& element : methodNameTypeMap) { 
      if (method == element.second) {
          methodName = element.first;
          break; 
      }
  }

  if (methodName == "") {
      api.error("No method is set.");
      return nullptr;
  }
    
  return Py_BuildValue("s", methodName.c_str());
}

//----------------------------------
// PathGroup_set_calculation_number 
//----------------------------------
//
PyDoc_STRVAR(PathGroup_set_calculation_number_doc,
  "set_calculation_number(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_calculation_number(PyPathGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int number;

  if (!PyArg_ParseTuple(args, api.format, &number)) {
     return api.argsError();
  }
    
  // [TODO:DaveP] need to check valume of number.
  self->pathGroup->SetCalculationNumber(number);
  return Py_None;
}

//----------------------------------
// PathGroup_get_calculation_number 
//----------------------------------
//
PyDoc_STRVAR(PathGroup_get_calculation_number_doc,
  "get_calculation_number(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_calculation_number(PyPathGroup* self, PyObject* args)
{
  int number = self->pathGroup->GetCalculationNumber();
  return Py_BuildValue("i",number);
}

//----------------
// PathGroup_read
//----------------
//
PyDoc_STRVAR(PathGroup_read_doc,
  "read(file_name) \n\ 
   \n\
   Read an SV path .pth file and create a PathGroup from it. \n\
   \n\
   Args: \n\
     file_name (str): The name of the SV .pth file.\n\
");

static PyObject * 
PathGroup_read(PyObject* self, PyObject* args)
{
  std::cout << "========== PathGroup_read ==========" << std::endl;
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fileName;

  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      return api.argsError();
  }

  std::cout << "[PathGroup_read] fileName: " << fileName << std::endl;
  sv3::PathGroup* pathGroup;

  try {
      pathGroup = sv3::PathIO().ReadFile(fileName);
      if (pathGroup == nullptr) { 
          api.error("Error reading file '" + std::string(fileName) + "'.");
          return nullptr;
      }
      int numElements = pathGroup->GetTimeSize();
      std::cout << "[PathGroup_read] numElements: " << numElements << std::endl;

  } catch (const std::exception& readException) {
      api.error("Error reading file '" + std::string(fileName) + "': " + readException.what());
      return nullptr;
  }

  /*
  for (int i=0; i<svPathGrp->GetTimeSize(); i++) {
        path->SetPathElement(static_cast<sv4guiPathElement*>(svPathGrp->GetPathElement(i)),i);
  }
  */

  return CreatePyPathGroup(pathGroup);
}


////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "path_group";
static char* MODULE_PATH_GROUP_CLASS = "PathGroup";
static char* MODULE_EXCEPTION = "path_group.PathGroupError";
static char* MODULE_EXCEPTION_OBJECT = "PathGroupError";

PyDoc_STRVAR(pathgroup_doc, "path_group functions");

//--------------------
// PyPathGroupMethods 
//--------------------
// Define the methods for the PathGroup class.
//
static PyMethodDef PyPathGroupMethods[] = {

  {"get_calculation_number", (PyCFunction)PathGroup_get_calculation_number, METH_NOARGS, PathGroup_get_calculation_number_doc},

  {"get_method", (PyCFunction)PathGroup_get_method, METH_NOARGS, PathGroup_get_method_doc}, 

  {"get_path", (PyCFunction)PathGroup_get_path, METH_VARARGS, PathGroup_get_path_doc},

  {"get_path_group_id", (PyCFunction)PathGroup_get_path_group_id,METH_VARARGS,PathGroup_get_path_group_id_doc},

  {"get_spacing", (PyCFunction)PathGroup_get_spacing, METH_NOARGS, PathGroup_get_spacing_doc},

  {"get_time_size", (PyCFunction)PathGroup_get_time_size, METH_NOARGS, PathGroup_get_time_size_doc},

  {"set_calculation_number", (PyCFunction)PathGroup_set_calculation_number, METH_NOARGS, PathGroup_set_calculation_number_doc},

  {"set_method", (PyCFunction)PathGroup_set_method, METH_NOARGS, PathGroup_set_method_doc},

  {"set_path", (PyCFunction)PathGroup_set_path, METH_VARARGS, PathGroup_set_path_doc},

  {"set_path_group_id", (PyCFunction)PathGroup_set_path_group_id, METH_VARARGS,PathGroup_set_path_group_id_doc},

  {"set_spacing", (PyCFunction)PathGroup_set_spacing, METH_NOARGS,PathGroup_set_spacing_doc},

  {NULL,NULL}
};

//----------------------------------
// Define the PyPathGroupType class 
//----------------------------------
// Define the Python type that stores PathGroup data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static PyTypeObject PyPathGroupType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  "path_group.PathGroup",     
  sizeof(PyPathGroup)
};

//------------------
// PyPathGroup_init
//------------------
//
// This is the __init__() method for the PathGroup class. 
//
// This function is used to initialize an object after it is created.
//
static int 
PyPathGroupInit(PyPathGroup* self, PyObject* args)
{
  static int numObjs = 1;
  std::cout << "[PyPathGroupInit] New PathGroup object: " << numObjs << std::endl;
  //self->path = new PathElement();
  //self->id = numObjs;
  numObjs += 1;
  return 0;
}

//----------------
// PyPathGroupNew 
//----------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyPathGroupNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyPathGroupNew] PyPathGroupNew " << std::endl;
  auto self = (PyPath*)type->tp_alloc(type, 0);
  if (self != NULL) {
      self->id = 1;
  }

  return (PyObject *) self;
}

//--------------------
// PyPathGroupDealloc 
//--------------------
//
static void
PyPathGroupDealloc(PyPathGroup* self)
{
  std::cout << "[PyPathGroupDealloc] Free PyPathGroup" << std::endl;
  delete self->pathGroup;
  Py_TYPE(self)->tp_free(self);
}

//------------------------
// SetPathGroupTypeFields 
//------------------------
// Set the Python type object fields that stores Path data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetPyPathGroupTypeFields(PyTypeObject& pathType)
{
  // Doc string for this type.
  pathType.tp_doc = "PathGroup  objects";
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  pathType.tp_new = PyPathGroupNew;
  pathType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  pathType.tp_init = (initproc)PyPathGroupInit;
  pathType.tp_dealloc = (destructor)PyPathGroupDealloc;
  pathType.tp_methods = PyPathGroupMethods;
}

//-------------------
// CreatePyPathGroup
//-------------------
// Create a PyPathGroupType object.
//
// If the 'pathGroup' argument is not null then use that 
// for the PyPathGroupType.pathGroup data.
//
PyObject *
CreatePyPathGroup(PathGroup* pathGroup)
{
  std::cout << "[CreatePyPathGroup] Create PathGroup object ... " << std::endl;
  auto pathGroupObj = PyObject_CallObject((PyObject*)&PyPathGroupType, NULL);
  auto pyPathGroup = (PyPathGroup*)pathGroupObj;

  if (pathGroup != nullptr) {
      delete pyPathGroup->pathGroup;
      pyPathGroup->pathGroup = pathGroup;
  }
  std::cout << "[CreatePyPath] pyPathGroup id: " << pyPathGroup->id << std::endl;
  return pathGroupObj;
}

// Define methods operating on the PathGroup Module level.
//
static PyMethodDef PyPathGroupModuleMethods[] =
{
  {"read", (PyCFunction)PathGroup_read, METH_VARARGS, PathGroup_read_doc},
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
static struct PyModuleDef PyPathGroupModule = {
   m_base,
   MODULE_NAME,
   pathgroup_doc, 
   perInterpreterStateSize,
   PyPathGroupModuleMethods
};

//--------------------
// PyInit_PyPathGroup
//--------------------
// The initialization function called by the Python interpreter when the module is loaded.
//
// [TODO:Davep] The global 'gRepository' is created here, as it is in all other modules init
//     function. Why is this not create in main()?
//
PyMODINIT_FUNC PyInit_PyPathGroup()
{

  if (gRepository==NULL) {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from sv3_PathGroup_init\n");
  }

  // Create PathGoup type. 
  //
  SetPyPathGroupTypeFields(PyPathGroupType);
  if (PyType_Ready(&PyPathGroupType) < 0) {
    fprintf(stdout,"Error in PyPathGroupType\n");
    return SV_PYTHON_ERROR;
  }

  auto module = PyModule_Create(&PyPathGroupModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing pathgroup module.\n");
    return SV_PYTHON_ERROR;
  }

  // Add pathgroup.PathGroupException exception.
  //
  // This defines a Python exception named sv.pathgroup.PathGroupException.
  // This can be used in a 'try' statement with an 'except' clause 'except sv.pathgroup.PathGroupExceptions:'
  // 
  PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, MODULE_EXCEPTION_OBJECT, PyRunTimeErr);
  Py_INCREF(&PyPathGroupType);
  PyModule_AddObject(module,MODULE_PATH_GROUP_CLASS,(PyObject*)&PyPathGroupType);
  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initPyPathGroup()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from cv_mesh_init\n");
  }

  // Initialize
  PyPathGroupType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&PyPathGroupType)<0)
  {
    fprintf(stdout,"Error in PyPathGroupType\n");
    return;
  }
  PyObject* pythonC;
  pythonC = Py_InitModule("PyPathGroup",PyPathGroupModule_methods);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing PyPathGroup\n");
    return;
  }
  PyRunTimeErr = PyErr_NewException("PyPathGroup.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  Py_INCREF(&PyPathGroupType);
  PyModule_AddObject(pythonC,"PyPathGroup",(PyObject*)&PyPathGroupType);
  return ;

}
#endif

