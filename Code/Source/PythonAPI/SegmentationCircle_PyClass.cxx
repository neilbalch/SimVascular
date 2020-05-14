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
/*
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
*/

//----------------------
// PyCircleSegmentation 
//----------------------
// Define the Circle class (type).
//
typedef struct {
  PySegmentation super;
  double radius;
} PyCircleSegmentation;

extern PyTypeObject PyPathFrameType;
extern bool PyPathFrameGetData(PyObject* object, int& id, std::array<double,3>&  position, std::array<double,3>& normal, 
  std::array<double,3>& tangent, std::string& msg);

//////////////////////////////////////////////////////
//        U t i l i t y     F u n c t i o n s       //
//////////////////////////////////////////////////////

//--------------------------------
// CircleSegmentationSetFrameData
//--------------------------------
//
bool
CircleSegmentationSetFrameData(PyUtilApiFunction& api, PyObject* frameObj, sv3::PathElement::PathPoint& pathPoint, PyObject* planeObj,
   vtkPlane** plane, std::array<double,3>& point)
{
  if ((frameObj != nullptr) &&  (planeObj != nullptr)) {
      api.error("Both a 'frame' and 'plane' argument was given; only one is allowed.");
      return false;
  }

  if ((frameObj == nullptr) &&  (planeObj == nullptr)) {
      api.error("A 'frame' or 'plane' argument must be given.");
      return false;
  }

  // Get the frame argument value.
  //
  if (frameObj != nullptr) {
      std::string emsg;
      if (!PyPathFrameGetData(frameObj, pathPoint.id, pathPoint.pos, pathPoint.rotation, pathPoint.tangent, emsg)) {
          api.error("The 'frame' argument " + emsg);
          return false;
      }
      point[0] = pathPoint.pos[0];
      point[1] = pathPoint.pos[1];
      point[2] = pathPoint.pos[2];
  }

  // Get the plane data.
  //
  *plane = nullptr;
  if (planeObj != nullptr) {
      *plane = (vtkPlane*)vtkPythonUtil::GetPointerFromObject(planeObj, "vtkPlane");
      if (*plane == nullptr) {
          PyErr_SetString(PyExc_ValueError, "The 'plane' argument must be a vtkPlane object.");
          return false;
      }
      (*plane)->GetOrigin(point.data());
  }

  return true;
}

//////////////////////////////////////////////////////
//          C l a s s    M e t h o d s              //
//////////////////////////////////////////////////////
//
// Python API functions. 

//-------------------------------
// CircleSegmentation_get_radius 
//-------------------------------
//
PyDoc_STRVAR(CircleSegmentation_get_radius_doc,
  "get_radius(r) \n\ 
   \n\
   Get the radius for a circle segmentation. \n\
   \n\
   Returns (float): The radius of the circle. \n\
");

static PyObject*
CircleSegmentation_get_radius(PyCircleSegmentation* self, PyObject* args)
{
  auto contour = dynamic_cast<sv3::circleContour*>(self->super.contour);
  auto radius = contour->GetRadius();
  return Py_BuildValue("d", radius);
}

//------------------------------
// CircleSegmentation_set_frame  
//------------------------------
//
PyDoc_STRVAR(CircleSegmentation_set_frame_doc,
  "set_frame(frame) \n\ 
   \n\
   Set the circle segmentation coordinate frame using a PathFrame object. \n\
   \n\
   Args: \n\
     frame (PathFrame): The PathFrame object defing the circle's center and coordinate frame. \n\
   \n\
");

static PyObject*
CircleSegmentation_set_frame(PyCircleSegmentation* self, PyObject* args, PyObject *kwargs)
{
  std::cout << "[CircleSegmentation_set_frame] ========== CircleSegmentation_set_frame ========== " << std::endl;
  auto api = PyUtilApiFunction("|O!O!", PyRunTimeErr, __func__);
  static char *keywords[] = {"frame", "plane", NULL};
  PyObject* frameArg = nullptr;
  PyObject* planeArg = nullptr;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &PyPathFrameType, &frameArg, &planeArg)) {
      return nullptr;
  }

  std::array<double,3> point;
  sv3::PathElement::PathPoint pathPoint;
  vtkPlane* plane = nullptr;

  // Extract data from the input arguments.
  if (!CircleSegmentationSetFrameData(api, frameArg, pathPoint, planeArg, &plane, point)) {
      return nullptr;
  }

  auto circleContour = dynamic_cast<sv3::circleContour*>(self->super.contour);

  // Set the circle path point if it is given, else set its plane geometry.
  //
  if (frameArg != nullptr) {
      circleContour->SetPathPoint(pathPoint);
  } else { 
      circleContour->SetPlaneGeometry(plane);
  }

  // Set the circle center (control point 0);
  int index = 0;
  circleContour->SetControlPoint(index, point);

  Py_RETURN_NONE;
}

//-------------------------------
// CircleSegmentation_set_radius 
//-------------------------------
//
PyDoc_STRVAR(CircleSegmentation_set_radius_doc,
  "set_radius(radius) \n\ 
   \n\
   Set the radius for a circle segmentation. \n\
   \n\
   Args: \n\
     radius (float): The radius of the circle. \n\
");

static PyObject*
CircleSegmentation_set_radius(PyCircleSegmentation* self, PyObject* args)
{
  auto api = PyUtilApiFunction("d", PyRunTimeErr, __func__);
  double radius = 0.0;

  if (!PyArg_ParseTuple(args, api.format, &radius)) {
      return nullptr;
  }

  if (radius <= 0.0) {
      api.error("The 'radius' argument must be > 0.");
      return nullptr;
  }

  auto contour = dynamic_cast<sv3::circleContour*>(self->super.contour);
  contour->SetRadius(radius);

  Py_RETURN_NONE;
}

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* SEGMENTATION_CIRCLE_CLASS = "Circle";
static char* SEGMENTATION_CIRCLE_MODULE_CLASS = "segmentation.Circle";

PyDoc_STRVAR(PyCircleSegmentationClass_doc, 
   "Circle(radius, plane=None, frame=None)  \n\
   \n\
   The CircleSegmentation class provides an interface for creating a circle segmentation. \n\
   A CircleSegmentation object is created using a vtkPlane or PathFrame object. \n\
   \n\
   Args: \n\
     radius (float): The circle radius. \n\
     plane (Optional[vktPlane]): A vktPlane object defing the circle's center and coordinate frame. \n\
     frame (Optional[PathFrame]): A PathFrame object defing the circle's center and coordinate frame. \n\
   \n\
");

//-----------------------------
// PyCircleSegmentationMethods 
//-----------------------------
//
static PyMethodDef PyCircleSegmentationMethods[] = {

  // { "set_center", (PyCFunction)CircleSegmentation_set_center, METH_VARARGS, CircleSegmentation_set_center_doc},

  { "get_radius", (PyCFunction)CircleSegmentation_get_radius, METH_VARARGS, CircleSegmentation_get_radius_doc},

  { "set_frame", (PyCFunction)CircleSegmentation_set_frame, METH_VARARGS|METH_KEYWORDS, CircleSegmentation_set_frame_doc},
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
// A 'radius' and 'frame' or 'plane' argumnet are required.
//
static int
PyCircleSegmentationInit(PyCircleSegmentation* self, PyObject* args, PyObject *kwargs)
{
  static int numObjs = 1;
  std::cout << "[PyCircleSegmentationInit] Init Circle Segmentation object: " << numObjs << std::endl;
  auto api = PyUtilApiFunction("O!|O!O", PyRunTimeErr, "CircleSegmentation");
  static char *keywords[] = {"radius", "frame", "plane", NULL};
  PyObject* radiusArg = nullptr;
  PyObject* frameArg = nullptr;
  PyObject* planeArg = nullptr;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &PyFloat_Type, &radiusArg, 
        &PyPathFrameType, &frameArg, &planeArg)) {
      return -1;
  }


  // Get the radius argument value.
  double radius = PyFloat_AsDouble(radiusArg);
  std::cout << "[PyCircleSegmentationInit] radius: " << radius << std::endl;
  if (radius <= 0.0) { 
      api.error("The 'radius' argument must be > 0.");
      return -1;
  }

  // Extract data from the input arguments.
  std::array<double,3> point;
  sv3::PathElement::PathPoint pathPoint;
  vtkPlane* plane = nullptr;

  if (!CircleSegmentationSetFrameData(api, frameArg, pathPoint, planeArg, &plane, point)) {
      return -1;
  }

  // Create the circle contour.
  self->super.contour = new sv3::circleContour();
  auto circleContour = dynamic_cast<sv3::circleContour*>(self->super.contour);

  // Set the circle path point if it is given, else set its plane geometry.
  //
  if (frameArg != nullptr) {
      circleContour->SetPathPoint(pathPoint);
  } else { 
      circleContour->SetPlaneGeometry(plane);
  }

  // Set the circle point and radius.
  //
  // The circle center is set to the projection of the 'point' 
  // onto the given plane or frame.
  //
  circleContour->SetControlPointByRadius(radius, point.data());

  numObjs += 1;
  return 0;
}

//-------------------------
// PyCircleSegmentationNew 
//-------------------------
//
static PyObject *
PyCircleSegmentationNew(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{ 
  std::cout << "[PyCircleSegmentationNew] New CircleSegmentation " << std::endl;
  auto self = (PyCircleSegmentation*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyCircleSegmentationNew] ERROR: alloc failed." << std::endl;
      return nullptr;
  }
  return (PyObject*)self;
}

//-----------------------------
// PyCircleSegmentationDealloc 
//-----------------------------
//
static void
PyCircleSegmentationDealloc(PyCircleSegmentation* self)
{ 
  std::cout << "[PyCircleSegmentationDealloc] **** Free PyCircleSegmentation ****" << std::endl;
  delete self->super.contour;
  Py_TYPE(self)->tp_free(self);
}

//-------------------------------------
// Define the PyCircleSegmentationType 
//-------------------------------------
// Define the Python type object that stores Segmentation data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static PyTypeObject PyCircleSegmentationType = {
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
  segType.tp_doc = PyCircleSegmentationClass_doc; 

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  segType.tp_new = PyCircleSegmentationNew;
  //.tp_new = PyType_GenericNew,

  segType.tp_base = &PySegmentationType;

  segType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  segType.tp_init = (initproc)PyCircleSegmentationInit;
  segType.tp_dealloc = (destructor)PyCircleSegmentationDealloc;
  segType.tp_methods = PyCircleSegmentationMethods;
};


