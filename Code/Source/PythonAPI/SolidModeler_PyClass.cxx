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

// Define the Python 'solid.Modeler' class. This class defines modeling operations that
// create new Python 'solid.Solid' objects. 
//
#ifndef PYAPI_SOLID_MODELER_H
#define PYAPI_SOLID_MODELER_H

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

//-------------------
// SolidModelerClass 
//-------------------
// Define the SolidModelerClass.
//
typedef struct {
  PyObject_HEAD
  int id;
  SolidModel_KernelT kernel;
} PySolidModelerClass;

////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//
// Python 'Modeler' class methods. 
//

//---------------------
// SolidModel_cylinder 
//---------------------
//
PyDoc_STRVAR(SolidModeler_cylinder_doc,
  "cylinder(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModeler_cylinder(PySolidModelerClass* self, PyObject* args)
{
  std::cout << "[SolidModeler_cylinder] ========== SolidModeler_cylinder ==========" << std::endl;
  auto api = SvPyUtilApiFunction("ddOO", PyRunTimeErr, __func__);
  char *objName;
  double radius;
  double length;
  PyObject* centerArg;
  PyObject* axisArg;

  if (!PyArg_ParseTuple(args, api.format, &radius, &length, &centerArg , &axisArg)) {
      return api.argsError();
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(centerArg, emsg)) {
      api.error("The cylinder center argument " + emsg);
      return nullptr;
  }

  if (!svPyUtilCheckPointData(axisArg, emsg)) {
      api.error("The cylinder axis argument " + emsg);
      return nullptr;
  }

  if (radius <= 0.0) { 
      api.error("The radius argument is <= 0.0."); 
      return nullptr;
  }

  if (length <= 0.0) { 
      api.error("The length argument is <= 0.0."); 
      return nullptr;
  }

  double center[3];
  double axis[3];
  for (int i = 0; i < 3; i++) {
    axis[i] = PyFloat_AsDouble(PyList_GetItem(axisArg,i));
    center[i] = PyFloat_AsDouble(PyList_GetItem(centerArg,i));
  }

  // Instantiate the new solid:
  //
  std::cout << "[SolidModel_cylinder] Kernel: " << self->kernel << std::endl;

  auto model = CreateSolidModelObject(self->kernel);

  std::cout << "[SolidModel_cylinder] Model: " << model << std::endl;
  if (model == NULL) {
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

  std::cout << "[SolidModel_cylinder] Create cylinder ... " << std::endl;
  if (model->MakeCylinder(radius, length, center, axis) != SV_OK) {
      delete model;
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

// CreateSolidModelType

  Py_RETURN_NONE;
}


////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* SOLID_MODELER_CLASS = "Modeler";
static char* SOLID_MODELER_MODULE_CLASS = "solid.Modeler";
// The name of the Modeler class veriable that contains all of the kernel types.
static char* SOLID_MODELER_CLASS_VARIBLE_NAMES = "names";

PyDoc_STRVAR(SolidModelerClass_doc, "solid modeling kernel class functions");

//---------------------
// SolidModelerMethods
//---------------------
//
static PyMethodDef PySolidModelerClassMethods[] = {

  { "cylinder", (PyCFunction)SolidModeler_cylinder, METH_VARARGS, SolidModeler_cylinder_doc },

  {NULL, NULL}
};

//--------------------
// PySolidModelerInit 
//--------------------
// This is the __init__() method for the solid.Modeler class. 
//
// This function is used to initialize an object after it is created.
//
static int
PySolidModelerInit(PySolidModelerClass* self, PyObject* args, PyObject *kwds)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, "SolidModeler");
  static int numObjs = 1;
  std::cout << "[PySolidModelerInit] New PySolidModeler object: " << numObjs << std::endl;
  char* kernelName = nullptr;
  if (!PyArg_ParseTuple(args, "s", &kernelName)) {
      return -1;
  }
  std::cout << "[PySolidModelerInit] Kernel name: " << kernelName << std::endl;
  auto kernel = kernelNameEnumMap.at(std::string(kernelName));

  self->id = numObjs;
  self->kernel = kernel;
  numObjs += 1;
  return 0;
}

//------------------------------------
// Define the SolidType type object
//------------------------------------
// Define the Python type object that stores contour.kernel types. 
//
static PyTypeObject PySolidModelerClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = SOLID_MODELER_MODULE_CLASS,
  .tp_basicsize = sizeof(PySolidModelerClass)
};

//-------------------
// PySolidModelerNew 
//-------------------
//
static PyObject *
PySolidModelerNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PySolidModelerNew] New SolidModeler" << std::endl;
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, "SolidModeler");
  char* kernelName = nullptr; 
  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  SolidModel_KernelT kernel;

  try {
      kernel = kernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." +
          " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  auto self = (PySolidModelerClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->id = 1;
  }

  return (PyObject *) self;
}

//---------------------
// PySolidModelerDealloc 
//---------------------
//
static void
PySolidModelerDealloc(PySolidModelerClass* self)
{
  std::cout << "[PySolidModelerDealloc] Free PySolidModeler: " << self->id << std::endl;
  //delete self->solidModel;
  Py_TYPE(self)->tp_free(self);
}

//-------------------------
// SetSolidModelerTypeFields 
//-------------------------
// Set the Python type object fields that stores SolidModeler data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetSolidModelerTypeFields(PyTypeObject& solidModelType)
{
  // Doc string for this type.
  solidModelType.tp_doc = SolidModelerClass_doc; 
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  solidModelType.tp_new = PySolidModelerNew;
  //solidModelType.tp_new = PyType_GenericNew,
  solidModelType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  solidModelType.tp_init = (initproc)PySolidModelerInit;
  solidModelType.tp_dealloc = (destructor)PySolidModelerDealloc;
  solidModelType.tp_methods = PySolidModelerClassMethods;
};

//----------------------
// CreateSolidModelerType 
//----------------------
static PySolidModelerClass * 
CreateSolidModelerType()
{
  return PyObject_New(PySolidModelerClass, &PySolidModelerClassType);
}

#endif


