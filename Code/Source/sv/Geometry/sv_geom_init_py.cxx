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

// The functions defined here implement the SV Python API 'geometry' module. 
//
// The module name is 'geometry'. 
//
// A Python exception sv.geometry.GeometryException is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.geometry.GeometryException:
//
// [TODO:DaveP] A lot of these functions create a cvPolyData object but
// don't delete it
// 
//    cvPolyData* dst;
//    sys_geom_set_array_for_local_op_sphere(src, &dst, radius, ctr, outArray, dataType); 
//
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
#include "sv_PolyData.h"
#include "sv_geom_init_py.h"
#include "sv_sys_geom.h"
#include "sv_SolidModel.h"
#include "sv_solid_init_py.h"
#include "sv_integrate_surface.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"
#include "sv_vtk_utils.h"
#include "vtkSmartPointer.h"
#include "sv_PyUtils.h"
#include "sv2_globals.h"

// Needed for Windows
#ifdef GetObject
#undef GetObject
#endif

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

//////////////////////////////////////////////////////
//        U t i l i t y     F u n c t i o n s       //
//////////////////////////////////////////////////////

//-----------------------
// GetRepositoryGeometry
//-----------------------
// Get a geometry from the repository and check that 
// its type is POLY_DATA_T.
//
static cvPolyData *
GetRepositoryGeometry(SvPyUtilApiFunction& api, char* name)
{ 
  auto geom = gRepository->GetObject(name);
  if (geom == NULL) {
      api.error("The geometry '"+std::string(name)+"' is not in the repository.");
      return nullptr;
  }
  auto type = gRepository->GetType(name);
  if (type != POLY_DATA_T) {
      api.error("'" + std::string(name) + "' is not polydata.");
      return nullptr;
  }

  return (cvPolyData*)geom;
}

//--------------------------
// RepositoryGeometryExists 
//--------------------------
//
static bool
RepositoryGeometryExists(SvPyUtilApiFunction& api, char* name)
{
  if (gRepository->Exists(name)) {
      api.error("The repository object '" + std::string(name) + "' already exists.");
      return true;
  }

  return false;
}

//-------------------------
// AddGeometryToRepository
//-------------------------
// Add a geometry to the repository.
//
static bool
AddGeometryToRepository(SvPyUtilApiFunction& api, char *name, cvPolyData *geom)
{
  if (!gRepository->Register(name, geom)) {
    delete geom;
    api.error("Error adding the geometry '" + std::string(name) + "' to the repository.");
    return false;
  }

  return true;
}

//--------------------
// GetGeometryObjects
//--------------------
// Get a list of geometry objects from a list of names.
//
static bool
GetGeometryObjects(SvPyUtilApiFunction& api, PyObject* geometryNames, std::vector<cvPolyData*>& geometryObjects) 
{
  if (!PyList_Check(geometryNames)) {
      api.error("The source geometries argument is not a Python list.");
      return false;
  }

  auto numSrcs = PyList_Size(geometryNames);
  if (numSrcs == 0) { 
      api.error("The source geometries argument list is empty.");
      return false;
  }

  geometryObjects.clear();

  for (int i = 0; i < numSrcs; i++ ) {
    #if PYTHON_MAJOR_VERSION == 2
    auto str = PyString_AsString(PyList_GetItem(geometryNames,i));
    #endif
    #if PYTHON_MAJOR_VERSION ==3
    auto str = PyBytes_AsString(PyUnicode_AsUTF8String(PyList_GetItem(geometryNames,i)));
    #endif
    auto src = GetRepositoryGeometry(api, str);
    if (src == NULL) {
      return nullptr;
    }
    geometryObjects.push_back((cvPolyData *)src);
  }

  return true;
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//-------------
// Geom_reduce
//-------------
//
PyDoc_STRVAR(Geom_reduce_doc,
  "reduce(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_reduce(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssd", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  double tol;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName, &tol)) {
      return api.argsError();
  }

  // Retrieve source object.
  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  // Check that the repository dstName object does not already exist.
  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_Reduce(src, tol, &dst) != SV_OK) {
      api.error("Error merging points for geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//------------
// Geom_union
//------------
//
PyDoc_STRVAR(Geom_union_doc,
  "union(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_union(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss|d", PyRunTimeErr, __func__);
  char *aName;
  char *bName;
  char *dstName;
  double tolerance = 1e-6;

  if (!PyArg_ParseTuple(args, api.format, &aName, &bName, &dstName, &tolerance)) {
      return api.argsError();
  }

  // Retrieve operands geometry.
  auto srcA = GetRepositoryGeometry(api, aName);
  if (srcA == nullptr ) {
      return nullptr;
  }
  auto srcB = GetRepositoryGeometry(api, bName);
  if (srcB == nullptr ) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_union(srcA, srcB, tolerance, &dst) != SV_OK) {
      api.error("Error performing a union operation of geometry '" + std::string(aName) + " with '" +  std::string(bName)+ ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s", dst->GetName());
}

//----------------
// Geom_intersect
//----------------
//
PyDoc_STRVAR(Geom_intersect_doc,
  "intersect(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_intersect(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss|d", PyRunTimeErr, __func__);
  char *aName;
  char *bName;
  char *dstName;
  double tolerance = 1e-6;

  if (!PyArg_ParseTuple(args, api.format, &aName, &bName, &dstName, &tolerance)) {
      return api.argsError();
  }

  // Retrieve operands geometry.
  auto srcA = GetRepositoryGeometry(api, aName);
  if (srcA == nullptr ) {
      return nullptr;
  }
  auto srcB = GetRepositoryGeometry(api, bName);
  if (srcB == nullptr ) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_intersect(srcA, srcB, tolerance, &dst) != SV_OK) {
      api.error("Error performing a Boolean intersection of geometry '" + std::string(aName) + " with '" +  std::string(bName)+ ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//---------------
// Geom_subtract 
//---------------
//
PyDoc_STRVAR(Geom_subtract_doc,
  "subtract(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_subtract(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss|d", PyRunTimeErr, __func__);
  char *aName;
  char *bName;
  char *dstName;
  double tolerance = 1e-6;

  if (!PyArg_ParseTuple(args, api.format, &aName, &bName, &dstName, &tolerance)) {
      return api.argsError();
  }

  // Retrieve operands geometry.
  auto srcA = GetRepositoryGeometry(api, aName);
  if (srcA == nullptr ) {
      return nullptr;
  }
  auto srcB = GetRepositoryGeometry(api, bName);
  if (srcB == nullptr ) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_subtract(srcA, srcB, tolerance, &dst) != SV_OK) {
      api.error("Error performing a Boolean subtract of geometry '" + std::string(aName) + " with '" +  std::string(bName)+ ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//--------------------
// Geom_check_surface  
//--------------------
//
PyDoc_STRVAR(Geom_check_surface_doc,
  "check_surface(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject* 
Geom_check_surface(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|d", PyRunTimeErr, __func__);
  char *srcName;
  double tol = 1e-6;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &tol)) {
      return api.argsError();
  }

  // Retrieve source object.
  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  int stats[2];
  if (sys_geom_checksurface(src, stats, tol) != SV_OK) {
    api.error("Error checking surface for geometry '" + std::string(srcName) + ".");
    return nullptr;
  }

  return Py_BuildValue("ii",stats[0],stats[1]);
}

//------------
// Geom_clean
//------------
//
PyDoc_STRVAR(Geom_clean_doc,
  "check_surface(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_clean(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName)) {
      return api.argsError();
  }

  // Retrieve source object.
  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  auto dst = sys_geom_Clean(src);
  if (dst == NULL) {
    api.error("Error cleaning geometry '" + std::string(srcName) + ".");
    return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------------
// Geom_set_ids_for_caps 
//-----------------------
//
PyDoc_STRVAR(Geom_set_ids_for_caps_doc,
  "set_ids_for_caps(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_set_ids_for_caps(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName)) {
      return api.argsError();
  }

  // Retrieve source object.
  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  int *doublecaps;
  int numfaces=0;
  cvPolyData *dst;

  if (sys_geom_set_ids_for_caps(src, &dst, &doublecaps, &numfaces) != SV_OK) {
      api.error("Error setting cap IDs for geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      delete [] doublecaps;
      return nullptr;
  }

  PyObject* pylist = PyList_New(numfaces);
  for (int i=0; i<numfaces; i++){
      PyList_SetItem(pylist, i, PyLong_FromLong(doublecaps[i]));
  }
  delete [] doublecaps;
  return pylist;
}

//----------------------------------
// Geom_set_array_for_local_op_face 
//----------------------------------
//
PyDoc_STRVAR(Geom_set_array_for_local_op_face_doc,
  "set_array_for_local_op_face(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_set_array_for_local_op_face(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssO|si", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  char *arrayName = 0;
  PyObject* values;
  char *outArray = "LocalOpsArray";
  int dataType = 1;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&arrayName,&values,&outArray,&dataType)) {
      return api.argsError();
  }

  // Retrieve source object.
  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  int nvals = PyList_Size(values);
  if (nvals == 0) {
      return SV_PYTHON_OK;
  }

  std::vector<int> vals;
  for (int i =0; i<nvals;i++) {
    vals.push_back(PyLong_AsLong(PyList_GetItem(values,i)));
  }
  
  if (PyErr_Occurred() != NULL) {
      api.error("Error parsing values list argument.");
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_set_array_for_local_op_face(src, &dst, arrayName,vals.data(),nvals,outArray,dataType) != SV_OK) {
      api.error("Error setting local op array for geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//------------------------------------
// Geom_set_array_for_local_op_sphere 
//------------------------------------
//
PyDoc_STRVAR(Geom_set_array_for_local_op_sphere_doc,
  "Geom_set_array_for_local_op_sphere(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_set_array_for_local_op_sphere(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssdO|si", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  double radius;
  char *outArray = "LocalOpsArray";
  int dataType = 1;
  double ctr[3];
  int nctr;
  PyObject* ctrList;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&radius,&ctrList,&outArray,&dataType)) {
      return api.argsError();
  }

  // Retrieve source object:
  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(ctrList, emsg)) {
      api.error("The sphere center argument " + emsg);
      return nullptr;
  }

  for(int i=0;i<3;i++) {
    ctr[i] = PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }

  cvPolyData* dst;
  if (sys_geom_set_array_for_local_op_sphere(src, &dst,radius,ctr,outArray,dataType) != SV_OK) {
      api.error("Error setting local op array for geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  // [TODO:DaveP] why?
  vtkPolyData *geom = ((cvPolyData*)(dst))->GetVtkPolyData();

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------------------------
// Geom_set_array_for_local_op_cells 
//-----------------------------------
//
PyDoc_STRVAR(Geom_set_array_for_local_op_cells_doc,
  "set_array_for_local_op_cells(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_set_array_for_local_op_cells(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssO|si", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  PyObject* values;
  char *outArray = "LocalOpsArray";
  int dataType = 1;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&values,&outArray,&dataType)) {
      return api.argsError();
  }

  // Retrieve source object:
  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  int nvals = PyList_Size(values);
  if (nvals == 0) {
      return SV_PYTHON_OK;
  }

  std::vector<int> vals;
  for (int i =0; i<nvals;i++) {
    vals.push_back(PyLong_AsLong(PyList_GetItem(values,i)));
  }

  if (PyErr_Occurred() != NULL) {
      api.error("Error parsing values list argument.");
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_set_array_for_local_op_cells(src, &dst, vals.data(), nvals, outArray, dataType) != SV_OK) {
    PyErr_SetString(PyRunTimeErr, "error creating array on surface" );
      api.error("Error setting local op array for geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  // [TODO:DaveP] dst is not deleted?

  return Py_BuildValue("s",dst->GetName());
}

//-----------------------------------
// Geom_set_array_for_local_op_blend 
//-----------------------------------
//
PyDoc_STRVAR(Geom_set_array_for_local_op_blend_doc,
  "set_array_for_local_op_blend(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_set_array_for_local_op_blend(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssOd|si", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  char *arrayName = 0;
  PyObject* values;
  double radius;
  char *outArray = "LocalOpsArray";
  int dataType = 1;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&arrayName,&values,&radius,&outArray,&dataType)) {
      return api.argsError();
  }

  // Retrieve source object:
  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  // [TODO:DaveP] need to check 'values' is list.

  if (PyList_Size(values) == 0) {
      return SV_PYTHON_OK;
  }

  int nvals = PyList_Size(values);
  std::vector<int> vals;

  for (int i =0; i<nvals;i++) {
    vals.push_back(PyLong_AsLong(PyList_GetItem(values,i)));
  }
  
  if (PyErr_Occurred() != NULL) {
      api.error("Error parsing values list argument.");
      return nullptr;
  }

  cvPolyData* dst;
  if (sys_geom_set_array_for_local_op_face_blend(src, &dst,arrayName,vals.data(),nvals,radius,outArray,dataType) != SV_OK) {
      api.error("Error setting local op array for geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//------------------------
// Geom_local_decimation 
//------------------------
//
PyDoc_STRVAR(Geom_local_decimation_doc,
  "local_decimation(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_local_decimation(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss|dss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  double target = 0.25;
  char *pointArrayName = 0;
  char *cellArrayName = 0;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&target,&pointArrayName,&cellArrayName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_local_quadric_decimation(src, &dst, target, pointArrayName,cellArrayName) != SV_OK) {
      api.error("Error decimating geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------------------
// Geom_local_laplacian_smooth 
//-----------------------------
//
PyDoc_STRVAR(Geom_local_laplacian_smooth_doc,
  "local_laplacian_smooth(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_local_laplacian_smooth(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss|idss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  int numiters = 100;
  double relax = 0.01;
  char *pointArrayName = 0;
  char *cellArrayName = 0;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&numiters,&relax,&pointArrayName,&cellArrayName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_local_laplacian_smooth(src, &dst, numiters,relax, pointArrayName,cellArrayName) != SV_OK) {
      api.error("Error in the laplacian smooth operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------------------
// Geom_local_constrain_smooth 
//-----------------------------
//
PyDoc_STRVAR(Geom_local_constrain_smooth_doc,
  "local_constrain_smooth(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_local_constrain_smooth(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss|idiss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  char *pointArrayName = 0;
  char *cellArrayName = 0;
  int numiters = 5;
  double constrainfactor = 0.7;
  int numcgsolves = 30;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&numiters,&constrainfactor, &numcgsolves,&pointArrayName,&cellArrayName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }
  
  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_local_constrain_smooth(src, &dst, numiters,constrainfactor,numcgsolves, pointArrayName,cellArrayName) != SV_OK) {
      api.error("Error in the local contrain smooth operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

// ------------------------------
// Geom_local_linear_subdivision 
// ------------------------------
//
PyDoc_STRVAR(Geom_local_linear_subdivision_doc,
  "local_linear_subdivision(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_local_linear_subdivision(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss|iss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  int numiters = 100;
  char *pointArrayName = 0;
  char *cellArrayName = 0;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&numiters,&pointArrayName,&cellArrayName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_local_linear_subdivision(src, &dst, numiters,pointArrayName,cellArrayName) != SV_OK) {
      api.error("Error in the local linear subdivision operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------------------------
// Geom_local_butterfly_subdivision 
//-----------------------------------
//
PyDoc_STRVAR(Geom_local_butterfly_subdivision_doc,
  "local_butterfly_subdivision(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_local_butterfly_subdivision(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss|iss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  int numiters = 100;
  char *pointArrayName = 0;
  char *cellArrayName = 0;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&numiters,&pointArrayName,&cellArrayName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_local_butterfly_subdivision(src, &dst, numiters,pointArrayName,cellArrayName) != SV_OK) {
      api.error("Error in the local butterfly subdivision operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------------------
// Geom_local_loop_subdivision 
//-----------------------------
//
PyDoc_STRVAR(Geom_local_loop_subdivision_doc,
  "local_butterfly_subdivision(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_local_loop_subdivision(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss|iss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  int numiters = 100;
  char *pointArrayName = 0;
  char *cellArrayName = 0;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&numiters,&pointArrayName,&cellArrayName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_local_loop_subdivision(src, &dst, numiters,pointArrayName,cellArrayName) != SV_OK) {
      api.error("Error in the local loop subdivision operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//------------------
// Geom_local_blend 
//------------------
//
// [TODO:DaveP] this could use named arguments.
//
PyDoc_STRVAR(Geom_local_blend_doc,
  "local_blend(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_local_blend(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss|iiiiidss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  char *pointArrayName = 0;
  char *cellArrayName = 0;
  int numblenditers = 2;
  int numsubblenditers = 2;
  int numsubdivisioniters = 1;
  int numcgsmoothiters = 3;
  int numlapsmoothiters = 50;
  double targetdecimation = 0.01;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName, &numblenditers, &numsubdivisioniters, &numcgsmoothiters, 
    &numlapsmoothiters, &targetdecimation, &pointArrayName, &cellArrayName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_local_blend(src, &dst, numblenditers,numsubblenditers, numsubdivisioniters, numcgsmoothiters, numlapsmoothiters, targetdecimation,
     pointArrayName,cellArrayName) != SV_OK ) {
      api.error("Error in the local blend operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------
// Geom_all_union 
//-----------------
//
PyDoc_STRVAR(Geom_all_union_doc,
  "all_union(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject* 
Geom_all_union(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Ois|d", PyRunTimeErr, __func__);
  PyObject* srcList;
  int interT;
  char *dstName;
  double tolerance = 1e-5;

  if (!PyArg_ParseTuple(args, api.format, &srcList, &interT, &dstName, &tolerance)) {
      return api.argsError();
  }

  // Check that sources are in the repository.
  //
  std::vector<cvPolyData*> srcs;
  if (!GetGeometryObjects(api, srcList, srcs)) {
      return nullptr;
  }
  auto numSrcs = srcs.size();

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_all_union(srcs.data(), numSrcs,interT, tolerance, &dst) != SV_OK) {
      api.error("Error in the all union operation.");
      return nullptr;
  }

  // Create new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if (geom == NULL) {
      api.error("Error creating solid model."); 
      return nullptr;
  }

  auto dstPd = dst->GetVtkPolyData();
  geom->SetVtkPolyDataObject(dstPd);

  if (!AddGeometryToRepository(api, dstName, dst)) {
      delete geom;
      return nullptr;
  }

  return Py_BuildValue("s",geom->GetName());
}

//-----------------------------
// Geom_convert_nurbs_to_poly 
//-----------------------------
//
// [TODO:DaveP] not sure about this function name.
//
PyDoc_STRVAR(Geom_convert_nurbs_to_poly_doc,
  "convert_nurbs_to_poly(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_convert_nurbs_to_poly(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOOs", PyRunTimeErr, __func__);
  char *srcName;
  PyObject* faceList;
  PyObject* idList;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &faceList, &idList, &dstName)) {
      return api.argsError();
  }

  auto model = GetRepositoryGeometry(api, srcName);
  if (model == NULL) {
      return nullptr;
  }

  if (!PyList_Check(faceList)) {
      api.error("Face list argument is not a Python list.");
      return nullptr;
  }

  if (!PyList_Check(idList)) {
      api.error("ID list argument is not a Python list.");
      return nullptr;
  }

  auto numFaces = PyList_Size(faceList);
  auto numIds = PyList_Size(idList);
  if (numFaces != numIds) {
      api.error("The number of IDs (" + std::to_string(numIds)+") != the number of faces ("+std::to_string(numFaces)+").");
  }

  // Check that sources are in the repository.
  //
  std::vector<cvPolyData*> faces;
  if (!GetGeometryObjects(api, faceList, faces)) {
      return nullptr;
  }

  std::vector<int> allids;
  for (int i=0; i<numIds;i++) {
      allids.push_back(PyLong_AsLong(PyList_GetItem(idList,i)));
  }

  if (PyErr_Occurred() != NULL) {
      api.error("Error parsing values ID list argument.");
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }
    
  // Instantiate the new solid:
  auto geom = cvSolidModel::pyDefaultInstantiateSolidModel( );
  if (geom == NULL ) {
      api.error("Error creating solid model."); 
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_assign_ids_based_on_faces(model, faces.data(), numFaces, allids.data(), &dst) != SV_OK) {
      delete dst;
      api.error("Error in the convert nurbs to poly operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  auto dstPd = dst->GetVtkPolyData();
  geom->SetVtkPolyDataObject(dstPd);

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",geom->GetName());
}

//----------------------------
// Geom_make_polys_consistent 
//----------------------------
//
PyDoc_STRVAR(Geom_make_polys_consistent_doc,
  "make_polys_consistent(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_make_polys_consistent(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_MakePolysConsistent(src, &dst) != SV_OK) {
      api.error("Error in the make polygons consistent operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//------------------------
// Geom_reverse_all_cells 
//------------------------
//
PyDoc_STRVAR(Geom_reverse_all_cells_doc,
  "reverse_all_cells(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_reverse_all_cells(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_ReverseAllCells(src, &dst) != SV_OK) {
      api.error("Error in the reverse all cells operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//------------------------------
// Geom_num_closed_line_regions 
//-----------------------------
//
PyDoc_STRVAR(Geom_num_closed_line_regions_doc,
  "num_closed_line_regions(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_num_closed_line_regions(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *srcName;

  if (!PyArg_ParseTuple(args, api.format, &srcName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  int num;
  if (sys_geom_NumClosedLineRegions(src, &num) != SV_OK) {
      api.error("Error in the num closed line regions operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  return Py_BuildValue("i",PyLong_FromLong(num));
}

//-----------------------------
// Geom_get_closed_line_region 
//-----------------------------
//
PyDoc_STRVAR(Geom_get_closed_line_region_doc,
  "get_closed_line_region(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_get_closed_line_region(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sis", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  int id;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &id, &dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_GetClosedLineRegion(src, id, &dst) != SV_OK) {
      api.error("Error in the get closed line region operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------
// Geom_pick 
//-----------
//
PyDoc_STRVAR(Geom_pick_doc,
  "pick(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_pick(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOs", PyRunTimeErr, __func__);
  char *objName;
  PyObject* posList;
  char *resultName;

  if (!PyArg_ParseTuple(args, api.format, &objName,&posList,&resultName)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, resultName)) {
      return nullptr;
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(posList, emsg)) {
      api.error("The point argument " + emsg);
      return nullptr;
  }

  double pos[3];
  for (int i=0;i<3;i++) {
      pos[i] = PyFloat_AsDouble(PyList_GetItem(posList,i));
  }

  cvPolyData* result;
  if (sys_geom_Pick(obj, pos, &result) != SV_OK ) {
      api.error("Error performing a pick operation on geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, resultName, result)) {
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//---------------------
// Geom_orient_profile 
//---------------------
//
PyDoc_STRVAR(Geom_orient_profile_doc,
  "orient_profile(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_orient_profile(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOOOs", PyRunTimeErr, __func__);
  char *srcName;
  PyObject* pathPosList;
  PyObject* pathTanList;
  PyObject* pathXhatList;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&pathPosList,&pathTanList,&pathXhatList,&dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  // Get position, tangent and xhat data.
  //
  std::string emsg;
  double ppt[3];
  if (!svPyUtilGetPointData(pathPosList, emsg, ppt)) {
      api.error("The point argument " + emsg);
      return nullptr;
  }

  double ptan[3];
  if (!svPyUtilGetPointData(pathTanList, emsg, ptan)) {
      api.error("The tangent argument " + emsg);
      return nullptr;
  }

  double xhat[3];
  if (!svPyUtilGetPointData(pathXhatList, emsg, xhat)) {
      api.error("The xhat argument " + emsg);
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_OrientProfile(src, ppt, ptan, xhat, &dst) != SV_OK) {
      api.error("Error in the orient profile operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//------------------------
// Geom_disorient_profile
//------------------------
//
// [TODO:DaveP] I can only wonder what 'disorient profile' does!
//
PyDoc_STRVAR(Geom_disorient_profile_doc,
  "disorient_profile(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_disorient_profile(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOOOs", PyRunTimeErr, __func__);
  char *srcName;
  PyObject* pathPosList;
  PyObject* pathTanList;
  PyObject* pathXhatList;
  char *dstName;

  if (!PyArg_ParseTuple(args,"sOOOs", &srcName,&pathPosList,&pathTanList,&pathXhatList,&dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  // Get position, tangent and xhat data.
  //
  std::string emsg;
  double ppt[3];
  if (!svPyUtilGetPointData(pathPosList, emsg, ppt)) {
      api.error("The point argument " + emsg);
      return nullptr;
  }

  double ptan[3];
  if (!svPyUtilGetPointData(pathTanList, emsg, ptan)) {
      api.error("The tangent argument " + emsg);
      return nullptr;
  }

  double xhat[3];
  if (!svPyUtilGetPointData(pathXhatList, emsg, xhat)) {
      api.error("The xhat argument " + emsg);
      return nullptr;
  }

  cvPolyData *dst;
  if ( sys_geom_DisorientProfile(src, ppt, ptan, xhat, &dst) != SV_OK) {
      api.error("Error in the disorient profile operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//--------------------
// Geom_align_profile 
//--------------------
//
PyDoc_STRVAR(Geom_align_profile_doc,
  "align_profile(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_align_profile(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssi", PyRunTimeErr, __func__);
  char *refName;
  char *srcName;
  char *dstName;
  int vecMtd = 1;

  if (!PyArg_ParseTuple(args, api.format, &refName, &srcName, &dstName, &vecMtd)) {
      return api.argsError();
  }

  auto ref = GetRepositoryGeometry(api, refName);
  if (ref == NULL) {
      return nullptr;
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;

  if ( vecMtd ) {
    dst = sys_geom_Align(ref, src);
  } else {
    dst = sys_geom_AlignByDist(ref, src);
  }

  if (dst == NULL) {
      api.error("Error in the align profile operation between reference '" + std::string(refName) + 
      " and source '" + std::string(refName) + "' geometries.");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//----------------
// Geom_translate 
//----------------
//
PyDoc_STRVAR(Geom_translate_doc,
  "translate(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_translate(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOs", PyRunTimeErr, __func__);
  char *srcName;
  PyObject* vecList;
  char *dstName;

  int n;

  if (!PyArg_ParseTuple(args,api.format, &srcName, &vecList, &dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  // Get vec data.
  //
  std::string emsg;
  double vec[3];
  if (!svPyUtilGetPointData(vecList, emsg, vec)) {
      api.error("The vec argument " + emsg);
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_Translate(src, vec, &dst) != SV_OK) {
      api.error("Error in the translate operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//----------------
// Geom_scale_avg 
//----------------
//
PyDoc_STRVAR(Geom_scale_avg_doc,
  "scale_avg(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_scale_avg(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sds", PyRunTimeErr, __func__);
  char *srcName;
  double factor;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &factor, &dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_ScaleAvg(src, factor, &dst) != SV_OK) {
      api.error("Error performing the scaling operation on geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-------------------------
// Geom_get_ordered_points 
//-------------------------
//
PyDoc_STRVAR(Geom_get_ordered_points_doc,
  "get_ordered_points(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_get_ordered_points(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *srcName;

  if (!PyArg_ParseTuple(args, api.format, &srcName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  cvPolyData *dst;
  double *pts;
  int num;

  if (sys_geom_GetOrderedPts(src, &pts, &num) != SV_OK) {
      api.error("Error geting ordered points from the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }
  
  // Convert returned points array to Python list.
  //
  // [TODO:DaveP] must remove the C-style array addressing.
  //
  PyObject* pylist = PyList_New(num);
  for (int i = 0; i < num; i++ ) {
    PyObject* tmplist = PyList_New(3);
    PyList_SetItem(tmplist, 0, PyFloat_FromDouble(pts[3*i]));
    PyList_SetItem(tmplist, 1, PyFloat_FromDouble(pts[3*i+1]));
    PyList_SetItem(tmplist, 2, PyFloat_FromDouble(pts[3*i+2]));
    PyList_SetItem(pylist, i, tmplist);
  }

  delete [] pts;

  return pylist;
}

//---------------------------
// Geom_write_ordered_points 
//---------------------------
//
PyDoc_STRVAR(Geom_write_ordered_points_doc,
  "write_ordered_points(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_write_ordered_points(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *objName;
  char *fileName;

  if (!PyArg_ParseTuple(args, api.format, &objName, &fileName)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  if (sys_geom_WriteOrderedPts(obj, fileName) != SV_OK) {
      api.error("Error writing geometry '" + std::string(objName) + " to the file '" + std::string(fileName) + "'.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------------
// Geom_write_lines 
//------------------
//
PyDoc_STRVAR(Geom_write_lines_doc,
  "write_lines(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_write_lines(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *objName;
  char *fileName;

  if (!PyArg_ParseTuple(args,api.format, &objName,&fileName)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  if (sys_geom_WriteLines(obj, fileName) != SV_OK) {
      api.error("Error writing lines geometry '" + std::string(objName) + " to the file '" + std::string(fileName) + "'.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------
// Geom_polys_closed 
//-------------------
//
PyDoc_STRVAR(Geom_polys_closed_doc,
  "polys_closed(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_polys_closed(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *srcName;

  if (!PyArg_ParseTuple(args,api.format, &srcName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  int closed;
  if (sys_geom_PolysClosed(src, &closed) != SV_OK) {
      api.error("Error performing a polys closed operation for the geometry '" + std::string(srcName)+ "'.");
      return nullptr;
  }
  
  return Py_BuildValue("N", PyBool_FromLong(closed));
}

//-------------------
// Geom_surface_area 
//-------------------
//
PyDoc_STRVAR(Geom_surface_area_doc,
  "surface_area(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_surface_area(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *srcName;

  if (!PyArg_ParseTuple(args, api.format, &srcName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  double area;
  if (sys_geom_SurfArea(src, &area) != SV_OK) {
      api.error("Error computing the area for the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  return Py_BuildValue("d",area);
}

//------------------------
// Geom_get_poly_centroid 
//------------------------
//
PyDoc_STRVAR(Geom_get_poly_centroid_doc,
  "get_poly_centroid(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_get_poly_centroid(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *srcName;

  if (!PyArg_ParseTuple(args, api.format, &srcName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  double centroid[3];
  if (sys_geom_getPolyCentroid(src, centroid) != SV_OK) {
      api.error("Error computing the centroid for the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  return Py_BuildValue("ddd",centroid[0], centroid[1], centroid[2]);
}

//----------------------
// Geom_print_tri_stats 
//----------------------
//
PyDoc_STRVAR(Geom_print_tri_stats_doc,
  "Geom_print_tri_stats(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_print_tri_stats(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *srcName;

  if (!PyArg_ParseTuple(args, api.format, &srcName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (sys_geom_PrintTriStats(src) != SV_OK) {
      api.error("Error printing tri stats for the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------------------
// Geom_print_small_polys 
//------------------------
//
PyDoc_STRVAR(Geom_print_small_polys_doc,
  "print_small_polys(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_print_small_polys(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *srcName;
  double sideTol;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &sideTol)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (sys_geom_PrintSmallPolys( (cvPolyData*)src, sideTol ) != SV_OK ) {
      api.error("Error printing small polys for the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------------
// Geom_remove_small_polys 
//-------------------------
//
PyDoc_STRVAR(Geom_remove_small_polys_doc,
  "remove_small_polys(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_remove_small_polys(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssd", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  double sideTol;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&sideTol)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_RmSmallPolys(src, sideTol, &dst) != SV_OK) {
      api.error("Error removing small polygons from the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------
// Geom_bbox
//-----------
//
PyDoc_STRVAR(Geom_bbox_doc,
  "Geom_bbox(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_bbox(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  double bbox[6];
  if (sys_geom_BBox(obj, bbox) != SV_OK) {
      api.error("Error getting the bounding box for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  PyObject* pylist = PyList_New(6);
  for (int i = 0; i < 6; i++ ) {
      PyList_SetItem(pylist, i, PyFloat_FromDouble(bbox[i]));
  }

  return pylist;
}

//---------------
// Geom_classify
//---------------
//
PyDoc_STRVAR(Geom_classify_doc,
  "classify(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_classify(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sO", PyRunTimeErr, __func__);
  char *objName;
  PyObject* ptList;

  if (!PyArg_ParseTuple(args,api.format, &objName,&ptList)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  double pt[3];
  std::string emsg;
  if (!svPyUtilGetPointData(ptList, emsg, pt)) {
      api.error("The point argument " + emsg);
      return nullptr;
  }

  int ans;
  if (sys_geom_Classify(obj, pt, &ans) != SV_OK) {
      api.error("Error classifying a point for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  return Py_BuildValue("i",ans);
}

//--------------------
// Geom_point_in_poly 
//--------------------
//
PyDoc_STRVAR(Geom_point_in_poly_doc,
  "point_in_poly(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_point_in_poly(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOi", PyRunTimeErr, __func__);
  char *objName;
  PyObject* ptList;
  int usePrevPoly = 0;

  if (!PyArg_ParseTuple(args, api.format, &objName,&ptList,&usePrevPoly)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  double pt[3];
  std::string emsg;
  if (!svPyUtilGetPointData(ptList, emsg, pt)) {
      api.error("The point argument " + emsg);
      return nullptr;
  }

  int ans;
  if (sys_geom_PtInPoly(obj, pt ,usePrevPoly, &ans) != SV_OK) {
      api.error("Error classifying a point in a poly for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  return Py_BuildValue("i",ans);
}

//-------------------
// Geom_merge_points 
//-------------------
//
PyDoc_STRVAR(Geom_merge_points_doc,
  "merge_points(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_merge_points(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssd", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  double tol = 1e10 * FindMachineEpsilon();

  if (!PyArg_ParseTuple(args, api.format, &srcName,&dstName,&tol)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  // [TODO:DaveP] we now see two diffetent return patters.
  auto dst = sys_geom_MergePts_tol(src, tol);
  if (dst == nullptr) {
      api.error("Error merging points poly for the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }
    
  return Py_BuildValue("s",dst->GetName());
}

//---------------------
// Geom_warp_3d_points 
//---------------------
//
PyDoc_STRVAR(Geom_warp_3d_points_doc,
  "warp_3d_points(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_warp_3d_points(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssd", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;
  double scale = 1.0;

  if (!PyArg_ParseTuple(args,api.format, &srcName,&dstName,&scale)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  auto dst = sys_geom_warp3dPts(src, scale);
  if (dst == nullptr) { 
      api.error("Error warping 3D points from the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------
// Geom_num_points 
//-----------------
//
PyDoc_STRVAR(Geom_num_points_doc,
  "num_points(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_num_points(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *srcName;

  if (!PyArg_ParseTuple(args, api.format, &srcName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  auto num = src->GetVtkPolyData()->GetNumberOfPoints();

  return Py_BuildValue("i", num);
}

//------------------
// Geom_sample_loop 
//------------------
//
PyDoc_STRVAR(Geom_sample_loop_doc,
  "sample_loop(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_sample_loop(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sis", PyRunTimeErr, __func__);
  char *srcName;
  int targetNumPts;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &targetNumPts, &dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  auto dst = sys_geom_sampleLoop( (cvPolyData*)src, targetNumPts );

  if (dst == NULL) {
      api.error("Error performing the sample loop operation on the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------
// Geom_loft_solid 
//-----------------
//
// [TODO:DaveP] need to have named arguments here. 
//
PyDoc_STRVAR(Geom_loft_solid_doc,
  "loft_solid(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_loft_solid(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Osiiiiii|iddd", PyRunTimeErr, __func__);
  PyObject* srcList;
  char *dstName;
  int numOutPtsInSegs;
  int numOutPtsAlongLength;
  int numLinearPtsAlongLength;
  int numModes;
  int useFFT;
  int useLinearSampleAlongLength;
  int splineType = 0;
  double bias = 0;
  double tension = 0;
  double continuity = 0;

  if (!PyArg_ParseTuple(args, api.format, &srcList, &dstName, &numOutPtsInSegs, &numOutPtsAlongLength, &numLinearPtsAlongLength, 
          &numModes, &useFFT, &useLinearSampleAlongLength, &splineType, &bias, &tension, &continuity)) {
      return api.argsError();
  }

  // [TODO:DaveP] we really need to check the values of all of the input arguments.
  //
  // splineType 0?

  // Check the list of source geometries.
  std::vector<cvPolyData*> srcs;
  if (!GetGeometryObjects(api, srcList, srcs)) {
      return nullptr;
  }
  auto numSrcs = srcs.size();

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  if (sys_geom_loft_solid(srcs.data(), numSrcs,useLinearSampleAlongLength,useFFT, numOutPtsAlongLength,numOutPtsInSegs,
          numLinearPtsAlongLength,numModes,splineType,bias,tension,continuity, &dst) != SV_OK) {
      delete dst;
      api.error("Error performing the loft operation.");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-----------------------------
// Geom_loft_solid_using_nurbs 
//-----------------------------
//
PyDoc_STRVAR(Geom_loft_solid_using_nurbs_doc,
  "loft_solid_using_nurbs(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject *
Geom_loft_solid_using_nurbs(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Osiiddssss", PyRunTimeErr, __func__);
  PyObject* srcList;
  char *dstName;
  int uDegree = 2;
  int vDegree = 2;
  double uSpacing = 0.01;
  double vSpacing = 0.01;
  char *uKnotSpanType;
  char *vKnotSpanType;
  char *uParametricSpanType;
  char *vParametricSpanType;

  if (!PyArg_ParseTuple(args, api.format, &srcList, &dstName, &uDegree, &vDegree, &uSpacing, &vSpacing, &uKnotSpanType, &vKnotSpanType, 
          &uParametricSpanType, &vParametricSpanType)) {
      return api.argsError();
  }

  // Check the list of source geometries.
  std::vector<cvPolyData*> srcs;
  if (!GetGeometryObjects(api, srcList, srcs)) {
      return nullptr;
  }
  auto numSrcs = srcs.size();

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  cvPolyData *dst;
  vtkSmartPointer<vtkSVNURBSSurface> NURBSSurface = vtkSmartPointer<vtkSVNURBSSurface>::New();

  if (sys_geom_loft_solid_with_nurbs(srcs.data(), numSrcs, uDegree, vDegree, uSpacing, vSpacing, uKnotSpanType, vKnotSpanType,
          uParametricSpanType, vParametricSpanType, NURBSSurface, &dst) != SV_OK) {
      delete dst;
      api.error("Error creating a lofted solid using nurbs.");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//---------------------
// Geom_winding_number 
//---------------------
//
PyDoc_STRVAR(Geom_winding_number_doc,
  "winding_number(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_winding_number(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  auto wnum = sys_geom_2DWindingNum(obj);

  return Py_BuildValue("i",wnum);
}

//---------------------
// Geom_polygon_normal 
//---------------------
//
PyDoc_STRVAR(Geom_polygon_normal_doc,
  "polygon_norma(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_polygon_normal(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  double normal[3];

  if (sys_geom_PolygonNormal(obj, normal) != SV_OK) {
      api.error("Error calculating the normal for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  return Py_BuildValue("ddd",normal[0],normal[1],normal[2]);
}

//--------------------
// Geom_average_point 
//--------------------
//
PyDoc_STRVAR(Geom_average_point_doc,
  "average_point(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_average_point(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  double pt[3];

  if (sys_geom_AvgPt(obj, pt) != SV_OK) {
      api.error("Error calculating the average point for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  return Py_BuildValue("ddd",pt[0], pt[1], pt[2]);
}

//-----------
// Geom_copy 
//-----------
//
PyDoc_STRVAR(Geom_copy_doc,
  "Geom_copy(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_copy(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *srcName;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  auto dst = sys_geom_DeepCopy(src);
  if (dst == NULL) {
      api.error("Error copying the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//----------------------
// Geom_reorder_polygon 
//----------------------
//
PyDoc_STRVAR(Geom_reorder_polygon_doc,
  "reorder_polygon(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_reorder_polygon(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sis", PyRunTimeErr, __func__);
  char *srcName;
  int start;
  char *dstName;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &start, &dstName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  auto dst = sys_geom_ReorderPolygon( (cvPolyData*)src, start );
  if (dst == NULL) {
      api.error("Error repordering a polygon for the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//---------------------------------
// Geom_spline_points_to_path_plan 
//---------------------------------
//
PyDoc_STRVAR(Geom_spline_points_to_path_plan_doc,
  "spline_points_to_path_plan(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_spline_points_to_path_plan(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sii|s", PyRunTimeErr, __func__);
  char *srcName;
  int numOutputPts;
  int flag;
  char *filename = NULL;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &numOutputPts, &flag, &filename)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  int result;
  char *output;
  if (filename == NULL) {
    result = pysys_geom_splinePtsToPathPlan(src->GetVtkPolyData(),numOutputPts, filename, flag, &output);
  } else {
    result = pysys_geom_splinePtsToPathPlan(src->GetVtkPolyData(),numOutputPts, filename, flag, NULL);
  }

  if (result != SV_OK) {
      api.error("Error writing spline points for the geometry '" + std::string(srcName) + ".");
      return nullptr;
  }

  if (filename == NULL) {
      return Py_BuildValue("s",output);
  } else {
      return SV_PYTHON_OK;
  } 
}

//------------------------
// Geom_integrate_surface 
//------------------------
//
PyDoc_STRVAR(Geom_integrate_surface_doc,
  "integrate_surface(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_integrate_surface(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOi", PyRunTimeErr, __func__);
  char *objName;
  PyObject* nrmList;
  int tensorType;

  if (!PyArg_ParseTuple(args, api.format, &objName, &nrmList, &tensorType)) {
      return api.argsError();
  }

  std::string emsg;
  double normal[3];
  if (!svPyUtilGetPointData(nrmList, emsg, normal)) {
      api.error("The normal argument " + emsg);
      return nullptr;
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  double q = 0.0;
  if (sys_geom_IntegrateSurface(obj, tensorType, normal, &q) != SV_OK) {
      api.error("Error calculating surface integral for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  return Py_BuildValue("d",q);
}

//-------------------------
// Geom_integrate_surface2 
//-------------------------
//
PyDoc_STRVAR(Geom_integrate_surface2_doc,
  "integrate_surface2(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_integrate_surface2(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("si", PyRunTimeErr, __func__);
  char *objName;
  int tensorType;

  if (!PyArg_ParseTuple(args, api.format, &objName, &tensorType)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  double q = 0.0;
  double area = 0.0;
  if (sys_geom_IntegrateSurface2(obj, tensorType, &q, &area) != SV_OK) {
      api.error("Error calculating surface integral for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  return Py_BuildValue("dd",q,area);
}

//-----------------------
// Geom_integrate_energy 
//-----------------------
//
PyDoc_STRVAR(Geom_integrate_energy_doc,
  "integrate_energy(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_integrate_energy(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOd", PyRunTimeErr, __func__);
  char *objName;
  PyObject* nrmList;
  double rho = 0.0;

  if (!PyArg_ParseTuple(args, api.format, &objName,&nrmList,&rho)) {
      return api.argsError();
  }

  std::string emsg;
  double normal[3];
  if (!svPyUtilGetPointData(nrmList, emsg, normal)) {
      api.error("The normal argument " + emsg);
      return nullptr;
  }
  
  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  double energy = 0.0;
  if (sys_geom_IntegrateEnergy(obj, rho, normal, &energy) != SV_OK ) {
      api.error("Error calculating the energy integral for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  return Py_BuildValue("d",energy);
}

//--------------------
// Geom_find_distance 
//--------------------
//
PyDoc_STRVAR(Geom_find_distance_doc,
  "find_distance(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_find_distance(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sO", PyRunTimeErr, __func__);
  char *objName;
  PyObject* ptList;

  if (!PyArg_ParseTuple(args, api.format, &objName,&ptList)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  std::string emsg;
  double pt[3];
  if (!svPyUtilGetPointData(ptList, emsg, pt)) {
      api.error("The point argument " + emsg);
      return nullptr;
  }

  auto distance = obj->FindDistance( pt[0], pt[1], pt[2] );

  return Py_BuildValue("d",distance);
}

//-------------------------
// Geom_interpolate_scalar 
//-------------------------
//
PyDoc_STRVAR(Geom_interpolate_scalar_doc,
  "interpolate_scalar(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_interpolate_scalar(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sO", PyRunTimeErr, __func__);
  char *objName;
  PyObject* ptList;

  if (!PyArg_ParseTuple(args, api.format, &objName,&ptList)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  std::string emsg;
  double pt[3];
  if (!svPyUtilGetPointData(ptList, emsg, pt)) {
      api.error("The point argument " + emsg);
      return nullptr;
  }

  double scalar = 0.0;
  if (sys_geom_InterpolateScalar(obj, pt, &scalar) != SV_OK) {
      api.error("Error calculating the scalar integral for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  return Py_BuildValue("d",scalar);
}

//-------------------------
// Geom_interpolate_vector 
//-------------------------
//
PyDoc_STRVAR(Geom_interpolate_vector_doc,
  "interpolate_vector(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_interpolate_vector(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sO", PyRunTimeErr, __func__);
  char *objName;
  PyObject* ptList;

  if (!PyArg_ParseTuple(args, api.format, &objName,&ptList)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  std::string emsg;
  double pt[3];
  if (!svPyUtilGetPointData(ptList, emsg, pt)) {
      api.error("The point argument " + emsg);
      return nullptr;
  }

  double vect[3] = {0.0, 0.0, 0.0};
  if ( sys_geom_InterpolateVector(obj, pt, vect) != SV_OK ) {
      api.error("Error interpolating a vector for the geometry '" + std::string(objName) + ".");
      return nullptr;
  }

  PyObject* pList = PyList_New(3);
  for (int i = 0; i<3; i++) {
    PyList_SetItem(pList,i,PyFloat_FromDouble(vect[i]));
  }
  return pList;
}

//--------------------------
// Geom_intersect_with_line 
//--------------------------
//
PyDoc_STRVAR(Geom_intersect_with_line_doc,
  "intersect_with_line(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_intersect_with_line(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOO", PyRunTimeErr, __func__);
  char *objName;
  PyObject *p1List;
  PyObject *p2List;

  if (!PyArg_ParseTuple(args, api.format, &objName, &p1List, &p2List)) {
      return api.argsError();
  }

  auto obj = GetRepositoryGeometry(api, objName);
  if (obj == NULL) {
      return nullptr;
  }

  std::string emsg;
  double pt1[3];
  if (!svPyUtilGetPointData(p1List, emsg, pt1)) {
      api.error("The point1 argument " + emsg);
      return nullptr;
  }

  double pt2[3];
  if (!svPyUtilGetPointData(p2List, emsg, pt2)) {
      api.error("The point2 argument " + emsg);
      return nullptr;
  }

  double intersect[3];
  if (sys_geom_IntersectWithLine(obj, pt1, pt2, intersect) != SV_OK) {
      api.error("Error intersecting the geometry '" + std::string(objName) + " with a line.");
      return nullptr;
  }

  return Py_BuildValue("ddd",intersect[0], intersect[1], intersect[2]);
}

//---------------------
// Geom_add_point_data 
//---------------------
//
PyDoc_STRVAR(Geom_add_point_data_doc,
  "add_point_data(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_add_point_data(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssii", PyRunTimeErr, __func__);
  char *srcNameA;
  char *srcNameB;
  char *dstName;
  int scflag = FALSE;
  int vflag = FALSE;

  if (!PyArg_ParseTuple(args, api.format, &srcNameA, &srcNameB, &dstName, &scflag, &vflag)) {
      return api.argsError();
  }

  auto srcA = GetRepositoryGeometry(api, srcNameA);
  if (srcA == NULL) {
      return nullptr;
  }

  auto srcB = GetRepositoryGeometry(api, srcNameB);
  if (srcB == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  sys_geom_math_scalar sc = SYS_GEOM_NO_SCALAR;
  sys_geom_math_vector v = SYS_GEOM_NO_VECTOR;

  if (scflag) {
      sc = SYS_GEOM_ADD_SCALAR;
  }
  if (vflag) {
      v = SYS_GEOM_ADD_VECTOR;
  }

  cvPolyData *dst;
  if ( sys_geom_mathPointData(srcA, srcB, sc, v, &dst) != SV_OK ) {
      api.error("Error adding point data for the geometry '" + std::string(srcNameA) + 
          " and " + std::string(srcNameB) +".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//--------------------------
// Geom_subtract_point_data
//--------------------------
//
PyDoc_STRVAR(Geom_subtract_point_data_doc,
  "subtract_point_data(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_subtract_point_data(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssii", PyRunTimeErr, __func__);
  char *srcNameA;
  char *srcNameB;
  char *dstName;
  int scflag = FALSE;
  int vflag = FALSE;

  if (!PyArg_ParseTuple(args, api.format, &srcNameA,&srcNameB,&dstName,&scflag,&vflag)) {
      return api.argsError();
  }

  auto srcA = GetRepositoryGeometry(api, srcNameA);
  if (srcA == NULL) {
      return nullptr;
  }

  auto srcB = GetRepositoryGeometry(api, srcNameB);
  if (srcB == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  sys_geom_math_scalar sc = SYS_GEOM_NO_SCALAR;
  sys_geom_math_vector v = SYS_GEOM_NO_VECTOR;
  if (scflag) {
      sc = SYS_GEOM_SUBTRACT_SCALAR;
  }
  if (vflag) {
      v = SYS_GEOM_SUBTRACT_VECTOR;
  }

  cvPolyData *dst;
  if (sys_geom_mathPointData(srcA, srcB, sc, v, &dst) != SV_OK) {
      api.error("Error subtracting point data for the geometry '" + std::string(srcNameA) + 
          " and " + std::string(srcNameB) +".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//--------------------------
// Geom_multiply_point_data 
//--------------------------
//
PyDoc_STRVAR(Geom_multiply_point_data_doc,
  "multiply_point_data(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_multiply_point_data(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssii", PyRunTimeErr, __func__);
  char *srcNameA;
  char *srcNameB;
  char *dstName;
  int scflag = FALSE;
  int vflag = FALSE;

  if (!PyArg_ParseTuple(args, api.format, &srcNameA, &srcNameB, &dstName, &scflag, &vflag)) {
      return api.argsError();
  }

  auto srcA = GetRepositoryGeometry(api, srcNameA);
  if (srcA == NULL) {
      return nullptr;
  }

  auto srcB = GetRepositoryGeometry(api, srcNameB);
  if (srcB == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  sys_geom_math_scalar sc = SYS_GEOM_NO_SCALAR;
  sys_geom_math_vector v = SYS_GEOM_NO_VECTOR;
  if (scflag) {
      sc = SYS_GEOM_MULTIPLY_SCALAR;
  }
  if (vflag) {
      v = SYS_GEOM_MULTIPLY_VECTOR;
  }

  cvPolyData *dst;
  if ( sys_geom_mathPointData( (cvPolyData*)srcA, (cvPolyData*)srcB, sc, v, (cvPolyData**)(&dst) ) != SV_OK ) {
      api.error("Error multiplying point data for the geometry '" + std::string(srcNameA) + 
          " and " + std::string(srcNameB) +".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//------------------------
// Geom_divide_point_data 
//------------------------
//
PyDoc_STRVAR(Geom_divide_point_data_doc,
  "Geom_divide_point_data(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_divide_point_data(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssii", PyRunTimeErr, __func__);
  char *srcNameA;
  char *srcNameB;
  char *dstName;
  int scflag = FALSE;
  int vflag = FALSE;

  if (!PyArg_ParseTuple(args, api.format, &srcNameA,&srcNameB,&dstName,&scflag,&vflag)) {
      return api.argsError();
  }

  auto srcA = GetRepositoryGeometry(api, srcNameA);
  if (srcA == NULL) {
      return nullptr;
  }

  auto srcB = GetRepositoryGeometry(api, srcNameB);
  if (srcB == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  sys_geom_math_scalar sc = SYS_GEOM_NO_SCALAR;
  sys_geom_math_vector v = SYS_GEOM_NO_VECTOR;

  if (scflag) {
      sc = SYS_GEOM_DIVIDE_SCALAR;
  }
  if (vflag) {
      v = SYS_GEOM_DIVIDE_VECTOR;
  }

  cvPolyData *dst;
  if (sys_geom_mathPointData(srcA, srcB, sc, v, &dst) != SV_OK) {
    PyErr_SetString(PyRunTimeErr, "point data math error" );
      api.error("Error dividing point data for the geometry '" + std::string(srcNameA) + 
          " and " + std::string(srcNameB) +".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//--------------
// Geom_project 
//---------------
//
PyDoc_STRVAR(Geom_project_doc,
  "project(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_project(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssii", PyRunTimeErr, __func__);
  char *srcNameA;
  char *srcNameB;
  char *dstName;
  int scflag = FALSE;
  int vflag = FALSE;

  if (!PyArg_ParseTuple(args,api.format, &srcNameA,&srcNameB,&dstName,&scflag,&vflag)) {
      return api.argsError();
  }

  auto srcA = GetRepositoryGeometry(api, srcNameA);
  if (srcA == NULL) {
      return nullptr;
  }

  auto srcB = GetRepositoryGeometry(api, srcNameB);
  if (srcB == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  sys_geom_math_scalar sc = SYS_GEOM_NO_SCALAR;
  sys_geom_math_vector v = SYS_GEOM_NO_VECTOR;

  if (scflag) {
      sc = SYS_GEOM_ADD_SCALAR;
  }
  if (vflag) {
      v = SYS_GEOM_ADD_VECTOR;
  }

  cvPolyData *dst;
  if (sys_geom_Project(srcA, srcB, sc, v, &dst) != SV_OK) {
      api.error("Error projecting point data for the geometry '" + std::string(srcNameA) + 
          " and " + std::string(srcNameB) +".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

//-------------------------------
// Geom_integrate_scalar_surface
//-------------------------------
//
PyDoc_STRVAR(Geom_integrate_scalar_surface_doc,
  "integrate_scalar_surface(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_integrate_scalar_surface(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *srcName;

  if (!PyArg_ParseTuple(args, api.format, &srcName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  double flux;

  if (sys_geom_IntegrateScalarSurf(src, &flux) != SV_OK) {
      api.error("Error integrating scalar over the surface for the geometry '" + std::string(srcName) + "'.");
      return nullptr;
  }

  return Py_BuildValue("d",flux);
}

//---------------------------------
// Geom_integrate_scalar_threshold
//---------------------------------
//
PyDoc_STRVAR(Geom_integrate_scalar_threshold_doc,
  "integrate_scalar_threshold(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_integrate_scalar_threshold(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sd", PyRunTimeErr, __func__);
  char *srcName;
  double wssthresh;

  if (!PyArg_ParseTuple(args, api.format, &srcName,&wssthresh)) {
      return api.argsError();
  }

  if (!PyArg_ParseTuple(args, api.format, &srcName)) {
      return api.argsError();
  }

  auto src = GetRepositoryGeometry(api, srcName);
  if (src == NULL) {
      return nullptr;
  }

  double flux, area;

  if (sys_geom_IntegrateScalarThresh(src, wssthresh, &flux, &area) != SV_OK) {
      api.error("Error in calculating the surface area for the geometry '" + std::string(srcName) + "'.");
      return nullptr;
  }

  return Py_BuildValue("dd",flux, area);
}

//-------------------------
// Geom_replace_point_data 
//-------------------------
//
PyDoc_STRVAR(Geom_replace_point_data_doc,
  "replace_point_data(kernel) \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Geom_replace_point_data(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sssii", PyRunTimeErr, __func__);
  char *srcNameA;
  char *srcNameB;
  char *dstName;
  int scflag = FALSE;
  int vflag = FALSE;

  if (!PyArg_ParseTuple(args, api.format, &srcNameA,&srcNameB,&dstName,&scflag,&vflag)) {
      return api.argsError();
  }

  auto srcA = GetRepositoryGeometry(api, srcNameA);
  if (srcA == NULL) {
      return nullptr;
  }

  auto srcB = GetRepositoryGeometry(api, srcNameB);
  if (srcB == NULL) {
      return nullptr;
  }

  if (RepositoryGeometryExists(api, dstName)) {
      return nullptr;
  }

  sys_geom_math_scalar sc = SYS_GEOM_NO_SCALAR;
  sys_geom_math_vector v = SYS_GEOM_NO_VECTOR;

  if (scflag) {
      sc = SYS_GEOM_ADD_SCALAR;
  }
  if (vflag) {
      v = SYS_GEOM_ADD_VECTOR;
  }

  cvPolyData *dst;

  if (sys_geom_ReplacePointData(srcA, srcB, sc, v, &dst) != SV_OK) {
      api.error("Error replacing point data for the geometry '" + std::string(srcNameA) + 
          " and " + std::string(srcNameB) +".");
      return nullptr;
  }

  if (!AddGeometryToRepository(api, dstName, dst)) {
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

//-------------------------
// Geometry module methods
//-------------------------
//
PyMethodDef pyGeom_methods[] =
{
  {"add_point_data", Geom_add_point_data, METH_VARARGS, Geom_add_point_data_doc},

  {"align_profile", Geom_align_profile, METH_VARARGS, Geom_align_profile_doc},

  {"all_union", Geom_all_union, METH_VARARGS, Geom_all_union_doc},

  // RenameL: AvgPt
  {"average_point", Geom_average_point, METH_VARARGS, Geom_average_point_doc},

  {"bbox", Geom_bbox, METH_VARARGS, Geom_bbox_doc},

  {"check_surface", Geom_check_surface, METH_VARARGS, Geom_check_surface_doc},

  {"classify", Geom_classify, METH_VARARGS, Geom_classify_doc},

  {"clean", Geom_clean, METH_VARARGS, Geom_clean_doc},

  {"copy", Geom_copy, METH_VARARGS, Geom_copy_doc},

  {"disorient_profile", Geom_disorient_profile, METH_VARARGS, Geom_disorient_profile_doc},

  {"divide_point_data", Geom_divide_point_data, METH_VARARGS, Geom_divide_point_data_doc},

  {"find_distance", Geom_find_distance, METH_VARARGS, Geom_find_distance_doc},

  {"get_closed_line_region", Geom_get_closed_line_region, METH_VARARGS, Geom_get_closed_line_region_doc},

  // Rename: GetOrderedPts
  {"get_ordered_points", Geom_get_ordered_points, METH_VARARGS, Geom_get_ordered_points_doc},

  {"get_poly_centroid", Geom_get_poly_centroid, METH_VARARGS, Geom_get_poly_centroid_doc},

  {"integrate_surface", Geom_integrate_surface, METH_VARARGS, Geom_integrate_surface_doc},

  {"integrate_surface2", Geom_integrate_surface2, METH_VARARGS, Geom_integrate_surface2_doc},

  {"integrate_energy", Geom_integrate_energy, METH_VARARGS, Geom_integrate_energy_doc},

  {"integrate_scalar_surface", Geom_integrate_scalar_surface, METH_VARARGS, Geom_integrate_scalar_surface_doc},

  // Renmae: IntegrateScalarThresh
  {"integrate_scalar_threshold", Geom_integrate_scalar_threshold, METH_VARARGS, Geom_integrate_scalar_threshold_doc},

  {"interpolate_scalar", Geom_interpolate_scalar, METH_VARARGS, Geom_interpolate_scalar_doc},

  {"interpolate_vector", Geom_interpolate_vector, METH_VARARGS, Geom_interpolate_vector_doc},

  {"intersect", Geom_intersect, METH_VARARGS, Geom_intersect_doc},

  {"intersect_with_line", Geom_intersect_with_line, METH_VARARGS, Geom_intersect_with_line_doc},

  {"local_blend", Geom_local_blend, METH_VARARGS, Geom_local_blend_doc},

  {"local_butterfly_subdivision", Geom_local_butterfly_subdivision, METH_VARARGS, Geom_local_butterfly_subdivision_doc},

  {"local_constrain_smooth", Geom_local_constrain_smooth, METH_VARARGS, Geom_local_constrain_smooth_doc},

  {"local_decimation", Geom_local_decimation, METH_VARARGS, Geom_local_decimation_doc},

  {"local_laplacian_smooth", Geom_local_laplacian_smooth, METH_VARARGS, Geom_local_laplacian_smooth_doc},

  {"local_linear_subdivision", Geom_local_linear_subdivision, METH_VARARGS, Geom_local_linear_subdivision_doc},

  {"local_loop_subdivision", Geom_local_loop_subdivision, METH_VARARGS, Geom_local_loop_subdivision_doc},

  {"loft_solid", Geom_loft_solid, METH_VARARGS, Geom_loft_solid_doc},

  // Rename: LoftSolidWithNURBS
  {"loft_solid_using_nurbs", Geom_loft_solid_using_nurbs, METH_VARARGS, Geom_loft_solid_using_nurbs_doc},

  {"make_polys_consistent", Geom_make_polys_consistent, METH_VARARGS, Geom_make_polys_consistent_doc},

  // Rename: MergePts
  {"merge_points", Geom_merge_points, METH_VARARGS, Geom_merge_points_doc},

  // Renamed: "model_name_model_from_polydata_names"
  {"convert_nurbs_to_poly", Geom_convert_nurbs_to_poly, METH_VARARGS, Geom_convert_nurbs_to_poly_doc},

  {"multiply_point_data", Geom_multiply_point_data, METH_VARARGS, Geom_multiply_point_data_doc},

  {"num_closed_line_regions", Geom_num_closed_line_regions, METH_VARARGS, Geom_num_closed_line_regions_doc},

  // Rename: NumPts
  {"num_points", Geom_num_points, METH_VARARGS, Geom_num_points_doc},

  {"orient_profile", Geom_orient_profile, METH_VARARGS, Geom_orient_profile_doc},

  {"pick", Geom_pick, METH_VARARGS, Geom_pick_doc},

  // Rename: PolygonNorm
  {"polygon_normal", Geom_polygon_normal, METH_VARARGS, Geom_polygon_normal_doc},

  {"polys_closed", Geom_polys_closed, METH_VARARGS, Geom_polys_closed_doc},

  {"print_small_polys", Geom_print_small_polys, METH_VARARGS, Geom_print_small_polys_doc},

  {"print_tri_stats", Geom_print_tri_stats, METH_VARARGS, Geom_print_tri_stats_doc},

  {"project", Geom_project, METH_VARARGS, Geom_project_doc},

  // Rename: PtInPoly
  {"point_in_poly", Geom_point_in_poly, METH_VARARGS, Geom_point_in_poly_doc},

  {"reduce", Geom_reduce, METH_VARARGS, Geom_reduce_doc},

  // Rename: ReorderPgn
  {"reorder_polygon", Geom_reorder_polygon, METH_VARARGS, Geom_reorder_polygon_doc},

  {"replace_point_data", Geom_replace_point_data, METH_VARARGS, Geom_replace_point_data_doc},

  {"reverse_all_cells", Geom_reverse_all_cells, METH_VARARGS, Geom_reverse_all_cells_doc},

  // Rename: RmSmallPolys
  {"remove_small_polys", Geom_remove_small_polys, METH_VARARGS, Geom_remove_small_polys_doc},

  {"sample_loop", Geom_sample_loop, METH_VARARGS, Geom_sample_loop_doc},

  {"scale_avg", Geom_scale_avg, METH_VARARGS, Geom_scale_avg_doc},

  {"set_array_for_local_op_cells", Geom_set_array_for_local_op_cells, METH_VARARGS, Geom_set_array_for_local_op_cells_doc},

  {"set_array_for_local_op_face", Geom_set_array_for_local_op_face, METH_VARARGS, Geom_set_array_for_local_op_face_doc},

  {"set_array_for_local_op_blend", Geom_set_array_for_local_op_blend, METH_VARARGS, Geom_set_array_for_local_op_blend_doc},

  {"set_array_for_local_op_sphere", Geom_set_array_for_local_op_sphere, METH_VARARGS, Geom_set_array_for_local_op_sphere_doc},

  {"set_ids_for_caps", Geom_set_ids_for_caps, METH_VARARGS, Geom_set_ids_for_caps_doc},

  // Rename: SplinePtsToPathPlan
  {"spline_points_to_path_plan", Geom_spline_points_to_path_plan, METH_VARARGS, Geom_spline_points_to_path_plan_doc},

  {"subtract", Geom_subtract, METH_VARARGS, Geom_subtract_doc},

  {"subtract_point_data", Geom_subtract_point_data, METH_VARARGS, Geom_subtract_point_data_doc},

  // Rename: SurfArea
  {"surface_area", Geom_surface_area, METH_VARARGS, Geom_surface_area_doc},

  {"translate", Geom_translate, METH_VARARGS, Geom_translate_doc},

  {"union", Geom_union, METH_VARARGS, Geom_union_doc},

  // Rename:Warp3dPts
  {"warp_3d_points", Geom_warp_3d_points, METH_VARARGS, Geom_warp_3d_points_doc},

  {"winding_number", Geom_winding_number, METH_VARARGS, Geom_winding_number_doc},

  {"write_lines", Geom_write_lines, METH_VARARGS, Geom_write_lines_doc},

  // Rename: WriteOrderedPts
  {"write_ordered_points", Geom_write_ordered_points, METH_VARARGS, Geom_write_ordered_points_doc},

  {NULL,NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MODULE_NAME = "geometry";

PyDoc_STRVAR(GeometryModule_doc, "geometry module functions");

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3

// Size of per-interpreter state of the module.
// Set to -1 if the module keeps state in global variables. 
static int perInterpreterStateSize = -1;

// Always initialize this to PyModuleDef_HEAD_INIT.
static PyModuleDef_Base m_base = PyModuleDef_HEAD_INIT;

// Define the module definition struct which holds all information 
// needed to create a module object. 

static struct PyModuleDef pyGeomModule= {
   m_base,
   MODULE_NAME, 
   GeometryModule_doc,
   perInterpreterStateSize, 
   pyGeom_methods
};

//----------------
// PyInit_pyGeom 
//----------------
// The initialization function called by the Python interpreter when the module is loaded.
//
PyMODINIT_FUNC 
PyInit_pyGeom(void)
{
  if (gRepository == NULL) {
    gRepository = new cvRepository();
    fprintf( stdout, "gRepository created from sv_geom_init\n" );
  }

  auto module = PyModule_Create(&pyGeomModule);

  // Add contour.ContourException exception.
  PyRunTimeErr = PyErr_NewException("pyGeom.error",NULL,NULL);
  Py_INCREF(PyRunTimeErr);
  PyModule_AddObject(module, "error",PyRunTimeErr);

  return module;
}

#endif


//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

//------------
// initpyGeom
//------------
//
PyMODINIT_FUNC initpyGeom(void)
{
  PyObject *pyC;
  if ( gRepository == NULL ) {
    gRepository = new cvRepository();
    fprintf( stdout, "gRepository created from sv_geom_init\n" );
  }
  pyC = Py_InitModule("pyGeom",pyGeom_methods);

  PyRunTimeErr = PyErr_NewException("pyGeom.error",NULL,NULL);
  Py_INCREF(PyRunTimeErr);
  PyModule_AddObject(pyC,"error",PyRunTimeErr);

}

#endif

