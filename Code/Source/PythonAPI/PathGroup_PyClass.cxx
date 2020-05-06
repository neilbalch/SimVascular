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

// The functions defined here implement the SV Python API 'path' module 'Group' class. 
// It provides an interface to the SV PathGroup class.
//
//     aorta_path_group = path.Group()
//
#include "sv3_PathGroup.h"
#include "PathGroup_PyClass.h"
#include "sv3_PathIO.h"

using sv3::PathGroup;

static PyObject * CreatePyPathGroup(PathGroup* pathGroup);

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//----------------
// PathGroup_read
//----------------
// Read in an SV .pth file and create a PathGroup object
// from its contents.
//
static sv3::PathGroup * 
PathGroup_read(char* fileName)
{
  std::cout << "========== PathGroup_read ==========" << std::endl;
  auto api = PyUtilApiFunction("", PyRunTimeErr, __func__);

  std::cout << "[PathGroup_read] fileName: " << fileName << std::endl;
  sv3::PathGroup* pathGroup;

  try {
      pathGroup = sv3::PathIO().ReadFile(fileName);
      if (pathGroup == nullptr) { 
          api.error("Error reading file '" + std::string(fileName) + "'.");
          return nullptr;
      }
      int numElements = pathGroup->GetTimeSize();
      std::cout << "[PathGroup_read] numElements: " << numElements << std::endl;

  } catch (const std::exception& readException) {
      api.error("Error reading file '" + std::string(fileName) + "': " + readException.what());
      return nullptr;
  }

  /*
  for (int i=0; i<svPathGrp->GetTimeSize(); i++) {
        path->SetPathElement(static_cast<sv4guiPathElement*>(svPathGrp->GetPathElement(i)),i);
  }
  */

  return pathGroup;
}

//////////////////////////////////////////////////////
//          C l a s s   M e t h o d s               //
//////////////////////////////////////////////////////
//
// SV Python Path Group methods. 

//--------------------
// PathGroup_set_path 
//--------------------
//
PyDoc_STRVAR(PathGroup_set_path_doc,
  "set_path(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_path(PyPathGroup* self, PyObject* args)
{
  auto api = PyUtilApiFunction("OI", PyRunTimeErr, __func__);
  PyObject* pathArg;
  int timeStep = -2;

  if (!PyArg_ParseTuple(args, api.format, &pathArg, &timeStep)) {
     return api.argsError();
  }

  auto pmsg = "[PathGroup_set_path] ";
  std::cout << pmsg << std::endl;

  // Check that the path argument is a SV Python Path object.
  if (!PyObject_TypeCheck(pathArg, &PyPathType)) {
      api.error("The 'path' argument is not a Path object.");
      return nullptr;
  }

  // Get the PathElement object.
  auto pathObj = (PyPathClass*)pathArg;
  auto path = pathObj->path;

  // Check time step.
  int timestepSize = self->pathGroup->GetTimeSize();

  if (timeStep < 0) {
      api.error("The 'time_step' argument must be >= 0.");
      return nullptr;
  }

  // Add the path to the group.
  if (timeStep+1 >= timestepSize) {
      self->pathGroup->Expand(timeStep);
      self->pathGroup->SetPathElement(path, timeStep);
  } else {
      self->pathGroup->SetPathElement(path, timeStep);
  }

  // [TODO:DaveP] Is this the right thing to do?
  Py_INCREF(pathArg);
            
  return Py_None; 
}

//-------------------------
// PathGroup_get_time_size 
//-------------------------
//
// [TODO:DaveP] bad method name: get_number_time_steps() ?
//
PyDoc_STRVAR(PathGroup_get_time_size_doc,
  "set_path(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_time_size(PyPathGroup* self, PyObject* args)
{
  int timestepSize = self->pathGroup->GetTimeSize();
  return Py_BuildValue("i", timestepSize); 
}

//--------------------
// PathGroup_get_path 
//--------------------
PyDoc_STRVAR(PathGroup_get_path_doc,
  "get_path(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_path(PyPathGroup* self, PyObject* args)
{
  auto api = PyUtilApiFunction("i", PyRunTimeErr, __func__);
  int index;
  char* pathName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &index)) {
     return api.argsError();
  }
        
  auto pathGroup = self->pathGroup;
  int numPaths = pathGroup->GetTimeSize();

  if (index > numPaths-1) {
      api.error("The index argument '" + std::to_string(index) + "' is must be between 0 and " +
        std::to_string(numPaths-1));
      return nullptr;
  }

  // Create a PyPath object from the path and return it as a PyObject*.
  auto path = pathGroup->GetPathElement(index);
  return CreatePyPath(path);
}

//-----------------------------
// PathGroup_get_path_group_id 
//-----------------------------
//
PyDoc_STRVAR(PathGroup_get_path_group_id_doc,
  "get_path_group_id(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_path_group_id(PyPathGroup* self, PyObject* args)
{
  int id = self->pathGroup->GetPathID();
  return Py_BuildValue("i",id); 
}
    
//-----------------------------
// PathGroup_set_path_group_id
//-----------------------------
//
PyDoc_STRVAR(PathGroup_set_path_group_id_doc,
  "set_path_group_id(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_path_group_id(PyPathGroup* self, PyObject* args)
{
  auto api = PyUtilApiFunction("i", PyRunTimeErr, __func__);
  int id;

  if (!PyArg_ParseTuple(args, api.format, &id)) {
      return api.argsError();
  }
    
  self->pathGroup->SetPathID(id);
  return Py_None;
}

//-----------------------
// PathGroup_set_spacing 
//-----------------------
//
PyDoc_STRVAR(PathGroup_set_spacing_doc,
  "set_spacing(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_spacing(PyPathGroup* self, PyObject* args)
{
  auto api = PyUtilApiFunction("d", PyRunTimeErr, __func__);
  double spacing;

  if (!PyArg_ParseTuple(args, api.format, &spacing)) {
     return api.argsError();
  }
    
  self->pathGroup->SetSpacing(spacing);
  return Py_None;
}

//-----------------------
// PathGroup_get_spacing 
//-----------------------
//
PyDoc_STRVAR(PathGroup_get_spacing_doc,
  "get_spacing(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_spacing(PyPathGroup* self, PyObject* args)
{
  double spacing = self->pathGroup->GetSpacing();
  return Py_BuildValue("d", spacing); 
}

//----------------------
// PathGroup_set_method 
//----------------------
//
PyDoc_STRVAR(PathGroup_set_method_doc,
  "set_method(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_method(PyPathGroup* self, PyObject* args)
{
  auto api = PyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* methodName;

  if (!PyArg_ParseTuple(args, api.format, &methodName)) {
     return api.argsError();
  }

  PathElement::CalculationMethod method;

  try {
      method = calcMethodNameTypeMap.at(std::string(methodName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown method name '" + std::string(methodName) + "'." +
          " Valid names are: " + calcMethodValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  self->pathGroup->SetMethod(method);
  return Py_None;
}

//----------------------
// PathGroup_get_method 
//----------------------
//
PyDoc_STRVAR(PathGroup_get_method_doc,
  "get_method(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_method(PyPathGroup* self, PyObject* args)
{
  auto api = PyUtilApiFunction("", PyRunTimeErr, __func__);
  PathElement::CalculationMethod method = self->pathGroup->GetMethod();
  std::string methodName;

  for (auto const& element : calcMethodNameTypeMap) { 
      if (method == element.second) {
          methodName = element.first;
          break; 
      }
  }

  if (methodName == "") {
      api.error("No method is set.");
      return nullptr;
  }
    
  return Py_BuildValue("s", methodName.c_str());
}

//----------------------------------
// PathGroup_set_calculation_number 
//----------------------------------
//
PyDoc_STRVAR(PathGroup_set_calculation_number_doc,
  "set_calculation_number(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_set_calculation_number(PyPathGroup* self, PyObject* args)
{
  auto api = PyUtilApiFunction("i", PyRunTimeErr, __func__);
  int number;

  if (!PyArg_ParseTuple(args, api.format, &number)) {
     return api.argsError();
  }
    
  // [TODO:DaveP] need to check value of number.
  self->pathGroup->SetCalculationNumber(number);
  return Py_None;
}

//----------------------------------
// PathGroup_get_calculation_number 
//----------------------------------
//
PyDoc_STRVAR(PathGroup_get_calculation_number_doc,
  "get_calculation_number(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
PathGroup_get_calculation_number(PyPathGroup* self, PyObject* args)
{
  int number = self->pathGroup->GetCalculationNumber();
  return Py_BuildValue("i", number);
}

//-----------------
// PathGroup_write
//-----------------
//
PyDoc_STRVAR(PathGroup_write_doc,
  "write(file_name) \n\ 
   \n\
   Write the path group to an SV .pth file.\n\
   \n\
   Args: \n\
     file_name (str): The name of the file to write the path group to.\n\
");

static PyObject *
PathGroup_write(PyPathGroup* self, PyObject* args)
{
  auto api = PyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* fileName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      return api.argsError();
  }

  try {
      if (sv3::PathIO().Write(fileName, self->pathGroup) != SV_OK) {
          api.error("Error writing path group to the file '" + std::string(fileName) + "'.");
          return nullptr;
      }
  } catch (const std::exception& readException) {
      api.error("Error writing path group to the file '" + std::string(fileName) + "': " + readException.what());
      return nullptr;
  }

  return SV_PYTHON_OK;
}


////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* PATH_GROUP_CLASS = "Group";
// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* PATH_GROUP_MODULE_CLASS = "path.Group";

//---------------
// PathGroup_doc 
//---------------
// Define the Group class documentation.
//
PyDoc_STRVAR(PathGroup_doc,
   "The Group class \n\
   \n\
");

//--------------------
// PyPathGroupMethods 
//--------------------
// Define the methods for the path.Group class.
//
static PyMethodDef PyPathGroupMethods[] = {

  {"get_calculation_number", (PyCFunction)PathGroup_get_calculation_number, METH_NOARGS, PathGroup_get_calculation_number_doc},

  {"get_method", (PyCFunction)PathGroup_get_method, METH_NOARGS, PathGroup_get_method_doc}, 

  {"get_path", (PyCFunction)PathGroup_get_path, METH_VARARGS, PathGroup_get_path_doc},

  {"get_path_group_id", (PyCFunction)PathGroup_get_path_group_id,METH_VARARGS,PathGroup_get_path_group_id_doc},

  {"get_spacing", (PyCFunction)PathGroup_get_spacing, METH_NOARGS, PathGroup_get_spacing_doc},

  {"get_time_size", (PyCFunction)PathGroup_get_time_size, METH_NOARGS, PathGroup_get_time_size_doc},

  {"set_calculation_number", (PyCFunction)PathGroup_set_calculation_number, METH_NOARGS, PathGroup_set_calculation_number_doc},

  {"set_method", (PyCFunction)PathGroup_set_method, METH_VARARGS, PathGroup_set_method_doc},

  {"set_path", (PyCFunction)PathGroup_set_path, METH_VARARGS, PathGroup_set_path_doc},

  {"set_path_group_id", (PyCFunction)PathGroup_set_path_group_id, METH_VARARGS,PathGroup_set_path_group_id_doc},

  {"set_spacing", (PyCFunction)PathGroup_set_spacing, METH_VARARGS, PathGroup_set_spacing_doc},

  {"write", (PyCFunction)PathGroup_write, METH_VARARGS, PathGroup_write_doc},

  {NULL, NULL}
};

//-----------------
// PyPathGroupType 
//-----------------
// Define the Python type that stores PathGroup data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static PyTypeObject PyPathGroupType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  PATH_GROUP_MODULE_CLASS, 
  sizeof(PyPathGroup)
};

//------------------
// PyPathGroup_init
//------------------
// This is the __init__() method for the path.Group class. 
//
// This implements the Python __init__ method for the Group class. 
// It is called after calling the __new__ method.
//
// Args:
//   fileName (str): An SV .pth file. A new PathGroup object is created from 
//       the contents of the file. (optional)
//
static int 
PyPathGroupInit(PyPathGroup* self, PyObject* args)
{
  static int numObjs = 1;
  std::cout << "[PyPathGroupInit] New PathGroup object: " << numObjs << std::endl;
  auto api = PyUtilApiFunction("|s", PyRunTimeErr, __func__);
  char* fileName = nullptr;
  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      api.argsError();
      return 1;
  }
  if (fileName != nullptr) {
      std::cout << "[PyPathGroupInit] File name: " << fileName << std::endl;
      self->pathGroup = PathGroup_read(fileName);
  } else {
      self->pathGroup = new sv3::PathGroup();
  }
  numObjs += 1;
  return 0;
}

//----------------
// PyPathGroupNew 
//----------------
// This implements the Python __new__ method. It is called before the
// __init__ method.
//
static PyObject *
PyPathGroupNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyPathGroupNew] PyPathGroupNew " << std::endl;
  auto self = (PyPathClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      self->id = 1;
  }
  return (PyObject*)self;
}

//--------------------
// PyPathGroupDealloc 
//--------------------
//
static void
PyPathGroupDealloc(PyPathGroup* self)
{
  std::cout << "[PyPathGroupDealloc] **** Free PyPathGroup **** " << std::endl;
  delete self->pathGroup;
  Py_TYPE(self)->tp_free(self);
}

//------------------------
// SetPathGroupTypeFields 
//------------------------
// Set the Python type object fields that stores Path data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetPyPathGroupTypeFields(PyTypeObject& pathType)
{
  // Doc string for this type.
  pathType.tp_doc = PathGroup_doc;
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  pathType.tp_new = PyPathGroupNew;
  pathType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  pathType.tp_init = (initproc)PyPathGroupInit;
  pathType.tp_dealloc = (destructor)PyPathGroupDealloc;
  pathType.tp_methods = PyPathGroupMethods;
}

//-------------------
// CreatePyPathGroup
//-------------------
// Create a PyPathGroupType object.
//
// If the 'pathGroup' argument is not null then use that 
// for the PyPathGroupType.pathGroup data.
//
PyObject *
CreatePyPathGroup(PathGroup* pathGroup)
{
  std::cout << "[CreatePyPathGroup] Create PathGroup object ... " << std::endl;
  auto pathGroupObj = PyObject_CallObject((PyObject*)&PyPathGroupType, NULL);
  auto pyPathGroup = (PyPathGroup*)pathGroupObj;

  if (pathGroup != nullptr) {
      delete pyPathGroup->pathGroup;
      pyPathGroup->pathGroup = pathGroup;
  }
  std::cout << "[CreatePyPath] pyPathGroup id: " << pyPathGroup->id << std::endl;
  return pathGroupObj;
}

