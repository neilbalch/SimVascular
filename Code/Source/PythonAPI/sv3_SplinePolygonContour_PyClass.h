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

// The functions defined here implement the SV Python API spline polygon countour class. 
//
// The class name is 'SplinePolygon'. 

#include "SimVascular.h"
#include "sv_misc_utils.h"
#include "sv3_Contour.h"
#include "sv3_Contour_PyModule.h"
#include "sv3_SplinePolygonContour.h"
#include "sv_arg.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"

#include "Python.h"
#include "sv_PyUtils.h"
#include "sv2_globals.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

//-----------------
// PySplinePolygonContour
//-----------------
// Define the SplinePolygon class (type).
//
typedef struct {
  PyContour super;
} PySplinePolygonContour;


//////////////////////////////////////////////////////
//          C l a s s    M e t h o d s              //
//////////////////////////////////////////////////////

//--------------------------------
// splinePolygonContour_available
//--------------------------------
//
PyDoc_STRVAR(SplinePolygonContour_available_doc,
  "available(kernel)                                    \n\ 
                                                                 \n\
   Set the computational kernel used to segment image data.       \n\
                                                                 \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: SplinePolygon, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject *  
SplinePolygonContour_available(PyObject* self, PyObject* args)
{
  return Py_BuildValue("s","polygonContour Available");
}

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* CONTOUR_SPLINE_POLYGON_CLASS = "SplinePolygon";
static char* CONTOUR_SPLINE_POLYGON_MODULE_CLASS = "contour.SplinePolygon";

PyDoc_STRVAR(PySplinePolygonContourClass_doc, "circle contour functions");

//------------------------------
// SplinePolygonContour_methods
//------------------------------
// Define the module methods.
//
PyMethodDef PySplinePolygonContourMethods[] = {

  {"available", SplinePolygonContour_available, METH_NOARGS, SplinePolygonContour_available_doc},

  {NULL, NULL}
};

//---------------------
// PySplinePolygonContourInit 
//---------------------
// This is the __init__() method for the Contour class. 
//
// This function is used to initialize an object after it is created.
//
static int
PySplinePolygonContourInit(PySplinePolygonContour* self, PyObject* args, PyObject *kwds)
{
  static int numObjs = 1;
  std::cout << "[PySplinePolygonContourInit] New SplinePolygon Contour object: " << numObjs << std::endl;
  //self->super.count = numObjs;
  self->super.contour = new sv3::ContourSplinePolygon();
  numObjs += 1;
  return 0;
}

//--------------------
// PySplinePolygonContourNew 
//--------------------
//
static PyObject *
PySplinePolygonContourNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{ 
  std::cout << "[PySplinePolygonContourNew] PySplinePolygonContourNew " << std::endl;
  auto self = (PySplinePolygonContour*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->super.id = 2;
  }
  return (PyObject *) self;
}

//------------------------
// PySplinePolygonContourDealloc 
//------------------------
//
static void
PySplinePolygonContourDealloc(PySplinePolygonContour* self)
{ 
  std::cout << "[PySplinePolygonContourDealloc] Free PySplinePolygonContour" << std::endl;
  delete self->super.contour;
  Py_TYPE(self)->tp_free(self);
}

//------------------------------------
// Define the ContourType type object
//------------------------------------
// Define the Python type object that stores Contour data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static PyTypeObject PySplinePolygonContourClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  .tp_name = CONTOUR_SPLINE_POLYGON_MODULE_CLASS, 
  .tp_basicsize = sizeof(PySplinePolygonContour)
};

//----------------------------
// SetSplinePolygonContourTypeFields
//----------------------------
// Set the Python type object fields that stores Contour data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetSplinePolygonContourTypeFields(PyTypeObject& contourType)
 {
  // Doc string for this type.
  contourType.tp_doc = "SplinePolygon Contour  objects";

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  contourType.tp_new = PySplinePolygonContourNew;
  //.tp_new = PyType_GenericNew,

  contourType.tp_base = &PyContourClassType;

  contourType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  contourType.tp_init = (initproc)PySplinePolygonContourInit;
  contourType.tp_dealloc = (destructor)PySplinePolygonContourDealloc;
  contourType.tp_methods = PySplinePolygonContourMethods;
};
