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

// The functions defined here implement the SV Python API circle segmentation class. 
//
// The class name is 'segmentation.Circle'.
//
#include "SimVascular.h"
#include "sv_misc_utils.h"
#include "sv3_Contour.h"
#include "Segmentation_PyModule.h"
#include "sv3_CircleContour.h"
#include "sv_arg.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"
#include "sv2_globals.h"
#include "Python.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

//----------------------
// PyCircleSegmentation 
//----------------------
// Define the Circle class (type).
//
typedef struct {
  PySegmentation super;
  double radius;
} PyCircleSegmentation;


//////////////////////////////////////////////////////
//          C l a s s    M e t h o d s              //
//////////////////////////////////////////////////////
//
// Python API functions. 

PyDoc_STRVAR(CircleSegmentation_set_radius_doc,
  "set_radius(radius) \n\ 
   \n\
   Set the radius for a circle segmentation. \n\
   \n\
   Args: \n\
     radius (float): The radius of the circle. \n\
");

//-------------------------------
// CircleSegmentation_set_radius 
//-------------------------------
//
static PyObject*
CircleSegmentation_set_radius(PyCircleSegmentation* self, PyObject* args)
{
  double radius = 0.0;

  if (!PyArg_ParseTuple(args, "d", &radius)) {
      return nullptr;
  }
  auto pmsg = "[PyCircleSegmentation::set_radius] ";
  std::cout << pmsg << "Set radius ..." << std::endl;
  std::cout << pmsg << "Radius: " << radius << std::endl;
  //auto contour = dynamic_cast<CircleContour*>(self->super.contour);
  //contour->SetRadius(radius);

  Py_RETURN_NONE;
}

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* SEGMENTATION_CIRCLE_CLASS = "Circle";
static char* SEGMENTATION_CIRCLE_MODULE_CLASS = "segmentation.Circle";

PyDoc_STRVAR(PyCircleSegmentationClass_doc, "Circle segmentation methods.");

//-----------------------------
// PyCircleSegmentationMethods 
//-----------------------------
//
static PyMethodDef PyCircleSegmentationMethods[] = {

  { "set_radius", (PyCFunction)CircleSegmentation_set_radius, METH_VARARGS, CircleSegmentation_set_radius_doc},

  {NULL, NULL}
};

//--------------------------
// PyCircleSegmentationInit 
//--------------------------
// This is the __init__() method for the CircleSegmentation class. 
//
// This function is used to initialize an object after it is created.
//
static int
PyCircleSegmentationInit(PyCircleSegmentation* self, PyObject* args, PyObject *kwds)
{
  static int numObjs = 1;
  std::cout << "[PyCircleSegmentationInit] New Circle Segmentation object: " << numObjs << std::endl;
  //self->super.count = numObjs;
  //self->super.contour = new CircleContour();
  self->super.contour = new sv3::circleContour();
  numObjs += 1;
  return 0;
}

//-------------------------
// PyCircleSegmentationNew 
//-------------------------
//
static PyObject *
PyCircleSegmentationNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{ 
  std::cout << "[PyCircleSegmentationNew] PyCircleSegmentationNew " << std::endl;
  auto self = (PyCircleSegmentation*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->super.id = 2;
  }
  return (PyObject *) self;
}

//-----------------------------
// PyCircleSegmentationDealloc 
//-----------------------------
//
static void
PyCircleSegmentationDealloc(PyCircleSegmentation* self)
{ 
  std::cout << "[PyCircleSegmentationDealloc] Free PyCircleSegmentation" << std::endl;
  delete self->super.contour;
  Py_TYPE(self)->tp_free(self);
}

//------------------------------------------
// Define the PyCircleSegmentationClassType 
//------------------------------------------
// Define the Python type object that stores Segmentation data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static PyTypeObject PyCircleSegmentationClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  .tp_name = SEGMENTATION_CIRCLE_MODULE_CLASS, 
  .tp_basicsize = sizeof(PyCircleSegmentation)
};

//---------------------------------
// SetCircleSegmentationTypeFields 
//---------------------------------
// Set the Python type object fields that stores Segmentation data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetCircleSegmentationTypeFields(PyTypeObject& segType)
 {
  // Doc string for this type.
  segType.tp_doc = "Circle segmentation objects";

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  segType.tp_new = PyCircleSegmentationNew;
  //.tp_new = PyType_GenericNew,

  segType.tp_base = &PySegmentationClassType;

  segType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  segType.tp_init = (initproc)PyCircleSegmentationInit;
  segType.tp_dealloc = (destructor)PyCircleSegmentationDealloc;
  segType.tp_methods = PyCircleSegmentationMethods;
};


