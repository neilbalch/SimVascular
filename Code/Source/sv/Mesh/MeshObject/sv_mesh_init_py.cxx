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

// The functions defined here implement the SV Python API Contour Module. 
//
// The module name is 'contour'. The module defines a 'Contour' class used
// to store contour data. The 'Contour' class cannot be imported and must
// be used prefixed by the module name. For example
//
//     ctr = contour.Countour()
//
// A Python exception sv.contour.ContourException is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.contour.ContourException:
//
#include "SimVascular.h"
#include "SimVascular_python.h"
#include "sv_MeshSystem.h"
#include "sv_MeshObject.h"
#include "sv_mesh_init_py.h"
#include "vtkPythonUtil.h"
#include "sv3_PyUtil.h"

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
  Py_XDECREF(self->geom);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

// -------------
// DeleteMesh
// -------------
// This is the deletion call-back for cvMeshObject object commands.

void DeleteMesh(pyMeshObject* self )
{
    cvMeshObject *geom =self->geom ;
    gRepository->UnRegister( geom->GetName() );
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
CheckMeshLoadUpdate(cvMeshObject *geom, std::string& msg) 
{
  if (geom == nullptr) {
      msg = "The Mesh object does not have geometry.";
      return false;
  }

  if (geom->GetMeshLoaded() == 0) {
      if (geom->Update() == SV_ERROR) {
          msg = "Error updating the mesh.";
          return false;
      }
  }

  return true;
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
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
  std::string format = "s|ss:" + functionName;

  char *resultName;
  char *meshFileName = NULL;
  char *solidFileName = NULL;

  if (!PyArg_ParseTuple(args, format.c_str(), &resultName, &meshFileName, &solidFileName)) {
    return Sv3PyUtilResetException(PyRunTimeErr);
  }

  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(resultName)) {
      auto msg = msgp + "The Mesh object '" + resultName + "' is already in the repository.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Create a new cvMeshObject object. 
  auto geom = cvMeshSystem::DefaultInstantiateMeshObject(meshFileName, solidFileName );
  if (geom == NULL) {
      auto msg = msgp + "Failed to create Mesh object.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Add mesh to the repository.
  if (!gRepository->Register(resultName, geom)) {
      delete geom;
      auto msg = msgp + "Error adding the Mesh object '" + resultName + "' to the repository.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom = geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//-----------------
// Mesh_get_object
//-----------------
//
// [TODO:DaveP] This sets the 'geom' data member of the pyContour struct for
// this object. Bad!
//
PyDoc_STRVAR(Mesh_get_mesh_doc,
  "Mesh_get_mesh(mesh)  \n\ 
   \n\
   Set the mesh geometry from a Mesh object stored in the repository. \n\
   \n\
   Args: \n\
     mesh (str): The name of the Mesh object. \n\
");

static PyObject * 
Mesh_get_mesh(pyMeshObject* self, PyObject* args)
{
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
  std::string format = "s:" + functionName;

  char *objName = NULL;
  if (!PyArg_ParseTuple(args, format.c_str(), &objName)) {
      return Sv3PyUtilResetException(PyRunTimeErr);
  }
   
  // Get the Mesh object from the repository. 
  auto rd = gRepository->GetObject(objName);
  if (rd == NULL) {
      auto msg = msgp + "The Mesh object '" + objName + "' is not in the repository.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Check its type.
  auto type = rd->GetType();
  if (type != MESH_T) {
      auto msg = msgp + "'" + objName + "' is not a Mesh object.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }
  
  auto geom = dynamic_cast<cvMeshObject*> (rd);
  Py_INCREF(geom);
  self->geom = geom;
  Py_DECREF(geom);
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
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
  std::string format = "s:" + functionName;

  char *kernelName;
  if(!PyArg_ParseTuple(args, format.c_str(), &kernelName)) {
      return Sv3PyUtilResetException(PyRunTimeErr);
  }

  cvMeshObject::KernelType kernelType = cvMeshObject::GetKernelType(kernelName);
  if (kernelType == cvMeshObject::KERNEL_INVALID) { 
      auto msg = msgp + "Unknown mesh kernel type '" + kernelName + "'.";
      msg += " Valid mesh kernel names are: GMsh, MeshSim or TetGen.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  if (cvMeshSystem::SetCurrentKernel(kernelType) != SV_OK) {
      auto msg = msgp + "Error setting the mesh kernel type to '" + kernelName + "'.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
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
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);

  std::string emsg;
  auto geom = self->geom;
  if (!CheckMeshLoadUpdate(geom, emsg)) {
      auto msg = msgp + emsg;
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Check the current mesh kernel.
  auto kernelType = geom->GetMeshKernel();
  if (kernelType == SM_KT_INVALID ) {
      auto msg = msgp + "The mesh kernel is not set.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
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
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);

  std::string emsg;
  auto geom = self->geom;
  if (!CheckMeshLoadUpdate(geom, emsg)) {
      auto msg = msgp + emsg;
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  if (geom->pyPrint() != SV_OK) {
      auto msg = msgp + "Error printing the mesh.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------------
// Mesh_update_docd
//------------------
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
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);

  auto geom = self->geom;
  if (geom == nullptr) {
      auto msg = msgp + "The Mesh object does not have geometry.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  if (geom->Update() != SV_OK) {
      auto msg = msgp + "Error updating the mesh.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
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
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
  std::string format = "s:" + functionName;

  char *kernelName;
  if (!PyArg_ParseTuple(args, format.c_str(), &kernelName)) {
      return Sv3PyUtilResetException(PyRunTimeErr);
  }

  auto geom = self->geom;
  if (geom == nullptr) {
      auto msg = msgp + "The Mesh object does not have geometry.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Check for a current valid kernel.
  auto kernel = SolidModel_KernelT_StrToEnum( kernelName );
  if (kernel == SM_KT_INVALID) {
      auto msg = msgp + "The mesh kernel is not set.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  geom->SetSolidModelKernel(kernel);
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
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
  std::string format = "s:" + functionName;

  char *file_name;
  if (!PyArg_ParseTuple(args, format.c_str(), &file_name)) {
      return Sv3PyUtilResetException(PyRunTimeErr);
  }

  std::string emsg;
  auto geom = self->geom;
  if (!CheckMeshLoadUpdate(geom, emsg)) {
      auto msg = msgp + emsg;
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  if (geom->WriteMetisAdjacency(file_name) != SV_OK) {
      auto msg = msgp + "Error writing the mesh adjacency to the file '" + file_name + "'.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
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
  Add the mesh geometry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the geometry. \n\
");

static PyObject * 
Mesh_get_polydata(pyMeshObject* self, PyObject* args)
{
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
  std::string format = "s:" + functionName;

  char *resultName;
  if (!PyArg_ParseTuple(args, format.c_str(), &resultName)) {
      return Sv3PyUtilResetException(PyRunTimeErr);
  }

  std::string emsg;
  auto geom = self->geom;
  if (!CheckMeshLoadUpdate(geom, emsg)) {
      auto msg = msgp + emsg; 
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(resultName)) {
      auto msg = msgp + "The repository object '" + resultName + "' already exists.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Get the cvPolyData:
  auto pd = geom->GetPolyData();
  if (pd == NULL) {
      auto msg = msgp + "Could not get polydata for the mesh.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
      delete pd;
      auto msg = msgp + "Could not add the polydata to the repository.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
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
  Add the mesh solid model geometry to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the solid model geometry. \n\
");

static PyObject * 
Mesh_get_solid(pyMeshObject* self, PyObject* args)
{
  std::string functionName = Sv3PyUtilGetFunctionName(__func__);
  std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
  std::string format = "s:" + functionName;

  char *resultName;
  if (!PyArg_ParseTuple(args, format.c_str(), &resultName)) {
      return Sv3PyUtilResetException(PyRunTimeErr);
  }

  std::string emsg;
  auto geom = self->geom;
  if (!CheckMeshLoadUpdate(geom, emsg)) {
      auto msg = msgp + emsg;
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(resultName)) {
      auto msg = msgp + "The repository object '" + resultName + "' already exists.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Get the cvPolyData:
  auto pd = geom->GetSolid();
  if (pd == NULL) {
      auto msg = msgp + "Could not get polydata for the mesh solid model.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
      delete pd;
      auto msg = msgp + "Could not add the polydata to the repository.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------------
// cvMesh_SetVtkPolyDataMtd
//----------------------
//

static PyObject * 
cvMesh_SetVtkPolyDataMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  char *objName;
  RepositoryDataT type;
  cvRepositoryData *obj;
  vtkPolyData *pd;
  if(!PyArg_ParseTuple(args,"s",&objName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char, objName");
    
  }

  if (geom->GetMeshLoaded() == 0)
  {
    if (geom->Update() == SV_ERROR)
    {
      PyErr_SetString(PyRunTimeErr, "error update.");
    }
  }
  // Do work of command:
  type = gRepository->GetType( objName );
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj must be of type cvPolyData");
    
  }

  obj = gRepository->GetObject( objName );
  switch (type) {
  case POLY_DATA_T:
    pd = ((cvPolyData *)obj)->GetVtkPolyData();
    break;
  default:
    PyErr_SetString(PyRunTimeErr, "error in SetVtkPolyData");
    
    break;
  }

  // set the vtkPolyData:
  if(!geom->SetVtkPolyDataObject(pd))
  {
    PyErr_SetString(PyRunTimeErr, "error set vtk polydate object.");
  }

  return SV_PYTHON_OK;
}

// -------------------------------
// cvMesh_GetUnstructuredGridMtd
// -------------------------------

static PyObject* cvMesh_GetUnstructuredGridMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  char *resultName;
  cvUnstructuredGrid *ug;
  if(!PyArg_ParseTuple(args,"s",&resultName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char, resultName");
    
  }

  // Do work of command:

  if (geom->GetMeshLoaded() == 0)
  {
    if (geom->Update() == SV_ERROR)
    {
      PyErr_SetString(PyRunTimeErr, "error update.");
    }
  }
  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( resultName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Get the cvUnstructuredGrid:
  ug = geom->GetUnstructuredGrid();
  if ( ug == NULL ) {
    PyErr_SetString(PyRunTimeErr, "error getting cvPolyData" );
    
  }

  // Register the result:
  if ( !( gRepository->Register( resultName, ug ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj " );
    delete ug;
    
  }

  return SV_PYTHON_OK;
}


// --------------------------
// cvMesh_GetFacePolyDataMtd
// --------------------------

static PyObject* cvMesh_GetFacePolyDataMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  char *resultName;
  cvPolyData *pd;
  int face;
  if(!PyArg_ParseTuple(args,"si",&resultName,&face))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char and one int, resultName, face");
    
  }
  // Do work of command:
  if (geom->GetMeshLoaded() == 0)
  {
    if (geom->Update() == SV_ERROR)
    {
      PyErr_SetString(PyRunTimeErr, "error update.");
    }
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( resultName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Get the cvPolyData:
  pd = geom->GetFacePolyData(face);
  if ( pd == NULL ) {
    PyErr_SetString(PyRunTimeErr, "error getting cvPolyData ");
    
  }

  // Register the result:
  if ( !( gRepository->Register( resultName, pd ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete pd;
    
  }

  return SV_PYTHON_OK;
}

//
// LogOn
//

PyObject* cvMesh_LogonCmd(PyObject* self, PyObject* args)
{

  char *logFileName;
  if(!PyArg_ParseTuple(args,"s",&logFileName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char, logFileName");
    
  }

  // Do work of command:

  cvMeshSystem* meshKernel = cvMeshSystem::GetCurrentKernel();

  // read in the results file
  if (meshKernel == NULL || meshKernel->LogOn(logFileName) == SV_ERROR) {
      PyErr_SetString(PyRunTimeErr, "error opening logfile");
      
  }

  return SV_PYTHON_OK;
}


// ------------------
// cvMesh_LogoffCmd
// ------------------

PyObject* cvMesh_LogoffCmd( PyObject* self, PyObject* args)
{
  cvMeshSystem* meshKernel = cvMeshSystem::GetCurrentKernel();

  if (meshKernel == NULL || meshKernel->LogOff() == SV_ERROR) {
      PyErr_SetString(PyRunTimeErr, "error turning off logfile ");
      
  }

  return SV_PYTHON_OK;
}

// -------------------------
// cvMesh_SetMeshOptionsMtd
// -------------------------

static PyObject* cvMesh_SetMeshOptionsMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  char *flags;
  PyObject* valueList;
  if(!PyArg_ParseTuple(args,"sO",&flags,&valueList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char and one list,flags and valuelist");
    
  }
  int numValues = PyList_Size(valueList);
  double *values = new double [numValues];
  for (int j=0 ; j<numValues;j++)
  {
    values[j]=PyFloat_AsDouble(PyList_GetItem(valueList,j));
  }
  // Do work of command:
  // Get the cvPolyData:
  if ( geom->SetMeshOptions(flags,numValues,values) == SV_ERROR ) {
    PyErr_SetString(PyRunTimeErr, "error in method ");
    delete [] values;
    
  }
  delete [] values;

  return SV_PYTHON_OK;
}

//
// LoadModel
//

PyObject* cvMesh_LoadModelMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  if (geom==NULL)
  {
      PyErr_SetString(PyRunTimeErr,"Mesh object not registered in repository");
      
  }
  char *FileName;
  if(!PyArg_ParseTuple(args,"s",&FileName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char,FileName");
    
  }

  // Do work of command:

  // read in the results file
  fprintf(stderr,"Filename: %s\n",FileName);
  if (geom->LoadModel(FileName) == SV_ERROR) {
      PyErr_SetString(PyRunTimeErr, "error loading solid model");
      
  }

  return SV_PYTHON_OK;
}

// -------------------
// Solid_GetBoundaryFacesMtd
// -------------------
//
PyObject* cvMesh_GetBoundaryFacesMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  double angle = 0.0;
  if(!PyArg_ParseTuple(args,"d",&angle))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one double,angle");
    
  }

  int status = geom->GetBoundaryFaces(angle);
  if ( status == SV_OK ) {
    return SV_PYTHON_OK;
  } else {
    PyErr_SetString(PyRunTimeErr, "GetBoundaryFaces: error on object");
    
  }
}

#ifdef SV_USE_MESHSIM_DISCRETE_MODEL

/*
//
// LoadDiscreteModel
//

*/

#endif

PyObject* cvMesh_LoadMeshMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  char *FileName;
  char *SurfFileName = 0;
  if(!PyArg_ParseTuple(args,"s|s",&FileName, &SurfFileName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char or one optional char,FileName, SurfFileName");
    
  }

  // Do work of command:

  // read in the results file
  if (geom->LoadMesh(FileName,SurfFileName) == SV_ERROR) {
      PyErr_SetString(PyRunTimeErr, "error loading mesh ");
      
  }

  return SV_PYTHON_OK;
}

PyObject* cvMesh_WriteStatsMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  char *FileName;

  if(!PyArg_ParseTuple(args,"s",&FileName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char ,FileName");
    
  }

  // Do work of command:
  if (geom->GetMeshLoaded() == 0)
  {
    if (geom->Update() == SV_ERROR)
    {
      PyErr_SetString(PyRunTimeErr, "error update.");
    }
  }

  // read in the results file
  if (geom->WriteStats(FileName) == SV_ERROR) {
      PyErr_SetString(PyRunTimeErr, "error writing stats file ");
      
  }

  return SV_PYTHON_OK;
}

PyObject* cvMesh_AdaptMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;

  if (geom->GetMeshLoaded() == 0)
  {
    if (geom->Update() == SV_ERROR)
    {
      PyErr_SetString(PyRunTimeErr, "error update.");
    }
  }
  // Do work of command:
  if (geom->Adapt() == SV_OK) {
    return SV_PYTHON_OK;
  } else {
    PyErr_SetString(PyRunTimeErr, "error adapt.");
  }
}

PyObject* cvMesh_WriteMeshMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  char *FileName;
  int smsver = 0;
  if(!PyArg_ParseTuple(args,"s|i",&FileName,&smsver))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char and one optional int ,FileName, smsver");
    
  }

  // Do work of command:
  if (geom->GetMeshLoaded() == 0)
  {
    if (geom->Update() == SV_ERROR)
    {
      PyErr_SetString(PyRunTimeErr, "error update.");
    }
  }

  // read in the results file
  if (geom->WriteMesh(FileName,smsver) == SV_ERROR) {
      PyErr_SetString(PyRunTimeErr, "error writing mesh ");
      
  }

  return SV_PYTHON_OK;
}


// -------------------
// cvMesh_NewMeshMtd
// -------------------
PyObject* cvMesh_NewMeshMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  if (geom->NewMesh() == SV_ERROR)
  {
      PyErr_SetString(PyRunTimeErr, "error creating new mesh ");
      
  }

  return SV_PYTHON_OK;
}


// ------------------------
// cvMesh_GenerateMeshMtd
// ------------------------

PyObject* cvMesh_GenerateMeshMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  if (geom->GenerateMesh() == SV_ERROR)
  {
      PyErr_SetString(PyRunTimeErr, "Error generating mesh ");
      
  }

  return SV_PYTHON_OK;
}


// -------------------------------
// cvMesh_SetSphereRefinementMtd
// -------------------------------

static PyObject* cvMesh_SetSphereRefinementMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  double size;
  PyObject* ctrList;
  double ctr[3];
  double r;
  int nctr;

  if(!PyArg_ParseTuple(args,"ddO",&size,&r,&ctrList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two doubles and one list, size, r, ctrList");
    
  }
  nctr=PyList_Size(ctrList);
  if ( nctr != 3 )
  {
    PyErr_SetString(PyRunTimeErr,"sphere requires a 3D center coordinate");
    
  }

  // Do work of command:

  if ( geom->SetSphereRefinement(size,r,ctr) == SV_ERROR )   {
    PyErr_SetString(PyRunTimeErr, "error in method " );
    
  }

  return SV_PYTHON_OK;
}

// -------------------------------
// cvMesh_SetSizeFunctionBasedMeshMtd
// -------------------------------

static PyObject* cvMesh_SetSizeFunctionBasedMeshMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  char *functionName;
  double size;

  if(!PyArg_ParseTuple(args,"ds",&size,&functionName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one double and one char, size and functionName");
    
  }

  // Do work of command:

  if ( geom->SetSizeFunctionBasedMesh(size,functionName) == SV_ERROR ) {
    PyErr_SetString(PyRunTimeErr, "error in setting size function" );
    
  }

  return SV_PYTHON_OK;
}


// ---------------------------------
// cvMesh_SetCylinderRefinementMtd
// ---------------------------------

static PyObject* cvMesh_SetCylinderRefinementMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  double size;
  PyObject* ctrList;
  PyObject* nrmList;
  double ctr[3];
  double nrm[3];
  double r;
  int nctr;
  int nnrm;
  double length;

  if(!PyArg_ParseTuple(args,"ddOO",&size,&r, &length,&ctrList,&nrmList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two doubles and two lists, size, r, ctrList,nrmList.");
    
  }

  // Do work of command:
  nctr=PyList_Size(ctrList);
  nnrm=PyList_Size(nrmList);
  if ( nctr != 3 ) {
    PyErr_SetString(PyRunTimeErr,"sphere requires a 3D center coordinate");
    
  }

  if ( nnrm != 3 ) {
    PyErr_SetString(PyRunTimeErr,"norm must be 3D");
    
  }

  // Do work of command:

  if ( geom->SetCylinderRefinement(size,r,length,ctr,nrm) == SV_ERROR ) {
    PyErr_SetString(PyRunTimeErr, "error in method ");
    
  }

  return SV_PYTHON_OK;
}


// ----------------------------
// cvMesh_SetBoundaryLayerMtd
// ----------------------------

static PyObject* cvMesh_SetBoundaryLayerMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  int type = 0;
  int id = 0;
  int side = 0;
  int nL = 0;
  double *H = NULL;
  PyObject* Hlist;

  if(!PyArg_ParseTuple(args,"iiiiO",&type,&id,&side,&nL,&Hlist))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import four ints and one list, type, id, side, nL, Hlist.");
    
  }

  // Parse coordinate lists:
  int numH=PyList_Size(Hlist);
  H = new double [numH];
  for (int i=0; i<numH;i++)
  {
    H[i]=PyFloat_AsDouble(PyList_GetItem(Hlist,i));
  }
  // Do work of command:

  if ( geom->SetBoundaryLayer(type,id,side,nL,H) == SV_ERROR ) {
    PyErr_SetString(PyRunTimeErr, "error in method ");
    delete [] H;
    
  }

  return SV_PYTHON_OK;
}

// ----------------------------
// cvMesh_SetWallsMtd
// ----------------------------

static PyObject* cvMesh_SetWallsMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;
  PyObject* wallsList;

  if(!PyArg_ParseTuple(args,"O",&wallsList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one list, wallsList");
    
  }


  // Parse coordinate lists:
  int numWalls=PyList_Size(wallsList);
  int *walls = new int [numWalls];
  for (int i=0;i<numWalls;i++)
  {
    walls[i]=PyLong_AsLong(PyList_GetItem(wallsList,i));
  }
  // Do work of command:

  if ( geom->SetWalls(numWalls,walls) == SV_ERROR ) {
    PyErr_SetString(PyRunTimeErr, "error in method ");
    delete [] walls;
    
  }

  return SV_PYTHON_OK;
}

//--------------------------
// cvMesh_GetModelFaceInfoMtd
//--------------------------
//
static PyObject* cvMesh_GetModelFaceInfoMtd( pyMeshObject* self, PyObject* args)
{
  cvMeshObject *geom = self->geom;

  char info[99999];
  geom->GetModelFaceInfo(info);

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

  {"new_object", 
      (PyCFunction)Mesh_new_object,
      METH_VARARGS,
      Mesh_new_object_doc
  },

  {"get_mesh", 
      (PyCFunction)Mesh_get_mesh, 
      METH_VARARGS, 
      Mesh_get_mesh_doc
  },

  { "LoadModel", (PyCFunction)cvMesh_LoadModelMtd,METH_VARARGS,NULL},

  { "GetBoundaryFaces",(PyCFunction)cvMesh_GetBoundaryFacesMtd,METH_VARARGS,NULL},

  { "LoadMesh", (PyCFunction)cvMesh_LoadMeshMtd,METH_VARARGS,NULL},

  { "NewMesh", (PyCFunction)cvMesh_NewMeshMtd,METH_VARARGS,NULL},

  { "SetMeshOptions", (PyCFunction)cvMesh_SetMeshOptionsMtd,METH_VARARGS,NULL},

  { "SetCylinderRefinement", (PyCFunction)cvMesh_SetCylinderRefinementMtd,METH_VARARGS,NULL},

  { "SetSphereRefinement",(PyCFunction)cvMesh_SetSphereRefinementMtd,METH_VARARGS,NULL},

  { "SetSizeFunctionBasedMesh", (PyCFunction)cvMesh_SetSizeFunctionBasedMeshMtd,METH_VARARGS,NULL},

  { "GenerateMesh", (PyCFunction)cvMesh_GenerateMeshMtd,METH_VARARGS,NULL},

  { "SetBoundaryLayer", (PyCFunction)cvMesh_SetBoundaryLayerMtd,METH_VARARGS,NULL},

  { "SetWalls", (PyCFunction)cvMesh_SetWallsMtd,METH_VARARGS,NULL},

  { "set_solid_kernel", 
      (PyCFunction)Mesh_set_solid_kernel,
      METH_VARARGS,
      Mesh_set_solid_kernel_doc
  },

  { "GetModelFaceInfo",(PyCFunction)cvMesh_GetModelFaceInfoMtd,METH_VARARGS,NULL},

  // The method "Update" must be called before any of the other
  // methods since it loads the mesh.  To avoid confusion, we
  // call this method directly prior to any other.
 // { "Update" ) ) {
    // ignore this call now, it is done implicitly (see above)
    //if ( (PyCFunction)cvMesh_UpdateMtd,METH_VARARGS,NULL},

  { "print",
      (PyCFunction)Mesh_print,
       METH_VARARGS,
       Mesh_print_doc
  },

  { "get_kernel", 
      (PyCFunction)Mesh_get_kernel,
      METH_VARARGS,
      Mesh_get_kernel_doc
  },

  { "write_metis_adjacency", 
      (PyCFunction)Mesh_write_metis_adjacency,
      METH_VARARGS,
      Mesh_write_metis_adjacency_doc
   },

  { "get_polydata", 
      (PyCFunction)Mesh_get_polydata,
      METH_VARARGS,
      Mesh_get_polydata_doc
   },

  { "get_solid", 
      (PyCFunction)Mesh_get_solid,
      METH_VARARGS,
      Mesh_get_solid_doc
  },

  { "SetVtkPolyData",(PyCFunction)cvMesh_SetVtkPolyDataMtd,METH_VARARGS,NULL},

  { "GetUnstructuredGrid", (PyCFunction)cvMesh_GetUnstructuredGridMtd,METH_VARARGS,NULL},

  { "GetFacePolyData", (PyCFunction)cvMesh_GetFacePolyDataMtd,METH_VARARGS,NULL},

  { "WriteMesh", (PyCFunction)cvMesh_WriteMeshMtd,METH_VARARGS,NULL},

  { "WriteStats",(PyCFunction)cvMesh_WriteStatsMtd,METH_VARARGS,NULL},

  { "Adapt",  (PyCFunction)cvMesh_AdaptMtd,METH_VARARGS,NULL},

  {NULL,NULL}
};

static PyTypeObject pyMeshObjectType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "pyMeshObject.pyMeshObject",             /* tp_name */
  sizeof(pyMeshObject),             /* tp_basicsize */
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
  Py_TPFLAGS_DEFAULT |
      Py_TPFLAGS_BASETYPE,   /* tp_flags */
  "pyMeshObject  objects",           /* tp_doc */
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

  {"set_kernel", 
      (PyCFunction)Mesh_set_kernel,
       METH_VARARGS,
       Mesh_set_kernel_doc
  },

  {"Logon", (PyCFunction)cvMesh_LogonCmd,METH_VARARGS,NULL},

  {"Logoff", (PyCFunction)cvMesh_LogoffCmd,METH_NOARGS,NULL},

  {NULL, NULL}
};

#if PYTHON_MAJOR_VERSION == 3
static struct PyModuleDef pyMeshObjectmodule = {
   PyModuleDef_HEAD_INIT,
   "pyMeshObject",   /* name of module */
   "", /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   pyMeshObjectModule_methods
};
#endif
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

#if PYTHON_MAJOR_VERSION == 3
PyMODINIT_FUNC PyInit_pyMeshObject()

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
    return SV_PYTHON_ERROR;

  }
  if(PySys_SetObject("MeshSystemRegistrar",Py_BuildValue("i",kernel))<0)
  {
    fprintf(stdout, "Unable to register MeshSystemRegistrar\n");
    return SV_PYTHON_ERROR;

  }
  // Initialize
  cvMeshSystem::SetCurrentKernel( cvMeshObject::KERNEL_INVALID );

  pyMeshObjectType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&pyMeshObjectType)<0)
  {
    fprintf(stdout,"Error in pyMeshObjectType\n");
    return SV_PYTHON_ERROR;
  }
  PyObject* pythonC;

  pythonC = PyModule_Create(&pyMeshObjectmodule);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyMeshObject\n");
    return SV_PYTHON_ERROR;
  }
  PyRunTimeErr = PyErr_NewException("pyMeshObject.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  Py_INCREF(&pyMeshObjectType);
  PyModule_AddObject(pythonC,"pyMeshObject",(PyObject*)&pyMeshObjectType);

  return pythonC;

}
#endif
