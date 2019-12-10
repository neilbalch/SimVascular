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

// The functions defined the Pythom 'solid.Model' class used to store solid modeling data. 
// The 'Model' class cannot be imported and must be used prefixed by the module 
//  name. 
//
//--------------
// PySolidModel
//--------------
//
typedef struct {
  PyObject_HEAD
  int id;
  SolidModel_KernelT kernel;
  cvSolidModel* solidModel;
} PySolidModelClass;

//////////////////////////////////////////////////////
//          U t i l i t y   F u n c t i o n s       //
//////////////////////////////////////////////////////

//-----------------
// CheckSolidModel
//-----------------
// Check if a solid model is in the repository
// and that its type is SOLID_MODEL_T.
//
static cvSolidModel *
CheckSolidModel(SvPyUtilApiFunction& api, char* name)
{
  auto model = gRepository->GetObject(name);
  if (model == NULL) {
      api.error("The solid model '"+std::string(name)+"' is not in the repository.");
      return nullptr;
  }
  auto type = gRepository->GetType(name);
  if (type != SOLID_MODEL_T) {
      api.error("'" + std::string(name) + "' is not a solid model.");
      return nullptr;
  }

  return (cvSolidModel*)model;
}

//-------------------------
// CheckSimplificationName
//-------------------------
// Check for a valid model simplification name. 
//
// Returns the equivalent SolidModel_SimplifyT type 
// or SM_Simplify_Invalid if the name is not valid. 
//
static SolidModel_SimplifyT
CheckSimplificationName(SvPyUtilApiFunction& api, char* name)
{
  if (!name) {
      return SM_Simplify_All;
  }

  auto smpType = SolidModel_SimplifyT_StrToEnum(name);
  if (smpType == SM_Simplify_Invalid ) {
      api.error("Unknown simplification argument '"+std::string(name)+ "'. Valid types are: All or None.");
  }

  return smpType;
}

//---------------
// CheckGeometry
//---------------
// Check if the solid model object has geometry.
//
// This is really used to set the error message 
// in a single place. 
//
/*
static cvSolidModel *
CheckGeometry(SvPyUtilApiFunction& api, PySolidModel *self)
{
  auto geom = self->solidModel;
  if (geom == NULL) {
      api.error("The solid model object does not have geometry.");
      return nullptr;
  }

  return geom;
}
*/

/////////////////////////////////////////////////////////////////
//              C l a s s   F u n c t i o n s                  //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the SolidModel class. 

////////////////////////////////////////////////////////
//           C l a s s    D e f i n i t i o n         //
////////////////////////////////////////////////////////

static char* SOLID_MODEL_CLASS = "Model";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* SOLID_MODEL_MODULE_CLASS = "solid.Model";

PyDoc_STRVAR(SolidModelClass_doc, "solid model class methods.");

//--------------------------
// PySolidModelClassMethods
//--------------------------
// Define method names for SolidModel class 
//
static PyMethodDef PySolidModelClassMethods[] = {

  {NULL,NULL}
};

//------------------
// PySolidModelInit 
//------------------
// This is the __init__() method for the SolidModel class. 
//
// This function is used to initialize an object after it is created.
//
static int
PySolidModelInit(PySolidModelClass* self, PyObject* args, PyObject *kwds)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, "SolidModel");
  static int numObjs = 1;
  std::cout << "[PySolidModelInit] New PySolidModel object: " << numObjs << std::endl;
  char* kernelName = nullptr; 
  if (!PyArg_ParseTuple(args, "s", &kernelName)) {
      return -1;
  }
  std::cout << "[PySolidModelInit] Kernel name: " << kernelName << std::endl;
  auto kernel = kernelNameEnumMap.at(std::string(kernelName));
  cvSolidModel* solidModel;

  try {
      solidModel = SolidCtorMap[kernel]();
  } catch (const std::bad_function_call& except) {
      api.error("The '" + std::string(kernelName) + "' kernel is not supported.");
      return -1;
  }

  self->id = numObjs;
  self->kernel = kernel; 
  self->solidModel = solidModel; 
  numObjs += 1;
  return 0;
}

//------------------
// PySolidModelType 
//------------------
// This is the definition of the SolidModel class.
//
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject PySolidModelClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = SOLID_MODEL_MODULE_CLASS, 
  .tp_basicsize = sizeof(PySolidModelClass) 
};

//-----------------
// PySolidModelNew 
//-----------------
//
static PyObject *
PySolidModelNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PySolidModelNew] New SolidModel" << std::endl;
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, "SolidModel");
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

  auto self = (PySolidModelClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->id = 1;
  }

  return (PyObject *) self;
}

//---------------------
// PySolidModelDealloc 
//---------------------
//
static void
PySolidModelDealloc(PySolidModelClass* self)
{
  std::cout << "[PySolidModelDealloc] Free PySolidModel: " << self->id << std::endl;
  //delete self->solidModel;
  Py_TYPE(self)->tp_free(self);
}

//-------------------------
// SetSolidModelTypeFields 
//-------------------------
// Set the Python type object fields that stores SolidModel data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetSolidModelTypeFields(PyTypeObject& solidModelType)
{
  // Doc string for this type.
  solidModelType.tp_doc = SolidModelClass_doc; 
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  solidModelType.tp_new = PySolidModelNew;
  //solidModelType.tp_new = PyType_GenericNew,
  solidModelType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  solidModelType.tp_init = (initproc)PySolidModelInit;
  solidModelType.tp_dealloc = (destructor)PySolidModelDealloc;
  solidModelType.tp_methods = PySolidModelClassMethods;
};

//----------------------
// CreateSolidModelType 
//----------------------
static PySolidModelClass * 
CreateSolidModelType()
{
  return PyObject_New(PySolidModelClass, &PySolidModelClassType);
}

