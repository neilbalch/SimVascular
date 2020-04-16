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

// The functions defined here implement the SV Python API meshing module group class. 
// It provides an interface to the SV meshing group class.
//
// The class name is 'Group'. It is referenced from the solid model module as 'solid.Group'.
//
//     mesh_group = meshing.Group()
//
#include "sv4gui_MitkMeshIO.h"
#include "sv4gui_Model.h"

//static PyObject * CreatePyMeshingGroup(sv4guiMesh::Pointer meshingGroup);

extern PyObject* PyTetGenOptionsCreateFromList(std::vector<std::string>& optionList);
extern  sv4guiModel::Pointer SolidGroup_read(char* fileName);

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//-------------------
// MeshingGroupRead
//-------------------
// Read in an SV .msh file and create a MeshingGroup object
// from its contents.
//
static sv4guiMitkMesh::Pointer 
MeshingGroupRead(char* fileName)
{
  std::cout << "========== MeshingGroupRead ==========" << std::endl;
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  std::cout << "[MeshingGroupRead] fileName: " << fileName << std::endl;
  sv4guiMitkMesh::Pointer group;
  bool readSurfaceMesh = false; 
  bool readVolumeMesh = false;

  try {
      group = sv4guiMitkMeshIO::ReadFromFile(std::string(fileName), readSurfaceMesh, readVolumeMesh);
  } catch (...) {
      api.error("Error reading the mesh group file '" + std::string(fileName) + "'.");
      std::cout << "[MeshingGroupRead] ERROR: can't read fileName: " << fileName << std::endl;
      return nullptr;
  }

  std::cout << "[MeshingGroupRead] File read and group returned." << std::endl;
  auto meshingGroup = dynamic_cast<sv4guiMitkMesh*>(group.GetPointer());
  int numMeshes = meshingGroup->GetTimeSize();
  std::cout << "[MeshingGroupRead] Number of meshes: " << numMeshes << std::endl;

  return group;
}

//----------------------
// MeshingGroupSetModel
//----------------------
// Set the solid model associated with the mesher.
//
// This will try to load the solid model .mdl file from 
// the SV project's Models directory.
//
bool 
MeshingGroupSetModel(SvPyUtilApiFunction& api, cvMeshObject* mesher, sv4guiMitkMesh* meshingGroup, 
    int index, std::string fileName, std::map<std::string,int>& faceIDMap)
{
  std::cout << "[MeshingGroupSetModel] ========== MeshingGroupSetModel ========== " << std::endl;
  // Get the file name of the solid model used by the mesher.
  auto modelName = meshingGroup->GetModelName();
  std::cout << "[MeshingGroupSetModel] Model name: " << modelName << std::endl;
  size_t strIndex = 0;
  strIndex = fileName.find("Meshes", strIndex);
  if (strIndex == std::string::npos) {
      api.error("No 'Models' directory found. The .msh file is not part of a SimVascular project.");
      return false;
  }
  fileName.erase(strIndex);
  auto modelDirName = fileName + "Models/";
  fileName = modelDirName + modelName + ".mdl";
  std::cout << "[MeshingGroupSetModel] fileName: " << fileName << std::endl;

  // Read the model .mdl file.
  sv4guiModel::Pointer solidGroupPtr = SolidGroup_read(const_cast<char*>(fileName.c_str()));
  if (solidGroupPtr == nullptr) {
      api.error("Unable to read the model file '" + fileName + "' used by the mesher.");
      return false;
  }
  auto solidGroup = dynamic_cast<sv4guiModel*>(solidGroupPtr.GetPointer());

  // Check for valid index.
  int numSolids = solidGroup->GetTimeSize();
  if ((index < 0) || (index > numSolids-1)) {
      api.error("There is no solid for time '" + std::to_string(index) + "'" );
      return false;
  }
  auto solidModelElement = solidGroup->GetModelElement(index);

  // Set the mesher solid modeling kernel. 
  auto solidType = solidGroup->GetType();
  std::transform(solidType.begin(), solidType.end(), solidType.begin(), ::toupper);
  auto solidKernel = SolidKernel_NameToEnum(solidType);
  std::cout << "[MeshingGroupSetModel] Solid type: " << solidType << std::endl;
  mesher->SetSolidModelKernel(solidKernel);

  // Load the solid model.
  //
  // [TODO:DaveP] There does not seem to be any model extension
  // information so hack it here.
  //
  std::string ext;
  if (solidKernel == SM_KT_POLYDATA) { 
      ext = ".vtp";
  } else if(solidKernel == SM_KT_OCCT) { 
      ext = ".brep";
  } else if (solidKernel == SM_KT_PARASOLID) { 
      ext = ".xmt_txt";
  }

  std::cout << "[MeshingGroupSetModel] Load solid model " << std::endl;
  fileName = modelDirName + modelName + ext;
  if (mesher->LoadModel(const_cast<char*>(fileName.c_str())) == SV_ERROR) {
      api.error("Error loading a solid model from the file '" + std::string(fileName) + "'.");
      return false;
  }

  // Set wall face IDs.
  std::cout << "[MeshingGroupSetModel] Set wall face IDs " << std::endl;
  std::vector<int> wallFaceIDs = solidModelElement->GetWallFaceIDs();
  std::cout << "[MeshingGroupSetModel] wall face IDs: " << wallFaceIDs.size() << std::endl;
  if (mesher->SetWalls(wallFaceIDs.size(), &wallFaceIDs[0]) != SV_OK) {
      api.error("Error setting wall IDs."); 
      return false;
  }

  std::cout << "[MeshingGroupSetModel] GetFaceNameIDMap " << std::endl;
  faceIDMap = solidModelElement->GetFaceNameIDMap();
  std::cout << "[MeshingGroupSetModel] Done. " << std::endl;
  return true;
}

//////////////////////////////////////////////////////
//       G r o u p  C l a s s  M e t h o d s        //
//////////////////////////////////////////////////////
//
// SV Python solid.Group methods. 

//-------------------------
// MeshingGroup_get_time_size 
//-------------------------
//
// [TODO:DaveP] bad method name: get_number_time_steps() ?
//
PyDoc_STRVAR(MeshingGroup_get_time_size_doc,
  "get_time_size_doc,(name) \n\ 
   \n\
   Store the polydata for the named contour into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject * 
MeshingGroup_get_time_size(PyMeshingGroup* self, PyObject* args)
{
/*
  int timestepSize = self->contourGroup->GetTimeSize();
  return Py_BuildValue("i", timestepSize); 
*/
}

//-----------------------
// MeshingGroup_get_size 
//-----------------------
//
PyDoc_STRVAR(MeshingGroup_number_of_models_doc,
  "get_size() \n\ 
   \n\
   Get the number of solid models in the group. \n\
   \n\
   Args: \n\
     None \n\
   Returns (int): The number of solid models in the group.\n\
");

static PyObject * 
MeshingGroup_number_of_models(PyMeshingGroup* self, PyObject* args)
{
  auto meshingGroup = self->meshingGroup;
  int numSolidModels = meshingGroup->GetTimeSize();
  std::cout << "[MeshingGroup_number_of_models] Number of solid models: " << numSolidModels << std::endl;
  return Py_BuildValue("i", numSolidModels); 
}

//-----------------------
// MeshingGroup_get_mesh 
//-----------------------
PyDoc_STRVAR(MeshingGroup_get_mesh_doc,
  "get_mesh(time) \n\ 
   \n\
   Get the mesh for the given time. The meshing options are also returned. \n\
   \n\
   Args: \n\
     time (int): The time to get the mesh for. \n\
   \n\
   Returns meshing.Mesher and meshing.Options objects. \n\
");

static PyObject * 
MeshingGroup_get_mesh(PyMeshingGroup* self, PyObject* args)
{
  std::cout << "================ MeshingGroup_get_mesh ================" << std::endl;
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int index;

  if (!PyArg_ParseTuple(args, api.format, &index)) {
     return api.argsError();
  }

  auto meshingGroup = self->meshingGroup;
  int numMeshes = meshingGroup->GetTimeSize();
  std::cout << "[MeshingGroup_get_mesh] Number of meshes: " << numMeshes << std::endl;

  // Check for valid index.
  if ((index < 0) || (index > numMeshes-1)) {
      api.error("The index argument '" + std::to_string(index) + "' is must be between 0 and " +
        std::to_string(numMeshes-1));
      return nullptr;
  }

  // Get the mesh for the given time index.
  sv4guiMesh* guiMesh = meshingGroup->GetMesh(index);
  if (guiMesh == nullptr) {
      api.error("ERROR getting the mesh for the index argument '" + std::to_string(index) + "'.");
      return nullptr;
  }

  // Get the mesh from the type read from a .msh file.
  auto meshType = guiMesh->GetType();
  std::cout << "[MeshingGroup_get_mesh] Mesh type: " <<  meshingGroup->GetType() << std::endl;
  std::transform(meshType.begin(), meshType.end(), meshType.begin(), ::toupper);
  cvMeshObject::KernelType meshKernel;
  try {
      meshKernel = kernelNameEnumMap.at(meshType);
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown meshing type '" + std::string(meshType) + "'." +
          " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  // Create a Python mesher object.
  auto pyMesherObj = PyMesherCreateObject(meshKernel);
  auto mesher = ((PyMeshingMesherClass*)pyMesherObj)->mesher;

  // Set the solid model associated with the mesher.
  std::map<std::string,int> faceIDMap;
  auto fileName = self->fileName;
  if (!MeshingGroupSetModel(api, mesher, meshingGroup, index, fileName, faceIDMap)) { 
      return nullptr;
  }
  std::cout << "[MeshingGroup_get_mesh] faceIDMap: " << faceIDMap.size() <<  std::endl;

  // Load the volume and surface meshes.
  size_t strIndex = 0;
  strIndex = fileName.find(".msh", strIndex);
  fileName.erase(strIndex);
  auto volFileName = fileName + ".vtu";
  auto surfFileName = fileName + ".vtp";
  mesher->LoadMesh(const_cast<char*>(volFileName.c_str()), const_cast<char*>(surfFileName.c_str()));

  // Create an options object and set meshing parameters from the 
  // command history read from the .msh file.
  //
  // Options must be processed after the solid model is loaded.
  //
  std::cout << "[MeshingGroup_get_mesh] Create an options object. " <<  std::endl;
  auto commands = guiMesh->GetCommandHistory();
  PyObject *options;
  try {
      ((PyMeshingMesherClass*)pyMesherObj)->CreateOptionsFromList(mesher, commands, faceIDMap, &options);
  } catch (const std::exception& exception) {
      api.error(exception.what());
      return nullptr;
  }

  // Return mesh and options objects.
  return Py_BuildValue("N,N", pyMesherObj, options); 
}

//--------------------
// MeshingGroup_write
//--------------------
//
// [TODO:DaveP] finish implementing this.
//
PyDoc_STRVAR(MeshingGroup_write_doc,
  "write(file_name) \n\ 
   \n\
   Write the contour group to an SV .pth file.\n\
   \n\
   Args: \n\
     file_name (str): The name of the file to write the contour group to.\n\
");

static PyObject *
MeshingGroup_write(PyMeshingGroup* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* fileName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      return api.argsError();
  }

/*

  try {
      if (sv3::ContourIO().Write(fileName, self->contourGroup) != SV_OK) {
          api.error("Error writing contour group to the file '" + std::string(fileName) + "'.");
          return nullptr;
      }
  } catch (const std::exception& readException) {
      api.error("Error writing contour group to the file '" + std::string(fileName) + "': " + readException.what());
      return nullptr;
  }
*/

  return SV_PYTHON_OK;
}


////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MESHING_GROUP_CLASS = "Group";
// Dotted name that includes both the module name and the name of the 
// type within the module.
static char* MESHING_GROUP_MODULE_CLASS = "meshing.Group";

PyDoc_STRVAR(MeshingGroup_doc, "meshing.Group functions");

//-----------------------
// PyMeshingGroupMethods 
//-----------------------
// Define the methods for the contour.Group class.
//
static PyMethodDef PyMeshingGroupMethods[] = {

  {"get_mesh", (PyCFunction)MeshingGroup_get_mesh, METH_VARARGS, MeshingGroup_get_mesh_doc},

  // {"number_of_models", (PyCFunction)MeshingGroup_number_of_models, METH_VARARGS, MeshingGroup_number_of_models_doc},

  //{"get_time_size", (PyCFunction)MeshingGroup_get_time_size, METH_NOARGS, MeshingGroup_get_time_size_doc},

  //{"write", (PyCFunction)MeshingGroup_write, METH_VARARGS, MeshingGroup_write_doc},

  {NULL, NULL}
};

//-------------------------
// PyMeshingGroupClassType 
//-------------------------
// Define the Python type that stores MeshingGroup data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
PyTypeObject PyMeshingGroupClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  MESHING_GROUP_MODULE_CLASS,     
  sizeof(PyMeshingGroup)
};

//---------------------
// PyMeshingGroup_init
//---------------------
// This is the __init__() method for the meshing.Group class. 
//
// This function is used to initialize an object after it is created.
//
// Arguments:
//
//   fileName - An SV .msh file. A new MeshingGroup object is created from 
//     the contents of the file. (optional)
//
static int 
PyMeshingGroupInit(PyMeshingGroup* self, PyObject* args)
{
  static int numObjs = 1;
  std::cout << "[PyMeshingGroupInit] New MeshingGroup object: " << numObjs << std::endl;
  auto api = SvPyUtilApiFunction("|s", PyRunTimeErr, __func__);
  char* fileName = nullptr;
  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      api.argsError();
      return 1;
  }
  if (fileName != nullptr) {
      std::cout << "[PyMeshingGroupInit] File name: " << fileName << std::endl;
      self->meshingGroupPointer = MeshingGroupRead(fileName);
      self->meshingGroup = dynamic_cast<sv4guiMitkMesh*>(self->meshingGroupPointer.GetPointer());
      self->fileName = std::string(fileName);
  } else {
      self->meshingGroup = sv4guiMitkMesh::New();
  }

  if (self->meshingGroup == nullptr) { 
      std::cout << "[PyMeshingGroupInit] ERROR reading File name: " << fileName << std::endl;
      return -1;
  }

  numObjs += 1;
  return 0;
}

//-------------------
// PyMeshingGroupNew 
//-------------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyMeshingGroupNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyMeshingGroupNew] PyMeshingGroupNew " << std::endl;
  auto self = (PyMeshingGroup*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyMeshingGroupNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }
  return (PyObject *) self;
}

//-----------------------
// PyMeshingGroupDealloc 
//-----------------------
//
static void
PyMeshingGroupDealloc(PyMeshingGroup* self)
{
  std::cout << "[PyMeshingGroupDealloc] Free PyMeshingGroup" << std::endl;
  // Can't delete meshingGroup because it has a protected detructor.
  //delete self->meshingGroup;
  Py_TYPE(self)->tp_free(self);
}

//---------------------------
// SetMeshingGroupTypeFields 
//---------------------------
// Set the Python type object fields that stores Contour data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetMeshingGroupTypeFields(PyTypeObject& solidType)
{
  // Doc string for this type.
  solidType.tp_doc = "MeshingGroup  objects";
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  solidType.tp_new = PyMeshingGroupNew;
  solidType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  solidType.tp_init = (initproc)PyMeshingGroupInit;
  solidType.tp_dealloc = (destructor)PyMeshingGroupDealloc;
  solidType.tp_methods = PyMeshingGroupMethods;
}

//----------------------
// CreatePyMeshingGroup
//----------------------
// Create a PyMeshingGroupType object.
//
// If the 'meshingGroup' argument is not null then use that 
// for the PyMeshingGroupType.meshingGroup data.
//
PyObject *
CreatePyMeshingGroup(sv4guiMitkMesh::Pointer meshingGroup)
{
  std::cout << std::endl;
  std::cout << "========== CreatePyMeshingGroup ==========" << std::endl;
  std::cout << "[CreatePyMeshingGroup] Create MeshingGroup object ... " << std::endl;
  auto meshingGroupObj = PyObject_CallObject((PyObject*)&PyMeshingGroupClassType, NULL);
  auto pyMeshingGroup = (PyMeshingGroup*)meshingGroupObj;

  if (meshingGroup != nullptr) {
      //delete pyMeshingGroup->meshingGroup;
      pyMeshingGroup->meshingGroup = meshingGroup;
  }
  std::cout << "[CreatePyContour] pyMeshingGroup id: " << pyMeshingGroup->id << std::endl;
  return meshingGroupObj;
}

