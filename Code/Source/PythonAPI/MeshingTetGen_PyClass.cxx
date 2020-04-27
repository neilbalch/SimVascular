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

// The functions defined here implement the SV Python API meshing module TetGen mesh generator class. 
//
// The class name is 'meshing.TetGen'.

#include "sv4gui_ModelUtils.h"

//----------------------
// PyMeshingTetGenClass
//----------------------
// Define the PyMeshingTetGenClass class.
//
typedef struct {
  PyMeshingMesherClass super;
} PyMeshingTetGenClass;

// Define the names assocuted with TetGen meshing parameters.
//
namespace MeshingTetGenParameters {
  // Parameter names.
  std::string AllowMultipleRegions("AllowMultipleRegions");
};

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

namespace MeshingTetGen {

//-------------------------------
// MeshingTetGenCheckModelLoaded
//-------------------------------
// Check if the mesh has a solid model.
//
bool
MeshingTetGenCheckModelLoaded(PyMeshingTetGenClass* self)
{
  auto mesher = self->super.mesher;
  return mesher->HasSolid();
}

//--------------------------
// MeshingTetGenCheckOption
//--------------------------
// Check if an option can be correctly set for the mesh. 
//
// The LocalEdgeSize option needs to have a model defined for the mesh.
//
bool
MeshingTetGenCheckOption(PyMeshingTetGenClass* self, std::string& name, SvPyUtilApiFunction& api)
{
  // The LocalEdgeSize option needs to have the model set for the mesh.
  if (name == TetGenOption::LocalEdgeSize) {
      if (!MeshingTetGenCheckModelLoaded(self)) {
          api.error("A model must be defined for the mesh. Use the 'load_model' method to define a model for the mesh.");
          return false;
      }
  }

  return true;
}

//----------------------
// InitMeshSizingArrays
//----------------------
//
void 
InitMeshSizingArrays(SvPyUtilApiFunction& api, cvMeshObject* mesher, PyObject* options)
{

}

//--------------------------------
// MeshingTetGenGenerateLocalSize
//--------------------------------
// Generate the local edge size mesh sizing array.
//
// If the size mesh sizing array does not exist it will
// be created. Each local edge size value updates the
// array with the edge size for each face ID.
//
void
GenerateLocalSizeArray(SvPyUtilApiFunction& api, cvTetGenMeshObject* mesher, PyObject* options)
{
  std::cout << "[GenerateLocalSizeArray] " << std::endl;
  std::cout << "[GenerateLocalSizeArray] ========== GenerateLocalSizeArray =========" << std::endl;
  auto optionName = TetGenOption::LocalEdgeSize;

  auto listObj = PyObject_GetAttrString(options, optionName);
  if (listObj == Py_None) {
      std::cout << "[GenerateLocalSizeArray] Could not find the option named '" << optionName << "'." << std::endl;
      return;
  }

  if (!PyList_Check(listObj)) {
      std::cout << "[GenerateLocalSizeArray] The option named '" << optionName << "' is not a list." << std::endl;
      return;
  }

  // Add local edge size values to the mesh size array.
  auto num = PyList_Size(listObj);
  std::cout << "[GenerateLocalSizeArray] List size: " << num << std::endl;
  for (int i = 0; i < num; i++) {
      auto item = PyList_GetItem(listObj, i);
      int faceID;
      double edgeSize;
      GetLocalEdgeSizeValues(item, faceID, edgeSize);
      std::cout << "[GenerateLocalSizeArray] Face ID: " << faceID << "  edgeSize: " << edgeSize << std::endl;
      if (mesher->GenerateLocalSizeSizingArray(faceID, edgeSize) != SV_OK) {
          std::cout << "[GenerateLocalSizeArray] ERROR generating local edge size array." << std::endl;
      }
  }
}

//----------------------------
// GenerateRadiusMeshingArray
//----------------------------
//
void
GenerateRadiusMeshingArray(SvPyUtilApiFunction& api, cvTetGenMeshObject* mesher, PyObject* options)
{
  std::cout << "[GenerateRadiusMeshingArray] " << std::endl;
  std::cout << "[GenerateRadiusMeshingArray] ========== GenerateRadiusMeshingArray =========" << std::endl;

  // Get the radius meshing option values.
  double scale; 
  vtkPolyData* centerlines;
  GetRadiusMeshingValues(options, &scale, &centerlines);
  std::cout << "[GenerateRadiusMeshingArray] Scale: " << scale << std::endl;
  if (centerlines == nullptr) {
      std::cout << "[GenerateRadiusMeshingArray] No centerlines " << std::endl;
      return;
  } else {
      std::cout << "[GenerateRadiusMeshingArray] Have centerlines " << std::endl;
  }

  // Calculate the distance to centerlines.
  auto solid = mesher->GetSolid();
  if (solid == nullptr) {
      api.error("A solid model must be defined for radius meshing.");
      return;
  }

  auto distance = sv4guiModelUtils::CalculateDistanceToCenterlines(centerlines, solid->GetVtkPolyData());
  if (distance == nullptr) {
      api.error("Unable to compute the distance to centerlines.");
      return;
  }
  std::cout << "[GenerateRadiusMeshingArray] Distance computed " << std::endl;

  mesher->SetVtkPolyDataObject(distance);

  char* sizeFunctionName = "DistanceToCenterlines";
  mesher->SetSizeFunctionBasedMesh(scale, sizeFunctionName);
}

//-------------------------------
// GenerateSphereRefinementArray 
//-------------------------------
//
void
GenerateSphereRefinementArray(SvPyUtilApiFunction& api, cvTetGenMeshObject* mesher, PyObject* options)
{
  std::cout << "[GenerateSphereRefinementArray] " << std::endl;
  std::cout << "[GenerateSphereRefinementArray] ========== GenerateSphereRefinementArray =========" << std::endl;

  auto optionName = TetGenOption::SphereRefinement;
  auto listObj = PyObject_GetAttrString(options, optionName);
  if (listObj == Py_None) {
      std::cout << "[GenerateSphereRefinementArray] Could not find the option named '" << optionName << "'." << std::endl;
      return;
  }

  if (!PyList_Check(listObj)) {
      std::cout << "[GenerateSphereRefinementArray] The option named '" << optionName << "' is not a list." << std::endl;
      return;
  }

  // Add sphere refinement values to the mesh size array.
  auto num = PyList_Size(listObj);
  std::cout << "[GenerateSphereRefinementArray] List size: " << num << std::endl;
  for (int i = 0; i < num; i++) {
      auto item = PyList_GetItem(listObj, i);
      double edgeSize;
      double radius;
      std::vector<double> center;
      GetSphereRefinementValues(item, edgeSize, radius, center);
      std::cout << "[GenerateSphereRefinementArray] Edge size: " << edgeSize << std::endl;
      std::cout << "[GenerateSphereRefinementArray] Radius: " << radius  << std::endl;

      if (mesher->SetSphereRefinement(edgeSize, radius, center.data()) != SV_OK) {
          std::cout << "[GenerateSphereRefinementArray] ERROR generating sphere refinement array." << std::endl;
          return ;
      }
  }
}

//-----------------------------
// GenGenerateMeshSizingArrays
//-----------------------------
// Generate mesh sizing arrays that set edge sizes for the elements (cells) 
// for the mesh surface model (see TGenUtils_SetLocalMeshSize()).
//
// In SV the mesh sizing arrays are computed in both cvTetGenMeshObject::SetMeshOptions()
// and sv4guiMeshTetGen::Execute(). The arrays are computed here so that they can be managed 
// within the Pythonn API. 
//
// Radius-based meshing arrays are computed first if that option is enabled because edge sizes
// are set for all surface elements. 
//
bool 
GenerateMeshSizingArrays(SvPyUtilApiFunction& api, cvTetGenMeshObject* mesher, PyObject* options)
{
  using namespace TetGenOption;
  std::cout << "[GenerateMeshSizingArrays] " << std::endl;
  std::cout << "[GenerateMeshSizingArrays] ========== GenerateMeshSizingArrays =========" << std::endl;

  InitMeshSizingArrays(api, mesher, options);

  if (RadiusMeshingIsOn(options)) {
      std::cout << "[MeshingTetGenGenerateMeshSizingArrays] Radius meshing is on" << std::endl;
      GenerateRadiusMeshingArray(api, mesher, options);
  }

  if (LocalEdgeSizeIsOn(options)) {
      std::cout << "[MeshingTetGenGenerateMeshSizingArrays] Local edge size is on" << std::endl;
      GenerateLocalSizeArray(api, mesher, options);
  }

  if (SphereRefinementIsOn(options)) {
      std::cout << "[MeshingTetGenGenerateMeshSizingArrays] Sphere refinement is on" << std::endl;
      GenerateSphereRefinementArray(api, mesher, options);
  }

  return true;
}

//------------
// SetOptions
//------------
// Set TetGen options from a Python object.
//
bool 
SetOptions(SvPyUtilApiFunction& api, cvMeshObject* mesher, PyObject* options)
{
  std::cout << "[SetOptions] " << std::endl;
  std::cout << "[SetOptions] ========== SetOptions =========" << std::endl;
  //auto options = (PyMeshingTetGenOptionsClass*)optionsObj;

  // Set options that are not a list.
  //
  for (auto const& entry : TetGenOption::pyToSvNameMap) {
      auto pyName = entry.first;
      if (TetGenOption::ListOptions.count(std::string(pyName)) != 0) {
          continue;
      }

      auto svName = entry.second;
      auto values = PyTetGenOptionsGetValues(options, pyName);
      int numValues = values.size();
      if (numValues == 0) { 
          continue;
      }

      std::cout << "[SetOptions] name: " << svName << "  num values: " << numValues << "  values: ";
      for (auto const val : values) {
          std::cout << val << " "; 
      }
      std::cout << std::endl; 
 
      if (mesher->SetMeshOptions(svName, values.size(), values.data()) == SV_ERROR) {
        api.error("Error setting TetGen meshing '" + std::string(pyName) + "' option.");
        return false;
      }
  }

  // Set options that are a list.
  //
  for (auto const& entry : TetGenOption::pyToSvNameMap) {
      auto pyName = entry.first;
      if (TetGenOption::ListOptions.count(std::string(pyName)) == 0) {
          continue;
      }
      auto svName = entry.second;
      auto valuesList = PyTetGenOptionsGetListValues(options, pyName);
      int numListValues = valuesList.size();
      std::cout << "[SetOptions] List name: " << svName << "  num values: " << numListValues << std::endl;
      if (numListValues == 0) { 
          continue;
      }
      for (auto& values : valuesList) { 
          std::cout << "[SetOptions] name: " << svName << "  num values: " << values.size() << "  values: ";
          for (auto const val : values) {
              std::cout << val << " "; 
          }
          std::cout << std::endl; 
          if (mesher->SetMeshOptions(svName, values.size(), values.data()) == SV_ERROR) {
            api.error("Error setting TetGen meshing '" + std::string(pyName) + "' option.");
            return false;
          }
      }
  }

  return true;
}

}; // namespace MeshingTetGen

/////////////////////////////////////////////////////////////////
//              C l a s s   F u n c t i o n s                  //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the PyMeshingTetGenClass class. 

//-------------------------
// MeshingTetGen_available 
//-------------------------
//
static PyObject *
MeshingTetGen_available(PyObject* self, PyObject* args )
{
  return Py_BuildValue("s", "The TetGen mesh generator is available");
}

//----------------------
// Mesher_generate_mesh 
//----------------------
//
PyDoc_STRVAR(MesherTetGen_generate_mesh_doc,
" generate_mesh()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *
MesherTetGen_generate_mesh(PyMeshingMesherClass* self, PyObject* args, PyObject* kwargs)
{
  using namespace MeshingTetGen;
  std::cout << std::endl;
  std::cout << "==================== MesherTetGen_generate_mesh ====================" << std::endl;

  auto api = SvPyUtilApiFunction("O!", PyRunTimeErr, __func__);
  static char *keywords[] = {"options", NULL};
  PyObject* options;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &PyTetGenOptionsType, &options)) {
      return api.argsError();
  }
  auto mesher = self->mesher;

  if (!SetOptions(api, mesher, options)) {
      return nullptr;
  }

  // Set wall face IDs.
  if (self->wallFaceIDs.size() != 0) { 
      int numIDs = self->wallFaceIDs.size();
      if (mesher->SetWalls(numIDs, self->wallFaceIDs.data()) == SV_ERROR) {
          api.error("Error setting walls.");
          return nullptr;
      }
  }

  // Generate mesh sizing function arrays, local edge size, radius meshing, etc.
  auto tetGenMesher = dynamic_cast<cvTetGenMeshObject*>(mesher);
  if (!GenerateMeshSizingArrays(api, tetGenMesher, options)) {
      return nullptr;
  }

  // Generate the mesh.
  if (mesher->GenerateMesh() == SV_ERROR) {
      api.error("Error generating a mesh.");
      return nullptr;
  }

  Py_RETURN_NONE;
}

////////////////////////////////////////////////////////
//           C l a s s    D e f i n i t i o n         //
////////////////////////////////////////////////////////

static char* MESHING_TETGEN_CLASS = "TetGen";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* MESHING_TETGEN_MODULE_CLASS = "meshing.TetGen";

PyDoc_STRVAR(PyMeshingTetGenClass_doc, "TetGen mesh generator class methods.");

//------------------------
// PyMeshingTetGenMethods
//------------------------
//
static PyMethodDef PyMeshingTetGenMethods[] = {
 { "generate_mesh", (PyCFunction)MesherTetGen_generate_mesh, METH_VARARGS|METH_KEYWORDS, MesherTetGen_generate_mesh_doc},
  {NULL, NULL}
};

//---------------------
// PyMeshingTetGenInit 
//---------------------
// This is the __init__() method for the MeshGenerator class. 
//
// This function is used to initialize an object after it is created.
//
static int 
PyMeshingTetGenInit(PyMeshingTetGenClass* self, PyObject* args, PyObject *kwds)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, "MeshGenerator");
  static int numObjs = 1;
  std::cout << "[PyMeshingTetGenClassInit] New PyMeshingTetGenClass object: " << numObjs << std::endl;
  self->super.mesher = new cvTetGenMeshObject();
  self->super.CreateOptionsFromList = PyTetGenOptionsCreateFromList;
  numObjs += 1;
  return 0;
}

//--------------------
// PyMeshingTetGenNew 
//--------------------
//
static PyObject *
PyMeshingTetGenNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyMeshingTetGenNew] PyMeshingTetGenNew " << std::endl;
  auto self = (PyMeshingMesherClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->super.id = 2;
  }
  return (PyObject*)self;
}

//------------------------
// PyMeshingTetGenDealloc 
//------------------------
//
static void
PyMeshingTetGenDealloc(PyMeshingTetGenClass* self)
{
  std::cout << "[PyMeshingTetGenDealloc] Free PyMeshingTetGen" << std::endl;
  delete self->super.mesher;
  Py_TYPE(self)->tp_free(self);
}

//--------------------------
// PyMeshingTetGenClassType 
//--------------------------
// Define the Python type object that stores PolyDataSolidClass data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
PyTypeObject PyMeshingTetGenClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  .tp_name = MESHING_TETGEN_MODULE_CLASS,
  .tp_basicsize = sizeof(PyMeshingTetGenClass)
};

//----------------------------
// SetMeshingTetGenTypeFields 
//----------------------------
// Set the Python type object fields that stores TetGen mesher data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
void
SetMeshingTetGenTypeFields(PyTypeObject& mesherType)
 {
  // Doc string for this type.
  mesherType.tp_doc = PyMeshingTetGenClass_doc;

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  mesherType.tp_new = PyMeshingTetGenNew;
  //.tp_new = PyType_GenericNew,

  // Subclass to PyMeshingMesherClassType.
  mesherType.tp_base = &PyMeshingMesherClassType; 

  mesherType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  mesherType.tp_init = (initproc)PyMeshingTetGenInit;
  mesherType.tp_dealloc = (destructor)PyMeshingTetGenDealloc;
  mesherType.tp_methods = PyMeshingTetGenMethods;
};

//------------------
// PyAPI_InitTetGen
//------------------
// Setup creating TetGen mesh generation objects.
//
// This is called from 'meshing' module init function PyInit_PyMeshing(). 
//
void
PyAPI_InitTetGen()
{
  PyMesherCtorMap[cvMeshObject::KERNEL_TETGEN] = []()->PyObject* {return PyObject_CallObject((PyObject*)&PyMeshingTetGenClassType, NULL);};
}

