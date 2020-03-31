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

// The functions defined here implement the SV Python API solid model module group class. 
// It provides an interface to the SV solid model group class.
//
// The class name is 'Group'. It is referenced from the solid model module as 'solid.Group'.
//
//     aorta_solid_group = solid.Group()
//
#include "sv4gui_ModelIO.h"

//static PyObject * CreatePySolidGroup(sv4guiModel::Pointer solidGroup);

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//-------------------
// SolidGroup_read
//-------------------
// Read in an SV .pth file and create a SolidGroup object
// from its contents.
//
static sv4guiModel::Pointer 
SolidGroup_read(char* fileName)
{
  std::cout << "========== SolidGroup_read ==========" << std::endl;
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  std::cout << "[SolidGroup_read] fileName: " << fileName << std::endl;
  sv4guiModel::Pointer group;

  try {
      group = sv4guiModelIO().CreateGroupFromFile(std::string(fileName));
  } catch (...) {
      api.error("Error reading the model group file '" + std::string(fileName) + "'.");
      std::cout << "[SolidGroup_read] ERROR: can't read fileName: " << fileName << std::endl;
      return nullptr;
  }

  std::cout << "[SolidGroup_read] File read and group returned." << std::endl;
  auto solidGroup = dynamic_cast<sv4guiModel*>(group.GetPointer());
  int numSolids = solidGroup->GetTimeSize();
  std::cout << "[SolidGroup_read] Number of solids: " << numSolids << std::endl;

  return group;
}

//////////////////////////////////////////////////////
//       G r o u p  C l a s s  M e t h o d s        //
//////////////////////////////////////////////////////
//
// SV Python solid.Group methods. 

//-------------------------
// SolidGroup_get_time_size 
//-------------------------
//
// [TODO:DaveP] bad method name: get_number_time_steps() ?
//
PyDoc_STRVAR(SolidGroup_get_time_size_doc,
  "get_time_size_doc,(name) \n\ 
   \n\
   Store the polydata for the named contour into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
SolidGroup_get_time_size(PySolidGroup* self, PyObject* args)
{
/*
  int timestepSize = self->contourGroup->GetTimeSize();
  return Py_BuildValue("i", timestepSize); 
*/
}

//-----------------------
// SolidGroup_get_size 
//-----------------------
//
PyDoc_STRVAR(SolidGroup_number_of_models_doc,
  "get_size() \n\ 
   \n\
   Get the number of solid models in the group. \n\
   \n\
   Args: \n\
     None \n\
   Returns (int): The number of solid models in the group.\n\
");

static PyObject * 
SolidGroup_number_of_models(PySolidGroup* self, PyObject* args)
{
  auto solidGroup = self->solidGroup;
  int numSolidModels = solidGroup->GetTimeSize();
  std::cout << "[SolidGroup_number_of_models] Number of solid models: " << numSolidModels << std::endl;
  return Py_BuildValue("i", numSolidModels); 
}

//----------------------------
// SolidGroup_get_solid_model 
//----------------------------
PyDoc_STRVAR(SolidGroup_get_model_doc,
  "get_model(name) \n\ 
   \n\
   Store the polydata for the named contour into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
SolidGroup_get_model(PySolidGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int index;
  char* solidName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &index)) {
     return api.argsError();
  }

  auto solidGroup = self->solidGroup;
  int numSolids = solidGroup->GetTimeSize();
  std::cout << "[SolidGroup_get_solid_model] Number of solids: " << numSolids << std::endl;

  // Check for valid index.
  if ((index < 0) || (index > numSolids-1)) {
      api.error("The index argument '" + std::to_string(index) + "' is must be between 0 and " +
        std::to_string(numSolids-1));
      return nullptr;
  }

  // Get the solid model for the given index.
  //
  auto solidModelElement = solidGroup->GetModelElement(index);

  if (solidModelElement == nullptr) {
      api.error("ERROR getting the solid model for the index argument '" + std::to_string(index) + "'.");
      return nullptr;
  }
  auto ctype = solidModelElement->GetType();
  std::cout << "[SolidGroup_get_solid_model] ctype: " << ctype << std::endl;

  auto faceNames = solidModelElement->GetFaceNames();
  std::cout << "[SolidGroup_get_solid_model] Number of faces: " << faceNames.size() << std::endl;

  auto solidModel = solidModelElement->GetInnerSolid();
  std::cout << "[SolidGroup_get_solid_model] solidModel: " << solidModel << std::endl;

  // No inner solid is created for models read from .vtp or .stl files
  // so create a PolyData solid model and set its polydata.
  //
  if (solidModel == nullptr) {
      auto polydata = solidModelElement->GetWholeVtkPolyData();
      std::cout << "[SolidGroup_get_solid_model] polydata: " << polydata << std::endl;
      solidModel = new cvPolyDataSolid();
      solidModel->SetVtkPolyDataObject(polydata);
  } 

  // Create a PySolidModel object from the SV cvSolidModel 
  // object and return it as a PyObject.
  return CreatePySolidModelObject(solidModel);
}

//-----------------
// SolidGroup_write
//-----------------
//
PyDoc_STRVAR(SolidGroup_write_doc,
  "write(file_name) \n\ 
   \n\
   Write the contour group to an SV .pth file.\n\
   \n\
   Args: \n\
     file_name (str): The name of the file to write the contour group to.\n\
");

static PyObject *
SolidGroup_write(PySolidGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* fileName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      return api.argsError();
  }

/*

  try {
      if (sv3::ContourIO().Write(fileName, self->contourGroup) != SV_OK) {
          api.error("Error writing contour group to the file '" + std::string(fileName) + "'.");
          return nullptr;
      }
  } catch (const std::exception& readException) {
      api.error("Error writing contour group to the file '" + std::string(fileName) + "': " + readException.what());
      return nullptr;
  }
*/

  return SV_PYTHON_OK;
}


////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* SOLID_GROUP_CLASS = "Group";
// Dotted name that includes both the module name and the name of the 
// type within the module.
static char* SOLID_GROUP_MODULE_CLASS = "solid.Group";

PyDoc_STRVAR(SolidGroup_doc, "solid.Group functions");

//--------------------
// PySolidGroupMethods 
//--------------------
// Define the methods for the contour.Group class.
//
static PyMethodDef PySolidGroupMethods[] = {

  {"get_model", (PyCFunction)SolidGroup_get_model, METH_VARARGS, SolidGroup_get_model_doc},

  {"number_of_models", (PyCFunction)SolidGroup_number_of_models, METH_VARARGS, SolidGroup_number_of_models_doc},

  //{"get_time_size", (PyCFunction)SolidGroup_get_time_size, METH_NOARGS, SolidGroup_get_time_size_doc},

  //{"write", (PyCFunction)SolidGroup_write, METH_VARARGS, SolidGroup_write_doc},

  {NULL, NULL}
};

//-------------------------
// PySolidGroupClassType 
//-------------------------
// Define the Python type that stores SolidGroup data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
PyTypeObject PySolidGroupClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  SOLID_GROUP_MODULE_CLASS,     
  sizeof(PySolidGroup)
};

//------------------
// PySolidGroup_init
//------------------
// This is the __init__() method for the contour.Group class. 
//
// This function is used to initialize an object after it is created.
//
// Arguments:
//
//   fileName - An SV .ctgr pth file. A new SolidGroup object is created from 
//     the contents of the file. (optional)
//
static int 
PySolidGroupInit(PySolidGroup* self, PyObject* args)
{
  static int numObjs = 1;
  std::cout << "[PySolidGroupInit] New SolidGroup object: " << numObjs << std::endl;
  auto api = SvPyUtilApiFunction("|s", PyRunTimeErr, __func__);
  char* fileName = nullptr;
  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      api.argsError();
      return 1;
  }
  if (fileName != nullptr) {
      std::cout << "[PySolidGroupInit] File name: " << fileName << std::endl;
      self->solidGroupPointer = SolidGroup_read(fileName);
      self->solidGroup = dynamic_cast<sv4guiModel*>(self->solidGroupPointer.GetPointer());
  } else {
      self->solidGroup = sv4guiModel::New();
  }
  if (self->solidGroup == nullptr) { 
      std::cout << "[PySolidGroupInit] ERROR reading File name: " << fileName << std::endl;
      return -1;
  }
  numObjs += 1;
  return 0;
}

//-----------------
// PySolidGroupNew 
//-----------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PySolidGroupNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PySolidGroupNew] PySolidGroupNew " << std::endl;
  auto self = (PySolidModelClass*)type->tp_alloc(type, 0);
  //auto self = (PyContour*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PySolidGroupNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }
  return (PyObject *) self;
}

//-----------------------
// PySolidGroupDealloc 
//-----------------------
//
static void
PySolidGroupDealloc(PySolidGroup* self)
{
  std::cout << "[PySolidGroupDealloc] Free PySolidGroup" << std::endl;
  // Can't delete solidGroup because it has a protected detructor.
  //delete self->solidGroup;
  Py_TYPE(self)->tp_free(self);
}

//---------------------------
// SetSolidGroupTypeFields 
//---------------------------
// Set the Python type object fields that stores Contour data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetSolidGroupTypeFields(PyTypeObject& solidType)
{
  // Doc string for this type.
  solidType.tp_doc = "SolidGroup  objects";
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  solidType.tp_new = PySolidGroupNew;
  solidType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  solidType.tp_init = (initproc)PySolidGroupInit;
  solidType.tp_dealloc = (destructor)PySolidGroupDealloc;
  solidType.tp_methods = PySolidGroupMethods;
}

//--------------------
// CreatePySolidGroup
//--------------------
// Create a PySolidGroupType object.
//
// If the 'solidGroup' argument is not null then use that 
// for the PySolidGroupType.solidGroup data.
//
PyObject *
CreatePySolidGroup(sv4guiModel::Pointer solidGroup)
{
  std::cout << std::endl;
  std::cout << "========== CreatePySolidGroup ==========" << std::endl;
  std::cout << "[CreatePySolidGroup] Create SolidGroup object ... " << std::endl;
  auto solidGroupObj = PyObject_CallObject((PyObject*)&PySolidGroupClassType, NULL);
  auto pySolidGroup = (PySolidGroup*)solidGroupObj;

  if (solidGroup != nullptr) {
      //delete pySolidGroup->solidGroup;
      pySolidGroup->solidGroup = solidGroup;
  }
  std::cout << "[CreatePyContour] pySolidGroup id: " << pySolidGroup->id << std::endl;
  return solidGroupObj;
}

