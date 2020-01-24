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
// 'Adaptive' mesh generator class. The class is used as a base classs for 
// adaptive mesh generators for TetGen and MeshSim. 
//
#include "sv_AdaptObject.h"

//------------------------
// PyMeshingAdaptiveClass 
//------------------------
//
typedef struct {
  PyObject_HEAD
  cvAdaptObject* adaptive_mesher;
  std::string name;
  int id;
} PyMeshingAdaptiveClass; 

//////////////////////////////////////////////////////
//              U t i l i t i e s                   //
//////////////////////////////////////////////////////

//----------------
// CheckAdaptMesh
//----------------
// Check if an adapt mesh object has been created. 
//
static cvAdaptObject *
CheckAdaptMesh(SvPyUtilApiFunction& api, PyMeshingAdaptiveClass* self)
{
  auto name = self->name;
  auto adapt = self->adaptive_mesher;
  if (adapt == NULL) {
      api.error("An adapt mesh object has not been created for '" + std::string(name) + "'.");
      return nullptr;
  }
  return adapt;
}

/////////////////////////////////////////////////////////////////
//              C l a s s   F u n c t i o n s                  //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the Adaptive class. 

//---------------------
// Adapt_check_options
//---------------------
//
PyDoc_STRVAR(Adapt_check_options_doc,
  "check_options() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_check_options(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  if (self->adaptive_mesher->CheckOptions() == SV_OK) {
      api.error("Error checking options.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------------
// Adapt_set_adapt_options 
//-------------------------
//
PyDoc_STRVAR(Adapt_set_adapt_options_doc,
  "set_adapt_options() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_set_adapt_options(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sd", PyRunTimeErr, __func__);
  char *flag = NULL;
  double value=0;

  if(!(PyArg_ParseTuple(args, api.format,&flag,&value))) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->SetAdaptOptions(flag,value) != SV_OK) {
      api.error("The options flag '"+ std::string(flag) + "' is not a valid."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}


//====================================================================== old methods =============================================//
#ifdef use_adapt_old_methods

//------------------
// Adapt_registrars
//------------------
// This routine is used for debugging the registrar/factory system.
//
PyDoc_STRVAR(Adapt_registrars_doc,
" registrars()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
Adapt_registrars(PyObject* self, PyObject* args)
{
  cvFactoryRegistrar *adaptObjectRegistrar = (cvFactoryRegistrar *) PySys_GetObject( "AdaptObjectRegistrar");

  char result[255];
  sprintf( result, "Adapt object registrar ptr -> %p\n", adaptObjectRegistrar );
  PyObject* pyList = PyList_New(6);
  PyList_SetItem(pyList,0,PyBytes_FromFormat(result));

  for (int i = 0; i < 5; i++) {
    sprintf( result, "GetFactoryMethodPtr(%i) = %p\n", i, (adaptObjectRegistrar->GetFactoryMethodPtr(i)));
    PyList_SetItem(pyList,i+1,PyBytes_FromFormat(result));
  }

  return pyList;
}

//--------------------
// Adapt_new_object
//--------------------
//
// [TODO:DaveP] pass in meshing kernel name. 
//
PyDoc_STRVAR(Adapt_new_object_doc,
  " new_object()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
Adapt_new_object(PyMeshingAdaptiveClass* self, PyObject* args)
{
/*
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *resultName = NULL;

  if (!PyArg_ParseTuple(args, api.format,&resultName)) {
      return api.argsError();
  }

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(resultName)) {
      api.error("The Mesh object '" + std::string(resultName) + "' is already in the repository.");
      return nullptr;
  }

  // Set the meshing kernel.
  //
  // [TODO:DaveP] get rid of using global.
  auto meshType = KERNEL_INVALID;
  auto kernelName = cvMeshSystem::GetCurrentKernelName();
  try {
        meshType = adaptKernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Invalid adaptive meshing kernel '" + std::string(kernelName) + "'. " + validKernelNames;
      api.error(msg);
      return nullptr;
  }

  // Create the adaptor object.
  auto adaptor = cvAdaptObject::DefaultInstantiateAdaptObject(meshType);
  if (adaptor == NULL) {
    api.error("Error creating the adaptive mesh object '" + std::string(resultName) + "'.");
    return nullptr;
  }

  // Register the solid:
  if (!gRepository->Register(resultName, adaptor)) {
      delete adaptor;
      api.error("Error adding the adaptive mesh object '" + std::string(resultName) + "' to the repository.");
      return nullptr;
  }

  Py_INCREF(adaptor);

  adaptive_mesher = adaptor;
  self->name = std::string(resultName);

  Py_DECREF(adaptor);
*/
  return SV_PYTHON_OK;
}

//-------------------------------------
// Adapt_create_internal_mesh_object 
//-------------------------------------
//
PyDoc_STRVAR(Adapt_create_internal_mesh_object_doc,
  " create_internal_mesh_object()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
Adapt_create_internal_mesh_object(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *meshFileName = NULL;
  char *solidFileName = NULL;

  if (!(PyArg_ParseTuple(args,"ss",&meshFileName,&solidFileName))) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->CreateInternalMeshObject(meshFileName,solidFileName) != SV_OK) {
      api.error("Error creating the internal mesh."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------
// Adapt_load_model 
//--------------------
//
PyDoc_STRVAR(Adapt_load_model_doc,
  "load_model() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_load_model(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *solidFileName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &solidFileName)) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->LoadModel(solidFileName) != SV_OK) {
      api.error("Error loading a model from the file '" + std::string(solidFileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------
// Adapt_load_mesh 
//-------------------
//
PyDoc_STRVAR(Adapt_load_mesh_doc,
  "load_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_load_mesh(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *meshFileName = NULL;

  if(!(PyArg_ParseTuple(args,"s",&meshFileName))) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->LoadMesh(meshFileName) != SV_OK) {
      api.error("Error loading a mesh from the file '" + std::string(meshFileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//---------------------------------
// Adapt_load_solution_from_file 
//---------------------------------
//
PyDoc_STRVAR(Adapt_load_solution_from_file_doc,
  "load_solution_from_file() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_load_solution_from_file(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fileName = NULL;

  if(!(PyArg_ParseTuple(args,"s",&fileName))) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->LoadSolutionFromFile(fileName) != SV_OK) {
      api.error("Error loading a solution from the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-----------------------------
// Adapt_load_ybar_from_file 
//-----------------------------
//
PyDoc_STRVAR(Adapt_load_ybar_from_file_doc,
  "load_ybar_from_file() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_load_ybar_from_file(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fileName = NULL;

  if(!(PyArg_ParseTuple(args,"s",&fileName))) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->LoadYbarFromFile(fileName) != SV_OK) {
      api.error("Error loading y bar from the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------------
// load_avg_speed_from_file  
//--------------------------
//
PyDoc_STRVAR(Adapt_load_avg_speed_from_file_doc,
  "load_avg_speed_from_file() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_load_avg_speed_from_file(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fileName = NULL;

  if(!(PyArg_ParseTuple(args,"s",&fileName))) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->LoadAvgSpeedFromFile(fileName) != SV_OK) {
      api.error("Error loading the average speed from the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------------------
// Adapt_load_hessian_from_file 
//--------------------------------
//
PyDoc_STRVAR(Adapt_load_hessian_from_file_doc,
  "load_hessian_from_file() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_load_hessian_from_file(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fileName = NULL;

  if(!(PyArg_ParseTuple(args,"s",&fileName))) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->LoadHessianFromFile(fileName) != SV_OK) {
      api.error("Error loading the Hessian from the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//---------------------------------
// Adapt_read_solution_from_mesh 
//---------------------------------
//
PyDoc_STRVAR(Adapt_read_solution_from_mesh_doc,
  "read_solution_from_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_read_solution_from_mesh(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->ReadSolutionFromMesh() != SV_OK) {
      api.error("Error reading the solution from the mesh.'");
      return nullptr;
  } 

  return SV_PYTHON_OK;
}

//-----------------------------
// Adapt_read_ybar_from_mesh 
//-----------------------------
//
PyDoc_STRVAR(Adapt_read_ybar_from_mesh_doc,
  "read_ybar_from_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_read_ybar_from_mesh(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->ReadYbarFromMesh() != SV_OK) {
      api.error("Error reading y bar from the mesh.'");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------------------------
// Adapt_read_avg_speed_from_mesh 
//----------------------------------
//
PyDoc_STRVAR(Adapt_read_avg_speed_from_mesh_doc,
  "read_avg_speed_from_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_read_avg_speed_from_mesh(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->ReadAvgSpeedFromMesh() == SV_OK) {
      api.error("Error reading average speed from the mesh.'");
      return nullptr;
  }

  return SV_PYTHON_OK;
}



//--------------------
// Adapt_set_metric 
//--------------------
//
PyDoc_STRVAR(Adapt_set_metric_doc,
  "set_metric() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_set_metric(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|ii", PyRunTimeErr, __func__);
  char *fileName = NULL;
  int option = -1;
  int strategy = -1;

  if(!PyArg_ParseTuple(args, api.format,&fileName, &option,&strategy)) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->SetMetric(fileName,option,strategy) != SV_OK) {
      api.error("Error setting metric.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------
// Adapt_setup_mesh 
//--------------------
//
PyDoc_STRVAR(Adapt_setup_mesh_doc,
  "setup_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_setup_mesh(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->SetupMesh() != SV_OK) {
      api.error("Error setting up mesh.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//---------------------
// Adapt_run_adaptor 
//---------------------
//
PyDoc_STRVAR(Adapt_run_adaptor_doc,
  "run_adaptor() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_run_adaptor(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->RunAdaptor() != SV_OK) {
      api.error("Error running adaptor.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------------
// Adapt_print_statistics 
//--------------------------
//
PyDoc_STRVAR(Adapt_print_statistics_doc,
  "print_statistics() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_print_statistics(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->PrintStats() != SV_OK) {
      api.error("Error printing statistics.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------------
// Adapt_get_adapted_mesh 
//--------------------------
//
PyDoc_STRVAR(Adapt_get_adapted_mesh_doc,
  "get_adapted_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_get_adapted_mesh(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->GetAdaptedMesh() != SV_OK) {
      api.error("Error getting adapted mesh.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//---------------------------
// Adapt_transfer_solution 
//---------------------------
//
PyDoc_STRVAR(Adapt_transfer_solution_doc,
  "transfer_solution() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_transfer_solution(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->TransferSolution() != SV_OK) {
      api.error("Error transferring solution.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------------
// Adapt_transfer_regions 
//--------------------------
//
PyDoc_STRVAR(Adapt_transfer_regions_doc,
  "transfer_regions() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_transfer_regions(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->TransferRegions() != SV_OK) {
      api.error("Error transferring regions.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-----------------------------
// Adapt_write_adapted_model 
//-----------------------------
//
PyDoc_STRVAR(Adapt_write_adapted_model_doc,
  "_write_adapted_model() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_write_adapted_model(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fileName = NULL;

  if(!PyArg_ParseTuple(args,api.format,&fileName)) { 
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->WriteAdaptedModel(fileName) != SV_OK) {
      api.error("Error writing model to the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------------------
// Adapt_write_adapted_mesh 
//----------------------------
//
PyDoc_STRVAR(Adapt_write_adapted_mesh_doc,
  "write_adapted_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_write_adapted_mesh(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fileName = NULL;

  if(!PyArg_ParseTuple(args,api.format,&fileName)) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->WriteAdaptedMesh(fileName) != SV_OK) {
      api.error("Error writing adapted mesh to the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------------------
// Adapt_write_adapted_solution 
//--------------------------------
//
PyDoc_STRVAR(Adapt_write_adapted_solution_doc,
  "write_adapted_solution() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
Adapt_write_adapted_solution(PyMeshingAdaptiveClass* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fileName = NULL;

  if(!PyArg_ParseTuple(args, api.format, &fileName)) {
      return api.argsError();
  }

  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->WriteAdaptedSolution(fileName) != SV_OK) {
      api.error("Error writing adapted solution to the file '" + std::string(fileName) + "'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

#endif  // ifdef use_adapt_old_methods

////////////////////////////////////////////////////////
//           C l a s s    D e f i n i t i o n         //
////////////////////////////////////////////////////////

static char* MESHING_ADAPTIVE_CLASS = "Adaptive";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* MESHING_ADAPTIVE_MODULE_CLASS = "meshing.Adaptive";

PyDoc_STRVAR(AdaptiveClass_doc, "Adaptive meshing methods.");

//-------------------------------
// PyMeshingAdaptiveClassMethods 
//-------------------------------
//
static PyMethodDef PyMeshingAdaptMethods[] = {

  { "check_options", (PyCFunction)Adapt_check_options, METH_VARARGS, Adapt_check_options_doc},

  // [DaveP] Set options for each adapt object ?
  // { "set_options", (PyCFunction)Adapt_set_adapt_options, METH_VARARGS, Adapt_set_adapt_options_doc},


// ================= old methods ============================================

#ifdef use_adapt_old_methods

  { "create_internal_mesh_object", (PyCFunction)Adapt_create_internal_mesh_object, METH_VARARGS, Adapt_create_internal_mesh_object_doc},

  { "get_adapted_mesh", (PyCFunction)Adapt_get_adapted_mesh,METH_VARARGS,Adapt_get_adapted_mesh_doc},

  { "load_avg_speed_from_file", (PyCFunction)Adapt_load_avg_speed_from_file,METH_VARARGS,Adapt_load_avg_speed_from_file_doc},

  { "load_hessian_from_file", (PyCFunction)Adapt_load_hessian_from_file,METH_VARARGS,Adapt_load_hessian_from_file_doc},

  { "load_model", (PyCFunction)Adapt_load_model,METH_VARARGS,Adapt_load_model_doc},

  { "load_mesh", (PyCFunction)Adapt_load_mesh, METH_VARARGS, Adapt_load_mesh_doc},

  { "load_solution_from_file", (PyCFunction)Adapt_load_solution_from_file, METH_VARARGS, Adapt_load_solution_from_file_doc},

  { "load_ybar_from_file", (PyCFunction)Adapt_load_ybar_from_file,METH_VARARGS,Adapt_load_ybar_from_file_doc},

  {"new_object", (PyCFunction)Adapt_new_object, METH_VARARGS, Adapt_new_object_doc},

  { "print_statistics", (PyCFunction)Adapt_print_statistics,METH_VARARGS,Adapt_print_statistics_doc},

  { "read_solution_from_mesh", (PyCFunction)Adapt_read_solution_from_mesh,METH_VARARGS,Adapt_read_solution_from_mesh_doc},

  { "read_ybar_from_mesh", (PyCFunction)Adapt_read_ybar_from_mesh,METH_VARARGS,Adapt_read_ybar_from_mesh_doc},

  { "read_avg_speed_from_mesh", (PyCFunction)Adapt_read_avg_speed_from_mesh,METH_VARARGS,Adapt_read_avg_speed_from_mesh_doc},

  { "run_adaptor", (PyCFunction)Adapt_run_adaptor,METH_VARARGS,Adapt_run_adaptor_doc},

  { "set_metric", (PyCFunction)Adapt_set_metric,METH_VARARGS,Adapt_set_metric_doc},

  { "setup_mesh", (PyCFunction)Adapt_setup_mesh,METH_VARARGS,Adapt_setup_mesh_doc},

  { "transfer_solution", (PyCFunction)Adapt_transfer_solution,METH_VARARGS,Adapt_transfer_solution_doc},

  { "transfer_regions", (PyCFunction)Adapt_transfer_regions,METH_VARARGS,Adapt_transfer_regions_doc},

  { "write_adapted_model", (PyCFunction)Adapt_write_adapted_model,METH_VARARGS,Adapt_write_adapted_model_doc},

  { "write_adapted_mesh", (PyCFunction)Adapt_write_adapted_mesh,METH_VARARGS,Adapt_write_adapted_mesh_doc},

  { "write_adapted_solution", (PyCFunction)Adapt_write_adapted_solution,METH_VARARGS,Adapt_write_adapted_solution_doc},

#endif 

  {NULL, NULL}

};

//----------------------------
// PyMeshingAdaptiveClassType 
//----------------------------
// This is the definition of the Python Adaptive class.
//
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject PyMeshingAdaptiveClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  MESHING_ADAPTIVE_MODULE_CLASS, 
  sizeof(PyMeshingAdaptiveClass)
};

// Include derived mesh generator classes.
#include "MeshingTetGenAdapt_PyClass.cxx"

//----------------
// PyAdaptCtorMap
//----------------
// Define a factory for creating Python Adaptive derived objects.
//
// An entry for KERNEL_MESHSIM is added later in PyAPI_InitMeshSim() 
// if the MeshSim interface is defined (by loading the MeshSim plugin).
//
using PyAdaptCtorMapType = std::map<KernelType, std::function<PyObject*()>>;
PyAdaptCtorMapType PyAdaptCtorMap = {
  {KernelType::KERNEL_TETGEN, []()->PyObject* {return PyObject_CallObject((PyObject*)&PyTetGenAdaptClassType, NULL);}},
};

//---------------------
// PyAdaptCreateObject 
//---------------------
// Create a Python adaptive mesher object for the given kernel.
//
static PyObject *
PyAdaptCreateObject(KernelType kernel)
{
  std::cout << "[PyAdaptCreateObject] ========== PyAdaptCreateObject ==========" << std::endl;
  PyObject* mesher;

  try {
      mesher = PyAdaptCtorMap[kernel]();
  } catch (...) {
      return nullptr;
  }

  return mesher;
}

//--------------------
// PyMeshingAdaptInit 
//--------------------
// This is the __init__() method for the Adapt class. 
//
// This function is used to initialize an object after it is created.
//
static int 
PyMeshingAdaptInit(PyMeshingAdaptiveClass* self, PyObject* args, PyObject *kwds)
{
  std::cout << "[PyMeshingAdaptInit] " << std::endl;
  std::cout << "[PyMeshingAdaptInit] ========== PyMeshingAdaptInit ==========" << std::endl;
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, "Adaptive");
  static int numObjs = 1;
  char* kernelName = nullptr;
  if (!PyArg_ParseTuple(args, "|s", &kernelName)) {
      return -1;
  }

  self->id = numObjs;
  //adaptive_mesherKernel = kernel;
  //adaptive_mesher = mesher;
  self->adaptive_mesher = nullptr;
  return 0;
}

//-------------------
// PyMeshingAdaptNew 
//-------------------
// Object creation function, equivalent to the Python __new__() method. 
//
static PyObject *
PyMeshingAdaptNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyMeshingAdaptNew] New Python Adaptive object." << std::endl;

  auto self = (PyMeshingAdaptiveClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->id = 1;
  }

  return (PyObject*)self;
}

//-----------------------
// PyMeshingAdaptDealloc 
//-----------------------
//
static void
PyMeshingAdaptDealloc(PyMeshingAdaptiveClass* self)
{ 
  std::cout << "[PyMeshingAdaptDealloc] Free PyMeshingAdaptiveClass " << self->id << std::endl;
  //delete self->solidModel;
  Py_TYPE(self)->tp_free(self);
}

//--------------------
// SetAdaptTypeFields 
//--------------------
// Set the Python type object fields that stores Mesher data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetAdaptTypeFields(PyTypeObject& meshType)
{
  // Doc string for this type.
  meshType.tp_doc = AdaptiveClass_doc;
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  meshType.tp_new = PyMeshingAdaptNew;
  //meshType.tp_new = PyType_GenericNew,
  meshType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  meshType.tp_init = (initproc)PyMeshingAdaptInit;
  meshType.tp_dealloc = (destructor)PyMeshingAdaptDealloc;
  meshType.tp_methods = PyMeshingAdaptMethods;
};

//-----------------
// CreateAdaptType 
//-----------------
//
static PyMeshingAdaptiveClass *
CreateAdaptType()
{
  return PyObject_New(PyMeshingAdaptiveClass, &PyMeshingAdaptiveClassType);
}



