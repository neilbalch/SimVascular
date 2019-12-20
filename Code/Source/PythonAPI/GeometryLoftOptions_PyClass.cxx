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

//--------------------------
// GeometryLoftOptionsClass 
//--------------------------
// Define the GeometryLoftOptionsClass. 
//
typedef struct {
PyObject_HEAD
  PyObject *num_pts;
  int numOutPtsInSegs;
  int numOutPtsAlongLength;
  int numLinearPtsAlongLength;
  int numModes;
  int useFFT;
  int useLinearSampleAlongLength;
  int splineType;
  double bias;
  double tension;
  double continuity;
} PyGeometryLoftOptionsClass;


////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//

//----------------------------
// GeometryLoftOptionsMethods
//----------------------------
//
static PyMethodDef PyGeometryLoftOptionsMethods[] = {
  {NULL, NULL}
};

static PyMemberDef Custom_members[] = {
    {"num_pts", T_OBJECT_EX, offsetof(PyGeometryLoftOptionsClass, num_pts), 0, "first name"},
    {NULL}  
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* GEOMETRY_LOFT_OPTIONS_CLASS = "LoftOptions";
static char* GEOMETRY_LOFT_OPTIONS_MODULE_CLASS = "geometry.LoftOptions";

PyDoc_STRVAR(GeometryLoftOptionsClass_doc, "solid modeling kernel class functions");

//--------------------------------
// PyGeometryLoftOptionsClassType 
//--------------------------------
// Define the Python type object that stores geometry.LoftOptions data. 
//
static PyTypeObject PyGeometryLoftOptionsClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = GEOMETRY_LOFT_OPTIONS_MODULE_CLASS,
  .tp_basicsize = sizeof(PyGeometryLoftOptionsClass)
};


//------------------
// PyGeometryLoftOptions_init
//------------------
// This is the __init__() method for the contour.Group class. 
//
// This function is used to initialize an object after it is created.
//
// Arguments:
//
//   fileName - An SV .ctgr pth file. A new GeometryLoftOptions object is created from 
//     the contents of the file. (optional)
//
static int 
PyGeometryLoftOptionsInit(PyGeometryLoftOptionsClass* self, PyObject* args)
{
  static int numObjs = 1;
  std::cout << "[PyGeometryLoftOptionsInit] New GeometryLoftOptions object: " << numObjs << std::endl;
  auto api = SvPyUtilApiFunction("|s", PyRunTimeErr, __func__);

/*
  char* fileName = nullptr;
  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      api.argsError();
      return 1;
  }
  if (fileName != nullptr) {
      std::cout << "[PyGeometryLoftOptionsInit] File name: " << fileName << std::endl;
      self->contourGroupPointer = GeometryLoftOptions_read(fileName);
      self->contourGroup = dynamic_cast<sv4guiGeometryLoftOptions*>(self->contourGroupPointer.GetPointer());
  } else {
      self->contourGroup = Dmg_create_contour_group();
  }
  if (self->contourGroup == nullptr) { 
      std::cout << "[PyGeometryLoftOptionsInit] ERROR reading File name: " << fileName << std::endl;
      return -1;
  }
  numObjs += 1;
*/
  return 0;
}

//-------------------
// PyGeometryLoftOptionsNew 
//-------------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyGeometryLoftOptionsNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyGeometryLoftOptionsNew] PyGeometryLoftOptionsNew " << std::endl;
  auto self = (PyGeometryLoftOptionsClass*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyGeometryLoftOptionsNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }
  return (PyObject *) self;
}

//-----------------------
// PyGeometryLoftOptionsDealloc 
//-----------------------
//
static void
PyGeometryLoftOptionsDealloc(PyGeometryLoftOptionsClass* self)
{
  std::cout << "[PyGeometryLoftOptionsDealloc] Free PyGeometryLoftOptions" << std::endl;
  // Can't delete contourGroup because it has a protected detructor.
  //delete self->contourGroup;
  Py_TYPE(self)->tp_free(self);
}

//---------------------------------------
// SetGeometryLoftOptionsClassTypeFields
//---------------------------------------
// Set the Python type object fields that stores loft option data. 
//
static void
SetGeometryLoftOptionsTypeFields(PyTypeObject& loftOptionsType)
 {
  loftOptionsType.tp_doc = GeometryLoftOptionsClass_doc; 
  loftOptionsType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  loftOptionsType.tp_dict = PyDict_New();
  loftOptionsType.tp_new = PyGeometryLoftOptionsNew;
  loftOptionsType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  loftOptionsType.tp_init = (initproc)PyGeometryLoftOptionsInit;
  loftOptionsType.tp_dealloc = (destructor)PyGeometryLoftOptionsDealloc;
  loftOptionsType.tp_methods = PyGeometryLoftOptionsMethods;
};

//-----------------------------
// SetGeometryLoftOptionsTypes
//-----------------------------
// Set the  loft optinnames in the GeometryLoftOptionsType dictionary.
//
// These are for read only attibutes.
//
static void
SetGeometryLoftOptionsClassTypes(PyTypeObject& loftOptionsType)
{
/*
  std::cout << "=============== SetGeometryLoftOptionsClassTypes ==========" << std::endl;

  //PyDict_SetItemString(loftOptionsType.tp_dict, "num_pts", PyLong_AsLong(10));

  PyObject *o = PyLong_FromLong(1);
  PyDict_SetItemString(loftOptionsType.tp_dict, "num_pts", o);

  //PyDict_SetItem(loftOptionsType.tp_dict, "num_pts", o);

  std::cout << "[SetGeometryLoftOptionsClassTypes] Done! " << std::endl;
*/
 
};

#endif


