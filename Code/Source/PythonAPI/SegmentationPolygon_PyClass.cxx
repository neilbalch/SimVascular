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

// The functions defined here implement the SV Python API polygon segmentation class. 
//
// The class name is 'segmentation.Polygon'. 
//
#include "SimVascular.h"
#include "sv_misc_utils.h"
#include "sv3_Contour.h"
#include "Segmentation_PyModule.h"
#include "sv3_PolygonContour.h"
#include "sv_arg.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"

#include "Python.h"
#include "sv2_globals.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

//-----------------------
// PyPolygonSegmentation
//-----------------------
// Define the Polygon class (type).
//
typedef struct {
  PySegmentation super;
} PyPolygonSegmentation;

//////////////////////////////////////////////////////
//          C l a s s    M e t h o d s              //
//////////////////////////////////////////////////////
//
// Python API functions. 

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* SEGMENTATION_POLYGON_CLASS = "Polygon";
static char* SEGMENTATION_POLYGON_MODULE_CLASS = "segmentation.Polygon";

PyDoc_STRVAR(PyPolygonSegmentationClass_doc, "polygon segmentation functions");

PyMethodDef PyPolygonSegmentationMethods[] = {
  {NULL, NULL}
};

//---------------------------
// PyPolygonSegmentationInit 
//---------------------------
// This is the __init__() method for the Segmentation class. 
//
// This function is used to initialize an object after it is created.
//
static int
PyPolygonSegmentationInit(PyPolygonSegmentation* self, PyObject* args, PyObject *kwds)
{
  static int numObjs = 1;
  std::cout << "[PyPolygonSegmentationInit] New Polygon Segmentation object: " << numObjs << std::endl;
  //self->super.count = numObjs;
  //self->super.contour = new PolygonContour();
  self->super.contour = new sv3::circleContour();
  numObjs += 1;
  return 0;
}

//--------------------------
// PyPolygonSegmentationNew 
//--------------------------
//
static PyObject *
PyPolygonSegmentationNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{ 
  std::cout << "[PyPolygonSegmentationNew] PyPolygonSegmentationNew " << std::endl;
  auto self = (PyPolygonSegmentation*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->super.id = 2;
  }
  return (PyObject *) self;
}

//------------------------------
// PyPolygonSegmentationDealloc 
//------------------------------
//
static void
PyPolygonSegmentationDealloc(PyPolygonSegmentation* self)
{ 
  std::cout << "[PyPolygonSegmentationDealloc] Free PyPolygonSegmentation" << std::endl;
  delete self->super.contour;
  Py_TYPE(self)->tp_free(self);
}

//--------------------------------
// PyPolygonSegmentationClassType 
//--------------------------------
// Define the Python type object that stores Segmentation data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static PyTypeObject PyPolygonSegmentationClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  .tp_name = SEGMENTATION_POLYGON_MODULE_CLASS, 
  .tp_basicsize = sizeof(PyPolygonSegmentation)
};

//----------------------------------
// SetPolygonSegmentationTypeFields 
//----------------------------------
// Set the Python type object fields that stores Segmentation  data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetPolygonSegmentationTypeFields(PyTypeObject& contourType)
 {
  // Doc string for this type.
  contourType.tp_doc = "Polygon segmentation objects";

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  contourType.tp_new = PyPolygonSegmentationNew;
  //.tp_new = PyType_GenericNew,

  contourType.tp_base = &PySegmentationClassType;
  contourType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  contourType.tp_init = (initproc)PyPolygonSegmentationInit;
  contourType.tp_dealloc = (destructor)PyPolygonSegmentationDealloc;
  contourType.tp_methods = PyPolygonSegmentationMethods;
};

