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

// The functions defined here implement the SV Python API 'solid' Module. 
//
// The module name is 'solid'. The module defines a 'SolidModel' class used
// to store solid modeling data. The 'SolidModel' class cannot be imported and must
// be used prefixed by the module name. For example
//
//     model = solid.SolidModel()
//
// A Python exception sv.solid.SolidModelException is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.solid.SolidModelException: 
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_solid_init_py.h"
#include "sv_SolidModel.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"
#include "sv_vtk_utils.h"
#include "sv_PolyData.h"
#include "sv_PolyDataSolid.h"
#include "sv_sys_geom.h"
#include "sv_PyUtils.h"

#include "sv_FactoryRegistrar.h"
#include <map>

// Needed for Windows.
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"
#include "Python.h"
#include <structmember.h>
#include "vtkPythonUtil.h"
#if PYTHON_MAJOR_VERSION == 3
#include "PyVTKObject.h"
#elif PYTHON_MAJOR_VERSION == 2
#include "PyVTKClass.h"
#endif

#include "sv_occt_init_py.h"
#include "sv_polydatasolid_init_py.h"

// Define a map between solid model kernel name and enum type.
static std::map<std::string,SolidModel_KernelT> kernelNameTypeMap =
{
    {"Discrete", SM_KT_DISCRETE},
    {"MeshSimSolid", SM_KT_MESHSIMSOLID},
    {"OpenCASCADE", SM_KT_OCCT},
    {"Parasolid", SM_KT_PARASOLID},
    {"PolyData", SM_KT_POLYDATA}
};

typedef struct
{
  PyObject_HEAD
  cvSolidModel* geom;
}pySolidModel;

static void pySolidModel_dealloc(pySolidModel* self)
{
  Py_XDECREF(self->geom);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

// createSolidModelType() references pySolidModelType and is used before 
// it is defined so we need to define its prototype here.
pySolidModel * createSolidModelType();

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
static cvSolidModel *
CheckGeometry(SvPyUtilApiFunction& api, pySolidModel *self)
{
  auto geom = self->geom;
  if (geom == NULL) {
      api.error("The solid model object does not have geometry.");
      return nullptr;
  }

  return geom;
}


//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//------------------------
// Solid_list_registrars  
//------------------------
// This routine is used for debugging the registrar/factory system.
//
PyDoc_STRVAR(Solid_list_registrars_doc,
" list_registrars()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
Solid_list_registrars(PyObject* self, PyObject* args)
{
  cvFactoryRegistrar* pySolidModelRegistrar =(cvFactoryRegistrar *) PySys_GetObject("solidModelRegistrar");
  char result[255];
  PyObject* pyList=PyList_New(6);
  sprintf( result, "Solid model registrar ptr -> %p\n", pySolidModelRegistrar );
  PyList_SetItem(pyList,0,PyString_FromFormat(result));
  for (int i = 0; i < CV_MAX_FACTORY_METHOD_PTRS; i++) {
    sprintf( result, "GetFactoryMethodPtr(%i) = %p\n",
      i, (pySolidModelRegistrar->GetFactoryMethodPtr(i)));
    PyList_SetItem(pyList,i+1,PyString_FromFormat(result));
  }
  return pyList;
}

//------------------
// Solid_set_kernel  
//------------------
//
PyDoc_STRVAR(Solid_set_kernel_doc,
" set_kernel(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Solid_set_kernel(PyObject* self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *kernelArg;

  if (!PyArg_ParseTuple(args, api.format, &kernelArg)) {
      return api.argsError();
  }

  SolidModel_KernelT kernel;

  try {
        kernel = kernelNameTypeMap.at(std::string(kernelArg));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown solid modeling kernel '" + std::string(kernelArg) + "'." +
        " Valid solid modeling kernel names are: Discrete, MeshSimSolid, OpenCASCADE, Parasolid or PolyData.";
      api.error(msg);
      return nullptr;
  }

  cvSolidModel::gCurrentKernel = kernel;
  return Py_BuildValue("s", kernelArg);
}

//------------------
// Solid_get_kernel 
//------------------
//
PyDoc_STRVAR(Solid_get_kernel_doc,
" get_kernel()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Solid_get_kernel(PyObject* self, PyObject* args)
{
  char *kernelName;
  kernelName = SolidModel_KernelT_EnumToStr(cvSolidModel::gCurrentKernel);
  return Py_BuildValue("s",kernelName);
}

/////////////////////////////////////////////////////////////////
//          M o d u l e  C l a s s  F u n c t i o n s          //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the SolidModel class. 

//----------------------
// SolidModel_get_model 
//----------------------
//
// [TODO:DaveP] This does not 'get' anything, it sets the 'geom' data
//   to the cvSolidModel object from the repository.
//
//   Rename to: set_model_from_repository()
//
//
PyDoc_STRVAR(SolidModel_get_model_doc,
" SolidModel_get_model(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_model(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  // Get solid model.
  auto rd = gRepository->GetObject(objName);
  if (rd == nullptr) {
      api.error("The solid model '" + std::string(objName) + "' is not in the repository.");
      return nullptr;
  }

  auto type = rd->GetType();
  if (type != SOLID_MODEL_T) {
      api.error("'" + std::string(objName) + "' is not a solid model.");
      return nullptr;
  }
  
  auto geom = dynamic_cast<cvSolidModel*>(rd);
  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK; 
}

//---------------------------
// SolidModel_polygon_points 
//---------------------------
//
PyDoc_STRVAR(SolidModel_polygon_points_doc,
  "polygon_points(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_polygon_points(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName)) {
      return api.argsError();
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
      api.error("The repository object '" + std::string(dstName) + "' already exists.");
      return nullptr;
  }

  // Get polydata model from the repository.
  auto pd = gRepository->GetObject(srcName);
  if (pd == nullptr) {
      api.error("The polydata '" + std::string(srcName) + "' is not in the repository.");
      return nullptr;
  }

  auto type = pd->GetType();
  if (type != POLY_DATA_T) {
      api.error("'" + std::string(srcName) + "' is not of type polydata.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  // Create the polygon solid:
  if (geom->MakePoly2dPts((cvPolyData*)pd) != SV_OK) {
      api.error("Error creating a polygon solid model from polydata.");
      return nullptr;
  }

  // Register the solid:
  if (!gRepository->Register(dstName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(dstName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);

  return SV_PYTHON_OK;
}

//--------------------
// SolidModel_polygon 
//--------------------
//
PyDoc_STRVAR(SolidModel_polygon_doc,
  "polygon(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_polygon(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName)) {
      return api.argsError();
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
      api.error("The repository object '" + std::string(dstName) + "' already exists.");
      return nullptr;
  }

  // Get polydata model from the repository.
  auto pd = gRepository->GetObject(srcName);
  if (pd == nullptr) {
      api.error("The polydata '" + std::string(srcName) + "' is not in the repository.");
      return nullptr;
  }

  auto type = pd->GetType();
  if (type != POLY_DATA_T) {
      api.error("'" + std::string(srcName) + "' is not of type polydata.");
      return nullptr;
  }

  // Instantiate the new solid.
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  // Create the polygon solid.
  if (geom->MakePoly2d((cvPolyData*)pd) != SV_OK) {
      api.error("Error creating a polygon solid model from polydata.");
      return nullptr;
  }

  // Register the solid:
  if (!gRepository->Register(dstName,geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(dstName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//-------------------
// SolidModel_circle 
//-------------------
//
PyDoc_STRVAR(SolidModel_circle_doc,
  "circle(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_circle(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sddd", PyRunTimeErr, __func__);
  char *objName;
  double radius;
  double ctr[2];

  if (!PyArg_ParseTuple(args, api.format, &objName, &radius, &(ctr[0]), &(ctr[1]))) {
      return api.argsError();
  }

  if (radius <= 0.0) {
      api.error("The radius argument <= 0.0."); 
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a circle solid model.");
      return nullptr;
  }

  if (geom->MakeCircle(radius, ctr) != SV_OK) {
      delete geom;
      api.error("Error creating a circle solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      api.error("Error adding the solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);

  return SV_PYTHON_OK;
}

//-------------------
// SolidModel_sphere 
//-------------------
//
// [TODO:DaveP] should this be named 'create_sphere' ?
//
PyDoc_STRVAR(SolidModel_sphere_doc,
  "sphere(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_sphere(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sdO", PyRunTimeErr, __func__);
  char *objName;
  PyObject * centerArg;
  double ctr[3];
  double r;

  if (!PyArg_ParseTuple(args, api.format, &objName, &r, &centerArg)) {
      return api.argsError();
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(centerArg, emsg)) {
      api.error("The sphere center argument " + emsg);
      return nullptr;
  }

  for(int i=0;i<PyList_Size(centerArg);i++) {
    ctr[i] = PyFloat_AsDouble(PyList_GetItem(centerArg,i));
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a sphere solid model.");
      return nullptr;
  }

  if (geom->MakeSphere(r, ctr) != SV_OK) {
      delete geom;
      api.error("Error creating a sphere solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//--------------------
// SolidModel_ellipse 
//--------------------
//
PyDoc_STRVAR(SolidModel_ellipse_doc,
  "ellipse(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_ellipse(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sdddd", PyRunTimeErr, __func__);
  char *objName;
  double xr, yr;
  double ctr[2];

  if (!PyArg_ParseTuple(args, api.format, &objName, &xr, &yr, &(ctr[0]), &(ctr[1]))) {
      return api.argsError();
  }

  if (xr <= 0.0) { 
      api.error("The width argument <= 0.0."); 
      return nullptr;
  }

  if (yr <= 0.0) {
      api.error("The height argument <= 0.0."); 
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a ellipse solid model.");
      return nullptr;
  }

  if (geom->MakeEllipse(xr, yr, ctr) != SV_OK) {
      delete geom;
      api.error("Error creating a ellipse solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//------------------
// SolidModel_box2d 
//------------------
//
PyDoc_STRVAR(SolidModel_box2d_doc,
  "box2d(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_box2d(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sdddd", PyRunTimeErr, __func__);
  char *objName;
  double boxDims[2];
  double ctr[2];

  if (!PyArg_ParseTuple(args, api.format, &objName, &(boxDims[0]), &(boxDims[1]), &(ctr[0]), &(ctr[1]))) {
      return api.argsError();
  }

  if (boxDims[0] <= 0.0) { 
      api.error("The box height argument <= 0.0");
      return nullptr;
  }

  if (boxDims[1] <= 0.0) {
      api.error("The box width argument <= 0.0");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL ) {
      api.error("Error creating a 2D box solid model.");
      return nullptr;
  }

  if (geom->MakeBox2d(boxDims, ctr) != SV_OK) {
      delete geom;
      api.error("Error creating a 2D box solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the 2D box solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//------------------
// SolidModel_box3d 
//------------------
//
PyDoc_STRVAR(SolidModel_box3d_doc,
  "box3d(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_box3d( pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOO", PyRunTimeErr, __func__);
  char *objName;
  double dims[3];
  double ctr[3];
  PyObject* dimList;
  PyObject* ctrList;

  if (!PyArg_ParseTuple(args, api.format, &objName, &dimList, &ctrList)) {
      return api.argsError();
  }

  // [TODO:DaveP] do better check here.
  if (PyList_Size(dimList) > 3) {
      api.error("The dimension list argument > 3.");
      return nullptr;
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(ctrList, emsg)) {
      api.error("The box center argument " + emsg);
      return nullptr;
  }

  for (int i=0;i<PyList_Size(dimList);i++) {
    dims[i]=PyFloat_AsDouble(PyList_GetItem(dimList,i));
  }
  for (int i=0;i<PyList_Size(ctrList);i++) {
    ctr[i]=PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }

  // Do work of command:

  if ((dims[0] <= 0.0) || (dims[1] <= 0.0) || (dims[2] <= 0.0)) {
      api.error("The box dimensions argument <= 0.0.");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a 3D box solid model.");
      return nullptr;
  }

  if (geom->MakeBox3d(dims, ctr) != SV_OK) {
      delete geom;
      api.error("Error creating a 3D box solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the 3D box solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//---------------------
// SolidModel_ellipsoid 
//---------------------
//
PyDoc_STRVAR(SolidModel_ellipsoid_doc,
  "ellipsoid(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_ellipsoid(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOO", PyRunTimeErr, __func__);
  char *objName;
  PyObject* rList;
  PyObject* ctrList;

  if (!PyArg_ParseTuple(args, api.format, &objName, &rList, &ctrList)) {
      return api.argsError();
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(ctrList, emsg)) {
      api.error("The ellipsoid center argument " + emsg);
      return nullptr;
  }

  if (!svPyUtilCheckPointData(rList, emsg)) {
      api.error("The ellipsoid radius vector argument " + emsg);
      return nullptr;
  }

  double ctr[3];
  double r[3];

  for (int i=0;i<3;i++) {
    r[i] = PyFloat_AsDouble(PyList_GetItem(rList,i));
    ctr[i] = PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating an ellipsoid sphere solid model.");
      return nullptr;
  }

  if (geom->MakeEllipsoid(r, ctr) != SV_OK) {
      delete geom;
      api.error("Error creating an ellipsoid sphere solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//---------------------
// SolidModel_cylinder 
//---------------------
PyDoc_STRVAR(SolidModel_cylinder_doc,
  "cylinder(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_cylinder(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sddOO", PyRunTimeErr, __func__);
  char *objName;
  double r, l;
  PyObject* ctrList;
  PyObject* axisList;

  if (!PyArg_ParseTuple(args, api.format, &objName, &r, &l, &ctrList, &axisList)) {
      return api.argsError();
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(ctrList, emsg)) {
      api.error("The cylinder center argument " + emsg);
      return nullptr;
  }

  if (!svPyUtilCheckPointData(axisList, emsg)) {
      api.error("The cylinder axis argument " + emsg);
      return nullptr;
  }

  if (r <= 0.0) { 
      api.error("The radius argument <= 0.0."); 
      return nullptr;
  }

  if (l <= 0.0) { 
      api.error("The length argument <= 0.0."); 
      return nullptr;
  }

  double ctr[3];
  double axis[3];
  for (int i=0;i<3;i++) {
    axis[i] = PyFloat_AsDouble(PyList_GetItem(axisList,i));
    ctr[i] = PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

  if (geom->MakeCylinder(r, l, ctr, axis) != SV_OK) {
      delete geom;
      api.error("Error creating a cylinder solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the cylinder solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//---------------------------
// SolidModel_truncated_cone 
//---------------------------
//
PyDoc_STRVAR(SolidModel_truncated_cone_doc,
  "truncated_cone(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_truncated_cone(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sddOO", PyRunTimeErr, __func__);
  char *objName;
  double r1, r2;
  PyObject* ptList;
  PyObject* dirList;

  if (!PyArg_ParseTuple(args, api.format, &objName, &r1, &r2, &ptList, &dirList)) {
      return api.argsError();
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(ptList, emsg)) {
      api.error("The truncated cone point list argument " + emsg);
      return nullptr;
  }

  if (!svPyUtilCheckPointData(ptList, emsg)) {
      api.error("The truncated cone direction vector argument " + emsg);
      return nullptr;
  }

  if (r1 <= 0.0) {
      api.error("The radius 1 argument <= 0.0."); 
      return nullptr;
  }

  if (r2 <= 0.0) {
      api.error("The radius 2 argument <= 0.0."); 
      return nullptr;
  }

  double pt[3];
  double dir[3];
  for (int i=0;i<3;i++) {
    pt[i]=PyFloat_AsDouble(PyList_GetItem(ptList,i));
    dir[i]=PyFloat_AsDouble(PyList_GetItem(dirList,i));
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a truncated cone solid model.");
      return nullptr;
  }

  if (geom->MakeTruncatedCone(pt, dir, r1, r2) != SV_OK) {
      delete geom;
      api.error("Error creating a truncated cone solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the truncated cone solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//------------------
// SolidModel_torus 
//------------------
//
PyDoc_STRVAR(SolidModel_torus_doc,
  "_torus(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_torus(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sddOO", PyRunTimeErr, __func__);
  char *objName;
  PyObject* ctrList;
  PyObject* axisList;
  double rmaj, rmin;

  if (!PyArg_ParseTuple(args, api.format, &objName, &rmaj, &rmin, &ctrList, &axisList)) {
      return api.argsError();
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(ctrList, emsg)) {
      api.error("The torus center argument " + emsg);
      return nullptr;
  }

  if (!svPyUtilCheckPointData(axisList, emsg)) {
      api.error("The torus axis argument " + emsg);
      return nullptr;
  }

  if (rmaj  <= 0.0) {
      api.error("The torus major radius argument <= 0.0."); 
      return nullptr;
  }

  if (rmin  <= 0.0) {
      api.error("The torus minor radius argument <= 0.0."); 
      return nullptr;
  }

  double ctr[3];
  double axis[3];
  for (int i=0;i<3;i++) {
    axis[i]=PyFloat_AsDouble(PyList_GetItem(axisList,i));
    ctr[i]=PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a torus solid model.");
      return nullptr;
  }

  if (geom->MakeTorus(rmaj, rmin, ctr, axis) != SV_OK) {
      delete geom;
      api.error("Error creating a torus solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the torus solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//-------------------------
// SolidModel_poly3d_solid 
//-------------------------
//
// [TODO:DaveP] not sure about this name.
//
PyDoc_STRVAR(SolidModel_poly3d_solid_doc,
  "poly3d_solid(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_poly3d_solid(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssd", PyRunTimeErr, __func__);
  char *objName;
  char *srcName;
  char *facetMethodName;
  double angle = 0.0;

  if (!PyArg_ParseTuple(args, api.format, &objName, &srcName, &facetMethodName, &angle)) {
      return api.argsError();
  }

  auto facetMethod = SolidModel_FacetT_StrToEnum( facetMethodName );
  if (facetMethod == SM_Facet_Invalid) {
      api.error("Unknown polysolid facet method argument type '"+std::string(facetMethodName)+
        "'. Valid methods are: Sew, Union or Webl."); 
      return nullptr;
  }

  // Retrieve cvPolyData source:
  auto pd = gRepository->GetObject( srcName );
  if (pd == NULL) {
      api.error("The polydata '" + std::string(srcName) + "' is not in the repository.");
      return nullptr;
  }

  auto type = pd->GetType();
  if (type != POLY_DATA_T ) {
      api.error("'" + std::string(srcName) + "' is not of type polydata.");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (geom->SetPoly3dFacetMethod(facetMethod) != SV_OK) {
      delete geom;
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if ( geom->MakePoly3dSolid( (cvPolyData*)pd , angle ) != SV_OK ) {
      delete geom;
      api.error("Error creating a solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//---------------------------
// SolidModel_poly3d_surface 
//---------------------------
//
PyDoc_STRVAR(SolidModel_poly3d_surface_doc,
  "poly3d_surface(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_poly3d_surface(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss", PyRunTimeErr, __func__);
  char *objName;
  char *srcName;
  char *facetMethodName;

  if (!PyArg_ParseTuple(args, api.format, &objName, &srcName, &facetMethodName)) {
      return api.argsError();
  }

  auto facetMethod = SolidModel_FacetT_StrToEnum( facetMethodName );
  if ( facetMethod == SM_Facet_Invalid ) {
      api.error("Unknown polysolid facet method argument type '"+std::string(facetMethodName)+
        "'. Valid methods are: Sew, Union or Webl."); 
      return nullptr;
  }

  // Retrieve cvPolyData source:
  auto pd = gRepository->GetObject( srcName );
  if ( pd == NULL ) {
      api.error("The polydata '" + std::string(srcName) + "' is not in the repository.");
      return nullptr;
  }
  auto type = pd->GetType();
  if (type != POLY_DATA_T) {
      api.error("'" + std::string(srcName) + "' is not of type polydata.");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
      api.error("Error creating a poly3d solid model.");
      return nullptr;
  }

  if ( geom->SetPoly3dFacetMethod( facetMethod ) != SV_OK ) {
      delete geom;
      api.error("Error setting facet method to '" + std::string(facetMethodName) + "'.");
      return nullptr;
  }

  if (geom->MakePoly3dSurface( (cvPolyData*)pd ) != SV_OK) {
      delete geom;
      api.error("Error creating a poly3d solid model.");
      return nullptr;
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//----------------------
// SolidModel_extrude_z 
//----------------------
//
PyDoc_STRVAR(SolidModel_extrude_z_doc,
  "poly3d_surface(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_extrude_z(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssd", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  double dist;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName, &dist)) {
      return api.argsError();
  }

  // Retrieve cvSolidModel source:
  auto src = gRepository->GetObject( srcName );
  if (src == NULL) {
      api.error("The solid model '"+std::string(srcName)+"' is not in the repository.");
      return nullptr;
  }
  
  auto type = src->GetType();
  if (type != SOLID_MODEL_T) {
      api.error("'" + std::string(srcName) + "' is not of type solid model.");
      return nullptr;
  }

  if (dist <= 0.0) {
      api.error("The extrude solid distance argument <= 0.0."); 
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
      api.error("The repository object '" + std::string(dstName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a extruded solid model.");
      return nullptr;
  }

  if ( geom->ExtrudeZ( (cvSolidModel *)src, dist ) != SV_OK ) {
      delete geom;
      api.error("Error creating a extruded solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(dstName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(dstName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//--------------------
// SolidModel_extrude 
//--------------------
//
PyDoc_STRVAR(SolidModel_extrude_doc,
  "extrude(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_extrude(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssOO", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  PyObject* pt1List;
  PyObject* pt2List;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName, &pt1List, &pt2List)) {
      return api.argsError();
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(pt1List, emsg)) {
      api.error("The extrude point1 argument " + emsg);
      return nullptr;
  }

  if (!svPyUtilCheckPointData(pt2List, emsg)) {
      api.error("The extrude point2 argument " + emsg);
      return nullptr;
  }


  double pt1[3],pt2[3];
  for (int i=0;i<PyList_Size(pt1List);i++) {
    pt1[i]=PyFloat_AsDouble(PyList_GetItem(pt1List,i));
  }
  for (int i=0;i<PyList_Size(pt2List);i++) {
    pt2[i]=PyFloat_AsDouble(PyList_GetItem(pt2List,i));
  }

  // Retrieve cvSolidModel source:
  auto src = gRepository->GetObject( srcName );
  if (src == NULL) {
      api.error("The solid model '"+std::string(srcName)+"' is not in the repository.");
      return nullptr;
  }

  auto type = src->GetType();
  if (type != SOLID_MODEL_T) {
      api.error("'" + std::string(srcName) + "' is not of type solid model.");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
      api.error("The repository object '" + std::string(dstName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a extruded solid model.");
      return nullptr;
  }

  // [TODO:DaveP] should be using std::vector<> here.
  auto dist = new double*[2];
  dist[0] = &pt1[0];
  dist[1] = &pt2[0];

  if (geom->Extrude((cvSolidModel*)src, dist) != SV_OK) {
      delete geom;
      delete dist;
      api.error("Error creating a extruded solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(dstName, geom)) {
    delete geom;
    delete dist;
    api.error("Error adding the extrude solid model '" + std::string(dstName) + "' to the repository.");
    return nullptr;
  }

  delete dist;
  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//----------------------------------------
// SolidModel_make_approximate_curve_loop 
//----------------------------------------
//
// [TODO:DaveP] here we have 'make' in the name unlike others.
//
PyDoc_STRVAR(SolidModel_make_approximate_curve_loop_doc,
  "make_approximate_curve_loop(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_make_approximate_curve_loop(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssdi", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  double tol;
  int closed = 1;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName, &tol, &closed)) {
      return api.argsError();
  }

  // Retrieve cvPolyData source:
  auto src = gRepository->GetObject( srcName );
  if (src == NULL) {
      api.error("The solid model '"+std::string(srcName)+"' is not in the repository.");
      return nullptr;
  }

  auto type = src->GetType();
  if (type != POLY_DATA_T) {
      api.error("'" + std::string(srcName) + "' is not of type solid model.");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
      api.error("The repository object '" + std::string(dstName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL ) {
      api.error("Error creating curve loop solid model.");
      return nullptr;
  }

  if (geom->MakeApproxCurveLoop( (cvPolyData *)src, tol, closed ) != SV_OK) {
      delete geom;
      api.error("Error creating curve loop solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(dstName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(dstName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//-----------------------------------------
// SolidModel_make_interpolated_curve_loop 
//-----------------------------------------
//
PyDoc_STRVAR(SolidModel_make_interpolated_curve_loop_doc,
  "make_interpolated_curve_loop(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_make_interpolated_curve_loop( pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss|i", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  int closed = 1;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName, &closed)) {
      return api.argsError();
  }

  // Retrieve cvPolyData source:
  auto src = gRepository->GetObject( srcName );
  if (src == NULL) {
      api.error("The solid model '"+std::string(srcName)+"' is not in the repository.");
      return nullptr;
  }

  auto type = src->GetType();
  if (type != POLY_DATA_T) {
      api.error("'" + std::string(srcName) + "' is not of type polydata.");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
      api.error("The repository object '" + std::string(dstName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a interpolated curve loop solid model.");
      return nullptr;
  }

  if (geom->MakeInterpCurveLoop( (cvPolyData *)src, closed ) != SV_OK) {
      delete geom;
      api.error("Error creating a interpolated curve loop solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(dstName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(dstName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//--------------------------------
// SolidModel_make_lofted_surface 
//--------------------------------
//
PyDoc_STRVAR(SolidModel_make_lofted_surface_doc,
  "make_lofted_surface(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_make_lofted_surface(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Os|iidddi", PyRunTimeErr, __func__);
  PyObject* srcList;
  char *dstName;
  int continuity=0;
  int partype=0;
  int smoothing=0;
  double w1=0.4,w2=0.2,w3=0.4;

  if (!PyArg_ParseTuple(args, api.format, &srcList, &dstName, &continuity, &partype, &w1, &w2, &w3, &smoothing)) {
      return api.argsError();
  }

  auto numSrcs = PyList_Size(srcList);
  if (numSrcs < 2) {
      api.error("The loft surface number of sources argument is less than two.");
      return nullptr;
  }

  // Check source curves.
  //
  std::vector<cvSolidModel*> source_curves;

  for (int i = 0; i < numSrcs; i++) {
      auto srcName = PyString_AsString(PyList_GetItem(srcList,i));
      auto src = gRepository->GetObject( PyString_AsString(PyList_GetItem(srcList,i)) );
      if (src == NULL) {
          api.error("The lofting source curve '"+std::string(srcName)+"' is not in the repository.");
          return nullptr;
      }

      auto type = src->GetType();
      if (type != SOLID_MODEL_T ) {
          api.error("The lofting source curve '"+std::string(srcName)+"' is not a solid model.");
          return nullptr;
      }
      source_curves.push_back((cvSolidModel*)src);
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
      api.error("The repository object '" + std::string(dstName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a lofted solid model.");
      return nullptr;
  }

  if (geom->MakeLoftedSurf(source_curves.data(), numSrcs, dstName, continuity, partype,
        w1, w2, w3, smoothing) != SV_OK) {
      delete geom;
      api.error("Error creating a lofted solid model.");
      return nullptr;
  }

  // Register the new solid.
  if (!gRepository->Register(dstName, geom)) {
      delete geom;
      api.error("Error adding the lofted surface solid model '" + std::string(dstName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//---------------------------------
// SolidModel_cap_surface_to_solid
//---------------------------------
//
PyDoc_STRVAR(SolidModel_cap_surface_to_solid_doc,
  "cap_surface_to_solid(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_cap_surface_to_solid(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName)) {
      return api.argsError();
  }

  auto src = gRepository->GetObject(srcName);
  if (src == NULL) {
      api.error("The solid model '"+std::string(srcName)+"' is not in the repository.");
      return nullptr;
  }

  auto type = src->GetType();
  if (type != SOLID_MODEL_T) {
      api.error("'" + std::string(srcName) + "' is not of type solid model.");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
      api.error("The repository object '" + std::string(dstName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL ) {
      api.error("Error creating a capped surfaces solid model.");
      return nullptr;
  }

  if (geom->CapSurfToSolid((cvSolidModel*)src) != SV_OK) {
      delete geom;
      api.error("Error creating a capped surfaces solid model.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(dstName, geom)) {
      delete geom;
      api.error("Error adding the capped surface solid model '" + std::string(dstName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//------------------------
// SolidModel_read_native 
//------------------------
//
PyDoc_STRVAR(SolidModel_read_native_doc,
  "read_native(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_read_native(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *objName, *fileName;

  if (!PyArg_ParseTuple(args, api.format, &objName, &fileName)) {
      return api.argsError();
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  if (cvSolidModel::gCurrentKernel == SM_KT_INVALID) { 
      api.error("The solid model kernel is not set.");
      return nullptr;
  }

  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if ( geom->ReadNative( fileName ) != SV_OK ) {
      delete geom;
      api.error("Error reading a solid model from the file '" + std::string(fileName) + "'.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//-----------------
// SolidModel_copy
//-----------------
//
PyDoc_STRVAR(SolidModel_copy_doc,
  "copy(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_copy(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName)) {
      return api.argsError();
  }

  // Retrieve source:
  auto srcGeom = gRepository->GetObject(srcName);
  if (srcGeom == NULL) {
      api.error("The solid model '"+std::string(srcName)+"' is not in the repository.");
      return nullptr;
  }

  auto src_t = gRepository->GetType(srcName);
  if (src_t != SOLID_MODEL_T) {
      api.error("'" + std::string(srcName) + "' is not of type solid model.");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
      api.error("The repository object '" + std::string(dstName) + "' already exists.");
      return nullptr;
  }

  // Instantiate the new solid:
  auto dstGeom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( dstGeom == NULL ) {
      api.error("Error creating solid model.");
      return nullptr;
  }

  if (dstGeom->Copy(*((cvSolidModel*)srcGeom)) != SV_OK) {
    delete dstGeom;
    api.error("Error copying solid model.");
    return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(dstName, dstGeom)) {
      delete dstGeom;
      api.error("Error adding the solid model '" + std::string(dstName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(dstGeom);
  self->geom=dstGeom;
  Py_DECREF(dstGeom);
  return SV_PYTHON_OK;
}

//----------------------
// SolidModel_intersect 
//----------------------
//
PyDoc_STRVAR(SolidModel_intersect_doc,
  "intersect(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_intersect( pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss|s", PyRunTimeErr, __func__);
  char *resultName;
  char *aName;
  char *bName;
  char *smpName=NULL;

  if (!PyArg_ParseTuple(args, api.format, &resultName, &aName, &bName, &smpName)) {
      return api.argsError();
  }

  // Parse the simplification flag if given.
  auto smp = CheckSimplificationName(api, smpName);
  if (smp == SM_Simplify_Invalid) {
      return nullptr;
  }

  // Check that operands are solid models.
  auto gmA = CheckSolidModel(api, aName);
  if (gmA == NULL) {
      return nullptr;
  }
  auto gmB = CheckSolidModel(api, bName);
  if (gmB == NULL) {
      return nullptr;
  }

  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (geom->Intersect( (cvSolidModel*)gmA, (cvSolidModel*)gmB, smp ) != SV_OK ) {
      delete geom;
      api.error("Error performing a Boolean intersection.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(resultName, geom)) {
    delete geom;
    api.error("Error adding the solid model '" + std::string(resultName) + "' to the repository.");
    return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//------------------
// SolidModel_union 
//------------------
//
PyDoc_STRVAR(SolidModel_union_doc,
  "union(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_union(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss|s", PyRunTimeErr, __func__);
  char *resultName;
  char *aName;
  char *bName;
  char *smpName=NULL;

  if (!PyArg_ParseTuple(args, api.format, &resultName, &aName, &bName, &smpName)) {
      return api.argsError();
  }

  // Parse the simplification flag if given.
  auto smp = CheckSimplificationName(api, smpName);
  if (smp == SM_Simplify_Invalid) {
      return nullptr;
  }

  // Check that operands are solid models.
  auto gmA = CheckSolidModel(api, aName);
  if (gmA == NULL) {
      return nullptr;
  }
  auto gmB = CheckSolidModel(api, bName);
  if (gmB == NULL) {
      return nullptr;
  }

  // Instantiate the new solid:
  auto result = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (result == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (result->Union((cvSolidModel*)gmA, (cvSolidModel*)gmB, smp) != SV_OK) {
      delete result;
      api.error("Error performing the Boolean union.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(resultName, result)) {
      delete result;
      api.error("Error adding the solid model '" + std::string(resultName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(result);
  self->geom=result;
  Py_DECREF(result);
  return SV_PYTHON_OK;
}

//---------------------
// SolidModel_subtract 
//---------------------
//
PyDoc_STRVAR(SolidModel_subtract_doc,
  "union(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_subtract(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss|s", PyRunTimeErr, __func__);
  char *resultName;
  char *aName;
  char *bName;
  char *smpName=NULL;

  if (!PyArg_ParseTuple(args, api.format, &resultName, &aName, &bName, &smpName)) {
      return api.argsError();
  }

  // Parse the simplification flag if given.
  auto smp = CheckSimplificationName(api, smpName);
  if (smp == SM_Simplify_Invalid) {
      return nullptr;
  }

  // Check that operands are solid models.
  auto gmA = CheckSolidModel(api, aName);
  if (gmA == NULL) {
      return nullptr;
  }
  auto gmB = CheckSolidModel(api, bName);
  if (gmB == NULL) {
      return nullptr;
  }

  // Instantiate the new solid:
  auto result = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (result == NULL) {
      api.error("Error creating a solid model.");
      return nullptr;
  }

  if (result->Subtract( (cvSolidModel*)gmA, (cvSolidModel*)gmB, smp) != SV_OK) {
      delete result;
      api.error("Error performing the Boolean subtract.");
      return nullptr;
  }

  // Register the new solid:
  if (!gRepository->Register(resultName, result)) {
      delete result;
      api.error("Error adding the solid model '" + std::string(resultName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(result);
  self->geom=result;
  Py_DECREF(result);
  return SV_PYTHON_OK;
}

// ------------------
// SolidModel_object
// ------------------
// [TODO:DaveP] is this useful?
//
static PyObject * 
SolidModel_object(pySolidModel* self,PyObject* args )
{
  if (PyTuple_Size(args)== 0 ) {
    //PrintMethods();
  }
  return SV_PYTHON_OK;
}

// [TODO:DaveP] is this used?
/*
PyObject * 
DeleteSolid( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = self->geom;

  gRepository->UnRegister( geom->GetName() );
  Py_INCREF(Py_None);
  return Py_None;
}
*/

//-----------------------
// SolidModel_new_object 
//-----------------------
//
// [TODO:DaveP] why is the solid modeler kernel not
//   passed as an argument here?
//
PyDoc_STRVAR(SolidModel_new_object_doc,
  "new_object(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
SolidModel_new_object(pySolidModel* self,PyObject *args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName, *fileName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Check curent kernel? Ugh! 
  if (cvSolidModel::gCurrentKernel == SM_KT_INVALID) { 
      api.error("The solid model kernel is not set.");
      return nullptr;
  }
    
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating solid model.");
      return nullptr;
  }

   // Register the new solid:
   if (!gRepository->Register(objName, geom)) {
      delete geom;
      api.error("Error adding the solid model '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom = geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//---------------------------
// SolidModel_get_class_name
//---------------------------
//
PyDoc_STRVAR(SolidModel_get_class_name_doc,
" get_class_name()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_class_name(pySolidModel* self, PyObject* args)
{
  return Py_BuildValue("s","SolidModel");
}

//------------------------
// SolidModel_find_extent
//------------------------
//
PyDoc_STRVAR(SolidModel_find_extent_doc,
" find_extent()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
SolidModel_find_extent(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto geom =self->geom;

  double extent;
  auto status = geom->FindExtent(&extent);

  if (status != SV_OK) {
      api.error("Error finding extent");
      return nullptr;
  }

  Py_BuildValue("d",extent);
  return SV_PYTHON_OK;
}

//---------------------------
// SolidModel_find_centroid
//---------------------------
PyDoc_STRVAR(SolidModel_find_centroid_doc,
" find_centroid()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *  
SolidModel_find_centroid(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  double centroid[3];
  int tdim;

  cvSolidModel *geom = self->geom;

  if (geom->GetSpatialDim(&tdim) != SV_OK) {
      api.error("Unable to get the spatial dimension of the solid model.");
      return nullptr;
  }

  if ((tdim != 2) && (tdim != 3)) {
      api.error("The spatial dimension " + std::to_string(tdim) + " is not supported.");
      return nullptr;
  }

  if (geom->FindCentroid(centroid) != SV_OK) {
      api.error("Error finding centroid of the solid model.");
      return nullptr;
  }

  PyObject* tmpPy = PyList_New(3);
  PyList_SetItem(tmpPy,0,PyFloat_FromDouble(centroid[0]));
  PyList_SetItem(tmpPy,1,PyFloat_FromDouble(centroid[1]));

  if (tdim == 3) {
      PyList_SetItem(tmpPy,2,PyFloat_FromDouble(centroid[2]));
  }

  return tmpPy;
}

//--------------------------------------
// SolidModel_get_topological_dimension
//--------------------------------------
//
PyDoc_STRVAR(SolidModel_get_topological_dimension_doc,
" get_topological_dimension()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_get_topological_dimension( pySolidModel *self ,PyObject* args  )
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto geom = self->geom;
  int tdim;

  if (geom->GetTopoDim(&tdim) != SV_OK) {
      api.error("Error getting the topological dimension of the solid model.");
      return nullptr;
  }

  return Py_BuildValue("d",tdim);
}

// ----------------------
// Solid_GetSpatialDimMtd
// ----------------------
PyDoc_STRVAR(SolidModel_get_spatial_dimension_doc,
" get_spatial_dimension()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *  
SolidModel_get_spatial_dimension(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto geom = self->geom;
  int sdim;

  if (geom->GetSpatialDim(&sdim) != SV_OK) {
      api.error("Error getting the spatial dimension of the solid model.");
      return nullptr;
  }

  return Py_BuildValue("d",sdim);
}

// -------------------
// Solid_ClassifyPtMtd
// -------------------
PyDoc_STRVAR(SolidModel_classify_point_doc,
" classify_point(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_classify_point(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("dd|di", PyRunTimeErr, __func__);
  double x, y;
  double z = std::numeric_limits<double>::infinity();
  int v = 0;
  int ans;
  int status;
  int tdim, sdim;

  if (!PyArg_ParseTuple(args, api.format,&x,&y,&z,&v)) {
      return api.argsError();
  }

  auto geom = self->geom;
  geom->GetTopoDim(&tdim);
  geom->GetSpatialDim(&sdim);

  if (!std::isinf(z)) {
      status = geom->ClassifyPt(x, y, z, v, &ans);
  } else {
      if ((tdim == 2) && (sdim == 2)) {
          status = geom->ClassifyPt( x, y, v, &ans);
      } else {
          api.error("the solid model must have a topological and spatial dimension of two.");
          return nullptr;
      }
  }

  if (status != SV_OK) {
      api.error("Error classifying a point for the solid model.");
      return nullptr;
  }

  return Py_BuildValue("d",ans);
}

// -----------------
// Solid_DistanceMtd
// -----------------
PyDoc_STRVAR(SolidModel_distance_doc,
" distance(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_distance(pySolidModel *self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Od", PyRunTimeErr, __func__);
  double pos[3];
  int npos;
  double upperLimit, dist;
  int sdim;
  int status;
  PyObject *posList;

  if (!PyArg_ParseTuple(args, api.format, &posList, &upperLimit)) {
      return api.argsError();
  }

  auto geom = self->geom;

  if (PyList_Size(posList) > 3) {
      api.error("The position argument is not between 1 and 3.");
      return nullptr;
  }

  npos = PyList_Size(posList);
  for (int i=0;i<npos;i++) {
      pos[i] = PyFloat_AsDouble(PyList_GetItem(posList,i));
  }

  // Check validity of given pos.
  if (geom->GetSpatialDim(&sdim) != SV_OK ) {
      api.error("Error getting the spatial dimension of the solid model.");
      return nullptr;
  }

  if ((sdim == 3) && (npos != 3)) {
      api.error("The postion argument is not a 3D point. A 3D solid model requires a 3D point.");
      return nullptr;
  } else if ((sdim == 2) && (npos != 2)) {
     PyErr_SetString(PyRunTimeErr,"objects in 2 spatial dims require a 2D position");
      api.error("The postion argument is not a 2D point. A 2D solid model requires a 2D point.");
      return nullptr;
  }

  status = geom->Distance( pos, upperLimit, &dist );

  if (geom->Distance( pos, upperLimit, &dist) != SV_OK) {
      api.error("Error computing the distance to the solid model.");
      return nullptr;
  }

  return Py_BuildValue("d",dist);
}

//----------------------
// SolidModel_translate 
//----------------------
//
PyDoc_STRVAR(SolidModel_translate_doc,
" translate(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_translate(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__);
  PyObject*  vecList;
  double vec[3];
  int nvec;
  int status;

  if (!PyArg_ParseTuple(args, api.format, &vecList)) {
      return api.argsError();
  }

  if (PyList_Size(vecList) > 3) {
      api.error("The translation vector argument is > 3.");
      return nullptr;
  }

  // [TODO:DaveP] do a better check here.
  nvec = PyList_Size(vecList);
  for(int i=0;i<nvec;i++) {
    vec[i] = PyFloat_AsDouble(PyList_GetItem(vecList,i));
  }

  auto geom = self->geom;

  if (geom->Translate(vec, nvec) != SV_OK) {
      api.error("Error translating the solid model.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// ---------------
// Solid_RotateMtd
// ---------------

PyDoc_STRVAR(SolidModel_rotate_doc,
" rotate(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_rotate( pySolidModel *self ,PyObject* args  )
{
  auto api = SvPyUtilApiFunction("Od", PyRunTimeErr, __func__);
  PyObject*  axisList;
  double rad;

  if (!PyArg_ParseTuple(args, api.format, &axisList, &rad)) {
      return api.argsError();
  }
  auto naxis = PyList_Size(axisList);
  if (naxis > 3) {
      api.error("The rotation axis argument is > 3.");
      return nullptr;
  }

  double axis[3];
  for(int i=0;i<naxis;i++) {
    axis[i]=PyFloat_AsDouble(PyList_GetItem(axisList,i));
  }

  auto geom = self->geom;

  if (geom->Rotate(axis, naxis, rad) != SV_OK) {
      api.error("Error rotating the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}


//------------------
// SolidModel_scale
//------------------
//
PyDoc_STRVAR(SolidModel_scale_doc,
" scale(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_scale(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__);
  double factor;

  if (!PyArg_ParseTuple(args, api.format, &factor)) {
      return api.argsError();
  }

  auto geom = self->geom;

  if (geom->Scale(factor) != SV_OK) {
      api.error("Error scaling the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------
// Solid_ReflectMtd
//----------------
//
PyDoc_STRVAR(SolidModel_reflect_doc,
" reflect(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_reflect(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("OO", PyRunTimeErr, __func__);
  PyObject* posList;
  PyObject* nrmList;
  int status;

  if (!PyArg_ParseTuple(args, api.format, &posList, &nrmList)) {
      return api.argsError();
  }

  // [TODO:DaveP] do more checking here.
  //   Can posList be < 3?
  //
  if (PyList_Size(posList) > 3) {
      api.error("The position argument is > 3.");
      return nullptr;
  }

  if (PyList_Size(nrmList) > 3) {
      api.error("The normal argument is > 3.");
      return nullptr;
  }

  double pos[3];
  for(int i=0;i<PyList_Size(posList);i++) {
    pos[i]=PyFloat_AsDouble(PyList_GetItem(posList,i));
  }

  double nrm[3];
  for(int i=0;i<PyList_Size(nrmList);i++) {
    nrm[i]=PyFloat_AsDouble(PyList_GetItem(nrmList,i));
  }

  auto geom = self->geom;

  if (geom->Reflect(pos, nrm) != SV_OK) {
      api.error("Error reflecting the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-----------------
// Solid_Apply4x4Mtd
//-----------------
//
PyDoc_STRVAR(SolidModel_apply4x4_doc,
" apply4x4(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_apply4x4(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__);
  PyObject* matList;
  PyObject* rowList;

  if (!PyArg_ParseTuple(args, api.format, &matList)) {
      return api.argsError();
  }

  if (PyList_Size(matList) != 4) {
      api.error("The matrix argument is not a 4x4 matrix.");
      return nullptr;
  }

  // Extract the 4x4 matrix.
  //
  double mat[4][4];
  for (int i=0;i<PyList_Size(matList);i++) {
      rowList = PyList_GetItem(matList,i);
      if (PyList_Size(rowList) != 4) {
          api.error("The matrix argument is not a 4x4 matrix.");
          return nullptr;
      }
      for (int j=0;j<PyList_Size(rowList);j++) {
          mat[i][j] = PyFloat_AsDouble(PyList_GetItem(rowList,j));
      }
  }

  auto geom = self->geom;
  if (geom->Apply4x4(mat) != SV_OK) {
      api.error("Error applyonig a 4x4 matrix to the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------------
// SolidModel_print
//------------------
//
PyDoc_STRVAR(SolidModel_print_doc,
" print(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *  
SolidModel_print(pySolidModel *self ,PyObject* args)
{
  auto geom = self->geom;
  geom->Print();
  return SV_PYTHON_OK;
}

// --------------
// Solid_CheckMtd
// --------------
PyDoc_STRVAR(SolidModel_check_doc,
" check()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *  
SolidModel_check(pySolidModel *self ,PyObject* args)
{
  auto geom = self->geom;
  int nerr;
  geom->Check( &nerr );
  return Py_BuildValue("i",nerr);
}

//-------------------------
// SolidModel_write_native
//-------------------------
//
PyDoc_STRVAR(SolidModel_write_native_doc,
" write_native(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_write_native(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__);
  char *fn;
  int status;
  int file_version = 0;

  if (!PyArg_ParseTuple(args, api.format, &fn, &file_version)) {
      return api.argsError();
  }

  auto geom = self->geom;
  if (geom->WriteNative(file_version, fn) != SV_OK) {
      api.error("Error writing the solid model to the file '" + std::string(fn) + "' using version '" + std::to_string(file_version)+"'."); 
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//-------------------------------
// SolidModel_write_vtk_polydata
//-------------------------------
//
PyDoc_STRVAR(SolidModel_write_vtk_polydata_doc,
" write_vtk_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_write_vtk_polydata(pySolidModel *self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fn;

  if (!PyArg_ParseTuple(args, api.format, &fn)) {
      return api.argsError();
  }

  auto geom = self->geom;
  if (geom->WriteVtkPolyData(fn) != SV_OK) {
      api.error("Error writing the solid model to the file '" + std::string(fn) + "'.");
      return nullptr;
  }
    
  return SV_PYTHON_OK;
}

//---------------------------
// SolidModel_write_geom_sim
//---------------------------
//
PyDoc_STRVAR(SolidModel_write_geom_sim_doc,
" write_geom_sim(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_write_geom_sim(pySolidModel *self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fn;

  if (!PyArg_ParseTuple(args, api.format, &fn)) {
      return api.argsError();
  }

  auto geom = self->geom;
  if (geom->WriteGeomSim(fn) != SV_OK) {
      api.error("Error writing the solid model to the file '" + std::string(fn) + "'.");
      return nullptr;
  } 

  return SV_PYTHON_OK;
}

//-------------------------
// SolidModel_get_polydata
//-------------------------
//
PyDoc_STRVAR(SolidModel_get_polydata_doc,
" get_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_get_polydata(pySolidModel *self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|d", PyRunTimeErr, __func__);
  char *resultName;
  double max_dist = -1.0;

  if (!PyArg_ParseTuple(args, api.format, &resultName, &max_dist)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  int useMaxDist = 0;
  if (max_dist > 0) {
      useMaxDist = 1;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The repository object '" + std::string(resultName) + "' already exists.");
      return nullptr;
  }

  // Get the cvPolyData:
  auto pd = geom->GetPolyData(useMaxDist, max_dist);
  if (pd == NULL) {
      api.error("Could not get polydata for the solid model.");
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
      delete pd;
      api.error("Could not add the polydata to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//----------------------
// Solid_SetVtkPolyDataMtd
//----------------------
//
PyDoc_STRVAR(SolidModel_set_vtk_polydata_doc,
" set_vtk_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_set_vtk_polydata(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  auto obj = gRepository->GetObject(objName);
  if (obj == nullptr) {
      api.error("The solid model '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  // Do work of command:
  auto type = gRepository->GetType( objName );
  if (type != POLY_DATA_T) {
      api.error("The solid model '" + std::string(objName)+"' is not of type polydata.");
      return nullptr;
  }

  auto pd = ((cvPolyData *)obj)->GetVtkPolyData();
  if (!geom->SetVtkPolyDataObject(pd)) {
      api.error("Error setting vtk polydata.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}


//------------------------------
// SolidModel_get_face_polydata
//------------------------------
//
PyDoc_STRVAR(SolidModel_get_face_polydata_doc,
" set_vtk_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_face_polydata(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("si|d", PyRunTimeErr, __func__);
  char *resultName;
  int faceid;
  double max_dist = -1.0;

  if(!PyArg_ParseTuple(args,api.format,&resultName,&faceid,&max_dist)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  int useMaxDist = 0;
  if (max_dist > 0) {
      useMaxDist = 1;
  }

  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The solid model '" + std::string(resultName) + "' is already in the repository.");
      return nullptr;
  }

  // Get the cvPolyData:
  auto pd = geom->GetFacePolyData(faceid,useMaxDist,max_dist);
  if (pd == NULL) {
      api.error("Error getting polydata for the solid model face ID '" + std::to_string(faceid) + "'.");
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
    delete pd;
    api.error("Error adding the polydata '" + std::string(resultName) + "' to the repository.");
    return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------------------
// SolidModel_get_face_normal 
//---------------------------
//
PyDoc_STRVAR(SolidModel_get_face_normal_doc,
" get_face_normal(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_face_normal(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("idd", PyRunTimeErr, __func__);
  int faceid;
  double u,v;

  if (!PyArg_ParseTuple(args,api.format,&faceid,&u,&v)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  double normal[3];

  if (geom->GetFaceNormal(faceid,u,v,normal) == SV_ERROR ) {
      api.error("Error getting the face normal for the solid model face ID '" + std::to_string(faceid) + "'.");
      return nullptr;
  }

  return Py_BuildValue("ddd",normal[0],normal[1],normal[2]);
}


// ---------------------------
// Solid_GetDiscontinuitiesMtd
// ---------------------------

PyDoc_STRVAR(SolidModel_get_discontinuities_doc,
" get_discontinuities(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_discontinuities(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *resultName;

  if(!PyArg_ParseTuple(args,api.format,&resultName)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The solid model '" + std::string(resultName) + "' is already in the repository.");
      return nullptr;
  }

  // Get the discontinuities as polydata.
  auto pd = geom->GetDiscontinuities();
  if (pd == NULL) {
      api.error("Error getting discontinuities for the solid model.");
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
      delete pd;
      api.error("Error adding the discontinuities polydata '" + std::string(resultName) + "' to the repository.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------------------------------------
// SolidModel_get_axial_isoparametric_curve
//------------------------------------------
//
PyDoc_STRVAR(SolidModel_get_axial_isoparametric_curve_doc,
" get_axial_isoparametric_curve(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static pySolidModel * 
SolidModel_get_axial_isoparametric_curve(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sd", PyRunTimeErr, __func__);
  char *resultName;
  double prm;

  if (!PyArg_ParseTuple(args, api.format, &resultName, &prm)) {
      api.argsError();
      return nullptr;
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The solid model '" + std::string(resultName) + "' is already in the repository.");
      return nullptr;
  }

  // Get the isoparametric curve on the given surface at the given
  // parameter value.
  if ((prm < 0.0) || (prm > 1.0)) {
      api.error("The curve parameter argument must be between 0.0 and 1.0.");
      return nullptr;
  }

  auto curve = geom->GetAxialIsoparametricCurve( prm );
  if (curve == NULL) {
      api.error("Error getting the isoparametric curve for the solid model.");
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, curve)) {
      delete curve;
      api.error("Error adding the isoparametric curve '" + std::string(resultName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(curve);
  pySolidModel* newCurve;
  newCurve = createSolidModelType();
  newCurve->geom=curve;
  Py_DECREF(curve);
  return newCurve;
}

//-----------------------
// SolidModel_get_kernel 
//-----------------------
//
PyDoc_STRVAR(SolidModel_get_kernel_doc,
" get_kernel(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_kernel(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  auto kernelType = geom->GetKernelT();
  if (kernelType == SM_KT_INVALID) {
      api.error("The solid model kernel is not set.");
      return nullptr;
  }
    
  auto kernelName = SolidModel_KernelT_EnumToStr( kernelType );
  return Py_BuildValue("s",kernelName);
}

//---------------------------
// SolidModel_get_label_keys 
//---------------------------
//
PyDoc_STRVAR(SolidModel_get_label_keys_doc,
" get_label_keys(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_label_keys(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  int numKeys;
  char **keys;

  geom->GetLabelKeys(&numKeys, &keys);
  PyObject* keyList = PyList_New(numKeys);

  for (int i = 0; i < numKeys; i++) {
    PyList_SetItem(keyList, i, PyString_FromString(keys[i]));
  }

  delete [] keys;

  return keyList;
}

//----------------------
// SolidModel_get_label
//----------------------
//
PyDoc_STRVAR(SolidModel_get_label_doc,
" get_label()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
SolidModel_get_label(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *key; 

  if (!PyArg_ParseTuple(args,api.format,&key)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  char *value;

  if (!geom->GetLabel(key, &value)) {
      api.error("The solid model key '" + std::string(key) + "' was not found.");
      return nullptr;
  }

  return Py_BuildValue("s",value);
}

//----------------------
// SolidModel_set_label 
//----------------------
//
PyDoc_STRVAR(SolidModel_set_label_doc,
" set_label(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_set_label(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *key, *value;

  if (!PyArg_ParseTuple(args,api.format,&key,&value)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (!geom->SetLabel(key, value)) {
      if (geom->IsLabelPresent(key)) {
          api.error("The solid model key '" + std::string(key) + "' is already being used.");
          return nullptr;
      } else {
          PyErr_SetString(PyRunTimeErr, "error setting label" );
          api.error("Error setting the solid model key '" + std::string(key) + ".");
          return nullptr;
      }
  }

  return SV_PYTHON_OK;
}

//------------------------
// SolidModel_clear_label 
//------------------------
//
PyDoc_STRVAR(SolidModel_clear_label_doc,
" clear_label(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_clear_label(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *key;

  if (!PyArg_ParseTuple(args,api.format,&key)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (!geom->IsLabelPresent(key)) {
      api.error("The solid model key '" + std::string(key) + "' is not defined.");
      return nullptr;
  }

  geom->ClearLabel(key);
  return SV_PYTHON_OK;
}

//-------------------------
// SolidModel_get_face_ids 
//-------------------------
//
PyDoc_STRVAR(SolidModel_get_face_ids_doc,
" get_face_ids_doc()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
SolidModel_get_face_ids(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  int numFaces;
  int *faces;

  if (geom->GetFaceIds(&numFaces, &faces) != SV_OK) {
      api.error("Error getting the face IDs for the solid model."); 
      return nullptr;
  }

  if (numFaces == 0) {
      Py_INCREF(Py_None);
      return Py_None;
  }

  auto faceList = PyList_New(numFaces);

  for (int i = 0; i < numFaces; i++) {
      auto faceID = std::to_string(faces[i]);
      PyList_SetItem(faceList, i, PyString_FromFormat(faceID.c_str()));
  }

  delete faces;
  return faceList;
}

//-------------------------------
// SolidModel_get_boundary_faces 
//-------------------------------
//
// [TODO:DaveP] This function does not 'get' anything.
//   Should this be renamed to 'extract_boundary_faces' ?
//
PyDoc_STRVAR(SolidModel_get_boundary_faces_doc,
" get_boundary_faces()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_get_boundary_faces(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__);
  double angle = 0.0;

  if (!PyArg_ParseTuple(args,api.format,&angle)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (geom->GetBoundaryFaces(angle) != SV_OK ) {
    api.error("Error getting boundary faces for the solid model using angle '" + std::to_string(angle) + ".");
    return nullptr;
  }

  return SV_PYTHON_OK;
}

//---------------------------
// SolidModel_get_region_ids 
//---------------------------
//
PyDoc_STRVAR(SolidModel_get_region_ids_doc,
" get_region_ids()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
SolidModel_get_region_ids(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  int numRegions, *regions;
  if (geom->GetRegionIds(&numRegions, &regions) != SV_OK) {
      api.error("Error getting regions IDs for the solid model.");
      return nullptr;
  }

  if (numRegions == 0) {
      Py_INCREF(Py_None);
      return Py_None;
  }

  // Create the list of region IDs.
  auto regionList = PyList_New(numRegions);
  for (int i = 0; i < numRegions; i++) {
      auto regionID = std::to_string(regions[i]);
      PyList_SetItem(regionList, i, PyString_FromFormat(regionID.c_str()));
  }

  delete regions;
  return regionList;
}

// --------------------
// Solid_GetFaceAttrMtd
// --------------------

PyDoc_STRVAR(SolidModel_get_face_attribute_doc,
" get_face_attribute(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_face_attribute( pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("si", PyRunTimeErr, __func__);
  char *key;
  int faceid;

  if (!PyArg_ParseTuple(args,api.format, &key, &faceid)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  char *value;
  if (!geom->GetFaceAttribute(key, faceid, &value)) {
      api.error("The solid model attribute was not found: key='" + std::string(key) +
        "  faceID='" + std::to_string(faceid) + "' .");
      return nullptr;
  }

  return Py_BuildValue("s",value);
}

//-------------------------------
// SolidModel_set_face_attribute 
//-------------------------------
//
// [TODO:DaveP] Should the args be "key, faceID, value" ?
//
PyDoc_STRVAR(SolidModel_set_face_attribute_doc,
" set_face_attribute(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_set_face_attribute(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssi", PyRunTimeErr, __func__);
  char *key, *value;
  int faceid;

  if (!PyArg_ParseTuple(args, api.format, &key, &value, &faceid)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (!geom->SetFaceAttribute(key, faceid, value) ) {
      api.error("Error setting the solid model attribute: key='" + std::string(key) +
        "  faceID='" + std::to_string(faceid) + "' .");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//---------------------------------
// SolidModel_get_region_attribute 
//---------------------------------
//
PyDoc_STRVAR(SolidModel_get_region_attribute_doc,
" get_region_attribute(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_region_attribute(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("si", PyRunTimeErr, __func__);
  char *key;
  int regionid;

  if (!PyArg_ParseTuple(args, api.format, &key, &regionid)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  char *value;
  if (!geom->GetRegionAttribute(key, regionid, &value)) {
      api.error("The solid model region attribute was not found: key='" + std::string(key) +
        "  RegionID='" + std::to_string(regionid) + "' .");
      return nullptr;
  }

  return Py_BuildValue("s",value);
}

//----------------------------------
// SolidModel_set_region_attribute 
//----------------------------------
//
PyDoc_STRVAR(SolidModel_set_region_attribute_doc,
" set_region_attribute(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_set_region_attribute(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssi", PyRunTimeErr, __func__);
  char *key, *value;
  int regionid;

  if (!PyArg_ParseTuple(args, api.format, &key, &value, &regionid)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (!geom->SetRegionAttribute(key, regionid, value)) {
      api.error("Error setting the solid model attribute: key='" + std::string(key) +
        "  regionID='" + std::to_string(regionid) + "' .");
      return nullptr;
  }
    
  return SV_PYTHON_OK;
}

//--------------------
// Solid_DeleteFacesMtd
//--------------------
//
PyDoc_STRVAR(SolidModel_delete_faces_doc,
" delete_faces(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_delete_faces(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__);
  PyObject* faceList;

  if (!PyArg_ParseTuple(args, api.format, &faceList)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (PyList_Size(faceList) == 0) {
      return SV_PYTHON_OK;
  }

  // [TODO:DaveP] We should check that the face IDs are
  // valid.

  // Create list of faces to delete.
  int nfaces = 0;
  int *faces = new int[PyList_Size(faceList)];
  for (int i = 0; i < PyList_Size(faceList); i++) {
      faces[i] = PyLong_AsLong(PyList_GetItem(faceList,i));
  }

  if (geom->DeleteFaces(nfaces, faces) != SV_OK) {
      api.error("Error deleting faces for the solid model."); 
      delete [] faces;
  }

  delete [] faces;
  return SV_PYTHON_OK;
}

//--------------------------
// SolidModel_delete_region 
//--------------------------
//
PyDoc_STRVAR(SolidModel_delete_region_doc,
" delete_region(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_delete_region(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int regionid;

  if (!PyArg_ParseTuple(args, api.format, &regionid)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (geom->DeleteRegion(regionid) != SV_OK) {
      api.error("Error deleting the solid model region: regionID ='" + std::to_string(regionid)+"."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------------------
// Solid_CreateEdgeBlendMtd
//------------------------
//
PyDoc_STRVAR(SolidModel_create_edge_blend_doc,
" create_edge_blend(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_create_edge_blend(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("iid|i", PyRunTimeErr, __func__);
  int faceA;
  int faceB;
  double radius;
  int filletshape = 0;

  if (!PyArg_ParseTuple(args, api.format, &faceA, &faceB, &radius, &filletshape)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (geom->CreateEdgeBlend(faceA, faceB, radius, filletshape) != SV_OK) {
      api.error("Error creating edge blend for the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------------
// SolidModel_combine_faces
//--------------------------
//
PyDoc_STRVAR(SolidModel_combine_faces_doc,
" combine_faces(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_combine_faces(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ii", PyRunTimeErr, __func__);
  int faceid1;
  int faceid2;

  if (!PyArg_ParseTuple(args, api.format, &faceid1, &faceid2)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (geom->CombineFaces(faceid1, faceid2) != SV_OK) {
    PyErr_SetString(PyRunTimeErr, "Combine Faces: Error");
      api.error("Error combining faces for the solid model: faceID1="+std::to_string(faceid1)+
        " faceID2="+std::to_string(faceid2)+".");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------------------
// SolidModel_remesh_face
//------------------------
//
PyDoc_STRVAR(SolidModel_remesh_face_doc,
" remesh_face(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_remesh_face(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Od", PyRunTimeErr, __func__);
  double size;
  PyObject* excludeList;

  if (!PyArg_ParseTuple(args, api.format, &excludeList, &size)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  if (PyList_Size(excludeList) == 0) {
      return SV_PYTHON_OK;
  }

  // Create list of face IDs.
  int nfaces = 0;
  int *faces = new int[PyList_Size(excludeList)];
  for (int i=0;i<PyList_Size(excludeList);i++) {
    faces[i] = PyLong_AsLong(PyList_GetItem(excludeList,i));
  }

  auto status = geom->RemeshFace(nfaces, faces, size);
  delete [] faces;

  if (status != SV_OK) {
      api.error("Error remeshing face for the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

//------------------------------------------------
// Define API function names for SolidModel class 
//------------------------------------------------

static PyMethodDef pySolidModel_methods[] = {

  { "apply4x4",
      (PyCFunction)SolidModel_apply4x4,
      METH_VARARGS,
      SolidModel_apply4x4_doc
  },

  { "box2d", 
      (PyCFunction)SolidModel_box2d,
      METH_VARARGS,
      SolidModel_box2d_doc
  },

  { "box3d", 
      (PyCFunction)SolidModel_box3d,
      METH_VARARGS,
      SolidModel_box3d_doc
  },

  { "cap_surface_to_solid", 
      (PyCFunction)SolidModel_cap_surface_to_solid,
      METH_VARARGS,     
      SolidModel_cap_surface_to_solid_doc
  },

  { "check", 
      (PyCFunction)SolidModel_check,
      METH_VARARGS,
      SolidModel_check_doc
  },

  { "circle", 
      (PyCFunction)SolidModel_circle,
      METH_VARARGS,
      SolidModel_circle_doc
  },

  { "classify_point",
      (PyCFunction)SolidModel_classify_point,
      METH_VARARGS,   
      SolidModel_classify_point_doc
  },

  { "clear_label", 
      (PyCFunction)SolidModel_clear_label,
      METH_VARARGS,
      SolidModel_clear_label_doc
  },

  { "combine_faces",
      (PyCFunction)SolidModel_combine_faces,
      METH_VARARGS,
      SolidModel_combine_faces_doc
  },

  { "copy", 
      (PyCFunction)SolidModel_copy,
      METH_VARARGS,
      SolidModel_copy_doc
  },

  { "create_edge_blend",
      (PyCFunction)SolidModel_create_edge_blend,
      METH_VARARGS,
      SolidModel_create_edge_blend_doc
  },

  { "cylinder", 
      (PyCFunction)SolidModel_cylinder,
      METH_VARARGS,
      SolidModel_cylinder_doc
  },

  { "delete_faces",
      (PyCFunction)SolidModel_delete_faces,
      METH_VARARGS,
      SolidModel_delete_faces_doc
  },

  { "delete_region",
      (PyCFunction)SolidModel_delete_region,
      METH_VARARGS,
      SolidModel_delete_region_doc
  },

  { "distance",
      (PyCFunction)SolidModel_distance,
      METH_VARARGS,
      SolidModel_distance_doc
   },

  { "ellipse", 
      (PyCFunction)SolidModel_ellipse,
      METH_VARARGS,
      SolidModel_ellipse_doc
  },

  { "ellipsoid", 
      (PyCFunction)SolidModel_ellipsoid,
      METH_VARARGS,
      SolidModel_ellipsoid_doc
  },

  { "extrude", 
      (PyCFunction)SolidModel_extrude,
      METH_VARARGS,
      SolidModel_extrude_doc
  },

  { "extrude_z", 
      (PyCFunction)SolidModel_extrude_z,
      METH_VARARGS,
      SolidModel_extrude_z_doc
  },

  { "find_centroid",
      (PyCFunction)SolidModel_find_centroid,
      METH_VARARGS,
      SolidModel_find_centroid_doc
   },

  { "find_extent", 
      (PyCFunction)SolidModel_find_extent,
      METH_VARARGS,
      SolidModel_find_extent_doc
   },

  { "get_axial_isoparametric_curve",
      (PyCFunction)SolidModel_get_axial_isoparametric_curve,
      METH_VARARGS,
      SolidModel_get_axial_isoparametric_curve_doc
  },

  { "get_boundary_faces",
      (PyCFunction)SolidModel_get_boundary_faces,
      METH_VARARGS,     
      SolidModel_get_boundary_faces_doc
  },

  { "get_class_name", 
      (PyCFunction)SolidModel_get_class_name,
      METH_NOARGS,
      SolidModel_get_class_name_doc
  },

  { "get_discontinuities",
      (PyCFunction)SolidModel_get_discontinuities,
      METH_VARARGS,
      SolidModel_get_discontinuities_doc
  },

  { "get_face_attribute",
      (PyCFunction)SolidModel_get_face_attribute,
      METH_VARARGS,
      SolidModel_get_face_attribute_doc
  },

  { "get_face_ids", 
      (PyCFunction)SolidModel_get_face_ids,
      METH_NOARGS,
      SolidModel_get_face_ids_doc
  },

  { "get_face_normal",
      (PyCFunction)SolidModel_get_face_normal,
      METH_VARARGS,     
      SolidModel_get_face_normal_doc
  },

  { "get_face_polydata",
      (PyCFunction)SolidModel_get_face_polydata,
      METH_VARARGS,
      SolidModel_get_face_polydata_doc
  },

  { "get_kernel",
      (PyCFunction)SolidModel_get_kernel,
      METH_VARARGS,
      SolidModel_get_kernel_doc
  },

  { "get_label", 
      (PyCFunction)SolidModel_get_label,
      METH_VARARGS,
      SolidModel_get_label_doc
  },

  { "get_label_keys",
      (PyCFunction)SolidModel_get_label_keys,
      METH_VARARGS,
      SolidModel_get_label_keys_doc
  },

  { "get_model", 
      (PyCFunction)SolidModel_get_model, 
      METH_VARARGS, 
      SolidModel_get_model_doc
  },

  { "get_polydata",
      (PyCFunction)SolidModel_get_polydata,
      METH_VARARGS,
      SolidModel_get_polydata_doc
  },

  { "get_region_attribute",
      (PyCFunction)SolidModel_get_region_attribute,
      METH_VARARGS,
      SolidModel_get_region_attribute_doc
  },

  { "get_region_ids",
      (PyCFunction)SolidModel_get_region_ids,
      METH_VARARGS,     
      SolidModel_get_region_ids_doc
  },

  { "get_spatial_dimension",
      (PyCFunction)SolidModel_get_spatial_dimension,
      METH_VARARGS,
      SolidModel_get_spatial_dimension_doc
  },

  { "get_topological_dimension",
      (PyCFunction)SolidModel_get_topological_dimension,
      METH_VARARGS,     
      SolidModel_get_topological_dimension_doc
   },

  { "intersect", 
      (PyCFunction)SolidModel_intersect,
      METH_VARARGS,
      SolidModel_intersect_doc
  },

  { "make_approximate_curve_loop",
      (PyCFunction)SolidModel_make_approximate_curve_loop,
      METH_VARARGS,
      SolidModel_make_approximate_curve_loop_doc
  },

  { "make_interpolated_curve_loop",
      (PyCFunction)SolidModel_make_interpolated_curve_loop,
      METH_VARARGS,
      SolidModel_make_interpolated_curve_loop_doc
  },

  { "make_lofted_surface", 
      (PyCFunction)SolidModel_make_lofted_surface,
      METH_VARARGS,
      SolidModel_make_lofted_surface_doc
  },

  { "new_object", 
      (PyCFunction)SolidModel_new_object,
      METH_VARARGS,
      SolidModel_new_object_doc
  },

  { "polygon",
      (PyCFunction)SolidModel_polygon,
      METH_VARARGS,
      SolidModel_polygon_doc
  },

  { "poly3d_solid", 
      (PyCFunction)SolidModel_poly3d_solid,
      METH_VARARGS,
      SolidModel_poly3d_solid_doc
  },

  { "poly3d_surface", 
      (PyCFunction)SolidModel_poly3d_surface,
      METH_VARARGS,
      SolidModel_poly3d_surface_doc
  },

  { "polygon_points", 
      (PyCFunction)SolidModel_polygon_points,
      METH_VARARGS,
      SolidModel_polygon_points_doc
  },

  { "print", 
      (PyCFunction)SolidModel_print,
      METH_VARARGS,
      SolidModel_print_doc
   },

  { "read_native", 
      (PyCFunction)SolidModel_read_native,
      METH_VARARGS,
      SolidModel_read_native_doc
  },

  { "reflect",
      (PyCFunction)SolidModel_reflect,
      METH_VARARGS,
      SolidModel_reflect_doc
  },

  { "remesh_face",
      (PyCFunction)SolidModel_remesh_face,
      METH_VARARGS,
      SolidModel_remesh_face_doc
  },

  { "rotate",
      (PyCFunction)SolidModel_rotate,
      METH_VARARGS,
      SolidModel_rotate_doc
  },

  { "scale",
      (PyCFunction)SolidModel_scale,
      METH_VARARGS,
      SolidModel_scale_doc
  },

  { "set_face_attribute",
      (PyCFunction)SolidModel_set_face_attribute,
      METH_VARARGS,
      SolidModel_set_face_attribute_doc
  },

  { "set_label",
      (PyCFunction)SolidModel_set_label,
      METH_VARARGS,
      SolidModel_set_label_doc
  },

  { "set_region_attribute", 
      (PyCFunction)SolidModel_set_region_attribute,
      METH_VARARGS,
      SolidModel_set_region_attribute_doc
  },

  { "set_vtk_polydata",
      (PyCFunction)SolidModel_set_vtk_polydata,
      METH_VARARGS,
      SolidModel_set_vtk_polydata_doc
  },

  { "sphere", 
      (PyCFunction)SolidModel_sphere,
      METH_VARARGS,
      SolidModel_sphere_doc
  },

  { "subtract", 
      (PyCFunction)SolidModel_subtract,
      METH_VARARGS,
      SolidModel_subtract_doc
  },

  { "torus", 
      (PyCFunction)SolidModel_torus,
      METH_VARARGS,
      SolidModel_torus_doc
  },

  { "translate", 
      (PyCFunction)SolidModel_translate,
      METH_VARARGS,
      SolidModel_translate_doc
  },

  { "truncated_cone", 
      (PyCFunction)SolidModel_truncated_cone,
      METH_VARARGS,
      SolidModel_truncated_cone_doc
  },

  { "union", 
      (PyCFunction)SolidModel_union,
      METH_VARARGS,
      SolidModel_union_doc
  },

  { "write_geom_sim", 
      (PyCFunction)SolidModel_write_geom_sim,
      METH_VARARGS,
      SolidModel_write_geom_sim_doc
   },

  { "write_native",
      (PyCFunction)SolidModel_write_native,
      METH_VARARGS,
      SolidModel_write_native_doc
   },

  { "write_vtk_polydata",
      (PyCFunction)SolidModel_write_vtk_polydata,
      METH_VARARGS,
      SolidModel_write_vtk_polydata_doc
   },

  {NULL,NULL}
};

//-------------------
// pySolidModel_init
//-------------------
// This is the __init__() method for the SolidModel class. 
//
// This function is used to initialize an object after it is created.
//
static int 
pySolidModel_init(pySolidModel* self, PyObject* args)
{
  fprintf(stdout,"pySolid Model tp_init called\n");
  return SV_OK;
}

//------------------
// pySolidModelType 
//------------------
// This is the definition of the SolidModel class.
//
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject pySolidModelType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "solid.SolidModel",        /* tp_name */
  sizeof(pySolidModel),      /* tp_basicsize */
  0,                         /* tp_itemsize */
  0,                         /* tp_dealloc */
  0,                         /* tp_print */
  0,                         /* tp_getattr */
  0,                         /* tp_setattr */
  0,                         /* tp_compare */
  0,                         /* tp_repr */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash */
  0,                         /* tp_call */
  0,                         /* tp_str */
  0,                         /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
  "SolidModel  objects",           /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  pySolidModel_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)pySolidModel_init,                            /* tp_init */
  0,                         /* tp_alloc */
  0,                  /* tp_new */
};


//----------------------
// createSolidModelType 
//----------------------
pySolidModel * createSolidModelType()
{
  return PyObject_New(pySolidModel, &pySolidModelType);
}


//------------------
// pySolid_methods
//------------------
// Methods for the 'solid' module.
//
static PyMethodDef pySolid_methods[] = {

  {"list_registrars", 
      (PyCFunction)Solid_list_registrars,
      METH_NOARGS,
      Solid_list_registrars_doc
  },

  { "set_kernel", 
      (PyCFunction)Solid_set_kernel,
      METH_VARARGS, 
      NULL
  },

  { "get_kernel", 
      (PyCFunction)Solid_get_kernel,
      METH_NOARGS,
      Solid_get_kernel_doc
  },

  {NULL, NULL}
};

static PyTypeObject pycvFactoryRegistrarType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "solid.pycvFactoryRegistrar",             /* tp_name */
  sizeof(pycvFactoryRegistrar),             /* tp_basicsize */
  0,                         /* tp_itemsize */
  0,                         /* tp_dealloc */
  0,                         /* tp_print */
  0,                         /* tp_getattr */
  0,                         /* tp_setattr */
  0,                         /* tp_compare */
  0,                         /* tp_repr */
  0,                         /* tp_as_number */
  0,                         /* tp_as_sequence */
  0,                         /* tp_as_mapping */
  0,                         /* tp_hash */
  0,                         /* tp_call */
  0,                         /* tp_str */
  0,                         /* tp_getattro */
  0,                         /* tp_setattro */
  0,                         /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
  "cvFactoryRegistrar wrapper  ",           /* tp_doc */
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python interpreter 
// when the module is loaded.

static char* MODULE_NAME = "solid";
static char* SOLID_MODEL_CLASS_NAME = "SolidModel";
static char* MODULE_EXCEPTION_NAME = "solid.SolidModelException";
static char* SOLID_MODEL_EXCEPTION_NAME = "SolidModelException";

PyDoc_STRVAR(Solid_module_doc, "solid module functions");

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3

PyMODINIT_FUNC PyInit_pySolid(void);

int Solid_pyInit()
{ 
  PyInit_pySolid();
  return SV_OK;
}

// Size of per-interpreter state of the module.
// Set to -1 if the module keeps state in global variables. 
static int perInterpreterStateSize = -1;

// Always initialize this to PyModuleDef_HEAD_INIT.
static PyModuleDef_Base m_base = PyModuleDef_HEAD_INIT;

// Define the module definition struct which holds all information 
// needed to create a module object. 

static struct PyModuleDef pySolidmodule = {
   m_base,
   MODULE_NAME, 
   Solid_module_doc, 
   perInterpreterStateSize, 
   pySolid_methods
};

//-----------------
// PyInit_pySolid  
//-----------------
// The initialization function called by the Python interpreter 
// when the 'solid' module is loaded.
//
PyMODINIT_FUNC
PyInit_pySolid(void)
{
  if (gRepository == NULL) {
      gRepository=new cvRepository();
      fprintf(stdout,"New gRepository created from cv_solid_init\n");
  }

  // Set the default modeling kernel. 
  cvSolidModel::gCurrentKernel = SM_KT_INVALID;
  #ifdef SV_USE_PARASOLID
  cvSolidModel::gCurrentKernel = SM_KT_PARASOLID;
  #endif

  // Create the SolidModeling class definition.
  pySolidModelType.tp_new = PyType_GenericNew;
  pycvFactoryRegistrarType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&pySolidModelType) < 0) {
    fprintf(stdout,"Error in pySolidModelType");
    return SV_PYTHON_ERROR;
  }

  if (PyType_Ready(&pycvFactoryRegistrarType) < 0) {
    fprintf(stdout,"Error in pySolidModelType");
    return SV_PYTHON_ERROR;
  }

  // Create the 'solid' module. 
  auto module = PyModule_Create(&pySolidmodule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing pySolid");
    return SV_PYTHON_ERROR;
  }

  // Add solid.SolidModelException exception.
  //
  PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION_NAME, NULL, NULL);
  PyModule_AddObject(module, SOLID_MODEL_EXCEPTION_NAME, PyRunTimeErr);

  // Add the 'SolidModel' class.
  Py_INCREF(&pySolidModelType);
  PyModule_AddObject(module, SOLID_MODEL_CLASS_NAME, (PyObject *)&pySolidModelType);

  Py_INCREF(&pycvFactoryRegistrarType);
  PyModule_AddObject(module, "pyCvFactoryRegistrar", (PyObject *)&pycvFactoryRegistrarType);

  // [TODO:DaveP] What does this do?
  pycvFactoryRegistrar* tmp = PyObject_New(pycvFactoryRegistrar, &pycvFactoryRegistrarType);
  tmp->registrar = (cvFactoryRegistrar *)&cvSolidModel::gRegistrar;
  PySys_SetObject("solidModelRegistrar", (PyObject *)tmp);

  return module;
}

#endif


//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC initpySolid(void);
int Solid_pyInit()
{ 
  initpySolid();
  return SV_OK;
}

PyMODINIT_FUNC
initpySolid(void)
{
    // Initialize-gRepository
  if (gRepository ==NULL)
  {
    gRepository=new cvRepository();
    fprintf(stdout,"New gRepository created from cv_solid_init\n");
  }
  //Initialize-gCurrentKernel
  cvSolidModel::gCurrentKernel = SM_KT_INVALID;
  #ifdef SV_USE_PARASOLID
  cvSolidModel::gCurrentKernel = SM_KT_PARASOLID;
  #endif

  pySolidModelType.tp_new=PyType_GenericNew;
  pycvFactoryRegistrarType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&pySolidModelType)<0)
  {
    fprintf(stdout,"Error in pySolidModelType");
    return;
  }
  if (PyType_Ready(&pycvFactoryRegistrarType)<0)
  {
    fprintf(stdout,"Error in pySolidModelType");
    return;
  }
  //Init our defined functions
  PyObject *pythonC;
  pythonC = Py_InitModule("pySolid", pySolid_methods);
  if (pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pySolid");
    return;
  }

  PyRunTimeErr=PyErr_NewException("pySolid.error",NULL,NULL);
  PyModule_AddObject(pythonC, "error",PyRunTimeErr);
  Py_INCREF(&pySolidModelType);
  Py_INCREF(&pycvFactoryRegistrarType);
  PyModule_AddObject(pythonC, "pySolidModel", (PyObject *)&pySolidModelType);
  PyModule_AddObject(pythonC, "pyCvFactoryRegistrar", (PyObject *)&pycvFactoryRegistrarType);

  pycvFactoryRegistrar* tmp = PyObject_New(pycvFactoryRegistrar, &pycvFactoryRegistrarType);
  tmp->registrar = (cvFactoryRegistrar *)&cvSolidModel::gRegistrar;
  PySys_SetObject("solidModelRegistrar", (PyObject *)tmp);


}
#endif

