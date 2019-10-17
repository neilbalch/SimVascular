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

// The functions defined here implement the SV Python API mesh adapt module. 
// This module is used for TetGen adaptive meshing.
//
// The module name is 'mesh_adapt'. The module defines an 'Adapt' class used
// to store mesh data. 
//
// Two Python types are defined
//
//   1) Apapt - Defined by pyAdaptObjectType.
//
//   2) pyAdaptObjectRegistrarType
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include <map>
#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_adapt_init_py.h"
#include "sv_AdaptObject.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"
#include "sv_vtk_utils.h"
#include "sv_PolyData.h"
#include "sv_sys_geom.h"
#include "vtkPythonUtil.h"
#include "sv_PyUtils.h"

#include "sv_FactoryRegistrar.h"

#include "Python.h"
// Needed for Windows
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
PyObject* PyRunTimeErr;

// Define a map between meshing kernel name and enum type.
static std::map<std::string,KernelType> kernelNameTypeMap =
{
    {"MeshSim", KERNEL_MESHSIM},
    {"TetGen", KERNEL_TETGEN}
};
static std::string validKernelNames = "Valid adaptive meshing kernel names are: MeshSim or TetGen."; 

//----------------------
// Define pyAdaptObject
//----------------------
//
class pyAdaptObject 
{
  public:
      PyObject_HEAD
      cvAdaptObject* adapt;
      std::string name;
}; 

pyAdaptObject_dealloc(pyAdaptObject* self)
{
  Py_XDECREF(self->adapt);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

// -------------
// DeleteAdapt
// -------------
// This is the deletion call-back for cvAdaptObject object commands.

/* [TODO:DaveP] is this used?
void DeleteAdapt( pyAdaptObject* self ) {
    cvAdaptObject *geom = self->geom;

    gRepository->UnRegister( geom->GetName() );
}
*/


//////////////////////////////////////////////////////
//              U t i l i t i e s                   //
//////////////////////////////////////////////////////


//----------------
// CheckAdaptMesh
//----------------
// Check if an adapt mesh object has been created. 
//
static cvAdaptObject *
CheckAdaptMesh(SvPyUtilApiFunction& api, pyAdaptObject* self)
{
  auto name = self->name;
  auto adapt = self->adapt;
  if (adapt == NULL) {
      api.error("An adapt mesh object has not been created for '" + std::string(name) + "'.");
      return nullptr;
  }
  return adapt;
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

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
// cvAdapt_new_object
//--------------------
//
// [TODO:DaveP] pass in meshing kernel name. 
//
PyDoc_STRVAR(cvAdapt_new_object_doc,
  " new_object()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
cvAdapt_new_object(pyAdaptObject* self, PyObject* args)
{
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
        meshType = kernelNameTypeMap.at(std::string(kernelName));
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

  self->adapt = adaptor;
  self->name = std::string(resultName);

  Py_DECREF(adaptor);
  return SV_PYTHON_OK;
}

//--------------------------------------
// cvAdapt_create_internal_mesh_object 
//--------------------------------------
//
PyDoc_STRVAR(cvAdapt_create_internal_mesh_object_doc,
  " create_internal_mesh_object()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
cvAdapt_create_internal_mesh_object(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_load_model 
//--------------------
//
PyDoc_STRVAR(cvAdapt_load_model_doc,
  "load_model() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_load_model(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_load_mesh 
//-------------------
//
PyDoc_STRVAR(cvAdapt_load_mesh_doc,
  "load_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_load_mesh(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_load_solution_from_file 
//---------------------------------
//
PyDoc_STRVAR(cvAdapt_load_solution_from_file_doc,
  "load_solution_from_file() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_load_solution_from_file(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_load_ybar_from_file 
//-----------------------------
//
PyDoc_STRVAR(cvAdapt_load_ybar_from_file_doc,
  "load_ybar_from_file() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_load_ybar_from_file(pyAdaptObject* self, PyObject* args)
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
PyDoc_STRVAR(cvAdapt_load_avg_speed_from_file_doc,
  "load_avg_speed_from_file() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_load_avg_speed_from_file(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_load_hessian_from_file 
//--------------------------------
//
PyDoc_STRVAR(cvAdapt_load_hessian_from_file_doc,
  "load_hessian_from_file() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_load_hessian_from_file(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_read_solution_from_mesh 
//---------------------------------
//
PyDoc_STRVAR(cvAdapt_read_solution_from_mesh_doc,
  "read_solution_from_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_read_solution_from_mesh(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_read_ybar_from_mesh 
//-----------------------------
//
PyDoc_STRVAR(cvAdapt_read_ybar_from_mesh_doc,
  "read_ybar_from_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_read_ybar_from_mesh(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_read_avg_speed_from_mesh 
//----------------------------------
//
PyDoc_STRVAR(cvAdapt_read_avg_speed_from_mesh_doc,
  "read_avg_speed_from_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_read_avg_speed_from_mesh(pyAdaptObject* self, PyObject* args)
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

//---------------------------
// cvAdapt_set_adapt_options 
//---------------------------
//
PyDoc_STRVAR(cvAdapt_set_adapt_options_doc,
  "set_adapt_options() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_set_adapt_options(pyAdaptObject* self, PyObject* args)
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

//-----------------------
// cvAdapt_check_options
//-----------------------
//
PyDoc_STRVAR(cvAdapt_check_options_doc,
  "check_options() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_check_options(pyAdaptObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto adapt = CheckAdaptMesh(api, self);
  if (adapt == nullptr) { 
      return nullptr;
  }

  if (adapt->CheckOptions() == SV_OK) {
      api.error("Error checking options.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//--------------------
// cvAdapt_set_metric 
//--------------------
//
PyDoc_STRVAR(cvAdapt_set_metric_doc,
  "set_metric() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_set_metric(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_setup_mesh 
//--------------------
//
PyDoc_STRVAR(cvAdapt_setup_mesh_doc,
  "setup_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_setup_mesh(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_run_adaptor 
//---------------------
//
PyDoc_STRVAR(cvAdapt_run_adaptor_doc,
  "run_adaptor() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_run_adaptor(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_print_statistics 
//--------------------------
//
PyDoc_STRVAR(cvAdapt_print_statistics_doc,
  "print_statistics() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_print_statistics(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_get_adapted_mesh 
//--------------------------
//
PyDoc_STRVAR(cvAdapt_get_adapted_mesh_doc,
  "get_adapted_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_get_adapted_mesh(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_transfer_solution 
//---------------------------
//
PyDoc_STRVAR(cvAdapt_transfer_solution_doc,
  "transfer_solution() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_transfer_solution(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_transfer_regions 
//--------------------------
//
PyDoc_STRVAR(cvAdapt_transfer_regions_doc,
  "transfer_regions() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_transfer_regions(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_write_adapted_model 
//-----------------------------
//
PyDoc_STRVAR(cvAdapt_write_adapted_model_doc,
  "_write_adapted_model() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_write_adapted_model(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_write_adapted_mesh 
//----------------------------
//
PyDoc_STRVAR(cvAdapt_write_adapted_mesh_doc,
  "write_adapted_mesh() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_write_adapted_mesh(pyAdaptObject* self, PyObject* args)
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
// cvAdapt_write_adapted_solution 
//--------------------------------
//
PyDoc_STRVAR(cvAdapt_write_adapted_solution_doc,
  "write_adapted_solution() \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
cvAdapt_write_adapted_solution(pyAdaptObject* self, PyObject* args)
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

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyMeshAdapt();
#elif PYTHON_MAJOR_VERSION == 3
PyMODINIT_FUNC PyInit_pyMeshAdapt();
#endif

// ----------
// Adapt_Init
// ----------
int Adapt_pyInit()
{
#if PYTHON_MAJOR_VERSION == 2
  initpyMeshAdapt();
#elif PYTHON_MAJOR_VERSION == 3
  PyInit_pyMeshAdapt();
#endif
  return SV_OK;
}

//---------------------
// pyAdaptObject_init
//---------------------
// This is the __init__() method for the Adapt class. 
//
// This function is used to initialize an object after it is created.
//
static int pyAdaptObject_init(pyAdaptObject* self, PyObject* args)
{
  fprintf(stdout, "Adapt object initialized.\n");
  return SV_OK;
}

//----------------------------
// Define API function names
//----------------------------
//
static PyMethodDef pyAdaptObject_methods[] = {

  { "check_options", (PyCFunction)cvAdapt_check_options,METH_VARARGS,cvAdapt_check_options_doc},

  { "create_internal_mesh_object", (PyCFunction)cvAdapt_create_internal_mesh_object, METH_VARARGS, cvAdapt_create_internal_mesh_object_doc},

  { "get_adapted_mesh", (PyCFunction)cvAdapt_get_adapted_mesh,METH_VARARGS,cvAdapt_get_adapted_mesh_doc},

  { "load_avg_speed_from_file", (PyCFunction)cvAdapt_load_avg_speed_from_file,METH_VARARGS,cvAdapt_load_avg_speed_from_file_doc},

  { "load_hessian_from_file", (PyCFunction)cvAdapt_load_hessian_from_file,METH_VARARGS,cvAdapt_load_hessian_from_file_doc},

  { "load_model", (PyCFunction)cvAdapt_load_model,METH_VARARGS,cvAdapt_load_model_doc},

  { "load_mesh", (PyCFunction)cvAdapt_load_mesh, METH_VARARGS, cvAdapt_load_mesh_doc},

  { "load_solution_from_file", (PyCFunction)cvAdapt_load_solution_from_file, METH_VARARGS, cvAdapt_load_solution_from_file_doc},

  { "load_ybar_from_file", (PyCFunction)cvAdapt_load_ybar_from_file,METH_VARARGS,cvAdapt_load_ybar_from_file_doc},

  {"new_object", (PyCFunction)cvAdapt_new_object, METH_VARARGS, cvAdapt_new_object_doc},

  { "print_statistics", (PyCFunction)cvAdapt_print_statistics,METH_VARARGS,cvAdapt_print_statistics_doc},

  { "read_solution_from_mesh", (PyCFunction)cvAdapt_read_solution_from_mesh,METH_VARARGS,cvAdapt_read_solution_from_mesh_doc},

  { "read_ybar_from_mesh", (PyCFunction)cvAdapt_read_ybar_from_mesh,METH_VARARGS,cvAdapt_read_ybar_from_mesh_doc},

  { "read_avg_speed_from_mesh", (PyCFunction)cvAdapt_read_avg_speed_from_mesh,METH_VARARGS,cvAdapt_read_avg_speed_from_mesh_doc},

  { "run_adaptor", (PyCFunction)cvAdapt_run_adaptor,METH_VARARGS,cvAdapt_run_adaptor_doc},

  { "set_adapt_options", (PyCFunction)cvAdapt_set_adapt_options,METH_VARARGS,cvAdapt_set_adapt_options_doc},

  { "set_metric", (PyCFunction)cvAdapt_set_metric,METH_VARARGS,cvAdapt_set_metric_doc},

  { "setup_mesh", (PyCFunction)cvAdapt_setup_mesh,METH_VARARGS,cvAdapt_setup_mesh_doc},

  { "transfer_solution", (PyCFunction)cvAdapt_transfer_solution,METH_VARARGS,cvAdapt_transfer_solution_doc},

  { "transfer_regions", (PyCFunction)cvAdapt_transfer_regions,METH_VARARGS,cvAdapt_transfer_regions_doc},

  { "write_adapted_model", (PyCFunction)cvAdapt_write_adapted_model,METH_VARARGS,cvAdapt_write_adapted_model_doc},

  { "write_adapted_mesh", (PyCFunction)cvAdapt_write_adapted_mesh,METH_VARARGS,cvAdapt_write_adapted_mesh_doc},

  { "write_adapted_solution", (PyCFunction)cvAdapt_write_adapted_solution,METH_VARARGS,cvAdapt_write_adapted_solution_doc},

  {NULL}

};

//------------------------------------------
// Define the pyAdaptObjectType type object
//------------------------------------------
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject pyAdaptObjectType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "mesh_adapt.Adapt",        /* tp_name */
  sizeof(pyAdaptObject),     /* tp_basicsize */
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
  "Adapt objects",           /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  pyAdaptObject_methods,     /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)pyAdaptObject_init,                            /* tp_init */
  0,                         /* tp_alloc */
  0,                  /* tp_new */
};

static PyMethodDef pyAdaptMesh_methods[] = {

  {"Registrars", Adapt_registrars, METH_NOARGS, Adapt_registrars_doc},

  {NULL, NULL}
};

//---------------------------------------------------
// Define the pyAdaptObjectRegistrarType type object
//---------------------------------------------------
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject pyAdaptObjectRegistrarType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "meshadapt.AdaptRegistrar",             /* tp_name */
  sizeof(pyAdaptObjectRegistrar),             /* tp_basicsize */
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
  "AdaptRegistrar wrapper  ",           /* tp_doc */
};


//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MODULE_NAME = "adapt_mesh";

PyDoc_STRVAR(AdaptMesh_doc, "adapt_mesh module functions.");

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
//
static struct PyModuleDef pyAdaptMeshmodule = {
   m_base,
   MODULE_NAME,  
   AdaptMesh_doc, 
   perInterpreterStateSize, 
   pyAdaptMesh_methods
};

//--------------------
// PyInit_pyMeshAdapt
//--------------------
// The initialization function called by the Python interpreter when the module is loaded.
//
// [TODO:Davep] The global 'gRepository' is created here, as it is in all other modules init
//     function. Why is this not create in main()?
//
PyMODINIT_FUNC
PyInit_pyMeshAdapt()
{
  // Associate the adapt object registrar with the python interpreter
  if (gRepository==NULL) {
    gRepository= new cvRepository();
    fprintf(stdout,"New gRepository created from cv_adapt_init\n");
  }

  // Initialize
  cvAdaptObject::gCurrentKernel = KERNEL_INVALID;

#ifdef USE_TETGEN_ADAPTOR
  cvAdaptObject::gCurrentKernel = KERNEL_TETGEN;
#endif

  // Create meshadapt module.
  //
  auto module = PyModule_Create(&pyAdaptMeshmodule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing pyMeshAdapt\n");
    return SV_PYTHON_ERROR;
  }

  // Create AdaptMeshType.
  //
  pyAdaptObjectType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&pyAdaptObjectType) < 0) {
    fprintf(stdout,"Error in pyAdaptMeshType\n");
    return SV_PYTHON_ERROR;
  }
  Py_INCREF(&pyAdaptObjectType);
  PyModule_AddObject(module, "Adapt", (PyObject*)&pyAdaptObjectType);

  // Create AdaptRegistrar type.
  //
  pyAdaptObjectRegistrarType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&pyAdaptObjectRegistrarType) < 0) {
    fprintf(stdout,"Error in pyAdaptObjectRegistrarType\n");
    return SV_PYTHON_ERROR;
  }
  Py_INCREF(&pyAdaptObjectRegistrarType);
  PyModule_AddObject(module, "AdaptRegistrar", (PyObject *)&pyAdaptObjectRegistrarType);

  pyAdaptObjectRegistrar* tmp = PyObject_New(pyAdaptObjectRegistrar, &pyAdaptObjectRegistrarType);
  tmp->registrar = (cvFactoryRegistrar *)&cvAdaptObject::gRegistrar;
  PySys_SetObject("AdaptRegistrar", (PyObject *)tmp);

  // Add meshadapt.MeshAdaptException exception.
  //
  PyRunTimeErr = PyErr_NewException("meshadapt.MeshAdaptException",NULL,NULL);
  PyModule_AddObject(module,"MeshAdaptException", PyRunTimeErr);

  return module;
 }

#endif


//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC
initpyMeshAdapt()
{

  // Associate the adapt object registrar with the python interpreter
  if (gRepository==NULL)
  {
    gRepository= new cvRepository();
    fprintf(stdout,"New gRepository created from cv_adapt_init\n");
  }
  // Initialize
  cvAdaptObject::gCurrentKernel = KERNEL_INVALID;

#ifdef USE_TETGEN_ADAPTOR
  cvAdaptObject::gCurrentKernel = KERNEL_TETGEN;
#endif

  pyAdaptObjectType.tp_new=PyType_GenericNew;
  pyAdaptObjectRegistrarType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&pyAdaptObjectType)<0)
  {
    fprintf(stdout,"Error in pyAdaptMeshType\n");
    return;
  }
  if (PyType_Ready(&pyAdaptObjectRegistrarType)<0)
  {
    fprintf(stdout,"Error in pyAdaptObjectRegistrarType\n");
    return;
  }
  PyObject* pythonC;
  pythonC = Py_InitModule("pyMeshAdapt",pyAdaptMesh_methods);

  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyMeshAdapt\n");
    return;

  }

  PyRunTimeErr = PyErr_NewException("pyMeshAdapt.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  
  Py_INCREF(&pyAdaptObjectType);
  Py_INCREF(&pyAdaptObjectRegistrarType);
  PyModule_AddObject(pythonC, "pyAdaptObjectRegistrar", (PyObject *)&pyAdaptObjectRegistrarType);
  PyModule_AddObject(pythonC,"pyAdaptObject",(PyObject*)&pyAdaptObjectType);
    
  pyAdaptObjectRegistrar* tmp = PyObject_New(pyAdaptObjectRegistrar, &pyAdaptObjectRegistrarType);
  tmp->registrar = (cvFactoryRegistrar *)&cvAdaptObject::gRegistrar;
  PySys_SetObject("AdaptObjectRegistrar", (PyObject *)tmp);
  return;
 }
#endif

