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

// Define the Python 'solid.Modeler' class. This class defines modeling operations 
// that create new Python 'solid.Model' objects. 
//
#ifndef PYAPI_SOLID_MODELER_H
#define PYAPI_SOLID_MODELER_H

#include <iostream>
#include <map>
#include <math.h>
#include <string>
#include <structmember.h>

//---------------------
// PySolidModelerClass 
//---------------------
// Define the SolidModelerClass.
//
// This is the data stored in a Python solid.Modeler object.
//
typedef struct {
  PyObject_HEAD
  int id;
  SolidModel_KernelT kernel;
} PySolidModelerClass;

//////////////////////////////////////////////////////
//          U t i l i t y   F u n c t i o n s       //
//////////////////////////////////////////////////////

//---------------------------
// SolidModelerUtil_GetModel 
//---------------------------
//
static cvSolidModel *
SolidModelerUtil_GetModelFromPyObj(PyObject* obj)
{
  // Check that the Python object is an SV Python Model object.
  if (!PyObject_TypeCheck(obj, &PySolidModelClassType)) {
      return nullptr;
  }
  return ((PySolidModelClass*)obj)->solidModel;
}

////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//
// Python 'Modeler' class methods. 
//

//--------------------
// SolidModeler_box3d 
//--------------------
//
PyDoc_STRVAR(SolidModeler_box_doc,
  "box(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModeler_box(PySolidModelClass* self, PyObject* args, PyObject* kwargs)
{
  std::cout << "[SolidModeler_box] ========== SolidModeler_box ==========" << std::endl;
  std::cout << "[SolidModel_box] Kernel: " << self->kernel << std::endl;
  auto api = SvPyUtilApiFunction("O|ddd", PyRunTimeErr, __func__);
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
  if (!svPyUtilCheckPointData(centerArg, emsg)) {
      api.error("The box center argument " + emsg);
      return nullptr;
  }

  // Get the center argument data.
  double center[3];
  for (int i = 0; i < PyList_Size(centerArg); i++) {
    center[i] = PyFloat_AsDouble(PyList_GetItem(centerArg,i));
  }
  std::cout << "[SolidModel_box] Center: " << center[0] << "  " << center[1] << "  " << center[2] << std::endl;

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

  std::cout << "[SolidModel_box] Width: " << width << std::endl;
  std::cout << "[SolidModel_box] Height: " << height << std::endl;
  std::cout << "[SolidModel_box] Length: " << length << std::endl;

  // Create the new solid object.
  auto pySolidModelObj = CreatePySolidModelObject(self->kernel);
  auto model = ((PySolidModelClass*)pySolidModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a 3D box solid model.");
      return nullptr;
  }

  // Create the box solid.
  //
  double dims[3] = {width, height, length};
  if (model->MakeBox3d(dims, center) != SV_OK) {
      Py_DECREF(pySolidModelObj);
      delete model;
      api.error("Error creating a 3D box solid model.");
      return nullptr;
  }

  return pySolidModelObj;
}

//---------------------
// SolidModeler_circle 
//---------------------
//
PyDoc_STRVAR(SolidModeler_circle_doc,
  "circle(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModeler_circle(PySolidModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("sddd", PyRunTimeErr, __func__);
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
  auto pySolidModelObj = CreatePySolidModelObject(self->kernel);
  auto model = ((PySolidModelClass*)pySolidModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a 3D box solid model.");
      return nullptr;
  }

  if (model->MakeCircle(radius, center) != SV_OK) {
      delete model;
      api.error("Error creating a circle solid model.");
      return nullptr;
  }

  return pySolidModelObj;
}

//-----------------------
// SolidModeler_cylinder 
//-----------------------
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
SolidModeler_cylinder(PySolidModelerClass* self, PyObject* args, PyObject* kwargs)
{
  std::cout << "[SolidModeler_cylinder] ========== SolidModeler_cylinder ==========" << std::endl;
  std::cout << "[SolidModel_cylinder] Kernel: " << self->kernel << std::endl;
  auto api = SvPyUtilApiFunction("ddOO", PyRunTimeErr, __func__);
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

  // Get argument data.
  //
  double center[3];
  double axis[3];
  for (int i = 0; i < 3; i++) {
      axis[i] = PyFloat_AsDouble(PyList_GetItem(axisArg,i));
      center[i] = PyFloat_AsDouble(PyList_GetItem(centerArg,i));
  }

  // Create the new solid.
  auto pySolidModelObj = CreatePySolidModelObject(self->kernel);
  if (pySolidModelObj == nullptr) { 
      api.error("Error creating a Python solid model object.");
      return nullptr;
  } 
  auto model = ((PySolidModelClass*)pySolidModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

  std::cout << "[SolidModel_cylinder] Create cylinder ... " << std::endl;
  if (model->MakeCylinder(radius, length, center, axis) != SV_OK) {
      Py_DECREF(pySolidModelObj);
      delete model;
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

  return pySolidModelObj;
}

//------------------------
// SolidModeler_ellipsoid 
//------------------------
// [TODO:DaveP] The cvSolidModel MakeEllipsoid method is not implemented.
//
PyDoc_STRVAR(SolidModeler_ellipsoid_doc,
  "ellipsoid()  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModeler_ellipsoid(PySolidModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("OO", PyRunTimeErr, __func__);
  static char *keywords[] = {"center", "radii", NULL};
  PyObject* rList;
  PyObject* centerList;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &rList, &centerList)) { 
      return api.argsError();
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(centerList, emsg)) {
      api.error("The ellipsoid center argument " + emsg);
      return nullptr;
  }

  if (!svPyUtilCheckPointData(rList, emsg)) {
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
  auto pySolidModelObj = CreatePySolidModelObject(self->kernel);
  if (pySolidModelObj == nullptr) {
      api.error("Error creating a Python solid model object.");
      return nullptr;
  }
  auto model = ((PySolidModelClass*)pySolidModelObj)->solidModel;
  if (model == NULL) {
      api.error("Error creating a solid model object.");
      return nullptr;
  }

  std::cout << "[SolidModeler_ellipsoid] Create ellipsoid ... " << std::endl;
  if (model->MakeEllipsoid(r, center) != SV_OK) {
      delete model;
      api.error("Error creating an ellipsoid solid model.");
      return nullptr;
  }

  return pySolidModelObj;
}

//------------------------
// SolidModeler_intersect 
//------------------------
//
PyDoc_STRVAR(SolidModeler_intersect_doc,
  "intersect(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModeler_intersect(PySolidModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("OO|s", PyRunTimeErr, __func__);
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
  auto model1 = SolidModelerUtil_GetModelFromPyObj(model1Arg);
  if (model1 == nullptr) {
      api.error("The first model argument is not a Model object.");
      return nullptr;
  }

  // Check that the model2 argument is a SV Python Model object.
  auto model2 = SolidModelerUtil_GetModelFromPyObj(model2Arg);
  if (model2 == nullptr) {
      api.error("The second model argument is not a Model object.");
      return nullptr;
  }

  // Create the new solid.
  auto pySolidModelObj = CreatePySolidModelObject(self->kernel);
  if (pySolidModelObj == nullptr) {
      api.error("Error creating a Python solid model object.");
      return nullptr;
  }
  auto model = ((PySolidModelClass*)pySolidModelObj)->solidModel;
  if (model == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (model->Intersect(model1, model2, smp) != SV_OK ) {
      delete model;
      api.error("Error performing a Boolean intersection.");
      return nullptr;
  }

  return pySolidModelObj;
}


//-------------------
// SolidModeler_read 
//-------------------
//
PyDoc_STRVAR(SolidModeler_read_doc,
  "read(file_name)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject *
SolidModeler_read(PySolidModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  static char *keywords[] = {"file_name", NULL};
  char *fileName;
  std::cout << "[SolidModeler_box] ========== SolidModeler_read ==========" << std::endl;
  std::cout << "[SolidModel_box] Kernel: " << self->kernel << std::endl;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &fileName)) { 
      return api.argsError();
  }

  // Create the new solid.
  auto pySolidModelObj = CreatePySolidModelObject(self->kernel);
  if (pySolidModelObj == nullptr) {
      api.error("Error creating a Python solid model object.");
      return nullptr;
  }
  auto model = ((PySolidModelClass*)pySolidModelObj)->solidModel;
  if (model == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (model->ReadNative(fileName) != SV_OK) {
      delete model;
      api.error("Error reading a solid model from the file '" + std::string(fileName) + "'.");
      return nullptr;
  }

  return pySolidModelObj;
}

//---------------------
// SolidModeler_sphere 
//---------------------
//
PyDoc_STRVAR(SolidModeler_sphere_doc,
  "sphere(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModeler_sphere(PySolidModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("dO", PyRunTimeErr, __func__);
  static char *keywords[] = {"radius", "center", NULL};
  PyObject* centerArg;
  double center[3];
  double radius;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &radius, &centerArg)) {
      return api.argsError();
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(centerArg, emsg)) {
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
  auto pySolidModelObj = CreatePySolidModelObject(self->kernel);
  if (pySolidModelObj == nullptr) { 
      api.error("Error creating a Python solid model object.");
      return nullptr;
  } 
  auto model = ((PySolidModelClass*)pySolidModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

  if (model->MakeSphere(radius, center) != SV_OK) {
      delete model;
      api.error("Error creating a sphere solid model.");
      return nullptr;
  }

  return pySolidModelObj;
}

//-----------------------
// SolidModeler_subtract 
//-----------------------
//
PyDoc_STRVAR(SolidModeler_subtract_doc,
  "subtract(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModeler_subtract(PySolidModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("OO|s", PyRunTimeErr, __func__);
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
  auto mainModel = SolidModelerUtil_GetModelFromPyObj(mainModelArg);
  if (mainModel == nullptr) { 
      api.error("The main model argument is not a Model object.");
      return nullptr;
  }

  // Check that the subtractModel argument is a SV Python Model object.
  auto subtractModel = SolidModelerUtil_GetModelFromPyObj(subtractModelArg);
  if (subtractModel == nullptr) { 
      api.error("The subtract model argument is not a Model object.");
      return nullptr;
  }

  // Create the new solid.
  auto pySolidModelObj = CreatePySolidModelObject(self->kernel);
  if (pySolidModelObj == nullptr) {
      api.error("Error creating a Python solid model object.");
      return nullptr;
  }
  auto model = ((PySolidModelClass*)pySolidModelObj)->solidModel;
  if (model == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (model->Subtract(mainModel, subtractModel, smp) != SV_OK) {
      delete model;
      api.error("Error performing the Boolean subtract.");
      return nullptr;
  }

  return pySolidModelObj;
}

//--------------------
// SolidModeler_union 
//--------------------
//
PyDoc_STRVAR(SolidModeler_union_doc,
  "union(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModeler_union(PySolidModelerClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("OO|s", PyRunTimeErr, __func__);
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
  auto model1 = SolidModelerUtil_GetModelFromPyObj(model1Arg);
  if (model1 == nullptr) { 
      api.error("The first model argument is not a Model object.");
      return nullptr;
  }

  // Check that the model2 argument is a SV Python Model object.
  auto model2 = SolidModelerUtil_GetModelFromPyObj(model2Arg);
  if (model2 == nullptr) { 
      api.error("The second model argument is not a Model object.");
      return nullptr;
  }

  // Create the new solid.
  auto pySolidModelObj = CreatePySolidModelObject(self->kernel);
  if (pySolidModelObj == nullptr) { 
      api.error("Error creating a Python solid model object.");
      return nullptr;
  } 
  auto model = ((PySolidModelClass*)pySolidModelObj)->solidModel; 
  if (model == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (model->Union(model1, model2, smp) != SV_OK) {
      delete model;
      api.error("Error performing the Boolean union.");
      return nullptr;
  }

  return pySolidModelObj;
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

  { "box", (PyCFunction)SolidModeler_box, METH_VARARGS | METH_KEYWORDS, SolidModeler_box_doc },

  // [TODO:DaveP] The cvSolidModel MakeCircle method is not implemented.
  //{ "circle", (PyCFunction)SolidModeler_circle, METH_VARARGS, SolidModeler_circle_doc },

  { "cylinder", (PyCFunction)SolidModeler_cylinder, METH_VARARGS | METH_KEYWORDS, SolidModeler_cylinder_doc },

  { "intersect", (PyCFunction)SolidModeler_intersect, METH_VARARGS | METH_KEYWORDS, SolidModeler_intersect_doc },

  // [TODO:DaveP] The cvSolidModel MakeEllipsoid method is not implemented.
  //{ "ellipsoid", (PyCFunction)SolidModeler_ellipsoid, METH_VARARGS | METH_KEYWORDS, SolidModeler_ellipsoid_doc},

  { "read", (PyCFunction)SolidModeler_read, METH_VARARGS|METH_KEYWORDS, SolidModeler_read_doc },

  { "sphere", (PyCFunction)SolidModeler_sphere, METH_VARARGS | METH_KEYWORDS, SolidModeler_sphere_doc },

  { "subtract", (PyCFunction)SolidModeler_subtract, METH_VARARGS | METH_KEYWORDS, SolidModeler_subtract_doc },

  { "union", (PyCFunction)SolidModeler_union, METH_VARARGS | METH_KEYWORDS, SolidModeler_union_doc},

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

//-------------------------
// PySolidModelerClassType 
//-------------------------
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
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, "Modeler");
  char* kernelName = nullptr; 
  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  std::cout << "[PySolidModelerNew] Kernel: " << kernelName << std::endl;
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

  auto self = (PySolidModelerClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->id = 1;
  }

  return (PyObject *) self;
}

//-----------------------
// PySolidModelerDealloc 
//-----------------------
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


