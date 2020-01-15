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

// Define the Python 'meshing.TetGenOptions' class that encapsulates the paramters
// used for generating a mesh using TetGen.
//
#ifndef PYAPI_MESHING_TETGEN_OPTIONS_H
#define PYAPI_MESHING_TETGEN_OPTIONS_H 

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

//---------------------
// MeshingOptionsClass 
//---------------------
// Define the MeshingOptionsClass. 
//
typedef struct {
PyObject_HEAD
  double global_edge_size;
  int surface_mesh_flag;
  int volume_mesh_flag;
} PyMeshingTetGenOptionsClass;

// PyMeshingTetGenOptionsClass attribute names.
//
namespace MeshingOptions {
  char* GLOBAL_EDGE_SIZE = "global_edge_size";
  char* SURFACE_MESH_FLAG = "surface_mesh_flag";
  char* VOLUME_MESH_FLAG = "volume_mesh_flag";
};

//------------------------
// PyTetGenOptionsGetInt
//------------------------
// Get an integer or boolean atttibute from the MeshingOptions object.
//
static int
MeshingOptionsGetInt(PyObject* meshingOptions, std::string name)
{
  auto obj = PyObject_GetAttrString(meshingOptions, name.c_str());
  auto value = PyInt_AsLong(obj);
  Py_DECREF(obj);
  return value;
}

//---------------------------
// PyTetGenOptionsGetDouble 
//---------------------------
// Get a double atttibute from the MeshingOptions object.
//
static double 
MeshingOptionsGetDouble(PyObject* meshingOptions, std::string name)
{
  auto obj = PyObject_GetAttrString(meshingOptions, name.c_str());
  auto value = PyFloat_AsDouble(obj);
  Py_DECREF(obj);
  return value;
}

////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//

//--------------------
// MeshingOptionsMethods
//--------------------
//
static PyMethodDef PyTetGenOptionsMethods[] = {
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    M e m b e r s                //
////////////////////////////////////////////////////////
//
// Define the PyMeshingTetGenOptionsClass attribute names.
//
// The attributes can be set/get directly in from the MeshingOptions object.
//
static PyMemberDef PyTetGenOptionsMembers[] = {
    {MeshingOptions::GLOBAL_EDGE_SIZE, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, global_edge_size), 0, "global_edge_size"},
    {MeshingOptions::SURFACE_MESH_FLAG, T_INT, offsetof(PyMeshingTetGenOptionsClass, surface_mesh_flag), 0, "surface_mesh_flag"},
    {MeshingOptions::VOLUME_MESH_FLAG, T_INT, offsetof(PyMeshingTetGenOptionsClass, volume_mesh_flag), 0, "volume_mesh_flag"},
    {NULL}  
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MESHING_TETGEN_OPTIONS_CLASS = "TetGenOptions";
static char* MESHING_TETGEN_OPTIONS_MODULE_CLASS = "meshing.TetGenOptions";

PyDoc_STRVAR(TetGenOptionsClass_doc, "TetGen meshing options class functions");

//---------------------
// PyTetGenOptionsType 
//---------------------
// Define the Python type object that implements the meshing.MeshingOptions class. 
//
static PyTypeObject PyTetGenOptionsType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = MESHING_TETGEN_OPTIONS_MODULE_CLASS,
  .tp_basicsize = sizeof(PyMeshingTetGenOptionsClass)
};

//----------------------
// PyTetGenOptions_init
//----------------------
// This is the __init__() method for the meshing.MeshingOptions class. 
//
// This function is used to initialize an object after it is created.
//
// Arguments:
//
//   fileName - An SV .ctgr pth file. A new MeshingOptions object is created from 
//     the contents of the file. (optional)
//
static int 
PyTetGenOptionsInit(PyMeshingTetGenOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  static int numObjs = 1;
  std::cout << "[PyTetGenOptionsInit] New MeshingOptions object: " << numObjs << std::endl;
  auto api = SvPyUtilApiFunction("|i", PyRunTimeErr, __func__);
  static char *keywords[] = {"surface_mesh_flag", NULL};
  int surface_mesh_flag = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &surface_mesh_flag)) {
      api.argsError();
      return -1;
  }
  std::cout << "[PyTetGenOptionsInit] surface_mesh_flag: " << surface_mesh_flag << std::endl;

  self->surface_mesh_flag = surface_mesh_flag;

  return 0;
}

//--------------------
// PyTetGenOptionsNew 
//--------------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyTetGenOptionsNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyTetGenOptionsNew] PyTetGenOptionsNew " << std::endl;
  auto self = (PyMeshingTetGenOptionsClass*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyTetGenOptionsNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }
  return (PyObject *) self;
}

//-------------------------
// PyTetGenOptionsDealloc 
//-------------------------
//
static void
PyTetGenOptionsDealloc(PyMeshingTetGenOptionsClass* self)
{
  std::cout << "[PyTetGenOptionsDealloc] Free PyTetGenOptions" << std::endl;
  Py_TYPE(self)->tp_free(self);
}

//----------------------------
// SetTetGenOptionsTypeFields 
//----------------------------
// Set the Python type object fields that stores loft option data. 
//
static void
SetTetGenOptionsTypeFields(PyTypeObject& meshingOpts)
 {
  meshingOpts.tp_doc = TetGenOptionsClass_doc; 
  meshingOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  meshingOpts.tp_dict = PyDict_New();
  meshingOpts.tp_new = PyTetGenOptionsNew;
  meshingOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  meshingOpts.tp_init = (initproc)PyTetGenOptionsInit;
  meshingOpts.tp_dealloc = (destructor)PyTetGenOptionsDealloc;
  meshingOpts.tp_methods = PyTetGenOptionsMethods;
  meshingOpts.tp_members = PyTetGenOptionsMembers;
};

//------------------------
// SetMeshingOptionsTypes
//------------------------
// Set the  loft optinnames in the MeshingOptionsType dictionary.
//
// These are for read only attibutes.
//
static void
SetTetGenOptionsClassTypes(PyTypeObject& meshingOptsType)
{
/*
  std::cout << "=============== SetMeshingOptionsClassTypes ==========" << std::endl;

  //PyDict_SetItemString(meshingOptsType.tp_dict, "num_pts", PyLong_AsLong(10));

  PyObject *o = PyLong_FromLong(1);
  PyDict_SetItemString(meshingOptsType.tp_dict, "num_pts", o);

  //PyDict_SetItem(meshingOptsType.tp_dict, "num_pts", o);

  std::cout << "[SetMeshingOptionsClassTypes] Done! " << std::endl;
*/
 
};

//-------------------------
// CreateTetGenOptionsType 
//-------------------------
//
static PyMeshingTetGenOptionsClass *
CreateTetGenOptionsType()
{
  return PyObject_New(PyMeshingTetGenOptionsClass, &PyTetGenOptionsType);
}

#endif

