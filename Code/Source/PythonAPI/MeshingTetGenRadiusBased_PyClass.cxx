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

// Define the Python 'meshing.TetGenRadiusBased' class that encapsulates the parameters
// used for generating a mesh using TetGen. Options are stored as Python class attributes 
// and are set directly in the object created from that class.
//
//     radiusBased = sv.meshing.TetGenRadiusBased()
//
#ifndef PYAPI_MESHING_TETGEN_RADIUS_BASED_H
#define PYAPI_MESHING_TETGEN_RADIUS_BASED_H 

#include <regex>
#include <string>
#include <structmember.h>
#include <vtkXMLPolyDataReader.h>
#include "vtkXMLPolyDataWriter.h"

PyObject * CreateTetGenRadiusBasedType(PyObject* args, PyObject* kwargs);

//---------------------------------
// PyMeshingTetGenRadiusBasedClass 
//---------------------------------
// Define the MeshingOptionsClass. 
//
typedef struct {
  PyObject_HEAD
  double edge_size;
  cvTetGenMeshObject* mesher;
  vtkPolyData* centerlines;
  vtkPolyData* centerlineDistanceData;
} PyMeshingTetGenRadiusBasedClass;

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//
// Methods for the TetGenRadiusBased class.

//-----------------------------------------
// PyTetGenRadiusBased_compute_centerlines
//-----------------------------------------
//
PyDoc_STRVAR(PyTetGenRadiusBased_compute_centerlines_doc,
  "compute_centerlines() \n\ 
  \n\
  Compute the centerlines used in radius-based meshing. \n\
  \n\
");

static PyObject *
PyTetGenRadiusBased_compute_centerlines(PyMeshingTetGenRadiusBasedClass* self, PyObject* args, PyObject* kwargs)
{
  std::cout << "========== PyTetGenRadiusBased_compute_centerlines ==========" << std::endl;
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 

  auto mesher = self->mesher;
  auto solid = mesher->GetSolid();
  if (solid == nullptr) {
      api.error("A solid model must be defined for the mesh to compute centerlines.");
      return nullptr;
  }

  // Compute centerlines.
  std::cout << "[compute_centerlines] Computing centerlines ..." << std::endl;
  auto centerlines = sv4guiModelUtils::CreateCenterlines(solid->GetVtkPolyData());
  if (centerlines == nullptr) {
      api.error("Unable to compute compute centerlines.");
      return nullptr;
  } 

  auto distance = sv4guiModelUtils::CalculateDistanceToCenterlines(centerlines, solid->GetVtkPolyData());
  if (distance == nullptr) {
      api.error("Unable to compute compute the distance to centerlines.");
      return nullptr;
  } 
  mesher->SetVtkPolyDataObject(distance);
  std::cout << "[compute_centerlines] Done. " << std::endl;

  self->centerlines = centerlines;
  self->centerlineDistanceData = distance;

  Py_RETURN_NONE; 
}

//-------------------------------------------
// PyTetGenRadiusBased_compute_size_function
//-------------------------------------------
//
PyDoc_STRVAR(PyTetGenRadiusBased_compute_size_function_doc,
  "compute_size_function(edge_size)  \n\ 
  \n\
  Compute the size function used to set anisotropic edge sizes. \n\
  \n\
  Args:  \n\
    edge_size (double): The edge size used to create anisotropic edge sizes from centerline radii.  \n\
");

static PyObject *
PyTetGenRadiusBased_compute_size_function(PyMeshingTetGenRadiusBasedClass* self, PyObject* args, PyObject* kwargs)
{
  std::cout << "========== PyTetGenRadiusBased_compute_size_function_doc ==========" << std::endl;
  auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__); 
  static char *keywords[] = {"edge_size", NULL};
  double edgeSize;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &edgeSize)) { 
    return api.argsError();
  }

  if (self->centerlines == nullptr) { 
      api.error("Centerlines have not been computed.");
      return nullptr;
  }

  auto mesher = self->mesher;

  // Compute the size function data array used for the radius-based meshing.
  char* sizeFunctionName = "DistanceToCenterlines";
  if (mesher->SetSizeFunctionBasedMesh(edgeSize, sizeFunctionName) != SV_OK) {
      api.error("Unable to compute compute the distance to centerlines size function.");
      return nullptr;
  }

  Py_RETURN_NONE; 
}

PyDoc_STRVAR(PyTetGenRadiusBased_load_centerlines_doc,
  "load_centerlines(file_name)  \n\ 
  \n\
  Load the centerlines used in radius-based meshing from a file. \n\
  \n\
  Args:  \n\
    file_name (str): The name of the file containing vtkPolyData centerline data. \n\
");

static PyObject *
PyTetGenRadiusBased_load_centerlines(PyMeshingTetGenRadiusBasedClass* self, PyObject* args, PyObject* kwargs)
{ 
  std::cout << "========== PyTetGenRadiusBased_load_centerlines ==========" << std::endl;
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  static char *keywords[] = {"file_name", NULL};
  char* fileName;
    
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &fileName)) {
    return api.argsError();
  }

  auto mesher = self->mesher;
  auto solid = mesher->GetSolid();
  if (solid == nullptr) {
      api.error("A solid model must be defined for the mesh to load centerlines.");
      return nullptr;
  }

  // Read the file.
  try {
      vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
      reader->SetFileName(fileName);
      reader->Update();
      self->centerlines = reader->GetOutput();
  } catch (...) {
      api.error("Unable to read the file named '" + std::string(fileName) + "'.");
      return nullptr;
  }
  std::cout << "[load_centerlines] Number of centerline points: " << self->centerlines->GetNumberOfPoints() << std::endl;

  auto distance = sv4guiModelUtils::CalculateDistanceToCenterlines(self->centerlines, solid->GetVtkPolyData());
  if (distance == nullptr) {
      api.error("Unable to compute compute the distance to centerlines.");
      return nullptr;
  } 
  mesher->SetVtkPolyDataObject(distance);
  self->centerlineDistanceData = distance;

  Py_RETURN_NONE;
}

//-------------------------------------
// PyTetGenRadiusBased_set_centerlines
//-------------------------------------
//
PyDoc_STRVAR(PyTetGenRadiusBased_set_centerlines_doc,
  "set_centerlines(centerlines)  \n\ 
  \n\
  Set the centerlines used in radius-based meshing from a file or a vtkPolyData object. \n\
  \n\
  Args:  \n\
    centerlines (vtkPolyData object): The vtkPolyData object containtin centerline data. (optional) \n\
");

static PyObject *
PyTetGenRadiusBased_set_centerlines(PyMeshingTetGenRadiusBasedClass* self, PyObject* args, PyObject* kwargs)
{
  std::cout << "========== PyTetGenRadiusBased_set_centerlines ==========" << std::endl;
  auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__); 
  static char *keywords[] = {"centerlines", NULL};
  PyObject* centerlinesArg;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &centerlinesArg)) { 
    return api.argsError();
  }

  auto mesher = self->mesher;

  Py_RETURN_NONE; 
}

//---------------------------------------
// PyTetGenRadiusBased_write_centerlines
//---------------------------------------
//
PyDoc_STRVAR(PyTetGenRadiusBased_write_centerlines_doc,
  "write_centerlines(file_name)  \n\ 
  \n\
  Write the centerlines computed for radius-based meshing to a file. \n\
  \n\
  Args:  \n\
    file_name (str): The name of the file to write the centerline data. \n\
");

static PyObject *
PyTetGenRadiusBased_write_centerlines(PyMeshingTetGenRadiusBasedClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  static char *keywords[] = {"file_name", NULL};
  char* fileName;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &fileName)) {
    return api.argsError();
  }

  if (self->centerlines == nullptr) { 
      api.error("Centerlines have not been computed.");
      return nullptr;
  }

  // Check that you can write to the file.
  ofstream cfile;
  cfile.open(fileName);
  if (!cfile.is_open()) {
      api.error("Unable to write to the file named '" + std::string(fileName) + "'.");
      return nullptr;
  }

  // Write the file.
  vtkSmartPointer<vtkXMLPolyDataWriter> writer  = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer->SetFileName(fileName);
  writer->SetInputData(self->centerlines);
  writer->Write();

  Py_RETURN_NONE;
}

//----------------------------
// PyTetGenRadiusBasedMethods 
//----------------------------
//
static PyMethodDef PyTetGenRadiusBasedMethods[] = {
  {"compute_centerlines", (PyCFunction)PyTetGenRadiusBased_compute_centerlines, METH_NOARGS, PyTetGenRadiusBased_compute_centerlines_doc},
  {"compute_size_function", (PyCFunction)PyTetGenRadiusBased_compute_size_function, METH_VARARGS|METH_KEYWORDS, PyTetGenRadiusBased_compute_size_function_doc},
  {"load_centerlines", (PyCFunction)PyTetGenRadiusBased_load_centerlines, METH_VARARGS|METH_KEYWORDS, PyTetGenRadiusBased_load_centerlines_doc},
  {"set_centerlines", (PyCFunction)PyTetGenRadiusBased_set_centerlines, METH_VARARGS|METH_KEYWORDS, PyTetGenRadiusBased_set_centerlines_doc},
  {"write_centerlines", (PyCFunction)PyTetGenRadiusBased_write_centerlines, METH_VARARGS|METH_KEYWORDS, PyTetGenRadiusBased_write_centerlines_doc},
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    M e m b e r s                //
////////////////////////////////////////////////////////
//
// Define the PyMeshingTetGenRadiusBasedClass attribute names.
//
// The attributes can be set/get directly in from the MeshingOptions object.
//
static PyMemberDef PyTetGenRadiusBasedMembers[] = {
    {NULL}  
};

////////////////////////////////////////////////////////
//          C l a s s    G e t / S e t                //
////////////////////////////////////////////////////////
//
// Define setters/getters for certain options.

//------------------------------
// PyTetGenRadiusBased_get_add_hole 
//------------------------------
//
static PyObject*
PyTetGenRadiusBased_get_add_hole(PyMeshingTetGenRadiusBasedClass* self, void* closure)
{
  //return self->add_hole;
}

static int 
PyTetGenRadiusBased_set_add_hole(PyMeshingTetGenRadiusBasedClass* self, PyObject* value, void* closure)
{
  if (!PyList_Check(value)) {
      PyErr_SetString(PyExc_ValueError, "The add_hole parameter must be a list of three floats.");
      return -1;
  }

  int num = PyList_Size(value);
  if (num != 3) {
      PyErr_SetString(PyExc_ValueError, "The add_hole parameter must be a list of three floats.");
      return -1;
  }

  std::vector<double> values;
  for (int i = 0; i < num; i++) {
      auto item = PyList_GetItem(value, i);
      auto value = PyFloat_AsDouble(item);
      values.push_back(value);
  }

  if (PyErr_Occurred()) {
      return -1;
  }

  //self->add_hole = Py_BuildValue("[d,d,d]", values[0], values[1], values[2]);
  return 0;
}

//----------------------------
// PyTetGenRadiusBasedGetSets
//----------------------------
//
PyGetSetDef PyTetGenRadiusBasedGetSets[] = {
    {NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MESHING_TETGEN_RADIUS_BASED_CLASS = "TetGenRadiusBased";
static char* MESHING_TETGEN_RADIUS_BASED_MODULE_CLASS = "meshing.TetGenRadiusBased";

PyDoc_STRVAR(TetGenRadiusBasedClass_doc, "TetGen meshing options class functions");

//-------------------------
// PyTetGenRadiusBasedType 
//-------------------------
// Define the Python type object that implements the meshing.TetGenRadiusBased class. 
//
static PyTypeObject PyTetGenRadiusBasedType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = MESHING_TETGEN_RADIUS_BASED_MODULE_CLASS,
  .tp_basicsize = sizeof(PyMeshingTetGenRadiusBasedClass)
};

//--------------------------
// PyTetGenRadiusBased_init
//--------------------------
// This is the __init__() method for the meshing.MeshingOptions class. 
//
// This function is used to initialize an object after it is created.
//
// Arguments:
//
static int 
PyTetGenRadiusBasedInit(PyMeshingTetGenRadiusBasedClass* self, PyObject* args, PyObject* kwargs)
{
  std::cout << "[PyTetGenRadiusBasedInit] Initialize a RadiusBased object." << std::endl;
  auto api = SvPyUtilApiFunction("O!", PyRunTimeErr, __func__);
  static char *keywords[] = { "mesher", NULL};
  PyObject* mesherArg = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &PyMeshingTetGenClassType, &mesherArg)) {
      api.argsError();
      return -1;
  }

  self->mesher = dynamic_cast<cvTetGenMeshObject*>(((PyMeshingMesherClass*)mesherArg)->mesher);
  std::cout << "[PyTetGenRadiusBasedInit] Mesher: " << self->mesher << std::endl;
  self->centerlines = nullptr;
  self->centerlineDistanceData = nullptr;
  std::cout << "[PyTetGenRadiusBasedInit] Done. " << std::endl;
  return 0;
}

//------------------------
// PyTetGenRadiusBasedNew 
//------------------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyTetGenRadiusBasedNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyTetGenRadiusBasedNew] PyTetGenRadiusBasedNew " << std::endl;
  auto self = (PyMeshingTetGenRadiusBasedClass*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyTetGenRadiusBasedNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }
  return (PyObject *) self;
}

//----------------------------
// PyTetGenRadiusBasedDealloc 
//----------------------------
//
static void
PyTetGenRadiusBasedDealloc(PyMeshingTetGenRadiusBasedClass* self)
{
  std::cout << "[PyTetGenRadiusBasedDealloc] Free PyTetGenRadiusBased" << std::endl;
  Py_TYPE(self)->tp_free(self);
}

//--------------------------------
// SetTetGenRadiusBasedTypeFields 
//--------------------------------
// Set the Python type object fields that stores loft option data. 
//
static void
SetTetGenRadiusBasedTypeFields(PyTypeObject& radiusBased)
 {
  radiusBased.tp_doc = TetGenRadiusBasedClass_doc; 
  radiusBased.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  radiusBased.tp_dict = PyDict_New();
  radiusBased.tp_new = PyTetGenRadiusBasedNew;
  radiusBased.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  radiusBased.tp_init = (initproc)PyTetGenRadiusBasedInit;
  radiusBased.tp_dealloc = (destructor)PyTetGenRadiusBasedDealloc;
  radiusBased.tp_methods = PyTetGenRadiusBasedMethods;
  radiusBased.tp_members = PyTetGenRadiusBasedMembers;
  radiusBased.tp_getset = PyTetGenRadiusBasedGetSets;
};

//------------------------
// SetMeshingOptionsTypes
//------------------------
// Set the  loft optinnames in the MeshingOptionsType dictionary.
//
// These are for read only attibutes.
//
static void
SetTetGenRadiusBasedClassTypes(PyTypeObject& meshingOptsType)
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

//-----------------------------
// CreateTetGenRadiusBasedType
//-----------------------------
//
PyObject *
CreateTetGenRadiusBasedType(PyObject* args, PyObject* kwargs)
{
  return PyObject_Call((PyObject*)&PyTetGenRadiusBasedType, args, kwargs);
}

#endif

