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

// The functions defined here implement the SV Python API mesh module Mesher class. 
// The 'Mesher' class used to store mesh data. 
//

#include "sv_TetGenMeshObject.h"

//----------------------
// PyMeshingMesherClass
//----------------------
//
typedef struct {
  PyObject_HEAD
  int id;
  SolidModel_KernelT modelKernel;
  cvMeshObject::KernelType mesherKernel;
  cvMeshObject* mesher;
} PyMeshingMesherClass;

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////


//---------------------
// CheckMesherLoadUpdate
//---------------------
//
static bool 
CheckMesherLoadUpdate(cvMeshObject *meshObject, std::string& msg) 
{
  if (meshObject == nullptr) {
      msg = "The Mesher object does not have meshObjectetry.";
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
// CheckMesher
//---------------
// Check if the mesh object has meshObjectetry.
//
// This is really used to set the error message 
// in a single place. 
//
static cvMeshObject *
CheckMesher(SvPyUtilApiFunction& api, PyMeshingMesherClass *self)
{
  auto meshObject = self->mesher;
  if (meshObject == NULL) {
      api.error("The Mesher object does not have meshObjectetry.");
      return nullptr;
  }

  return meshObject;
}

/////////////////////////////////////////////////////////////////
//              C l a s s   F u n c t i o n s                  //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the Mesher class. 


//--------------------------
// Mesher_load_model
//--------------------------
//
PyDoc_STRVAR(Mesher_load_model_doc,
  "load_model(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_load_model(PyMeshingMesherClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  static char *keywords[] = {"file_name", NULL};
  char *fileName;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &fileName)) {
    return api.argsError();
  }
  auto mesher = self->mesher;

  // Read in the solid model file.
  if (mesher->LoadModel(fileName) == SV_ERROR) {
      api.error("Error loading solid model from the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  Py_RETURN_NONE; 
}

//-----------------------------------
// Mesher_set_meshing_options
//-----------------------------------
//
PyDoc_STRVAR(Mesher_set_meshing_options_doc,
" set_meshing_options(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_set_meshing_options(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("O!", PyRunTimeErr, __func__); 
  char *optionName;
  PyObject* valueList;

  if (!PyArg_ParseTuple(args, api.format, &optionName, &valueList)) {
    return api.argsError();
  }
  auto mesher = self->mesher;

  int numValues = PyList_Size(valueList);
  std::vector<double> values;
  for (int j = 0; j < numValues; j++) {
    values.push_back(PyFloat_AsDouble(PyList_GetItem(valueList,j)));
  }

  // [TODO:DaveP] The SetMesherOptions() function does not return an error
  // if the option is not recognized.
  //

/*
  if (mesher->SetMesherOptions(optionName, numValues, values.data()) == SV_ERROR) {
    api.error("Error setting meshing options.");
    return nullptr;
  }
*/

  Py_RETURN_NONE; 
}

//----------------------------------------
// Mesher_set_solid_modeler_kernel 
//----------------------------------------
//
PyDoc_STRVAR(Mesher_set_solid_modeler_kernel_doc,
  "set_solid_kernel(kernel)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     kernel (str): The name of the solid modeling kernel to set. Valid kernel names are: ??? \n\
");

static PyObject * 
Mesher_set_solid_modeler_kernel(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *kernelName;
  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }
  auto mesher = self->mesher;

  // Check for a valid kernel name.
  //
  SolidModel_KernelT kernel = SolidKernel_NameToEnum(std::string(kernelName));

  if (kernel == SM_KT_INVALID) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." +
          " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  mesher->SetSolidModelKernel(kernel);
  Py_RETURN_NONE; 
}


//================================================  o l d  c l a s s   f u n c t i o n s ================================

#ifdef use_old_class_funcs

//---------------------------
// Mesher_write_metis_adjacency
//---------------------------
//
PyDoc_STRVAR(Mesher_write_metis_adjacency_doc,
  "write_metis_adjacency(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Mesher_write_metis_adjacency(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *file_name;
  if (!PyArg_ParseTuple(args, api.format, &file_name)) {
      return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMesherLoadUpdate(meshObject, emsg)) {
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
// cvMesher_GetPolyDataMtd
// ----------------------

PyDoc_STRVAR(Mesher_get_polydata_doc,
" Mesher.get_polydata(name)  \n\ 
  \n\
  Add the mesh meshObjectetry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the meshObjectetry. \n\
");

static PyObject * 
Mesher_get_polydata(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *resultName;
  if (!PyArg_ParseTuple(args, api.format, &resultName)) {
      return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMesherLoadUpdate(meshObject, emsg)) {
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
// Mesher_get_solid
//----------------
//
PyDoc_STRVAR(Mesher_get_solid_doc,
" Mesher.Mesher_get_solid(name)  \n\ 
  \n\
  Add the mesh solid model meshObjectetry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the solid model meshObjectetry. \n\
");

static PyObject * 
Mesher_get_solid(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *resultName;
  if (!PyArg_ParseTuple(args, api.format, &resultName)) {
      return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMesherLoadUpdate(meshObject, emsg)) {
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
// Mesher_set_vtk_polydata
//-----------------------
//
PyDoc_STRVAR(Mesher_set_vtk_polydata_doc,
" Mesher.set_vtk_polydata(name)  \n\ 
  \n\
  Add the mesh solid model meshObjectetry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the solid model meshObjectetry. \n\
");

static PyObject * 
Mesher_set_vtk_polydata(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMesherLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  auto obj = gRepository->GetObject(objName);
  if (obj == nullptr) {
      api.error("The Mesher object '"+std::string(objName)+"' is not in the repository.");
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
// Mesher_get_unstructured_grid 
//-----------------------------
//
PyDoc_STRVAR(Mesher_get_unstructured_grid_doc,
" get_unstructured_grid(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_get_unstructured_grid(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *resultName;
  if (!PyArg_ParseTuple(args, api.format, &resultName)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMesherLoadUpdate(meshObject, emsg)) {
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
// cvMesher_GetFacePolyDataMtd
// --------------------------

PyDoc_STRVAR(Mesher_get_face_polydata_doc,
" get_face_polydata(name)  \n\ 
  \n\
  ???  \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_get_face_polydata(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("si", PyRunTimeErr, __func__); 
  char *resultName;
  int face;
  if(!PyArg_ParseTuple(args, api.format, &resultName, &face)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMesherLoadUpdate(meshObject, emsg)) {
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
PyDoc_STRVAR(Mesher_logging_on_doc,
" logging_on(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_logging_on(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *logFileName;

  if (!PyArg_ParseTuple(args, api.format, &logFileName)) {
    return api.argsError();
  }

  auto meshKernel = cvMesherSystem::GetCurrentKernel();
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
// cvMesher_LogoffCmd
// ------------------

PyDoc_STRVAR(Mesher_logging_off_doc,
" logging_off(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_logging_off(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshKernel = cvMesherSystem::GetCurrentKernel();
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



//-------------------------
// Mesher_get_boundary_faces 
//-------------------------
//
PyDoc_STRVAR(Mesher_get_boundary_faces_doc,
" Mesher_get_boundary_faces(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_get_boundary_faces(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__); 
  double angle = 0.0;

  if (!PyArg_ParseTuple(args, api.format, &angle)) {
    return api.argsError();
    
  }

  auto meshObject = CheckMesher(api, self);
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
// Mesher_load_mesh
//----------------
//
PyDoc_STRVAR(Mesher_load_mesh_doc,
" load_mesh(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_load_mesh(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|s", PyRunTimeErr, __func__); 
  char *FileName;
  char *SurfFileName = 0;

  if (!PyArg_ParseTuple(args, api.format, &FileName, &SurfFileName)) {
    return api.argsError();
  }

  auto meshObject = CheckMesher(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  // Read in the mesh file.
  if (meshObject->LoadMesher(FileName,SurfFileName) == SV_ERROR) {
      api.error("Error reading in a mesh from the file '" + std::string(FileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

PyDoc_STRVAR(Mesher_write_stats_doc,
" write_stats(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_write_stats(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *fileName;

  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMesherLoadUpdate(meshObject, emsg)) {
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
// Mesher_adapt
//------------
//
PyDoc_STRVAR(Mesher_adapt_doc,
" adapt(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_adapt(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMesherLoadUpdate(meshObject, emsg)) {
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
// Mesher_write
//------------
//
PyDoc_STRVAR(Mesher_write_doc,
" write(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_write(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__); 
  char *fileName;
  int smsver = 0;

  if (!PyArg_ParseTuple(args, api.format, &fileName, &smsver)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMesherLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Write the mesh to a file.
  if (meshObject->WriteMesher(fileName,smsver) == SV_ERROR) {
      api.error("Error writing the mesh to the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------
//cvMesher_NewMesherMtd
//-------------------

PyDoc_STRVAR(Mesher_new_mesh_doc,
" new_mesh(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_new_mesh( PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshObject = CheckMesher(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  if (meshObject->NewMesher() == SV_ERROR) {
      api.error("Error creating a new mesh."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------
// Mesher_generate_mesh 
//--------------------
//
PyDoc_STRVAR(Mesher_generate_mesh_doc,
" generate_mesh()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
Mesher_generate_mesh(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshObject = CheckMesher(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }


  if (meshObject->GenerateMesher() == SV_ERROR) {
      api.error("Error generating a mesh."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------------------
// Mesher_set_sphere_refinement
//----------------------------
//
PyDoc_STRVAR(Mesher_set_sphere_refinement_doc,
" set_sphere_refinement(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_set_sphere_refinement(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ddO", PyRunTimeErr, __func__); 
  double size;
  PyObject* centerArg;
  double radius;

  if (!PyArg_ParseTuple(args, api.format, &size, &radius, &centerArg)) {
    return api.argsError();
  }

  auto meshObject = CheckMesher(api, self);
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
// cvMesher_SetSizeFunctionBasedMesherMtd
// -------------------------------

PyDoc_STRVAR(Mesher_set_size_function_based_mesh_doc,
" set_size_function_based_mesh(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_set_size_function_based_mesh(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ds", PyRunTimeErr, __func__); 
  char *functionName;
  double size;

  if (!PyArg_ParseTuple(args, api.format, &size, &functionName)) {
    return api.argsError();
  }

  auto meshObject = CheckMesher(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  if (meshObject->SetSizeFunctionBasedMesher(size,functionName) == SV_ERROR) {
      api.error("Error setting size function. size=" + std::to_string(size) + "  function=" + std::string(functionName)+"."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}


// ---------------------------------
// cvMesher_SetCylinderRefinementMtd
// ---------------------------------

PyDoc_STRVAR(Mesher_set_cylinder_refinement_doc,
" set_cylinder_refinement(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_set_cylinder_refinement(PyMeshingMesherClass* self, PyObject* args)
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

  auto meshObject = CheckMesher(api, self);
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
// Mesher_set_boundary_layer
//-------------------------

PyDoc_STRVAR(Mesher_set_boundary_layer_doc,
" set_boundary_layer(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_set_boundary_layer(PyMeshingMesherClass* self, PyObject* args)
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

  auto meshObject = CheckMesher(api, self);
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
// cvMesher_SetWallsMtd
// ----------------------------

PyDoc_STRVAR(Mesher_set_walls_doc,
" set_walls(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_set_walls(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__); 
  PyObject* wallsList;

  if (!PyArg_ParseTuple(args, api.format, &wallsList)) {
    return api.argsError();
  }

  auto meshObject = CheckMesher(api, self);
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
// Mesher_get_model_face_info 
//--------------------------
//
PyDoc_STRVAR(Mesher_get_model_face_info_doc,
" get_model_face_info(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_get_model_face_info(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshObject = CheckMesher(api, self); 
  if (meshObject == nullptr) { 
      return nullptr;
  }

  char info[99999];
  meshObject->GetModelFaceInfo(info);

  return Py_BuildValue("s",info);
}

#endif // use_old_class_funcs

////////////////////////////////////////////////////////
//           C l a s s    D e f i n i t i o n         //
////////////////////////////////////////////////////////

static char* MESH_GENERATOR_CLASS = "Mesher";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* MESH_GENERATOR_MODULE_CLASS = "mesh.Mesher";

PyDoc_STRVAR(MesherClass_doc, "mesher class methods.");

//------------------------
// PyMeshingMesherMethods
//------------------------
//
static PyMethodDef PyMeshingMesherMethods[] = {

  { "load_model", (PyCFunction)Mesher_load_model, METH_VARARGS|METH_KEYWORDS, Mesher_load_model_doc },

  { "set_meshing_options", (PyCFunction)Mesher_set_meshing_options, METH_VARARGS, Mesher_set_meshing_options_doc },

  { "set_solid_modeler_kernel", (PyCFunction)Mesher_set_solid_modeler_kernel, METH_VARARGS, Mesher_set_solid_modeler_kernel_doc},


//================================================  o l d  c l a s s   f u n c t i o n s ================================

#ifdef use_old_class_funcs
  { "adapt",  (PyCFunction)Mesher_adapt, METH_VARARGS, Mesher_adapt_doc },

  { "generate_mesh", (PyCFunction)Mesher_generate_mesh, METH_VARARGS, Mesher_generate_mesh_doc },

  { "get_boundary_faces", (PyCFunction)Mesher_get_boundary_faces, METH_VARARGS, Mesher_get_boundary_faces_doc },

  { "get_face_polydata", (PyCFunction)Mesher_get_face_polydata, METH_VARARGS, Mesher_get_face_polydata_doc },

  { "get_kernel", (PyCFunction)Mesher_get_kernel, METH_VARARGS, Mesher_get_kernel_doc },

  {"get_mesh", (PyCFunction)Mesher_get_mesh, METH_VARARGS, Mesher_get_mesh_doc },

  { "get_model_face_info", (PyCFunction)Mesher_get_model_face_info, METH_VARARGS, Mesher_get_model_face_info_doc },

  { "get_polydata", (PyCFunction)Mesher_get_polydata, METH_VARARGS, Mesher_get_polydata_doc },

  { "get_solid", (PyCFunction)Mesher_get_solid, METH_VARARGS, Mesher_get_solid_doc },

  { "get_unstructured_grid", (PyCFunction)Mesher_get_unstructured_grid, METH_VARARGS, Mesher_get_unstructured_grid_doc },

  { "load_mesh", (PyCFunction)Mesher_load_mesh, METH_VARARGS, Mesher_load_mesh_doc },

  { "new_mesh", (PyCFunction)Mesher_new_mesh, METH_VARARGS, Mesher_new_mesh_doc },

  {"new_object", (PyCFunction)Mesher_new_object, METH_VARARGS, Mesher_new_object_doc },

  { "print", (PyCFunction)Mesher_print, METH_VARARGS, Mesher_print_doc },

  { "set_boundary_layer", (PyCFunction)Mesher_set_boundary_layer, METH_VARARGS, NULL },

  { "set_cylinder_refinement", (PyCFunction)Mesher_set_cylinder_refinement, METH_VARARGS, Mesher_set_cylinder_refinement_doc },

  { "set_size_function_based_mesh", (PyCFunction)Mesher_set_size_function_based_mesh, METH_VARARGS, Mesher_set_size_function_based_mesh_doc },

  { "set_sphere_refinement", (PyCFunction)Mesher_set_sphere_refinement, METH_VARARGS, Mesher_set_sphere_refinement_doc },

  { "set_vtk_polydata", (PyCFunction)Mesher_set_vtk_polydata, METH_VARARGS, Mesher_set_vtk_polydata_doc },

  { "set_walls", (PyCFunction)Mesher_set_walls, METH_VARARGS, Mesher_set_walls_doc },

  { "write_mesh", (PyCFunction)Mesher_write, METH_VARARGS, Mesher_write_doc },

  { "write_metis_adjacency", (PyCFunction)Mesher_write_metis_adjacency, METH_VARARGS, Mesher_write_metis_adjacency_doc },

  { "write_stats", (PyCFunction)Mesher_write_stats, METH_VARARGS, Mesher_write_stats_doc },

#endif // use_old_class_funcs

  {NULL,NULL}
};


//--------------------------
// PyMeshingMesherClassType 
//--------------------------
// This is the definition of the Mesher class.
//
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject PyMeshingMesherClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  MESH_GENERATOR_MODULE_CLASS,
  sizeof(PyMeshingMesherClass)
};

// Include derived mesh generator classes.
#include "MeshingTetGen_PyClass.cxx"

//-----------------
// PyMesherCtorMap
//-----------------
// Define an object factory for creating Python Mesher derived objects.
//
// An entry for SM_KT_PARASOLID is added later in PyAPI_InitParasolid() 
// if the Parasolid interface is defined (by loading the Parasolid plugin).
//
using PyMesherCtorMapType = std::map<cvMeshObject::KernelType, std::function<PyObject*()>>;
PyMesherCtorMapType PyMesherCtorMap = {
  {cvMeshObject::KernelType::KERNEL_TETGEN, []()->PyObject* {return PyObject_CallObject((PyObject*)&PyMeshingTetGenClassType, NULL);}},
};

//----------------
// CreatePyMesher 
//----------------
//
static PyObject *
CreatePyMesher(cvMeshObject::KernelType kernel)
{
  std::cout << "[CreatePyMesher] ========== CreatePyMesher ==========" << std::endl;
  PyObject* mesher;

  try {
      mesher = PyMesherCtorMap[kernel]();
  } catch (const std::bad_function_call& except) {
      return nullptr;
  }

  return mesher;
}

//---------------------
// PyMeshingMesherInit
//---------------------
// This is the __init__() method for the Mesher class. 
//
// This function is used to initialize an object after it is created.
//
static int 
PyMeshingMesherInit(PyMeshingMesherClass* self, PyObject* args, PyObject *kwds)
{
  std::cout << "[PyMeshingMesherInit] " << std::endl;
  std::cout << "[PyMeshingMesherInit] ========== PyMeshingMesherInit ==========" << std::endl;
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, "Mesher");
  static int numObjs = 1;
  char* kernelName = nullptr;
  if (!PyArg_ParseTuple(args, "|s", &kernelName)) {
      return -1;
  }

/*
  std::cout << "[PyMeshingMesherInit] Kernel name: " << kernelName << std::endl;
  auto kernel = kernelNameEnumMap.at(std::string(kernelName));
  cvMeshObject* mesher;

  try {
      mesher = MesherCtorMap[kernel]();
  } catch (const std::bad_function_call& except) {
      api.error("The '" + std::string(kernelName) + "' kernel is not supported.");
      return -1;
  }
  */

  self->id = numObjs;
  //self->mesherKernel = kernel;
  //self->mesher = mesher;
  self->mesher = nullptr;
  return 0;
}

//--------------------
// PyMeshingMesherNew
//--------------------
// Object creation function, equivalent to the Python __new__() method. 
//
static PyObject *
PyMeshingMesherNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyMeshingMesherNew] New Python Mesher object." << std::endl;
/*
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, "Mesher");
  char* kernelName = nullptr;
  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  cvMeshObject::KernelType kernel;

  try {
      kernel = kernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." +
          " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }
*/

  auto self = (PyMeshingMesherClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->id = 1;
  }

  return (PyObject*)self;
}

//------------------------
// PyMeshingMesherDealloc 
//------------------------
//
static void
PyMeshingMesherDealloc(PyMeshingMesherClass* self)
{
  std::cout << "[PyMeshingMesherDealloc] Free PyMeshingMesher: " << self->id << std::endl;
  //delete self->solidModel;
  Py_TYPE(self)->tp_free(self);
}

//---------------------
// SetMesherTypeFields 
//---------------------
// Set the Python type object fields that stores Mesher data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetMesherTypeFields(PyTypeObject& meshType)
{
  // Doc string for this type.
  meshType.tp_doc = MesherClass_doc;
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  meshType.tp_new = PyMeshingMesherNew;
  //meshType.tp_new = PyType_GenericNew,
  meshType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  meshType.tp_init = (initproc)PyMeshingMesherInit;
  meshType.tp_dealloc = (destructor)PyMeshingMesherDealloc;
  meshType.tp_methods = PyMeshingMesherMethods;
};

//-----------------
// CreateMesherType 
//------------------
static PyMeshingMesherClass *
CreateMesherType()
{
  return PyObject_New(PyMeshingMesherClass, &PyMeshingMesherClassType);
}

//----------------
// PyCreateMesher 
//----------------
//
static PyObject *
PyCreateMesher(cvMeshObject::KernelType kernel)
{
  std::cout << "[PyCreateMesher] ========== PyCreateMesher ==========" << std::endl;
  PyObject* mesher;

  try {
      mesher = PyMesherCtorMap[kernel]();
  } catch (...) {
      return nullptr;
  }

  return mesher;
}
