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

// The functions defined here implement the SV Python API contour module group class. 
// It provides an interface to the SV contour group class.
//
// The class name is 'Group'. It is referenced from the contour module as 'contour.Group'.
//
//     aorta_cont_group = contour.Group()
//
#include "sv4gui_ContourGroup_PyClass.h"
#include "sv4gui_dmg_init_py.h"

static PyObject * CreatePyContourGroup(sv4guiContourGroup::Pointer contourGroup);

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//-------------------
// ContourGroup_read
//-------------------
// Read in an SV .pth file and create a ContourGroup object
// from its contents.
//
static sv4guiContourGroup::Pointer 
//static sv4guiContourGroup * 
ContourGroup_read(char* fileName)
{
  std::cout << "========== ContourGroup_read ==========" << std::endl;
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  std::cout << "[ContourGroup_read] fileName: " << fileName << std::endl;
  sv4guiContourGroup::Pointer group = Dmg_read_contour_group_file(std::string(fileName));

  if (group == nullptr) {
      api.error("Error reading the contour group file '" + std::string(fileName) + "'.");
      std::cout << "[ContourGroup_read] ERROR: can't read fileName: " << fileName << std::endl;
      return nullptr;
  } 

  std::cout << "[ContourGroup_read] File read and group returned." << std::endl;
  auto contourGroup = dynamic_cast<sv4guiContourGroup*>(group.GetPointer());
  int numContours = contourGroup->GetSize();
  std::cout << "[ContourGroup_read] Number of contours: " << numContours << std::endl;

  return group;
}

//////////////////////////////////////////////////////
//       G r o u p  C l a s s  M e t h o d s        //
//////////////////////////////////////////////////////
//
// SV Python Contour Group methods. 

//-------------------------
// ContourGroup_get_time_size 
//-------------------------
//
// [TODO:DaveP] bad method name: get_number_time_steps() ?
//
PyDoc_STRVAR(ContourGroup_get_time_size_doc,
  "set_contour(name) \n\ 
   \n\
   Store the polydata for the named contour into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
ContourGroup_get_time_size(PyContourGroup* self, PyObject* args)
{
/*
  int timestepSize = self->contourGroup->GetTimeSize();
  return Py_BuildValue("i", timestepSize); 
*/
}

//-----------------------
// ContourGroup_get_size 
//-----------------------
//
PyDoc_STRVAR(ContourGroup_number_of_contours_doc,
  "get_size() \n\ 
   \n\
   Get the number of contours in the group. \n\
   \n\
   Args: \n\
     None \n\
   Returns (int): The number of contours in the group.\n\
");

static PyObject * 
ContourGroup_number_of_contours(PyContourGroup* self, PyObject* args)
{
  auto contourGroup = self->contourGroup;
  int numContours = contourGroup->GetSize();
  std::cout << "[ContourGroup_number_of_contours] Number of contours: " << numContours << std::endl;
  return Py_BuildValue("i", numContours); 
}

//--------------------------
// ContourGroup_get_contour 
//--------------------------
PyDoc_STRVAR(ContourGroup_get_contour_doc,
  "get_contour(name) \n\ 
   \n\
   Store the polydata for the named contour into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
ContourGroup_get_contour(PyContourGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int index;
  char* contourName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &index)) {
     return api.argsError();
  }

  auto contourGroup = self->contourGroup;
  int numContours = contourGroup->GetSize();
  std::cout << "[ContourGroup_get_contour] Number of contours: " << numContours << std::endl;

  // Check for valid index.
  if ((index < 0) || (index > numContours-1)) {
      api.error("The index argument '" + std::to_string(index) + "' is must be between 0 and " +
        std::to_string(numContours-1));
      return nullptr;
  }

  // Get the contour for the given index.
  //
  auto contour = contourGroup->GetContour(index);

  if (contour == nullptr) {
      api.error("ERROR getting the contour for the index argument '" + std::to_string(index) + "'.");
      return nullptr;
  }
  auto kernel = contour->GetKernel();
  auto ctype = contour->GetType();
  auto contourType = ContourKernel_get_name(kernel);
  std::cout << "[ContourGroup_get_contour] ctype: " << ctype << std::endl;
  std::cout << "[ContourGroup_get_contour] kernel: " << kernel << std::endl;
  std::cout << "[ContourGroup_get_contour] Contour type: " << contourType << std::endl;

  // Create a PyContour object from the SV Contour object 
  // and return it as a PyObject*.
  return PyCreateContour(contour);
}

//-----------------
// ContourGroup_write
//-----------------
//
PyDoc_STRVAR(ContourGroup_write_doc,
  "write(file_name) \n\ 
   \n\
   Write the contour group to an SV .pth file.\n\
   \n\
   Args: \n\
     file_name (str): The name of the file to write the contour group to.\n\
");

static PyObject *
ContourGroup_write(PyContourGroup* self, PyObject* args)
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

static char* CONTOUR_GROUP_CLASS = "Group";
// Dotted name that includes both the module name and the name of the 
// type within the module.
static char* CONTOUR_GROUP_MODULE_CLASS = "contour.Group";

PyDoc_STRVAR(contourgroup_doc, "contour.Group functions");

//--------------------
// PyContourGroupMethods 
//--------------------
// Define the methods for the contour.Group class.
//
static PyMethodDef PyContourGroupMethods[] = {

  {"get_contour", (PyCFunction)ContourGroup_get_contour, METH_VARARGS, ContourGroup_get_contour_doc},

  {"number_of_contours", (PyCFunction)ContourGroup_number_of_contours, METH_VARARGS, ContourGroup_number_of_contours_doc},

  {"get_time_size", (PyCFunction)ContourGroup_get_time_size, METH_NOARGS, ContourGroup_get_time_size_doc},

  {"write", (PyCFunction)ContourGroup_write, METH_VARARGS, ContourGroup_write_doc},

  {NULL, NULL}
};

//-------------------------
// PyContourGroupClassType 
//-------------------------
// Define the Python type that stores ContourGroup data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static PyTypeObject PyContourGroupClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  CONTOUR_GROUP_MODULE_CLASS,     
  sizeof(PyContourGroup)
};

//------------------
// PyContourGroup_init
//------------------
// This is the __init__() method for the contour.Group class. 
//
// This function is used to initialize an object after it is created.
//
// Arguments:
//
//   fileName - An SV .ctgr pth file. A new ContourGroup object is created from 
//     the contents of the file. (optional)
//
static int 
PyContourGroupInit(PyContourGroup* self, PyObject* args)
{
  static int numObjs = 1;
  std::cout << "[PyContourGroupInit] New ContourGroup object: " << numObjs << std::endl;
  auto api = SvPyUtilApiFunction("|s", PyRunTimeErr, __func__);
  char* fileName = nullptr;
  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      api.argsError();
      return 1;
  }
  if (fileName != nullptr) {
      std::cout << "[PyContourGroupInit] File name: " << fileName << std::endl;
      self->contourGroupPointer = ContourGroup_read(fileName);
      self->contourGroup = dynamic_cast<sv4guiContourGroup*>(self->contourGroupPointer.GetPointer());
  } else {
      self->contourGroup = Dmg_create_contour_group();
  }
  if (self->contourGroup == nullptr) { 
      std::cout << "[PyContourGroupInit] ERROR reading File name: " << fileName << std::endl;
      return -1;
  }
  numObjs += 1;
  return 0;
}

//-------------------
// PyContourGroupNew 
//-------------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyContourGroupNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyContourGroupNew] PyContourGroupNew " << std::endl;
  auto self = (PyContour*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyContourGroupNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }
  return (PyObject *) self;
}

//-----------------------
// PyContourGroupDealloc 
//-----------------------
//
static void
PyContourGroupDealloc(PyContourGroup* self)
{
  std::cout << "[PyContourGroupDealloc] Free PyContourGroup" << std::endl;
  // Can't delete contourGroup because it has a protected detructor.
  //delete self->contourGroup;
  Py_TYPE(self)->tp_free(self);
}

//---------------------------
// SetContourGroupTypeFields 
//---------------------------
// Set the Python type object fields that stores Contour data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetContourGroupTypeFields(PyTypeObject& contourType)
{
  // Doc string for this type.
  contourType.tp_doc = "ContourGroup  objects";
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  contourType.tp_new = PyContourGroupNew;
  contourType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  contourType.tp_init = (initproc)PyContourGroupInit;
  contourType.tp_dealloc = (destructor)PyContourGroupDealloc;
  contourType.tp_methods = PyContourGroupMethods;
}

//----------------------
// CreatePyContourGroup
//----------------------
// Create a PyContourGroupType object.
//
// If the 'contourGroup' argument is not null then use that 
// for the PyContourGroupType.contourGroup data.
//
PyObject *
CreatePyContourGroup(sv4guiContourGroup::Pointer contourGroup)
{
  std::cout << "[CreatePyContourGroup] Create ContourGroup object ... " << std::endl;
  auto contourGroupObj = PyObject_CallObject((PyObject*)&PyContourGroupClassType, NULL);
  auto pyContourGroup = (PyContourGroup*)contourGroupObj;

  if (contourGroup != nullptr) {
      //delete pyContourGroup->contourGroup;
      pyContourGroup->contourGroup = contourGroup;
  }
  std::cout << "[CreatePyContour] pyContourGroup id: " << pyContourGroup->id << std::endl;
  return contourGroupObj;
}

