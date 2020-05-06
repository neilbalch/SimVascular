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

// Define the Python 'geometry.LoftNurbsOptions' class that encapsulates the paramters
// used for creating a lofted solid.
//
#ifndef PYAPI_GEOMETRY_LOFT_NURBS_OPTIONS_H
#define PYAPI_GEOMETRY_LOFT_NURBS_OPTIONS_H 

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

//-----------------------
// LoftNurbsOptionsClass 
//-----------------------
// Define the LoftNurbsOptionsClass. 
//
// knot span type can be 'equal', 'avg', or 'endderiv'
// parametric span type can be 'equal', 'chord', or 'centripetal'
//
typedef struct {
  PyObject_HEAD
  int u_degree;
  int v_degree;
  double u_spacing;
  double v_spacing;
  PyObject* u_knot_span_type;
  PyObject* v_knot_span_type;
  PyObject* u_parametric_span_type;
  PyObject* v_parametric_span_type;
} PyLoftNurbsOptionsClass;

// PyLoftNurbsOptionsClass attribute names.
//
namespace LoftNurbsOptions {
  char* U_DEGREE = "u_degree";
  char* V_DEGREE = "v_degree";
  char* U_SPACING = "u_spacing";
  char* V_SPACING = "v_spacing";
  char* U_KNOT_SPAN_TYPE = "u_knot_span_type";
  char* V_KNOT_SPAN_TYPE = "v_knot_span_type";
  char* U_PARAMETRIC_SPAN_TYPE = "u_parametric_span_type";
  char* V_PARAMETRIC_SPAN_TYPE = "v_parametric_span_type";
};

//--------------------------
// PyLoftNurbsOptionsGetInt
//--------------------------
// Get an integer or boolean atttibute from the LoftNurbsOptions object.
//
static int
LoftNurbsOptionsGetInt(PyObject* loftOptions, std::string name)
{
  auto obj = PyObject_GetAttrString(loftOptions, name.c_str());
  auto value = PyInt_AsLong(obj);
  Py_DECREF(obj);
  return value;
}

//-----------------------------
// PyLoftNurbsOptionsGetDouble 
//-----------------------------
// Get a double atttibute from the LoftNurbsOptions object.
//
static double 
LoftNurbsOptionsGetDouble(PyObject* loftOptions, std::string name)
{
  auto obj = PyObject_GetAttrString(loftOptions, name.c_str());
  auto value = PyFloat_AsDouble(obj);
  Py_DECREF(obj);
  return value;
}

static char * 
LoftNurbsOptionsGetString(PyObject* loftOptions, std::string name)
{
  auto obj = PyObject_GetAttrString(loftOptions, name.c_str());
  auto value = PyString_AsString(obj);
  Py_DECREF(obj);
  return value;
}


////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//

//--------------------
// LoftNurbsOptionsMethods
//--------------------
//
static PyMethodDef PyLoftNurbsOptionsMethods[] = {
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    M e m b e r s                //
////////////////////////////////////////////////////////
//
// Define the PyLoftNurbsOptionsClass attribute names.
//
// The attributes can be set/get directly in from the LoftNurbsOptions object.
//
static PyMemberDef PyLoftNurbsOptionsMembers[] = {
    {LoftNurbsOptions::U_DEGREE, T_INT, offsetof(PyLoftNurbsOptionsClass, u_degree), 0, "u degree"},
    {LoftNurbsOptions::V_DEGREE, T_INT, offsetof(PyLoftNurbsOptionsClass, v_degree), 0, "v degree"},
    {LoftNurbsOptions::U_SPACING, T_DOUBLE, offsetof(PyLoftNurbsOptionsClass, u_spacing), 0, "u spacing"},
    {LoftNurbsOptions::V_SPACING, T_DOUBLE, offsetof(PyLoftNurbsOptionsClass, v_spacing), 0, "v spacing"},

    {LoftNurbsOptions::U_KNOT_SPAN_TYPE, T_OBJECT_EX, offsetof(PyLoftNurbsOptionsClass, u_knot_span_type), 0, "u knot span type"},

    {LoftNurbsOptions::V_KNOT_SPAN_TYPE, T_OBJECT_EX, offsetof(PyLoftNurbsOptionsClass, v_knot_span_type), 0, "v knot span type"},
    {LoftNurbsOptions::U_PARAMETRIC_SPAN_TYPE, T_OBJECT_EX, offsetof(PyLoftNurbsOptionsClass, u_parametric_span_type), 0, "u parametric span type"},
    {LoftNurbsOptions::V_PARAMETRIC_SPAN_TYPE, T_OBJECT_EX, offsetof(PyLoftNurbsOptionsClass, v_parametric_span_type), 0, "v parametric span type"},
    {NULL}  
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* GEOMETRY_LOFT_NURBS_OPTIONS_CLASS = "LoftNurbsOptions";
static char* GEOMETRY_LOFT_NURBS_OPTIONS_MODULE_CLASS = "geometry.LoftNurbsOptions";

PyDoc_STRVAR(LoftNurbsOptionsClass_doc, "Geometry loft nurbs options methods.");

//------------------------
// PyLoftNurbsOptionsType 
//------------------------
// Define the Python type object that implements the geometry.LoftNurbsOptions class. 
//
static PyTypeObject PyLoftNurbsOptionsType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = GEOMETRY_LOFT_NURBS_OPTIONS_MODULE_CLASS,
  .tp_basicsize = sizeof(PyLoftNurbsOptionsClass)
};

//-------------------------
// PyLoftNurbsOptions_init
//-------------------------
// This is the __init__() method for the geometry.LoftNurbsOptions class. 
//
// This function is used to initialize an object after it is created.
//
// Arguments:
//
static int 
PyLoftNurbsOptionsInit(PyLoftNurbsOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("|O", PyRunTimeErr, __func__);
  static char *keywords[] = {"u_knot_span_type", NULL};

  int u_degree = 2;
  int v_degree = 2;
  double u_spacing = 0.01;
  double v_spacing = 0.01;
  PyObject* u_knot_span_type = nullptr;
  PyObject* v_knot_span_type = nullptr;
  PyObject* u_parametric_span_type = nullptr;
  PyObject* v_parametric_span_type = nullptr;
  PyObject* tmp; 

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &u_knot_span_type)) {
      api.argsError();
      return -1;
  }

  self->u_degree = u_degree;
  self->v_degree = v_degree;
  self->u_spacing = u_spacing;
  self->v_spacing = v_spacing;

  if (u_knot_span_type) {
      tmp = self->u_knot_span_type;
      Py_INCREF(u_knot_span_type);
      self->u_knot_span_type = u_knot_span_type;
      Py_XDECREF(tmp);
  } else {
      self->u_knot_span_type = Py_BuildValue("s", "equal");
  }

  if (v_knot_span_type) {
      tmp = self->v_knot_span_type;
      Py_INCREF(v_knot_span_type);
      self->v_knot_span_type = v_knot_span_type;
      Py_XDECREF(tmp);
  } else {
      self->v_knot_span_type = Py_BuildValue("s", "equal");
  }

  if (u_parametric_span_type) {
      tmp = self->u_parametric_span_type;
      Py_INCREF(u_parametric_span_type);
      self->u_parametric_span_type = u_parametric_span_type;
      Py_XDECREF(tmp);
  } else {
      self->u_parametric_span_type = Py_BuildValue("s", "equal");
  }

  if (v_parametric_span_type) {
      tmp = self->v_parametric_span_type;
      Py_INCREF(v_parametric_span_type);
      self->v_parametric_span_type = v_parametric_span_type;
      Py_XDECREF(tmp);
  } else {
      self->v_parametric_span_type = Py_BuildValue("s", "equal");
  }

  return 0;
}

//-----------------------
// PyLoftNurbsOptionsNew 
//-----------------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyLoftNurbsOptionsNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyLoftNurbsOptionsNew] PyLoftNurbsOptionsNew " << std::endl;
  auto self = (PyLoftNurbsOptionsClass*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyLoftNurbsOptionsNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }

  self->u_knot_span_type = PyUnicode_FromString("equal");

  return (PyObject *) self;
}

//---------------------------
// PyLoftNurbsOptionsDealloc 
//---------------------------
//
static void
PyLoftNurbsOptionsDealloc(PyLoftNurbsOptionsClass* self)
{
  std::cout << "[PyLoftNurbsOptionsDealloc] Free PyLoftNurbsOptions" << std::endl;
  Py_TYPE(self)->tp_free(self);
}

//-------------------------------
// SetLoftNurbsOptionsTypeFields
//-------------------------------
// Set the Python type object fields that stores loft option data. 
//
static void
SetLoftNurbsOptionsTypeFields(PyTypeObject& loftOpts)
 {
  loftOpts.tp_doc = LoftNurbsOptionsClass_doc; 
  loftOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  loftOpts.tp_dict = PyDict_New();
  loftOpts.tp_new = PyLoftNurbsOptionsNew;
  loftOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  loftOpts.tp_init = (initproc)PyLoftNurbsOptionsInit;
  loftOpts.tp_dealloc = (destructor)PyLoftNurbsOptionsDealloc;
  loftOpts.tp_methods = PyLoftNurbsOptionsMethods;
  loftOpts.tp_members = PyLoftNurbsOptionsMembers;
};

//---------------------
// SetLoftNurbsOptionsTypes
//---------------------
// Set the  loft optinnames in the LoftNurbsOptionsType dictionary.
//
// These are for read only attibutes.
//
static void
SetLoftNurbsOptionsClassTypes(PyTypeObject& loftOptsType)
{
/*
  std::cout << "=============== SetLoftNurbsOptionsClassTypes ==========" << std::endl;

  //PyDict_SetItemString(loftOptsType.tp_dict, "num_pts", PyLong_AsLong(10));

  PyObject *o = PyLong_FromLong(1);
  PyDict_SetItemString(loftOptsType.tp_dict, "num_pts", o);

  //PyDict_SetItem(loftOptsType.tp_dict, "num_pts", o);

  std::cout << "[SetLoftNurbsOptionsClassTypes] Done! " << std::endl;
*/
 
};

#endif


