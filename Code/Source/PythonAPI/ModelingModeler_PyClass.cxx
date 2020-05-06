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

// Define the Python 'modeling.Modeler' class. This class defines modeling operations 
// that create new Python 'modeling.Model' objects. 
//
#ifndef PYAPI_MODELING_MODELER_H
#define PYAPI_MODELING_MODELER_H

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

//---------------------
// PyModelingModelerClass 
//---------------------
// Define the ModelingModelerClass.
//
// This is the data stored in a Python solid.Modeler object.
//
typedef struct {
  PyObject_HEAD
  int id;
  SolidModel_KernelT kernel;
} PyModelingModelerClass;

//////////////////////////////////////////////////////
//          U t i l i t y   F u n c t i o n s       //
//////////////////////////////////////////////////////

//---------------------------
// ModelingModelerUtil_GetModel 
//---------------------------
//
static cvSolidModel *
ModelingModelerUtil_GetModelFromPyObj(PyObject* obj)
{
  // Check that the Python object is an SV Python Model object.
  if (!PyObject_TypeCheck(obj, &PyModelingModelClassType)) {
      return nullptr;
  }
  return ((PyModelingModelClass*)obj)->solidModel;
}

////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//
// Python 'Modeler' class methods. 
//

//-----------------------
// ModelingModeler_box3d 
//-----------------------
//
PyDoc_STRVAR(ModelingModeler_box_doc,
  "box(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
ModelingModeler_box(PyModelingModelClass* self, PyObject* args, PyObject* kwargs)
{
  std::cout << "[ModelingModeler_box] ========== ModelingModeler_box ==========" << std::endl;
  std::cout << "[ModelingModel_box] Kernel: " << self->kernel << std::endl;
  auto api = PyUtilApiFunction("O|ddd", PyRunTimeErr, __func__);
  static char *keywords[] = {"center", "width", "height", "length", NULL};
  double width = 1.0;
  double height = 1.0;
  double length = 1.0;
  PyObject* centerArg;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &centerArg, &width, &height, &length)) { 
      return api.argsError();
  }

  // Check that the center argument is valid.
  std::string emsg;
  if (!PyUtilCheckPointData(centerArg, emsg)) {
      api.error("The box center argument " + emsg);
      return nullptr;
  }

  // Get the center argument data.
  double center[3];
  for (int i = 0; i < PyList_Size(centerArg); i++) {
    center[i] = PyFloat_AsDouble(PyList_GetItem(centerArg,i));
  }
  std::cout << "[ModelingModel_box] Center: " << center[0] << "  " << center[1] << "  " << center[2] << std::endl;

  if (width <= 0.0) { 
      api.error("The box width argument is <= 0.0.");
      return nullptr;
  }

  if (height <= 0.0) { 
      api.error("The box height argument is <= 0.0.");
      return nullptr;
  }

  if (length <= 0.0) { 
      api.error("The box length argument is <= 0.0.");
      return nullptr;
  }

  std::cout << "[ModelingModel_box] Width: " << width << std::endl;
  std::cout << "[ModelingModel_box] Height: " << height << std::endl;
  std::cout << "[ModelingModel_box] Length: " << length << std::endl;

  // Create the new solid object.
  auto pyModelingModelObj = CreatePyModelingModelObject(self->kernel);
  auto model = ((PyModelingModelClass*)pyModelingModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a 3D box solid model.");
      return nullptr;
  }

  // Create the box solid.
  //
  double dims[3] = {width, height, length};
  if (model->MakeBox3d(dims, center) != SV_OK) {
      Py_DECREF(pyModelingModelObj);
      delete model;
      api.error("Error creating a 3D box solid model.");
      return nullptr;
  }

  return pyModelingModelObj;
}

//------------------------
// ModelingModeler_circle 
//------------------------
//
PyDoc_STRVAR(ModelingModeler_circle_doc,
  "circle(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
ModelingModeler_circle(PyModelingModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("sddd", PyRunTimeErr, __func__);
  static char *keywords[] = {"radius", "x", "y", NULL};
  double radius;
  double center[2];

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &radius, &(center[0]), &(center[1]))) { 
      return api.argsError();
  }

  if (radius <= 0.0) {
      api.error("The radius argument <= 0.0."); 
      return nullptr;
  }

  // Create the new solid object.
  auto pyModelingModelObj = CreatePyModelingModelObject(self->kernel);
  auto model = ((PyModelingModelClass*)pyModelingModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a 3D box solid model.");
      return nullptr;
  }

  if (model->MakeCircle(radius, center) != SV_OK) {
      delete model;
      api.error("Error creating a circle solid model.");
      return nullptr;
  }

  return pyModelingModelObj;
}

//--------------------------
// ModelingModeler_cylinder 
//--------------------------
//
PyDoc_STRVAR(ModelingModeler_cylinder_doc,
  "cylinder(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
ModelingModeler_cylinder(PyModelingModelerClass* self, PyObject* args, PyObject* kwargs)
{
  std::cout << "[ModelingModeler_cylinder] ========== ModelingModeler_cylinder ==========" << std::endl;
  std::cout << "[ModelingModel_cylinder] Kernel: " << self->kernel << std::endl;
  auto api = PyUtilApiFunction("ddOO", PyRunTimeErr, __func__);
  static char *keywords[] = {"radius", "length", "center", "axis", NULL};
  double radius;
  double length;
  PyObject* centerArg;
  PyObject* axisArg;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &radius, &length, &centerArg , &axisArg)) {
      return api.argsError();
  }

  // Check argument values.
  //
  std::string emsg;
  if (!PyUtilCheckPointData(centerArg, emsg)) {
      api.error("The cylinder center argument " + emsg);
      return nullptr;
  }

  if (!PyUtilCheckPointData(axisArg, emsg)) {
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

  // Get argument data.
  //
  double center[3];
  double axis[3];
  for (int i = 0; i < 3; i++) {
      axis[i] = PyFloat_AsDouble(PyList_GetItem(axisArg,i));
      center[i] = PyFloat_AsDouble(PyList_GetItem(centerArg,i));
  }

  // Create the new solid.
  auto pyModelingModelObj = CreatePyModelingModelObject(self->kernel);
  if (pyModelingModelObj == nullptr) { 
      api.error("Error creating a Python solid model object.");
      return nullptr;
  } 
  auto model = ((PyModelingModelClass*)pyModelingModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

  std::cout << "[ModelingModel_cylinder] Create cylinder ... " << std::endl;
  if (model->MakeCylinder(radius, length, center, axis) != SV_OK) {
      Py_DECREF(pyModelingModelObj);
      delete model;
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

  return pyModelingModelObj;
}

//---------------------------
// ModelingModeler_ellipsoid 
//---------------------------
// [TODO:DaveP] The cvModelingModel MakeEllipsoid method is not implemented.
//
PyDoc_STRVAR(ModelingModeler_ellipsoid_doc,
  "ellipsoid()  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
ModelingModeler_ellipsoid(PyModelingModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("OO", PyRunTimeErr, __func__);
  static char *keywords[] = {"center", "radii", NULL};
  PyObject* rList;
  PyObject* centerList;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &rList, &centerList)) { 
      return api.argsError();
  }

  std::string emsg;
  if (!PyUtilCheckPointData(centerList, emsg)) {
      api.error("The ellipsoid center argument " + emsg);
      return nullptr;
  }

  if (!PyUtilCheckPointData(rList, emsg)) {
      api.error("The ellipsoid radius vector argument " + emsg);
      return nullptr;
  }

  double center[3];
  double r[3];

  for (int i=0;i<3;i++) {
    r[i] = PyFloat_AsDouble(PyList_GetItem(rList,i));
    center[i] = PyFloat_AsDouble(PyList_GetItem(centerList,i));
  }

  // Create the new solid.
  auto pyModelingModelObj = CreatePyModelingModelObject(self->kernel);
  if (pyModelingModelObj == nullptr) {
      api.error("Error creating a Python solid model object.");
      return nullptr;
  }
  auto model = ((PyModelingModelClass*)pyModelingModelObj)->solidModel;
  if (model == NULL) {
      api.error("Error creating a solid model object.");
      return nullptr;
  }

  std::cout << "[ModelingModeler_ellipsoid] Create ellipsoid ... " << std::endl;
  if (model->MakeEllipsoid(r, center) != SV_OK) {
      delete model;
      api.error("Error creating an ellipsoid solid model.");
      return nullptr;
  }

  return pyModelingModelObj;
}

//---------------------------
// ModelingModeler_intersect 
//---------------------------
//
PyDoc_STRVAR(ModelingModeler_intersect_doc,
  "intersect(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
ModelingModeler_intersect(PyModelingModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("OO|s", PyRunTimeErr, __func__);
  static char *keywords[] = {"model1", "model2", "simplification", NULL};
  PyObject* model1Arg;
  PyObject* model2Arg;
  char *smpName=NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &model1Arg, &model2Arg, &smpName)) { 
      return api.argsError();
  }

  // Parse the simplification flag if given.
  auto smp = CheckSimplificationName(api, smpName);
  if (smp == SM_Simplify_Invalid) {
      return nullptr;
  }

  // Check that the model1 argument is a SV Python Model object.
  auto model1 = ModelingModelerUtil_GetModelFromPyObj(model1Arg);
  if (model1 == nullptr) {
      api.error("The first model argument is not a Model object.");
      return nullptr;
  }

  // Check that the model2 argument is a SV Python Model object.
  auto model2 = ModelingModelerUtil_GetModelFromPyObj(model2Arg);
  if (model2 == nullptr) {
      api.error("The second model argument is not a Model object.");
      return nullptr;
  }

  // Create the new solid.
  auto pyModelingModelObj = CreatePyModelingModelObject(self->kernel);
  if (pyModelingModelObj == nullptr) {
      api.error("Error creating a Python solid model object.");
      return nullptr;
  }
  auto model = ((PyModelingModelClass*)pyModelingModelObj)->solidModel;
  if (model == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (model->Intersect(model1, model2, smp) != SV_OK ) {
      delete model;
      api.error("Error performing a Boolean intersection.");
      return nullptr;
  }

  return pyModelingModelObj;
}

//----------------------
// ModelingModeler_read 
//----------------------
//
PyDoc_STRVAR(ModelingModeler_read_doc,
  "read(file_name)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject *
ModelingModeler_read(PyModelingModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("s", PyRunTimeErr, __func__);
  static char *keywords[] = {"file_name", NULL};
  char *fileName;
  std::cout << "[ModelingModeler_box] ========== ModelingModeler_read ==========" << std::endl;
  std::cout << "[ModelingModel_box] Kernel: " << self->kernel << std::endl;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &fileName)) { 
      return api.argsError();
  }

  // Create the new solid.
  auto pyModelingModelObj = CreatePyModelingModelObject(self->kernel);
  if (pyModelingModelObj == nullptr) {
      api.error("Error creating a Python solid model object.");
      return nullptr;
  }
  auto model = ((PyModelingModelClass*)pyModelingModelObj)->solidModel;
  if (model == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (model->ReadNative(fileName) != SV_OK) {
      delete model;
      api.error("Error reading a solid model from the file '" + std::string(fileName) + "'.");
      return nullptr;
  }

  return pyModelingModelObj;
}

//------------------------
// ModelingModeler_sphere 
//------------------------
//
PyDoc_STRVAR(ModelingModeler_sphere_doc,
  "sphere(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
ModelingModeler_sphere(PyModelingModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("dO", PyRunTimeErr, __func__);
  static char *keywords[] = {"radius", "center", NULL};
  PyObject* centerArg;
  double center[3];
  double radius;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &radius, &centerArg)) {
      return api.argsError();
  }

  std::string emsg;
  if (!PyUtilCheckPointData(centerArg, emsg)) {
      api.error("The sphere center argument " + emsg);
      return nullptr;
  }

  if (radius <= 0.0) {
      api.error("The radius argument is <= 0.0.");
      return nullptr;
  }

  for (int i = 0; i < PyList_Size(centerArg); i++) {
      center[i] = PyFloat_AsDouble(PyList_GetItem(centerArg, i));
  }

  // Create the new solid.
  auto pyModelingModelObj = CreatePyModelingModelObject(self->kernel);
  if (pyModelingModelObj == nullptr) { 
      api.error("Error creating a Python solid model object.");
      return nullptr;
  } 
  auto model = ((PyModelingModelClass*)pyModelingModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

  if (model->MakeSphere(radius, center) != SV_OK) {
      delete model;
      api.error("Error creating a sphere solid model.");
      return nullptr;
  }

  return pyModelingModelObj;
}

//--------------------------
// ModelingModeler_subtract 
//--------------------------
//
PyDoc_STRVAR(ModelingModeler_subtract_doc,
  "subtract(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
ModelingModeler_subtract(PyModelingModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("OO|s", PyRunTimeErr, __func__);
  static char *keywords[] = {"main", "subtract", "simplification", NULL};
  PyObject* mainModelArg;
  PyObject* subtractModelArg;
  char *smpName=NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &mainModelArg, &subtractModelArg, &smpName)) { 
      return api.argsError();
  }

  // Parse the simplification flag if given.
  auto smp = CheckSimplificationName(api, smpName);
  if (smp == SM_Simplify_Invalid) {
      return nullptr;
  }

  // Check that the mainModel argument is a SV Python Model object.
  auto mainModel = ModelingModelerUtil_GetModelFromPyObj(mainModelArg);
  if (mainModel == nullptr) { 
      api.error("The main model argument is not a Model object.");
      return nullptr;
  }

  // Check that the subtractModel argument is a SV Python Model object.
  auto subtractModel = ModelingModelerUtil_GetModelFromPyObj(subtractModelArg);
  if (subtractModel == nullptr) { 
      api.error("The subtract model argument is not a Model object.");
      return nullptr;
  }

  // Create the new solid.
  auto pyModelingModelObj = CreatePyModelingModelObject(self->kernel);
  if (pyModelingModelObj == nullptr) {
      api.error("Error creating a Python solid model object.");
      return nullptr;
  }
  auto model = ((PyModelingModelClass*)pyModelingModelObj)->solidModel;
  if (model == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (model->Subtract(mainModel, subtractModel, smp) != SV_OK) {
      delete model;
      api.error("Error performing the Boolean subtract.");
      return nullptr;
  }

  return pyModelingModelObj;
}

//-----------------------
// ModelingModeler_union 
//-----------------------
//
PyDoc_STRVAR(ModelingModeler_union_doc,
  "union(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
ModelingModeler_union(PyModelingModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("OO|s", PyRunTimeErr, __func__);
  static char *keywords[] = {"model1", "model2", "simplification", NULL};
  PyObject* model1Arg;
  PyObject* model2Arg;
  char *smpName = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &model1Arg, &model2Arg, &smpName)) { 
      return api.argsError();
  }

  // Parse the simplification flag if given.
  auto smp = CheckSimplificationName(api, smpName);
  if (smp == SM_Simplify_Invalid) {
      return nullptr;
  }

  // Check that the model1 argument is a SV Python Model object.
  auto model1 = ModelingModelerUtil_GetModelFromPyObj(model1Arg);
  if (model1 == nullptr) { 
      api.error("The first model argument is not a Model object.");
      return nullptr;
  }

  // Check that the model2 argument is a SV Python Model object.
  auto model2 = ModelingModelerUtil_GetModelFromPyObj(model2Arg);
  if (model2 == nullptr) { 
      api.error("The second model argument is not a Model object.");
      return nullptr;
  }

  // Create the new solid.
  auto pyModelingModelObj = CreatePyModelingModelObject(self->kernel);
  if (pyModelingModelObj == nullptr) { 
      api.error("Error creating a Python solid model object.");
      return nullptr;
  } 
  auto model = ((PyModelingModelClass*)pyModelingModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (model->Union(model1, model2, smp) != SV_OK) {
      delete model;
      api.error("Error performing the Boolean union.");
      return nullptr;
  }

  return pyModelingModelObj;
}

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODELING_MODELER_CLASS = "Modeler";
static char* MODELING_MODELER_MODULE_CLASS = "modeling.Modeler";
// The name of the Modeler class veriable that contains all of the kernel types.
static char* MODELING_MODELER_CLASS_VARIBLE_NAMES = "names";

PyDoc_STRVAR(ModelingModelerClass_doc, "Modeling modeler class functions");

//---------------------
// ModelingModelerMethods
//---------------------
//
static PyMethodDef PyModelingModelerClassMethods[] = {

  { "box", (PyCFunction)ModelingModeler_box, METH_VARARGS | METH_KEYWORDS, ModelingModeler_box_doc },

  // [TODO:DaveP] The cvModelingModel MakeCircle method is not implemented.
  //{ "circle", (PyCFunction)ModelingModeler_circle, METH_VARARGS, ModelingModeler_circle_doc },

  { "cylinder", (PyCFunction)ModelingModeler_cylinder, METH_VARARGS | METH_KEYWORDS, ModelingModeler_cylinder_doc },

  { "intersect", (PyCFunction)ModelingModeler_intersect, METH_VARARGS | METH_KEYWORDS, ModelingModeler_intersect_doc },

  // [TODO:DaveP] The cvModelingModel MakeEllipsoid method is not implemented.
  //{ "ellipsoid", (PyCFunction)ModelingModeler_ellipsoid, METH_VARARGS | METH_KEYWORDS, ModelingModeler_ellipsoid_doc},

  { "read", (PyCFunction)ModelingModeler_read, METH_VARARGS|METH_KEYWORDS, ModelingModeler_read_doc },

  { "sphere", (PyCFunction)ModelingModeler_sphere, METH_VARARGS | METH_KEYWORDS, ModelingModeler_sphere_doc },

  { "subtract", (PyCFunction)ModelingModeler_subtract, METH_VARARGS | METH_KEYWORDS, ModelingModeler_subtract_doc },

  { "union", (PyCFunction)ModelingModeler_union, METH_VARARGS | METH_KEYWORDS, ModelingModeler_union_doc},

  {NULL, NULL}
};

//-----------------------
// PyModelingModelerInit 
//-----------------------
// This is the __init__() method for the solid.Modeler class. 
//
// This function is used to initialize an object after it is created.
//
static int
PyModelingModelerInit(PyModelingModelerClass* self, PyObject* args, PyObject *kwds)
{
  auto api = PyUtilApiFunction("", PyRunTimeErr, "ModelingModeler");
  static int numObjs = 1;
  std::cout << "[PyModelingModelerInit] New PyModelingModeler object: " << numObjs << std::endl;
  char* kernelName = nullptr;
  if (!PyArg_ParseTuple(args, "s", &kernelName)) {
      return -1;
  }
  std::cout << "[PyModelingModelerInit] Kernel name: " << kernelName << std::endl;
  auto kernel = kernelNameEnumMap.at(std::string(kernelName));

  self->id = numObjs;
  self->kernel = kernel;
  numObjs += 1;
  return 0;
}

//----------------------------
// PyModelingModelerClassType 
//----------------------------
// Define the Python type object that stores contour.kernel types. 
//
static PyTypeObject PyModelingModelerClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = MODELING_MODELER_MODULE_CLASS,
  .tp_basicsize = sizeof(PyModelingModelerClass)
};

//----------------------
// PyModelingModelerNew 
//----------------------
//
static PyObject *
PyModelingModelerNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyModelingModelerNew] New ModelingModeler" << std::endl;
  auto api = PyUtilApiFunction("s", PyRunTimeErr, "Modeler");
  char* kernelName = nullptr; 
  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  std::cout << "[PyModelingModelerNew] Kernel: " << kernelName << std::endl;
  SolidModel_KernelT kernel;

  try {
      kernel = kernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." +
          " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  try {
    auto ctore = CvSolidModelCtorMap.at(kernel);
  } catch (const std::out_of_range& except) {
      api.error("No modeler is defined for the kernel name '" + std::string(kernelName) + "'."); 
      return nullptr;
  }

  auto self = (PyModelingModelerClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->id = 1;
  }

  return (PyObject *) self;
}

//--------------------------
// PyModelingModelerDealloc 
//--------------------------
//
static void
PyModelingModelerDealloc(PyModelingModelerClass* self)
{
  std::cout << "[PyModelingModelerDealloc] Free PyModelingModeler: " << self->id << std::endl;
  //delete self->solidModel;
  Py_TYPE(self)->tp_free(self);
}

//------------------------------
// SetModelingModelerTypeFields 
//------------------------------
// Set the Python type object fields that stores ModelingModeler data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetModelingModelerTypeFields(PyTypeObject& solidModelType)
{
  // Doc string for this type.
  solidModelType.tp_doc = ModelingModelerClass_doc; 
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  solidModelType.tp_new = PyModelingModelerNew;
  //solidModelType.tp_new = PyType_GenericNew,
  solidModelType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  solidModelType.tp_init = (initproc)PyModelingModelerInit;
  solidModelType.tp_dealloc = (destructor)PyModelingModelerDealloc;
  solidModelType.tp_methods = PyModelingModelerClassMethods;
};

//---------------------------
// CreateModelingModelerType 
//---------------------------
static PyModelingModelerClass * 
CreateModelingModelerType()
{
  return PyObject_New(PyModelingModelerClass, &PyModelingModelerClassType);
}

#endif


