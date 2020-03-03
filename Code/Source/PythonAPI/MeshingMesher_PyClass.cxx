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

// The functions defined here implement the SV Python API meshing module 
// 'Mesher' mesh generator class. The class used is a base classs for 
// mesh generators (e.g. TetGen and MeshSim). 
//

#include <functional>

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

//-----------------------
// CheckMesherLoadUpdate
//-----------------------
//
static bool 
CheckMesherLoadUpdate(cvMeshObject* mesher, std::string& msg) 
{
  if (mesher->GetMeshLoaded() == 0) {
      if (mesher->Update() == SV_ERROR) {
          msg = "No mesh has been generated.";
          return false;
      }
  }

  return true;
}

/////////////////////////////////////////////////////////////////
//              C l a s s   F u n c t i o n s                  //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the Mesher class. 

//--------------
// Mesher_adapt
//--------------
//
PyDoc_STRVAR(Mesher_adapt_doc,
" adapt()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
Mesher_adapt(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  std::string emsg;

  auto mesher = self->mesher;

  if (mesher->Adapt() != SV_OK) {
      api.error("Error performing adapt mesh operation."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------------------------
// Mesher_compute_model_boundary_faces 
//-------------------------------------
//
PyDoc_STRVAR(Mesher_compute_model_boundary_faces_doc,
" compute_model_boundary_faces(angle)  \n\ 
  \n\
  Compute the boundary faces for the solid model. \n\
  \n\
  Args:                                    \n\
    angle (double): The angle in degrees used to determine the boundary faces of the solid model. \n\
");

static PyObject * 
Mesher_compute_model_boundary_faces(PyMeshingMesherClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__); 
  static char *keywords[] = {"angle", NULL};
  double angle = 0.0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &angle)) {
    return api.argsError();
  }

  auto mesher = self->mesher; 

  if (mesher->GetBoundaryFaces(angle) != SV_OK) {
      api.error("Error getting boundary faces for angle '" + std::to_string(angle) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------------
// Mesher_generate_mesh 
//----------------------
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
  auto mesher = self->mesher;

  if (mesher->GenerateMesh() == SV_ERROR) {
      api.error("Error generating a mesh."); 
      return nullptr;
  }

  Py_RETURN_NONE; 
}

//--------------------------
// Mesher_get_face_polydata 
//--------------------------

PyDoc_STRVAR(Mesher_get_face_polydata_doc,
" get_face_polydata(face_id)  \n\ 
  \n\
  Get the mesh face vtk polydata for the given face ID.  \n\
  \n\
  Args:                                    \n\
    face_id (int): The face ID to get the polydata for. \n\
");

static PyObject * 
Mesher_get_face_polydata(PyMeshingMesherClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__); 
  static char *keywords[] = {"face_id", NULL};
  int faceID;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &faceID)) {
    return api.argsError();
  }

  std::string emsg;
  auto mesher = self->mesher;
  if (!CheckMesherLoadUpdate(mesher, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Get the cvPolyData:
  auto cvPolydata = mesher->GetFacePolyData(faceID);
  if (cvPolydata == NULL) {
    api.error("Could not get mesh polydata for the face ID '" + std::to_string(faceID) + "'.");
    return nullptr;
  }

  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata = cvPolydata->GetVtkPolyData();
  return svPyUtilGetVtkObject(api, polydata); 
}

//-----------------
// Mesher_get_mesh 
//-----------------
//
PyDoc_STRVAR(Mesher_get_mesh_doc,
" get_mesh()  \n\ 
  \n\
  Get the mesh that has been generated. \n\
  \n\
  Args:                                    \n\
");

static PyObject * 
Mesher_get_mesh(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 

  std::string emsg;
  auto mesher = self->mesher;
  if (!CheckMesherLoadUpdate(mesher, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Get the cvUnstructuredGrid:
  auto mesh = mesher->GetUnstructuredGrid();
  if (mesh == NULL) {
      api.error("Could not get the unstructured grid for the mesh.");
      return nullptr;
  }

  auto vtkUnstructuredGrid = mesh->GetVtkUnstructuredGrid();
  return vtkPythonUtil::GetObjectFromPointer(vtkUnstructuredGrid);
}

//----------------------------
// Mesher_get_model_face_info 
//----------------------------
//
PyDoc_STRVAR(Mesher_get_model_face_info_doc,
" get_model_face_info()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
Mesher_get_model_face_info(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto mesher = self->mesher; 
  // [TODO:DaveP] Yuk!
  char info[99999];
  mesher->GetModelFaceInfo(info);
  return Py_BuildValue("s",info);
}

//---------------------------
// Mesher_get_model_polydata 
//---------------------------
//
PyDoc_STRVAR(Mesher_get_model_polydata_doc,
" get_model_polydata  \n\ 
  \n\
  Get the vtk polydata for the mesh solid model. \n\
  \n\
");

static PyObject * 
Mesher_get_model_polydata(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 

  std::string emsg;
  auto mesher = self->mesher;
  if (!CheckMesherLoadUpdate(mesher, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Get the cvPolyData:
  auto cvPolydata = mesher->GetSolid();
  if (cvPolydata == NULL) {
      api.error("Could not get polydata for the mesh solid model.");
      return nullptr;
  }

  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata = cvPolydata->GetVtkPolyData();
  return svPyUtilGetVtkObject(api, polydata); 
}

//------------------
// Mesher_load_mesh
//------------------
//
PyDoc_STRVAR(Mesher_load_mesh_doc,
" load_mesh(file_name)  \n\ 
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
  char *fileName;
  char *surfFileName = 0;

  if (!PyArg_ParseTuple(args, api.format, &fileName, &surfFileName)) {
    return api.argsError();
  }

  auto mesher = self->mesher; 

  // Read in the mesh file.
  if (mesher->LoadMesh(fileName, surfFileName) == SV_ERROR) {
      api.error("Error reading in a mesh from the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  Py_RETURN_NONE; 
}

//-------------------
// Mesher_load_model
//-------------------
//
PyDoc_STRVAR(Mesher_load_model_doc,
  "load_model(file_name)  \n\ 
  \n\
  Load a solid model from a file into the mesher. \n\
  \n\
  Args:                                    \n\
    file_name (str): Name in the solid model file. \n\
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
      api.error("Error loading a solid model from the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  Py_RETURN_NONE; 
}

//--------------------
// Mesher_get_surface 
//--------------------

PyDoc_STRVAR(Mesher_get_surface_doc,
" get_surface()  \n\ 
  \n\
  Get the mesh surface as VTK polydata. \n\
  \n\
");

static PyObject * 
Mesher_get_surface(PyMeshingMesherClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 

  std::string emsg;
  auto mesher = self->mesher;
  if (!CheckMesherLoadUpdate(mesher, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Get the cvPolyData:
  auto cvPolydata = mesher->GetPolyData();
  if (cvPolydata == NULL) {
      api.error("Could not get polydata for the mesh.");
      return nullptr;
  }

  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata = cvPolydata->GetVtkPolyData();
  return svPyUtilGetVtkObject(api, polydata); 
}

//---------------------------
// Mesher_set_boundary_layer
//---------------------------

PyDoc_STRVAR(Mesher_set_boundary_layer_options_doc,
" set_boundary_layer(number_of_layers, constant_thickness, thickness_factor, layer_decreasing_ratio)  \n\ 
  \n\
  Set the options for boundary layer meshing. \n\
  \n\
  Args:                                    \n\
    number_of_layers (int): The number of boundary layers to create. \n\
    constant_thickness (bool): If True then the boundary layers will have a constant thickness. \n\
    thickness_factor (float):  \n\
    layer_decreasing_ratio (float):  \n\
");

static PyObject * 
Mesher_set_boundary_layer_options(PyMeshingMesherClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("iO!dd", PyRunTimeErr, __func__); 
  static char *keywords[] = {"number_of_layers", "constant_thickness", "thickness_factor", "layer_decreasing_ratio", NULL};
  int numLayers;
  int constant_thickness; 
  double thickness_factor;
  double layer_decreasing_ratio;

  PyObject* layerRatioArg;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &numLayers, &PyBool_Type, &constant_thickness, &thickness_factor, 
        &layer_decreasing_ratio)) { 
    return api.argsError();
  }

  // Set the options for boundary layer meshing.
  //
  // type, id and side are not used.
  //
  int type = 0;
  int id = 0;
  int side = 0;
  double paramValues[3] = { thickness_factor, layer_decreasing_ratio, static_cast<double>(constant_thickness) };
  if (self->mesher->SetBoundaryLayer(type, id, side, numLayers, paramValues) == SV_ERROR) {
      api.error("Error setting boundary layer.");
      return nullptr;
  }

  Py_RETURN_NONE; 
}

//----------------------------
// Mesher_set_meshing_options
//----------------------------
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

//---------------------------------
// Mesher_set_solid_modeler_kernel 
//---------------------------------
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

//------------------
// Mesher_set_walls 
//------------------

PyDoc_STRVAR(Mesher_set_walls_doc,
" set_walls(face_ids)  \n\ 
  \n\
  Set the given faces to be of type wall. \n\
  \n\
  Args:                                    \n\
    face_ids (list[int]): The face IDs to set to type wall. \n\
");

static PyObject * 
Mesher_set_walls(PyMeshingMesherClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("O!", PyRunTimeErr, __func__); 
  static char *keywords[] = {"face_ids", NULL};
  PyObject* faceIDsList;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &PyList_Type, &faceIDsList)) {
    return api.argsError();
  }

  int numIDs = PyList_Size(faceIDsList);
  if (numIDs == 0) { 
      api.error("The 'face_ids' list argument is empty.");
      return nullptr;
  }

  // Get face IDs. 
  //
  std::vector<int> faceIDs;
  for (int i = 0; i < numIDs; i++) {
      auto item = PyList_GetItem(faceIDsList, i);
      if (!PyLong_Check(item)) {
          api.error("The 'face_ids' argument is not a list of integers.");
          return nullptr;
      }
      faceIDs.push_back(PyLong_AsLong(item));
  } 

  auto mesher = self->mesher;

  if (mesher->SetWalls(numIDs, faceIDs.data()) == SV_ERROR) {
      api.error("Error setting walls.");
      return nullptr;
  }

  Py_RETURN_NONE; 
}

//-------------------
// Mesher_write_mesh
//-------------------
//
PyDoc_STRVAR(Mesher_write_mesh_doc,
" write_mesh(file_name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    file_name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Mesher_write_mesh(PyMeshingMesherClass* self, PyObject* args, PyObject* kwargs)
{
  std::cout << "[Mesher_write_mesh] ========== Mesher_write_mesh ==========" << std::endl;
  auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__); 
  static char *keywords[] = {"file_name", NULL};
  char *fileName;
  int smsver = 0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &fileName, &smsver)) {
    return api.argsError();
  }

  std::string emsg;
  auto mesher = self->mesher;
  if (!CheckMesherLoadUpdate(mesher, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  std::cout << "[Mesher_write_mesh] fileName: " << fileName << std::endl;
  // Write the mesh to a file.
  if (mesher->WriteMesh(fileName,smsver) == SV_ERROR) {
      api.error("Error writing the mesh to the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

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


#endif // use_old_class_funcs


////////////////////////////////////////////////////////
//           C l a s s    D e f i n i t i o n         //
////////////////////////////////////////////////////////

static char* MESHING_MESHER_CLASS = "Mesher";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* MESHING_MESHER_MODULE_CLASS = "mesh.Mesher";

PyDoc_STRVAR(MesherClass_doc, "mesher class methods.");

//------------------------
// PyMeshingMesherMethods
//------------------------
//
static PyMethodDef PyMeshingMesherMethods[] = {

  // [TODO:DaveP] Adapt crashes so don't expose it.
  //{ "adapt",  (PyCFunction)Mesher_adapt, METH_VARARGS, Mesher_adapt_doc },

  { "compute_model_boundary_faces", (PyCFunction)Mesher_compute_model_boundary_faces, METH_VARARGS|METH_KEYWORDS, Mesher_compute_model_boundary_faces_doc},

  { "generate_mesh", (PyCFunction)Mesher_generate_mesh, METH_VARARGS|METH_KEYWORDS, Mesher_generate_mesh_doc},

  { "get_face_polydata", (PyCFunction)Mesher_get_face_polydata, METH_VARARGS|METH_KEYWORDS, Mesher_get_face_polydata_doc },

  { "get_mesh", (PyCFunction)Mesher_get_mesh, METH_VARARGS|METH_KEYWORDS, Mesher_get_mesh_doc},

  { "get_model_face_info", (PyCFunction)Mesher_get_model_face_info, METH_VARARGS, Mesher_get_model_face_info_doc },

  { "get_model_polydata", (PyCFunction)Mesher_get_model_polydata, METH_VARARGS, Mesher_get_model_polydata_doc },

  { "get_surface", (PyCFunction)Mesher_get_surface, METH_VARARGS, Mesher_get_surface_doc },

  { "load_mesh", (PyCFunction)Mesher_load_mesh, METH_VARARGS, Mesher_load_mesh_doc },

  { "load_model", (PyCFunction)Mesher_load_model, METH_VARARGS|METH_KEYWORDS, Mesher_load_model_doc },

  { "set_boundary_layer_options", (PyCFunction)Mesher_set_boundary_layer_options, METH_VARARGS|METH_KEYWORDS, Mesher_set_boundary_layer_options_doc },

  { "set_meshing_options", (PyCFunction)Mesher_set_meshing_options, METH_VARARGS, Mesher_set_meshing_options_doc },

  { "set_solid_modeler_kernel", (PyCFunction)Mesher_set_solid_modeler_kernel, METH_VARARGS, Mesher_set_solid_modeler_kernel_doc},

  { "set_walls", (PyCFunction)Mesher_set_walls, METH_VARARGS|METH_KEYWORDS, Mesher_set_walls_doc },

  { "write_mesh", (PyCFunction)Mesher_write_mesh, METH_VARARGS|METH_KEYWORDS, Mesher_write_mesh_doc },


//================================================  o l d  c l a s s   f u n c t i o n s ================================

#ifdef use_old_class_funcs
  { "get_kernel", (PyCFunction)Mesher_get_kernel, METH_VARARGS, Mesher_get_kernel_doc },

  { "set_cylinder_refinement", (PyCFunction)Mesher_set_cylinder_refinement, METH_VARARGS, Mesher_set_cylinder_refinement_doc },

  { "set_size_function_based_mesh", (PyCFunction)Mesher_set_size_function_based_mesh, METH_VARARGS, Mesher_set_size_function_based_mesh_doc },

  { "set_sphere_refinement", (PyCFunction)Mesher_set_sphere_refinement, METH_VARARGS, Mesher_set_sphere_refinement_doc },

  { "set_vtk_polydata", (PyCFunction)Mesher_set_vtk_polydata, METH_VARARGS, Mesher_set_vtk_polydata_doc },

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
  MESHING_MESHER_MODULE_CLASS,
  sizeof(PyMeshingMesherClass)
};

// Include derived mesh generator classes.
#include "MeshingTetGen_PyClass.cxx"
#include "MeshingMeshSim_PyClass.cxx"

//-----------------
// PyMesherCtorMap
//-----------------
// Define a factory for creating Python Mesher derived objects.
//
// An entry for KERNEL_MESHSIM is added later in PyAPI_InitMeshSim() 
// if the MeshSim interface is defined (by loading the MeshSim plugin).
//
using PyMesherCtorMapType = std::map<cvMeshObject::KernelType, std::function<PyObject*()>>;
PyMesherCtorMapType PyMesherCtorMap = {
  {cvMeshObject::KernelType::KERNEL_TETGEN, []()->PyObject* {return PyObject_CallObject((PyObject*)&PyMeshingTetGenClassType, NULL);}},
};

//-----------------------
// PyMesherCreateObject 
//-----------------------
// Create a Python mesher object for the given kernel.
//
static PyObject *
PyMesherCreateObject(cvMeshObject::KernelType kernel)
{
  std::cout << "[PyCreateMesher] ========== PyCreateMesher ==========" << std::endl;
  PyObject* mesher;

  try {
      mesher = PyMesherCtorMap[kernel]();
  } catch (...) {
      std::cout << "[PyCreateMesher] No mesh class for kernel " << kernel << std::endl;
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

