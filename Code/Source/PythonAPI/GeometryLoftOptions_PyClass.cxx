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

// Define the Python 'geometry.LoftOptions' class that encapsulates the paramters
// used for creating a lofted solid.
//
#ifndef PYAPI_GEOMETRY_LOFT_OPTIONS_H
#define PYAPI_GEOMETRY_LOFT_OPTIONS_H 

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

//------------------
// LoftOptionsClass 
//------------------
// Define the LoftOptionsClass. 
//
typedef struct {
PyObject_HEAD
  int num_out_pts_in_segs;
  int num_out_pts_along_length;
  int num_linear_pts_along_length;
  int num_modes;
  int use_fft;
  int use_linear_sample_along_length;
  int spline_type;
  double bias;
  double tension;
  double continuity;
} PyLoftOptionsClass;

// PyLoftOptionsClass attribute names.
//
namespace LoftOptions {
  char* NUM_OUT_PTS_IN_SEGS = "num_out_pts_in_segs";
  char* NUM_OUT_PTS_ALONG_LENGTH = "num_out_pts_along_length";
  char* NUM_LINEAR_PTS_ALONG_LENGTH = "num_linear_pts_along_length";
  char* NUM_MODES = "num_modes"; 
  char* USE_FFT = "use_fft"; 
  char* USE_LINEAR_SAMPLE_ALONG_LENGTH = "use_linear_sample_along_length";
  char* SPLINE_TYPE = "spline_type"; 
  char* BIAS = "bias"; 
  char* TENSION = "tension"; 
  char* CONTINUITY = "continuity";
};

//---------------------
// PyLoftOptionsGetInt
//---------------------
// Get an integer or boolean atttibute from the LoftOptions object.
//
static int
LoftOptionsGetInt(PyObject* loftOptions, std::string name)
{
  auto obj = PyObject_GetAttrString(loftOptions, name.c_str());
  auto value = PyInt_AsLong(obj);
  Py_DECREF(obj);
  return value;
}

//------------------------
// PyLoftOptionsGetDouble 
//------------------------
// Get a double atttibute from the LoftOptions object.
//
static double 
LoftOptionsGetDouble(PyObject* loftOptions, std::string name)
{
  auto obj = PyObject_GetAttrString(loftOptions, name.c_str());
  auto value = PyFloat_AsDouble(obj);
  Py_DECREF(obj);
  return value;
}


////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//

//--------------------
// LoftOptionsMethods
//--------------------
//
static PyMethodDef PyLoftOptionsMethods[] = {
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    M e m b e r s                //
////////////////////////////////////////////////////////
//
// Define the PyLoftOptionsClass attribute names.
//
// The attributes can be set/get directly in from the LoftOptions object.
//
static PyMemberDef PyLoftOptionsMembers[] = {
    {LoftOptions::BIAS, T_DOUBLE, offsetof(PyLoftOptionsClass, bias), 0, "first name"},
    {LoftOptions::CONTINUITY, T_DOUBLE, offsetof(PyLoftOptionsClass, continuity), 0, "first name"},
    {LoftOptions::NUM_LINEAR_PTS_ALONG_LENGTH, T_INT, offsetof(PyLoftOptionsClass, num_linear_pts_along_length), 0, "first name"},
    {LoftOptions::NUM_MODES, T_INT, offsetof(PyLoftOptionsClass, num_modes), 0, "first name"},
    {LoftOptions::NUM_OUT_PTS_IN_SEGS, T_INT, offsetof(PyLoftOptionsClass, num_out_pts_in_segs), 0, "first name"},
    {LoftOptions::NUM_OUT_PTS_ALONG_LENGTH, T_INT, offsetof(PyLoftOptionsClass, num_out_pts_along_length), 0, "first name"},
    {LoftOptions::SPLINE_TYPE, T_INT, offsetof(PyLoftOptionsClass, spline_type), 0, "first name"},
    {LoftOptions::TENSION, T_DOUBLE, offsetof(PyLoftOptionsClass, tension), 0, "first name"},
    {LoftOptions::USE_FFT, T_BOOL, offsetof(PyLoftOptionsClass, use_fft), 0, "first name"},
    {LoftOptions::USE_LINEAR_SAMPLE_ALONG_LENGTH, T_BOOL, offsetof(PyLoftOptionsClass, use_linear_sample_along_length), 0, "first name"},
    {NULL}  
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* GEOMETRY_LOFT_OPTIONS_CLASS = "LoftOptions";
static char* GEOMETRY_LOFT_OPTIONS_MODULE_CLASS = "geometry.LoftOptions";

PyDoc_STRVAR(LoftOptionsClass_doc, "solid modeling kernel class functions");

//-------------------
// PyLoftOptionsType 
//-------------------
// Define the Python type object that implements the geometry.LoftOptions class. 
//
static PyTypeObject PyLoftOptionsType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = GEOMETRY_LOFT_OPTIONS_MODULE_CLASS,
  .tp_basicsize = sizeof(PyLoftOptionsClass)
};

//--------------------
// PyLoftOptions_init
//--------------------
// This is the __init__() method for the geometry.LoftOptions class. 
//
// This function is used to initialize an object after it is created.
//
// Arguments:
//
//   fileName - An SV .ctgr pth file. A new LoftOptions object is created from 
//     the contents of the file. (optional)
//
static int 
PyLoftOptionsInit(PyLoftOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  static int numObjs = 1;
  std::cout << "[PyLoftOptionsInit] New LoftOptions object: " << numObjs << std::endl;
  auto api = SvPyUtilApiFunction("|i", PyRunTimeErr, __func__);
  static char *keywords[] = {"num_out_pts_in_segs", NULL};
  int num_out_pts_in_segs = 30;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &num_out_pts_in_segs)) {
      api.argsError();
      return -1;
  }
  std::cout << "[PyLoftOptionsInit] num_out_pts_in_segs: " << num_out_pts_in_segs << std::endl;

  self->num_out_pts_in_segs = num_out_pts_in_segs;
  self->num_out_pts_along_length = 60;
  self->num_linear_pts_along_length = 600;
  self->num_modes = 20;
  self->use_fft = 0;
  self->use_linear_sample_along_length = 1;
  self->spline_type = 0;
  self->bias = 0.0;
  self->tension = 0.0;
  self->continuity = 0.0;

  return 0;
}

//------------------
// PyLoftOptionsNew 
//------------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyLoftOptionsNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyLoftOptionsNew] PyLoftOptionsNew " << std::endl;
  auto self = (PyLoftOptionsClass*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyLoftOptionsNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }
  return (PyObject *) self;
}

//----------------------
// PyLoftOptionsDealloc 
//----------------------
//
static void
PyLoftOptionsDealloc(PyLoftOptionsClass* self)
{
  std::cout << "[PyLoftOptionsDealloc] Free PyLoftOptions" << std::endl;
  Py_TYPE(self)->tp_free(self);
}

//--------------------------
// SetLoftOptionsTypeFields
//--------------------------
// Set the Python type object fields that stores loft option data. 
//
static void
SetLoftOptionsTypeFields(PyTypeObject& loftOpts)
 {
  loftOpts.tp_doc = LoftOptionsClass_doc; 
  loftOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  loftOpts.tp_dict = PyDict_New();
  loftOpts.tp_new = PyLoftOptionsNew;
  loftOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  loftOpts.tp_init = (initproc)PyLoftOptionsInit;
  loftOpts.tp_dealloc = (destructor)PyLoftOptionsDealloc;
  loftOpts.tp_methods = PyLoftOptionsMethods;
  loftOpts.tp_members = PyLoftOptionsMembers;
};

//---------------------
// SetLoftOptionsTypes
//---------------------
// Set the  loft optinnames in the LoftOptionsType dictionary.
//
// These are for read only attibutes.
//
static void
SetLoftOptionsClassTypes(PyTypeObject& loftOptsType)
{
/*
  std::cout << "=============== SetLoftOptionsClassTypes ==========" << std::endl;

  //PyDict_SetItemString(loftOptsType.tp_dict, "num_pts", PyLong_AsLong(10));

  PyObject *o = PyLong_FromLong(1);
  PyDict_SetItemString(loftOptsType.tp_dict, "num_pts", o);

  //PyDict_SetItem(loftOptsType.tp_dict, "num_pts", o);

  std::cout << "[SetLoftOptionsClassTypes] Done! " << std::endl;
*/
 
};

#endif


