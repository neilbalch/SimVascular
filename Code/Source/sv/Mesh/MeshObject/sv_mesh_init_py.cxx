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

// The functions defined here implement the SV Python API mesh module. 
//
// The module name is 'mesh'. The module defines a 'Mesh' class used
// to store mesh data. The 'Mesh' class cannot be imported and must
// be used prefixed by the module name. For example
//
//     mesh = mesh.Mesh()
//
// A Python exception sv.mesh.MeshException is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.mesh.MeshException:
//
#include "SimVascular.h"
#include "SimVascular_python.h"
#include "sv_MeshSystem.h"
#include "sv_MeshObject.h"
#include "sv_mesh_init_py.h"
#include "vtkPythonUtil.h"
#include "sv_PyUtils.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
#include "sv_PolyData.h"
#include "sv_arg.h"
#include "sv_VTK.h"
#include "sv_misc_utils.h"
#include "Python.h"

// Needed for Windows.
#ifdef GetObject
#undef GetObject
#endif

#include <iostream>
#include "sv2_globals.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

static void MeshPrintMethods();

static void 
pyMeshObject_dealloc(pyMeshObject* self)
{
  Py_XDECREF(self->meshObject);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

// -------------
// DeleteMesh
// -------------
// This is the deletion call-back for cvMeshObject object commands.

void DeleteMesh(pyMeshObject* self )
{
    cvMeshObject *meshObject =self->meshObject ;
    gRepository->UnRegister( meshObject->GetName() );
}

// -------------
// fakeDeleteMesh
// -------------
// This is the deletion call-back for cvMeshObject object commands.

void fakeDeleteMesh(pyMeshObject* self) 
{
}

// ------------
// MeshPrintMethods
// ------------

static void MeshPrintMethods()
{

  // Note:  I've commented out some of the currently
  // unimplemented methods in the mesh object.  Since I may
  // want these in the future, instead of removing the methods
  // from the object I just hide them from the user.  This
  // way all I have to do is bind in code in the MegaMeshObject
  // and these commands are ready to go.

  PySys_WriteStdout( "GetFacePolyData\n");
  PySys_WriteStdout( "GetKernel\n");
  PySys_WriteStdout( "GetPolyData\n");
  PySys_WriteStdout( "GetSolid\n");
  PySys_WriteStdout( "SetVtkPolyData\n");
  PySys_WriteStdout( "GetUnstructuredGrid\n");
  PySys_WriteStdout( "Print\n");
  PySys_WriteStdout( "Update\n");
  PySys_WriteStdout( "WriteMetisAdjacency\n");
  PySys_WriteStdout( "*** methods to generate meshes ***\n");
  PySys_WriteStdout( "LoadModel\n");
  /*
#ifdef SV_USE_MESHSIM_DISCRETE_MODEL
  PySys_WriteStdout( "LoadDiscreteModel\n");
#endif
  */
  PySys_WriteStdout( "LoadMesh\n");
  PySys_WriteStdout( "NewMesh\n");
  PySys_WriteStdout( "SetBoundaryLayer\n");
  PySys_WriteStdout( "SetWalls\n");
  PySys_WriteStdout( "SetMeshOptions\n");
  PySys_WriteStdout( "SetCylinderRefinement\n");
  PySys_WriteStdout( "SetSphereRefinement\n");
  PySys_WriteStdout( "SetSizeFunctionBasedMesh\n");
  PySys_WriteStdout( "GenerateMesh\n");
  PySys_WriteStdout( "WriteMesh\n");
  PySys_WriteStdout( "WriteStats\n");
  PySys_WriteStdout( "Adapt\n");
  PySys_WriteStdout( "SetSolidKernel\n");
  PySys_WriteStdout( "GetModelFaceInfo\n");

  return;
}

//---------------------
// CheckMeshLoadUpdate
//---------------------
//
static bool 
CheckMeshLoadUpdate(cvMeshObject *meshObject, std::string& msg) 
{
  if (meshObject == nullptr) {
      msg = "The Mesh object does not have meshObjectetry.";
      return false;
  }

  if (meshObject->GetMeshLoaded() == 0) {
      if (meshObject->Update() == SV_ERROR) {
          msg = "Error updating the mesh.";
          return false;
      }
  }

  return true;
}

//---------------
// CheckMesh
//---------------
// Check if the mesh object has meshObjectetry.
//
// This is really used to set the error message 
// in a single place. 
//
static cvMeshObject *
CheckMesh(SvPyUtilApiFunction& api, pyMeshObject *self)
{
  auto meshObject = self->meshObject;
  if (meshObject == NULL) {
      api.error("The Mesh object does not have meshObjectetry.");
      return nullptr;
  }

  return meshObject;
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//-----------------
// Mesh_new_object
//-----------------
//
PyDoc_STRVAR(Mesh_new_object_doc,
  "new_object(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Mesh_new_object(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|ss", PyRunTimeErr, __func__); 

  char *resultName;
  char *meshFileName = NULL;
  char *solidFileName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &resultName, &meshFileName, &solidFileName)) {
    return api.argsError();
  }

  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The Mesh object '" + std::string(resultName) + "' is already in the repository.");
      return nullptr;
  }

  // Create a new cvMeshObject object. 
  auto meshObject = cvMeshSystem::DefaultInstantiateMeshObject(meshFileName, solidFileName );
  if (meshObject == NULL) {
      api.error("Failed to create Mesh object.");
      return nullptr;
  }

  // Add mesh to the repository.
  if (!gRepository->Register(resultName, meshObject)) {
      delete meshObject;
      api.error("Error adding the Mesh object '" + std::string(resultName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(meshObject);
  self->meshObject = meshObject;
  Py_DECREF(meshObject);
  return SV_PYTHON_OK;
}

//-----------------
// Mesh_get_object
//-----------------
//
// [TODO:DaveP] This sets the 'meshObject' data member of the pyContour struct for
// this object. Bad!
//
PyDoc_STRVAR(Mesh_get_mesh_doc,
  "Mesh_get_mesh(mesh)  \n\ 
   \n\
   Set the mesh meshObjectetry from a Mesh object stored in the repository. \n\
   \n\
   Args: \n\
     mesh (str): The name of the Mesh object. \n\
");

static PyObject * 
Mesh_get_mesh(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *objName = NULL;
  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }
   
  // Get the Mesh object from the repository. 
  auto rd = gRepository->GetObject(objName);
  if (rd == NULL) {
      api.error("The Mesh object '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  // Check its type.
  auto type = rd->GetType();
  if (type != MESH_T) {
      api.error("'"+std::string(objName)+"' is not a Mesh object.");
      return nullptr;
  }
  
  auto meshObject = dynamic_cast<cvMeshObject*> (rd);
  Py_INCREF(meshObject);
  self->meshObject = meshObject;
  Py_DECREF(meshObject);
  return SV_PYTHON_OK; 
}
    
//----------------------
// Mesh_set_mesh_kernel
//----------------------
//
// [TODO:DaveP] Mesh kernel is a bit obscure, why not mesh library, or API?
//
//     Maybe 'set_mesher' ?
//           'set_meshing_interface()' ?
//
PyDoc_STRVAR(Mesh_set_kernel_doc,
  "set_kernel(kernel)  \n\ 
   \n\
   Set the meshing kernel. \n\
   \n\
   Args: \n\
     kernel (str): The name of the mesh kernel to set. Valid kernel names are: GMsh, MeshSim or TetGen. \n\
");

static PyObject * 
Mesh_set_kernel(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *kernelName;
  if(!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  cvMeshObject::KernelType kernelType = cvMeshObject::GetKernelType(kernelName);
  if (kernelType == cvMeshObject::KERNEL_INVALID) { 
      auto msg = "Unknown mesh kernel type '" + std::string(kernelName) + "'." + 
        " Valid mesh kernel names are: GMsh, MeshSim or TetGen.";
      api.error(msg);
      return nullptr;
  }

  if (cvMeshSystem::SetCurrentKernel(kernelType) != SV_OK) {
      api.error("Error setting the mesh kernel type to '"+std::string(kernelName)+"'.");
      return nullptr;
  }

  return Py_BuildValue("s", kernelName);
}

//-----------------
// Mesh_get_kernel
//-----------------
//
PyDoc_STRVAR(Mesh_get_kernel_doc,
  "get_kernel()  \n\ 
   \n\
   Get the meshing kernel. \n\
   \n\
   Args: \n\
       None \n\
   Returns: The name (str) of the mesh kernel. \n\
");

static PyObject *  
Mesh_get_kernel(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Check the current mesh kernel.
  auto kernelType = meshObject->GetMeshKernel();
  if (kernelType == SM_KT_INVALID ) {
      api.error("The mesh kernel is not set.");
      return nullptr;
  } 

  auto kernelName = cvMeshObject::GetKernelName( kernelType );
  return Py_BuildValue("s",kernelName);
}

//-------------
// Mesh_print 
//-------------
//
PyDoc_STRVAR(Mesh_print_doc,
  "print()  \n\ 
   \n\
   Print ???  \n\
   \n\
");

static PyObject * 
Mesh_print(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg); 
      return nullptr;
  }

  if (meshObject->pyPrint() != SV_OK) {
      api.error("Error printing the mesh.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------
// Mesh_update
//-------------
//
//[TODO:DaveP] Is this used?
//
PyDoc_STRVAR(Mesh_update_doc,
  "update()  \n\ 
   \n\
   Update the mesh. \n\
   \n\
");

static PyObject * 
Mesh_update(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  auto meshObject = CheckMesh(api, self); 
  if (meshObject == nullptr) { 
      return nullptr;
  }

  if (meshObject->Update() != SV_OK) {
      api.error("Error updating the mesh.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-----------------------
// Mesh_set_solid_kernel 
//-----------------------
//
PyDoc_STRVAR(Mesh_set_solid_kernel_doc,
  "set_solid_kernel(kernel)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     kernel (str): The name of the solid modeling kernel to set. Valid kernel names are: ??? \n\
");

static PyObject * 
Mesh_set_solid_kernel(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *kernelName;
  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  // Check for a current valid kernel.
  auto kernel = SolidModel_KernelT_StrToEnum( kernelName );
  if (kernel == SM_KT_INVALID) {
      api.error("The mesh kernel is not set.");
      return nullptr;
  }

  meshObject->SetSolidModelKernel(kernel);
  return Py_BuildValue("s",kernelName);
}

// ---------------------------
// Mesh_write_metis_adjacency
// ---------------------------
//
PyDoc_STRVAR(Mesh_write_metis_adjacency_doc,
  "write_metis_adjacency(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Mesh_write_metis_adjacency(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *file_name;
  if (!PyArg_ParseTuple(args, api.format, &file_name)) {
      return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg); 
      return nullptr;
  }

  if (meshObject->WriteMetisAdjacency(file_name) != SV_OK) {
      api.error("Error writing the mesh adjacency to the file '"+std::string(file_name)+"'.");
      return nullptr;
  } 

  return SV_PYTHON_OK;
}

// ----------------------
// cvMesh_GetPolyDataMtd
// ----------------------

PyDoc_STRVAR(Mesh_get_polydata_doc,
" Mesh.get_polydata(name)  \n\ 
  \n\
  Add the mesh meshObjectetry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the meshObjectetry. \n\
");

static PyObject * 
Mesh_get_polydata(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *resultName;
  if (!PyArg_ParseTuple(args, api.format, &resultName)) {
      return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The repository object '" + std::string(resultName) + "' already exists.");
      return nullptr;
  }

  // Get the cvPolyData:
  auto pd = meshObject->GetPolyData();
  if (pd == NULL) {
      api.error("Could not get polydata for the mesh.");
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
      delete pd;
      api.error("Could not add the polydata to the repository.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------
// Mesh_get_solid
//----------------
//
PyDoc_STRVAR(Mesh_get_solid_doc,
" Mesh.Mesh_get_solid(name)  \n\ 
  \n\
  Add the mesh solid model meshObjectetry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the solid model meshObjectetry. \n\
");

static PyObject * 
Mesh_get_solid(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *resultName;
  if (!PyArg_ParseTuple(args, api.format, &resultName)) {
      return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The repository object '" + std::string(resultName) + "' already exists.");
      return nullptr;
  }

  // Get the cvPolyData:
  auto pd = meshObject->GetSolid();
  if (pd == NULL) {
      api.error("Could not get polydata for the mesh solid model.");
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
      delete pd;
      api.error("Could not add the polydata to the repository.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-----------------------
// Mesh_set_vtk_polydata
//-----------------------
//
PyDoc_STRVAR(Mesh_set_vtk_polydata_doc,
" Mesh.set_vtk_polydata(name)  \n\ 
  \n\
  Add the mesh solid model meshObjectetry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the solid model meshObjectetry. \n\
");

static PyObject * 
Mesh_set_vtk_polydata(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  auto obj = gRepository->GetObject(objName);
  if (obj == nullptr) {
      api.error("The Mesh object '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  auto type = gRepository->GetType(objName);
  if (type != POLY_DATA_T) {
      api.error("The mesh object '" + std::string(objName)+"' is not of type cvPolyData.");
      return nullptr;
  }

  auto pd = ((cvPolyData *)obj)->GetVtkPolyData();
  if (pd == NULL) {
      api.error("Could not get polydata for the mesh.");
      return nullptr;
  }

  // Set the vtkPolyData.
  if (!meshObject->SetVtkPolyDataObject(pd)) {
      api.error("Could not set the polydata for the mesh.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-----------------------------
// Mesh_get_unstructured_grid 
//-----------------------------
//
PyDoc_STRVAR(Mesh_get_unstructured_grid_doc,
" get_unstructured_grid(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_get_unstructured_grid(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *resultName;
  if (!PyArg_ParseTuple(args, api.format, &resultName)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( resultName ) ) {
      api.error("The repository object '" + std::string(resultName) + "' already exists.");
      return nullptr;
  }

  // Get the cvUnstructuredGrid:
  auto ug = meshObject->GetUnstructuredGrid();
  if (ug == NULL) {
      api.error("Could not get the unstructured grid for the mesh.");
      return nullptr;
  }

  // Register the result:
  if ( !( gRepository->Register( resultName, ug ) ) ) {
    delete ug;
    api.error("Could not add the unstructured grid to the repository.");
    return nullptr;
  }

  return SV_PYTHON_OK;
}

// --------------------------
// cvMesh_GetFacePolyDataMtd
// --------------------------

PyDoc_STRVAR(Mesh_get_face_polydata_doc,
" get_face_polydata(name)  \n\ 
  \n\
  ???  \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_get_face_polydata(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("si", PyRunTimeErr, __func__); 
  char *resultName;
  int face;
  if(!PyArg_ParseTuple(args, api.format, &resultName, &face)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(resultName)) {
      api.error("The repository object '" + std::string(resultName) + "' already exists.");
      return nullptr;
  }

  // Get the cvPolyData:
  auto pd = meshObject->GetFacePolyData(face);
  if (pd == NULL) {
    api.error("Could not get mesh polydata for the face '" + std::to_string(face) + "'.");
    return nullptr;
  }

  // Register the result:
  if ( !( gRepository->Register( resultName, pd ) ) ) {
      delete pd;
      api.error("Could not add the polydata to the repository.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//
// LogOn
//
PyDoc_STRVAR(Mesh_logging_on_doc,
" logging_on(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_logging_on(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *logFileName;

  if (!PyArg_ParseTuple(args, api.format, &logFileName)) {
    return api.argsError();
  }

  auto meshKernel = cvMeshSystem::GetCurrentKernel();
  if (meshKernel == NULL) {
      api.error("The mesh kernel is not set.");
      return nullptr;
  }

  // Read in the results file.
  if (meshKernel->LogOn(logFileName) == SV_ERROR) {
      api.error("Unable to open the log file '" + std::string(logFileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

// ------------------
// cvMesh_LogoffCmd
// ------------------

PyDoc_STRVAR(Mesh_logging_off_doc,
" logging_off(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_logging_off(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshKernel = cvMeshSystem::GetCurrentKernel();
  if (meshKernel == NULL) {
      api.error("The mesh kernel is not set.");
      return nullptr;
  }

  if (meshKernel->LogOff() == SV_ERROR) {
      api.error("Unable to turn off logging."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------------
// Mesh_set_meshing_options
//--------------------------
//
PyDoc_STRVAR(Mesh_set_meshing_options_doc,
" set_meshing_options(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_set_meshing_options(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sO", PyRunTimeErr, __func__); 
  char *optionName;
  PyObject* valueList;

  if (!PyArg_ParseTuple(args, api.format, &optionName, &valueList)) {
    return api.argsError();
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  int numValues = PyList_Size(valueList);
  std::vector<double> values;
  for (int j = 0; j < numValues; j++) {
    values.push_back(PyFloat_AsDouble(PyList_GetItem(valueList,j)));
  }

  // [TODO:DaveP] The SetMeshOptions() function does not return an error
  // if the option is not recognized.
  //
  if (meshObject->SetMeshOptions(optionName, numValues, values.data()) == SV_ERROR) {
    api.error("Error setting meshing options.");
    return nullptr;
  }

  return SV_PYTHON_OK;
}

//
// LoadModel
//

PyDoc_STRVAR(Mesh_load_model_doc,
" Mesh_load_model(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_load_model(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *FileName;

  if (!PyArg_ParseTuple(args, api.format, &FileName)) {
    return api.argsError();
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  // Read in the solid model file.
  if (meshObject->LoadModel(FileName) == SV_ERROR) {
      api.error("Error loading solid model from the file '" + std::string(FileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------------
// Mesh_get_boundary_faces 
//-------------------------
//
PyDoc_STRVAR(Mesh_get_boundary_faces_doc,
" Mesh_get_boundary_faces(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_get_boundary_faces(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__); 
  double angle = 0.0;

  if (!PyArg_ParseTuple(args, api.format, &angle)) {
    return api.argsError();
    
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  if (meshObject->GetBoundaryFaces(angle) != SV_OK) {
      api.error("Error getting boundary faces for angle '" + std::to_string(angle) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------
// Mesh_load_mesh
//----------------
//
PyDoc_STRVAR(Mesh_load_mesh_doc,
" load_mesh(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_load_mesh(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|s", PyRunTimeErr, __func__); 
  char *FileName;
  char *SurfFileName = 0;

  if (!PyArg_ParseTuple(args, api.format, &FileName, &SurfFileName)) {
    return api.argsError();
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  // Read in the mesh file.
  if (meshObject->LoadMesh(FileName,SurfFileName) == SV_ERROR) {
      api.error("Error reading in a mesh from the file '" + std::string(FileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

PyDoc_STRVAR(Mesh_write_stats_doc,
" write_stats(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_write_stats(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *fileName;

  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  if (meshObject->WriteStats(fileName) == SV_ERROR) {
      api.error("Error writing mesh statistics to the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------
// Mesh_adapt
//------------
//
PyDoc_STRVAR(Mesh_adapt_doc,
" adapt(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_adapt(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  if (meshObject->Adapt() != SV_OK) {
      api.error("Error performing adapt mesh operation."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------
// Mesh_write
//------------
//
PyDoc_STRVAR(Mesh_write_doc,
" write(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_write(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__); 
  char *fileName;
  int smsver = 0;

  if (!PyArg_ParseTuple(args, api.format, &fileName, &smsver)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Write the mesh to a file.
  if (meshObject->WriteMesh(fileName,smsver) == SV_ERROR) {
      api.error("Error writing the mesh to the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------
//cvMesh_NewMeshMtd
//-------------------

PyDoc_STRVAR(Mesh_new_mesh_doc,
" new_mesh(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_new_mesh( pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  if (meshObject->NewMesh() == SV_ERROR) {
      api.error("Error creating a new mesh."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------
// Mesh_generate_mesh 
//--------------------
//
PyDoc_STRVAR(Mesh_generate_mesh_doc,
" generate_mesh()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
Mesh_generate_mesh(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }


  if (meshObject->GenerateMesh() == SV_ERROR) {
      api.error("Error generating a mesh."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------------------
// Mesh_set_sphere_refinement
//----------------------------
//
PyDoc_STRVAR(Mesh_set_sphere_refinement_doc,
" set_sphere_refinement(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_set_sphere_refinement(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ddO", PyRunTimeErr, __func__); 
  double size;
  PyObject* centerArg;
  double radius;

  if (!PyArg_ParseTuple(args, api.format, &size, &radius, &centerArg)) {
    return api.argsError();
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(centerArg, emsg)) {
      api.error("The sphere center argument " + emsg);
      return nullptr;
  }

  double center[3];
  for (int i = 0; i < 3; i++) {
      center[i] = PyFloat_AsDouble(PyList_GetItem(centerArg,i));
  }

  if (meshObject->SetSphereRefinement(size, radius, center) == SV_ERROR )   {
      std::string centerStr = "  center=(" + std::to_string(center[0]) + ", " + std::to_string(center[1]) + ", " + 
        std::to_string(center[2]) + ")"; 
      api.error("Error setting sphere refinement: radius=" + std::to_string(radius) + 
        "  size= " + std::to_string(size)+ centerStr + ".");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

// -------------------------------
// cvMesh_SetSizeFunctionBasedMeshMtd
// -------------------------------

PyDoc_STRVAR(Mesh_set_size_function_based_mesh_doc,
" set_size_function_based_mesh(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_set_size_function_based_mesh(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ds", PyRunTimeErr, __func__); 
  char *functionName;
  double size;

  if (!PyArg_ParseTuple(args, api.format, &size, &functionName)) {
    return api.argsError();
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  if (meshObject->SetSizeFunctionBasedMesh(size,functionName) == SV_ERROR) {
      api.error("Error setting size function. size=" + std::to_string(size) + "  function=" + std::string(functionName)+"."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}


// ---------------------------------
// cvMesh_SetCylinderRefinementMtd
// ---------------------------------

PyDoc_STRVAR(Mesh_set_cylinder_refinement_doc,
" set_cylinder_refinement(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_set_cylinder_refinement(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ddOO", PyRunTimeErr, __func__); 
  double size;
  PyObject* centerArg;
  PyObject* normalArg;
  double radius;
  double length;

  if (!PyArg_ParseTuple(args, api.format, &size, &radius, &length, &centerArg, &normalArg)) {
    return api.argsError();
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  std::string emsg;
  if (!svPyUtilCheckPointData(centerArg, emsg)) {
      api.error("The cylinder center argument " + emsg);
      return nullptr;
  }
  if (!svPyUtilCheckPointData(normalArg, emsg)) {
      api.error("The normal argument " + emsg);
      return nullptr;
  }

  double center[3];
  for (int i = 0; i < 3; i++) {
      center[i] = PyFloat_AsDouble(PyList_GetItem(centerArg,i));
  }

  double normal[3];
  for (int i = 0; i < 3; i++) {
      normal[i] = PyFloat_AsDouble(PyList_GetItem(normalArg,i));
  }

  if (meshObject->SetCylinderRefinement(size,radius,length,center,normal) == SV_ERROR ) {
      std::string centerStr = "  center=(" + std::to_string(center[0]) + ", " + std::to_string(center[1]) + ", " + 
        std::to_string(center[2]) + ")";
      std::string normalStr = "  normal=(" + std::to_string(normal[0]) + ", " + std::to_string(normal[1]) + ", " + 
        std::to_string(normal[2]) + ")";
      api.error("Error setting cylinder refinement parameters. size=" + std::to_string(size) + 
        "  length=" + std::to_string(length) + "  radius=" + std::to_string(radius) +
        centerStr + normalStr + ".");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------------
// Mesh_set_boundary_layer
//-------------------------

PyDoc_STRVAR(Mesh_set_boundary_layer_doc,
" set_boundary_layer(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_set_boundary_layer(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("iiiiO", PyRunTimeErr, __func__); 
  int type = 0;
  int id = 0;
  int side = 0;
  int nL = 0;
  PyObject* Hlist;

  if (!PyArg_ParseTuple(args, api.format, &type, &id, &side, &nL, &Hlist)) {
    return api.argsError();
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  // Parse coordinate lists:
  int numH = PyList_Size(Hlist);
  auto H = new double [numH];
  for (int i=0; i<numH;i++) {
    H[i] = PyFloat_AsDouble(PyList_GetItem(Hlist,i));
  }

  if (meshObject->SetBoundaryLayer(type,id,side,nL,H) == SV_ERROR ) {
      delete [] H;
      api.error("Error setting boundary layer.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

// ----------------------------
// cvMesh_SetWallsMtd
// ----------------------------

PyDoc_STRVAR(Mesh_set_walls_doc,
" set_walls(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_set_walls(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__); 
  PyObject* wallsList;

  if (!PyArg_ParseTuple(args, api.format, &wallsList)) {
    return api.argsError();
  }

  auto meshObject = CheckMesh(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  // Parse coordinate lists:
  int numWalls=PyList_Size(wallsList);
  int *walls = new int [numWalls];

  for (int i=0;i<numWalls;i++) {
    walls[i]=PyLong_AsLong(PyList_GetItem(wallsList,i));
  }

  if (meshObject->SetWalls(numWalls,walls) == SV_ERROR ) {
      delete [] walls;
      api.error("Error setting walls.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------------
// Mesh_get_model_face_info 
//--------------------------
//
PyDoc_STRVAR(Mesh_get_model_face_info_doc,
" get_model_face_info(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesh_get_model_face_info(pyMeshObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshObject = CheckMesh(api, self); 
  if (meshObject == nullptr) { 
      return nullptr;
  }

  char info[99999];
  meshObject->GetModelFaceInfo(info);

  return Py_BuildValue("s",info);
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyMeshObject();
#elif PYTHON_MAJOR_VERSION == 3
PyMODINIT_FUNC PyInit_pyMeshObject();
#endif

// ----------
// cvMesh_Init
// ----------

int Mesh_pyInit()
{
#if PYTHON_MAJOR_VERSION == 2
  initpyMeshObject();
#elif PYTHON_MAJOR_VERSION == 3
  PyInit_pyMeshObject();
#endif
  return SV_OK;
}

//-------------------
// pyMeshObject_init
//-------------------
// This is the __init__() method for the Mesh class. 
//
// This function is used to initialize an object after it is created.
//
static int 
pyMeshObject_init(pyMeshObject* self, PyObject* args)
{
  fprintf(stdout,"pyMeshObject initialized.\n");
  return SV_OK;
}

//----------------------------
// Define API function names
//----------------------------

static PyMethodDef pyMeshObject_methods[] = {

  { "adapt",  (PyCFunction)Mesh_adapt, METH_VARARGS, Mesh_adapt_doc },

  { "generate_mesh", (PyCFunction)Mesh_generate_mesh, METH_VARARGS, Mesh_generate_mesh_doc },

  { "get_boundary_faces", (PyCFunction)Mesh_get_boundary_faces, METH_VARARGS, Mesh_get_boundary_faces_doc },

  { "get_face_polydata", (PyCFunction)Mesh_get_face_polydata, METH_VARARGS, Mesh_get_face_polydata_doc },

  { "get_kernel", (PyCFunction)Mesh_get_kernel, METH_VARARGS, Mesh_get_kernel_doc },

  {"get_mesh", (PyCFunction)Mesh_get_mesh, METH_VARARGS, Mesh_get_mesh_doc },

  { "get_model_face_info", (PyCFunction)Mesh_get_model_face_info, METH_VARARGS, Mesh_get_model_face_info_doc },

  { "get_polydata", (PyCFunction)Mesh_get_polydata, METH_VARARGS, Mesh_get_polydata_doc },

  { "get_solid", (PyCFunction)Mesh_get_solid, METH_VARARGS, Mesh_get_solid_doc },

  { "get_unstructured_grid", (PyCFunction)Mesh_get_unstructured_grid, METH_VARARGS, Mesh_get_unstructured_grid_doc },

  { "load_mesh", (PyCFunction)Mesh_load_mesh, METH_VARARGS, Mesh_load_mesh_doc },

  { "load_model", (PyCFunction)Mesh_load_model, METH_VARARGS, Mesh_load_model_doc },

  { "new_mesh", (PyCFunction)Mesh_new_mesh, METH_VARARGS, Mesh_new_mesh_doc },

  {"new_object", (PyCFunction)Mesh_new_object, METH_VARARGS, Mesh_new_object_doc },

  { "print", (PyCFunction)Mesh_print, METH_VARARGS, Mesh_print_doc },

  { "set_boundary_layer", (PyCFunction)Mesh_set_boundary_layer, METH_VARARGS, NULL },

  { "set_cylinder_refinement", (PyCFunction)Mesh_set_cylinder_refinement, METH_VARARGS, Mesh_set_cylinder_refinement_doc },

  { "set_meshing_options", (PyCFunction)Mesh_set_meshing_options, METH_VARARGS, Mesh_set_meshing_options_doc },

  { "set_size_function_based_mesh", (PyCFunction)Mesh_set_size_function_based_mesh, METH_VARARGS, Mesh_set_size_function_based_mesh_doc },

  { "set_solid_kernel", (PyCFunction)Mesh_set_solid_kernel, METH_VARARGS, Mesh_set_solid_kernel_doc },

  { "set_sphere_refinement", (PyCFunction)Mesh_set_sphere_refinement, METH_VARARGS, Mesh_set_sphere_refinement_doc },

  { "set_vtk_polydata", (PyCFunction)Mesh_set_vtk_polydata, METH_VARARGS, Mesh_set_vtk_polydata_doc },

  { "set_walls", (PyCFunction)Mesh_set_walls, METH_VARARGS, Mesh_set_walls_doc },

  { "write_mesh", (PyCFunction)Mesh_write, METH_VARARGS, Mesh_write_doc },

  { "write_metis_adjacency", (PyCFunction)Mesh_write_metis_adjacency, METH_VARARGS, Mesh_write_metis_adjacency_doc },

  { "write_stats", (PyCFunction)Mesh_write_stats, METH_VARARGS, Mesh_write_stats_doc },

  {NULL,NULL}
};

static PyTypeObject pyMeshObjectType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "mesh.Mesh",               /* tp_name */
  sizeof(pyMeshObject),      /* tp_basicsize */
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
  "Mesh  objects",           /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  pyMeshObject_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)pyMeshObject_init,                            /* tp_init */
  0,                         /* tp_alloc */
  0,                  /* tp_new */
};


static PyMethodDef pyMeshObjectModule_methods[] =
{
  //{"mesh_newObject", (PyCFunction)cvMesh_NewObjectCmd,METH_VARARGS,NULL},

  {"logging_off", (PyCFunction)Mesh_logging_off,
      METH_NOARGS,
      Mesh_logging_off_doc
  },

  {"logging_on", 
      (PyCFunction)Mesh_logging_on,
      METH_VARARGS,
      Mesh_logging_on_doc
  },

  {"set_kernel", 
      (PyCFunction)Mesh_set_kernel,
       METH_VARARGS,
       Mesh_set_kernel_doc
  },

  {NULL, NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MODULE_NAME = "mesh";
static char* MODULE_MESH_CLASS = "Mesh";
static char* MODULE_EXCEPTION = "mesh.MeshException";
static char* MODULE_EXCEPTION_OBJECT = "MeshException";

PyDoc_STRVAR(Mesh_module_doc, "mesh module functions");

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

static struct PyModuleDef pyMeshObjectmodule = {
   m_base,
   MODULE_NAME, 
   Mesh_module_doc,
   perInterpreterStateSize, 
   pyMeshObjectModule_methods
};

//---------------------
// PyInit_pyMeshObject 
//---------------------
// The initialization function called by the Python interpreter when the module is loaded.
//
PyMODINIT_FUNC PyInit_pyMeshObject()
{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL) {
    gRepository = new cvRepository();
  }

  int (*kernel)(cvMeshObject::KernelType, cvMeshSystem*)=(&cvMeshSystem::RegisterKernel);

  if (Py_BuildValue("i",kernel)==nullptr) {
    fprintf(stdout,"Unable to create MeshSystemRegistrar\n");
    return SV_PYTHON_ERROR;
  }

  if(PySys_SetObject("MeshSystemRegistrar",Py_BuildValue("i",kernel))<0) {
    fprintf(stdout, "Unable to register MeshSystemRegistrar\n");
    return SV_PYTHON_ERROR;
  }

  // Initialize
  cvMeshSystem::SetCurrentKernel( cvMeshObject::KERNEL_INVALID );

  pyMeshObjectType.tp_new=PyType_GenericNew;

  if (PyType_Ready(&pyMeshObjectType)<0) {
    fprintf(stdout,"Error in pyMeshObjectType\n");
    return SV_PYTHON_ERROR;
  }

  auto module = PyModule_Create(&pyMeshObjectmodule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing mesh module.\n");
    return SV_PYTHON_ERROR;
  }

  // Add mesh.MeshException exception.
  PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  // Add 'Mesh' object.
  Py_INCREF(&pyMeshObjectType);
  PyModule_AddObject(module, MODULE_MESH_CLASS, (PyObject*)&pyMeshObjectType);

  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

//----------------
//initpyMeshObject
//----------------
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyMeshObject()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from cv_mesh_init\n");
  }
  int (*kernel)(cvMeshObject::KernelType, cvMeshSystem*)=(&cvMeshSystem::RegisterKernel);
  if (Py_BuildValue("i",kernel)==nullptr)
  {
    fprintf(stdout,"Unable to create MeshSystemRegistrar\n");
    return;

  }
  if(PySys_SetObject("MeshSystemRegistrar",Py_BuildValue("i",kernel))<0)
  {
    fprintf(stdout, "Unable to register MeshSystemRegistrar\n");
    return;

  }
  // Initialize
  cvMeshSystem::SetCurrentKernel( cvMeshObject::KERNEL_INVALID );

  pyMeshObjectType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&pyMeshObjectType)<0)
  {
    fprintf(stdout,"Error in pyMeshObjectType\n");
    return;

  }
  PyObject* pythonC;
  pythonC = Py_InitModule("pyMeshObject",pyMeshObjectModule_methods);

  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyMeshObject\n");
    return;

  }
  PyRunTimeErr = PyErr_NewException("pyMeshObject.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  Py_INCREF(&pyMeshObjectType);
  PyModule_AddObject(pythonC,"pyMeshObject",(PyObject*)&pyMeshObjectType);
  return;

}
#endif

