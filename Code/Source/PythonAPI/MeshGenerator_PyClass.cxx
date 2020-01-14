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

// The functions defined here implement the SV Python API mesh module MeshGenerator class. 
// The 'MeshGenerator' class used to store mesh data. The 'MeshGenerator' class cannot be imported 
// and must be used prefixed by the module name. For example
//
//     mesh = mesh.Mesh()
//

//----------------------
// PyMeshGeneratorClass
//----------------------
//
typedef struct {
  PyObject_HEAD
  int id;
  SolidModel_KernelT modelKernel;
  cvMeshObject::KernelType meshKernel;
  cvMeshObject* mesher;
} PyMeshGeneratorClass;

//----------------------
// MeshGeneratorCtorMap
//----------------------
// Define an object factory for creating cvMeshObject objects.
//
// An entry for SM_KT_PARASOLID is added later in PyAPI_InitParasolid() 
// if the Parasolid interface is defined (by loading the Parasolid plugin).
//
using MeshGeneratorCtorMapType = std::map<cvMeshObject::KernelType, std::function<cvMeshObject*()>>;
MeshGeneratorCtorMapType MeshGeneratorCtorMap = {
    {cvMeshObject::KernelType::KERNEL_TETGEN, []() -> cvMeshObject* { return new cvTetGenMeshObject(); } },
};


//////////////////////////////////////////////////////
//          U t i l i t y   F u n c t i o n s       //
//////////////////////////////////////////////////////

//---------------------
// CheckMeshGeneratorLoadUpdate
//---------------------
//
static bool 
CheckMeshGeneratorLoadUpdate(cvMeshObject *meshObject, std::string& msg) 
{
  if (meshObject == nullptr) {
      msg = "The MeshGenerator object does not have meshObjectetry.";
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
// CheckMeshGenerator
//---------------
// Check if the mesh object has meshObjectetry.
//
// This is really used to set the error message 
// in a single place. 
//
static cvMeshObject *
CheckMeshGenerator(SvPyUtilApiFunction& api, PyMeshGeneratorClass *self)
{
  auto meshObject = self->mesh;
  if (meshObject == NULL) {
      api.error("The MeshGenerator object does not have meshObjectetry.");
      return nullptr;
  }

  return meshObject;
}

/////////////////////////////////////////////////////////////////
//              C l a s s   F u n c t i o n s                  //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the MeshGenerator class. 



//================================================  o l d  c l a s s   f u n c t i o n s ================================

#ifdef use_old_class_funcs

//-----------------------
// MeshGenerator_set_solid_kernel 
//-----------------------
//
PyDoc_STRVAR(MeshGenerator_set_solid_kernel_doc,
  "set_solid_kernel(kernel)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     kernel (str): The name of the solid modeling kernel to set. Valid kernel names are: ??? \n\
");

static PyObject * 
MeshGenerator_set_solid_kernel(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *kernelName;
  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  auto meshObject = CheckMeshGenerator(api, self);
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

//---------------------------
// MeshGenerator_write_metis_adjacency
//---------------------------
//
PyDoc_STRVAR(MeshGenerator_write_metis_adjacency_doc,
  "write_metis_adjacency(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
MeshGenerator_write_metis_adjacency(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *file_name;
  if (!PyArg_ParseTuple(args, api.format, &file_name)) {
      return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshGeneratorLoadUpdate(meshObject, emsg)) {
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
// cvMeshGenerator_GetPolyDataMtd
// ----------------------

PyDoc_STRVAR(MeshGenerator_get_polydata_doc,
" MeshGenerator.get_polydata(name)  \n\ 
  \n\
  Add the mesh meshObjectetry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the meshObjectetry. \n\
");

static PyObject * 
MeshGenerator_get_polydata(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *resultName;
  if (!PyArg_ParseTuple(args, api.format, &resultName)) {
      return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshGeneratorLoadUpdate(meshObject, emsg)) {
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
// MeshGenerator_get_solid
//----------------
//
PyDoc_STRVAR(MeshGenerator_get_solid_doc,
" MeshGenerator.MeshGenerator_get_solid(name)  \n\ 
  \n\
  Add the mesh solid model meshObjectetry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the solid model meshObjectetry. \n\
");

static PyObject * 
MeshGenerator_get_solid(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *resultName;
  if (!PyArg_ParseTuple(args, api.format, &resultName)) {
      return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshGeneratorLoadUpdate(meshObject, emsg)) {
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
// MeshGenerator_set_vtk_polydata
//-----------------------
//
PyDoc_STRVAR(MeshGenerator_set_vtk_polydata_doc,
" MeshGenerator.set_vtk_polydata(name)  \n\ 
  \n\
  Add the mesh solid model meshObjectetry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the solid model meshObjectetry. \n\
");

static PyObject * 
MeshGenerator_set_vtk_polydata(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshGeneratorLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  auto obj = gRepository->GetObject(objName);
  if (obj == nullptr) {
      api.error("The MeshGenerator object '"+std::string(objName)+"' is not in the repository.");
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
// MeshGenerator_get_unstructured_grid 
//-----------------------------
//
PyDoc_STRVAR(MeshGenerator_get_unstructured_grid_doc,
" get_unstructured_grid(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_get_unstructured_grid(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *resultName;
  if (!PyArg_ParseTuple(args, api.format, &resultName)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshGeneratorLoadUpdate(meshObject, emsg)) {
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
// cvMeshGenerator_GetFacePolyDataMtd
// --------------------------

PyDoc_STRVAR(MeshGenerator_get_face_polydata_doc,
" get_face_polydata(name)  \n\ 
  \n\
  ???  \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_get_face_polydata(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("si", PyRunTimeErr, __func__); 
  char *resultName;
  int face;
  if(!PyArg_ParseTuple(args, api.format, &resultName, &face)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshGeneratorLoadUpdate(meshObject, emsg)) {
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
PyDoc_STRVAR(MeshGenerator_logging_on_doc,
" logging_on(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_logging_on(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *logFileName;

  if (!PyArg_ParseTuple(args, api.format, &logFileName)) {
    return api.argsError();
  }

  auto meshKernel = cvMeshGeneratorSystem::GetCurrentKernel();
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
// cvMeshGenerator_LogoffCmd
// ------------------

PyDoc_STRVAR(MeshGenerator_logging_off_doc,
" logging_off(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_logging_off(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshKernel = cvMeshGeneratorSystem::GetCurrentKernel();
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
// MeshGenerator_set_meshing_options
//--------------------------
//
PyDoc_STRVAR(MeshGenerator_set_meshing_options_doc,
" set_meshing_options(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_set_meshing_options(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sO", PyRunTimeErr, __func__); 
  char *optionName;
  PyObject* valueList;

  if (!PyArg_ParseTuple(args, api.format, &optionName, &valueList)) {
    return api.argsError();
  }

  auto meshObject = CheckMeshGenerator(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  int numValues = PyList_Size(valueList);
  std::vector<double> values;
  for (int j = 0; j < numValues; j++) {
    values.push_back(PyFloat_AsDouble(PyList_GetItem(valueList,j)));
  }

  // [TODO:DaveP] The SetMeshGeneratorOptions() function does not return an error
  // if the option is not recognized.
  //
  if (meshObject->SetMeshGeneratorOptions(optionName, numValues, values.data()) == SV_ERROR) {
    api.error("Error setting meshing options.");
    return nullptr;
  }

  return SV_PYTHON_OK;
}

//
// LoadModel
//

PyDoc_STRVAR(MeshGenerator_load_model_doc,
" MeshGenerator_load_model(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_load_model(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *FileName;

  if (!PyArg_ParseTuple(args, api.format, &FileName)) {
    return api.argsError();
  }

  auto meshObject = CheckMeshGenerator(api, self);
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
// MeshGenerator_get_boundary_faces 
//-------------------------
//
PyDoc_STRVAR(MeshGenerator_get_boundary_faces_doc,
" MeshGenerator_get_boundary_faces(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_get_boundary_faces(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__); 
  double angle = 0.0;

  if (!PyArg_ParseTuple(args, api.format, &angle)) {
    return api.argsError();
    
  }

  auto meshObject = CheckMeshGenerator(api, self);
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
// MeshGenerator_load_mesh
//----------------
//
PyDoc_STRVAR(MeshGenerator_load_mesh_doc,
" load_mesh(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_load_mesh(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|s", PyRunTimeErr, __func__); 
  char *FileName;
  char *SurfFileName = 0;

  if (!PyArg_ParseTuple(args, api.format, &FileName, &SurfFileName)) {
    return api.argsError();
  }

  auto meshObject = CheckMeshGenerator(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  // Read in the mesh file.
  if (meshObject->LoadMeshGenerator(FileName,SurfFileName) == SV_ERROR) {
      api.error("Error reading in a mesh from the file '" + std::string(FileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

PyDoc_STRVAR(MeshGenerator_write_stats_doc,
" write_stats(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_write_stats(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__); 
  char *fileName;

  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshGeneratorLoadUpdate(meshObject, emsg)) {
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
// MeshGenerator_adapt
//------------
//
PyDoc_STRVAR(MeshGenerator_adapt_doc,
" adapt(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_adapt(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshGeneratorLoadUpdate(meshObject, emsg)) {
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
// MeshGenerator_write
//------------
//
PyDoc_STRVAR(MeshGenerator_write_doc,
" write(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_write(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__); 
  char *fileName;
  int smsver = 0;

  if (!PyArg_ParseTuple(args, api.format, &fileName, &smsver)) {
    return api.argsError();
  }

  std::string emsg;
  auto meshObject = self->meshObject;
  if (!CheckMeshGeneratorLoadUpdate(meshObject, emsg)) {
      api.error(emsg);
      return nullptr;
  }

  // Write the mesh to a file.
  if (meshObject->WriteMeshGenerator(fileName,smsver) == SV_ERROR) {
      api.error("Error writing the mesh to the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------
//cvMeshGenerator_NewMeshGeneratorMtd
//-------------------

PyDoc_STRVAR(MeshGenerator_new_mesh_doc,
" new_mesh(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_new_mesh( PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshObject = CheckMeshGenerator(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  if (meshObject->NewMeshGenerator() == SV_ERROR) {
      api.error("Error creating a new mesh."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------
// MeshGenerator_generate_mesh 
//--------------------
//
PyDoc_STRVAR(MeshGenerator_generate_mesh_doc,
" generate_mesh()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
MeshGenerator_generate_mesh(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshObject = CheckMeshGenerator(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }


  if (meshObject->GenerateMeshGenerator() == SV_ERROR) {
      api.error("Error generating a mesh."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------------------
// MeshGenerator_set_sphere_refinement
//----------------------------
//
PyDoc_STRVAR(MeshGenerator_set_sphere_refinement_doc,
" set_sphere_refinement(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_set_sphere_refinement(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ddO", PyRunTimeErr, __func__); 
  double size;
  PyObject* centerArg;
  double radius;

  if (!PyArg_ParseTuple(args, api.format, &size, &radius, &centerArg)) {
    return api.argsError();
  }

  auto meshObject = CheckMeshGenerator(api, self);
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
// cvMeshGenerator_SetSizeFunctionBasedMeshGeneratorMtd
// -------------------------------

PyDoc_STRVAR(MeshGenerator_set_size_function_based_mesh_doc,
" set_size_function_based_mesh(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_set_size_function_based_mesh(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ds", PyRunTimeErr, __func__); 
  char *functionName;
  double size;

  if (!PyArg_ParseTuple(args, api.format, &size, &functionName)) {
    return api.argsError();
  }

  auto meshObject = CheckMeshGenerator(api, self);
  if (meshObject == nullptr) {
      return nullptr;
  }

  if (meshObject->SetSizeFunctionBasedMeshGenerator(size,functionName) == SV_ERROR) {
      api.error("Error setting size function. size=" + std::to_string(size) + "  function=" + std::string(functionName)+"."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}


// ---------------------------------
// cvMeshGenerator_SetCylinderRefinementMtd
// ---------------------------------

PyDoc_STRVAR(MeshGenerator_set_cylinder_refinement_doc,
" set_cylinder_refinement(name)  \n\ 
  \n\
  Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_set_cylinder_refinement(PyMeshGeneratorClass* self, PyObject* args)
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

  auto meshObject = CheckMeshGenerator(api, self);
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
// MeshGenerator_set_boundary_layer
//-------------------------

PyDoc_STRVAR(MeshGenerator_set_boundary_layer_doc,
" set_boundary_layer(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_set_boundary_layer(PyMeshGeneratorClass* self, PyObject* args)
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

  auto meshObject = CheckMeshGenerator(api, self);
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
// cvMeshGenerator_SetWallsMtd
// ----------------------------

PyDoc_STRVAR(MeshGenerator_set_walls_doc,
" set_walls(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_set_walls(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__); 
  PyObject* wallsList;

  if (!PyArg_ParseTuple(args, api.format, &wallsList)) {
    return api.argsError();
  }

  auto meshObject = CheckMeshGenerator(api, self);
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
// MeshGenerator_get_model_face_info 
//--------------------------
//
PyDoc_STRVAR(MeshGenerator_get_model_face_info_doc,
" get_model_face_info(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
MeshGenerator_get_model_face_info(PyMeshGeneratorClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__); 
  auto meshObject = CheckMeshGenerator(api, self); 
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

static char* MESH_GENERATOR_CLASS = "MeshGenerator";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* MESH_GENERATOR_MODULE_CLASS = "mesh.MeshGenerator";

PyDoc_STRVAR(MeshGeneratorClass_doc, "mesh class methods.");

//---------------
// PyMeshGeneratorMethods
//---------------
//
static PyMethodDef PyMeshGeneratorMethods[] = {

//================================================  o l d  c l a s s   f u n c t i o n s ================================

#ifdef use_old_class_funcs
  { "adapt",  (PyCFunction)MeshGenerator_adapt, METH_VARARGS, MeshGenerator_adapt_doc },

  { "generate_mesh", (PyCFunction)MeshGenerator_generate_mesh, METH_VARARGS, MeshGenerator_generate_mesh_doc },

  { "get_boundary_faces", (PyCFunction)MeshGenerator_get_boundary_faces, METH_VARARGS, MeshGenerator_get_boundary_faces_doc },

  { "get_face_polydata", (PyCFunction)MeshGenerator_get_face_polydata, METH_VARARGS, MeshGenerator_get_face_polydata_doc },

  { "get_kernel", (PyCFunction)MeshGenerator_get_kernel, METH_VARARGS, MeshGenerator_get_kernel_doc },

  {"get_mesh", (PyCFunction)MeshGenerator_get_mesh, METH_VARARGS, MeshGenerator_get_mesh_doc },

  { "get_model_face_info", (PyCFunction)MeshGenerator_get_model_face_info, METH_VARARGS, MeshGenerator_get_model_face_info_doc },

  { "get_polydata", (PyCFunction)MeshGenerator_get_polydata, METH_VARARGS, MeshGenerator_get_polydata_doc },

  { "get_solid", (PyCFunction)MeshGenerator_get_solid, METH_VARARGS, MeshGenerator_get_solid_doc },

  { "get_unstructured_grid", (PyCFunction)MeshGenerator_get_unstructured_grid, METH_VARARGS, MeshGenerator_get_unstructured_grid_doc },

  { "load_mesh", (PyCFunction)MeshGenerator_load_mesh, METH_VARARGS, MeshGenerator_load_mesh_doc },

  { "load_model", (PyCFunction)MeshGenerator_load_model, METH_VARARGS, MeshGenerator_load_model_doc },

  { "new_mesh", (PyCFunction)MeshGenerator_new_mesh, METH_VARARGS, MeshGenerator_new_mesh_doc },

  {"new_object", (PyCFunction)MeshGenerator_new_object, METH_VARARGS, MeshGenerator_new_object_doc },

  { "print", (PyCFunction)MeshGenerator_print, METH_VARARGS, MeshGenerator_print_doc },

  { "set_boundary_layer", (PyCFunction)MeshGenerator_set_boundary_layer, METH_VARARGS, NULL },

  { "set_cylinder_refinement", (PyCFunction)MeshGenerator_set_cylinder_refinement, METH_VARARGS, MeshGenerator_set_cylinder_refinement_doc },

  { "set_meshing_options", (PyCFunction)MeshGenerator_set_meshing_options, METH_VARARGS, MeshGenerator_set_meshing_options_doc },

  { "set_size_function_based_mesh", (PyCFunction)MeshGenerator_set_size_function_based_mesh, METH_VARARGS, MeshGenerator_set_size_function_based_mesh_doc },

  { "set_solid_kernel", (PyCFunction)MeshGenerator_set_solid_kernel, METH_VARARGS, MeshGenerator_set_solid_kernel_doc },

  { "set_sphere_refinement", (PyCFunction)MeshGenerator_set_sphere_refinement, METH_VARARGS, MeshGenerator_set_sphere_refinement_doc },

  { "set_vtk_polydata", (PyCFunction)MeshGenerator_set_vtk_polydata, METH_VARARGS, MeshGenerator_set_vtk_polydata_doc },

  { "set_walls", (PyCFunction)MeshGenerator_set_walls, METH_VARARGS, MeshGenerator_set_walls_doc },

  { "write_mesh", (PyCFunction)MeshGenerator_write, METH_VARARGS, MeshGenerator_write_doc },

  { "write_metis_adjacency", (PyCFunction)MeshGenerator_write_metis_adjacency, METH_VARARGS, MeshGenerator_write_metis_adjacency_doc },

  { "write_stats", (PyCFunction)MeshGenerator_write_stats, METH_VARARGS, MeshGenerator_write_stats_doc },

#endif // use_old_class_funcs

  {NULL,NULL}
};

//---------------------
// PyMeshGeneratorInit
//---------------------
// This is the __init__() method for the MeshGenerator class. 
//
// This function is used to initialize an object after it is created.
//
static int 
PyMeshGeneratorInit(PyMeshGeneratorClass* self, PyObject* args, PyObject *kwds)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, "MeshGenerator");
  static int numObjs = 1;
  std::cout << "[PyMeshGeneratorClassInit] New PyMeshGeneratorClass object: " << numObjs << std::endl;
  char* kernelName = nullptr;
  if (!PyArg_ParseTuple(args, "s", &kernelName)) {
      return -1;
  }
  std::cout << "[PyMeshGeneratorClasslInit] Kernel name: " << kernelName << std::endl;
  auto kernel = kernelNameEnumMap.at(std::string(kernelName));
  cvMeshObject* mesher;

  try {
      mesher = MeshGeneratorCtorMap[kernel]();
  } catch (const std::bad_function_call& except) {
      api.error("The '" + std::string(kernelName) + "' kernel is not supported.");
      return -1;
  }

  self->id = numObjs;
  self->kernel = kernel;
  self->mesher = mesher;
  return 0;
}

//--------------------------
// PyMeshGeneratorClassType 
//--------------------------
// This is the definition of the MeshGenerator class.
//
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject PyMeshGeneratorClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  MESH_GENERATOR_MODULE_CLASS,
  sizeof(PyMeshGeneratorClass)
};

//--------------------
// PyMeshGeneratorNew
//--------------------
// Object creation function, equivalent to the Python __new__() method. 
//
static PyObject *
PyMeshGeneratorNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyMeshGeneratorNew] New Python MeshGenerator object." << std::endl;
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, "MeshGenerator");
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

  auto self = (PyMeshGeneratorClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->id = 1;
  }

  return (PyObject*)self;
}

//------------------------
// PyMeshGeneratorDealloc 
//------------------------
//
static void
PyMeshGeneratorDealloc(PyMeshGeneratorClass* self)
{
  std::cout << "[PyMeshGeneratorDealloc] Free PyMeshGenerator: " << self->id << std::endl;
  //delete self->solidModel;
  Py_TYPE(self)->tp_free(self);
}

//----------------------------
// SetMeshGeneratorTypeFields 
//----------------------------
// Set the Python type object fields that stores MeshGenerator data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetMeshGeneratorTypeFields(PyTypeObject& meshType)
{
  // Doc string for this type.
  meshType.tp_doc = MeshGeneratorClass_doc;
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  meshType.tp_new = PyMeshGeneratorNew;
  //meshType.tp_new = PyType_GenericNew,
  meshType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  meshType.tp_init = (initproc)PyMeshGeneratorInit;
  meshType.tp_dealloc = (destructor)PyMeshGeneratorDealloc;
  meshType.tp_methods = PyMeshGeneratorMethods;
};

//-------------------------
// CreateMeshGeneratorType 
//-------------------------
static PyMeshGeneratorClass *
CreateMeshGeneratorType()
{
  return PyObject_New(PyMeshGeneratorClass, &PyMeshGeneratorClassType);
}

