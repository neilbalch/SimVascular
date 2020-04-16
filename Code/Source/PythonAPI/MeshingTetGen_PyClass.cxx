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

 //void (*CreateOptionsFromList)(cvMeshObject*, std::vector<std::string>&, std::map<std::string,int>&, PyObject**);

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

/* [TODO:DaveP] I don't think we need this
//------------------------
// MeshingTetGenSetOption
//------------------------
bool
MeshingTetGenSetOption(cvMeshObject* mesher, std::string& name, std::vector<double>& values)
{
  std::cout << "[MeshingTetGenSetOption]  ========= name '" << name << "' ==============" << std::endl;
  if (name == std::string(TetGenOption::SphereRefinement))  {
      std::cout << "[MeshingTetGenSetOption]  @@@@@@ not option @@@@@@ " << name << std::endl;
      double edgeSize = values[0];
      double radius = values[1];
      double center[3] = { values[2], values[3], values[4] };
      if (mesher->SetSphereRefinement(edgeSize, radius, center) != SV_OK) {
          return false;
      }

  } else { 
      std::cout << "[MeshingTetGenSetOption]  ###### option ###### " << name << std::endl;
      if (mesher->SetMeshOptions(const_cast<char*>(name.c_str()), values.size(), values.data()) == SV_ERROR) {
          return false; 
      }
  }
  if (mesher->SetMeshOptions(const_cast<char*>(name.c_str()), values.size(), values.data()) == SV_ERROR) {
    return false; 
  }

 return true;
}
*/

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

//---------------------------
// MeshingTetGenSetParameter 
//---------------------------
// Set meshing parameters.
//
// These are meshing parameters that are set using cvMeshObject methods.
//
void
MeshingTetGenSetParameter(cvTetGenMeshObject* mesher, std::string& name, std::vector<std::string>& tokens)
{
  if (name == MeshingParameters::SphereRefinement) {
      auto edgeSize = std::stod(tokens[0]);
      auto radius = std::stod(tokens[1]);
      double center[3] = {std::stod(tokens[2]), std::stod(tokens[3]), std::stod(tokens[4])};
      if (mesher->SetSphereRefinement(edgeSize, radius, center) != SV_OK) {
          throw("Failed to set sphere refinement parameter.");
      }
  } else if (name == MeshingTetGenParameters::AllowMultipleRegions) {
       bool value = (std::stoi(tokens[0]) == 1);
       mesher->SetAllowMultipleRegions(value);
  }
}


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

//------------------------------
// MeshingTetGen_create_options
//------------------------------
//
PyDoc_STRVAR(MeshingTetGen_create_options_doc,
  "create_options(global_edge_size, surface_mesh_flag=1, volume_mesh_flag=1, mesh_wall_first=1)  \n\ 
  \n\
  Create a TetGenOptions object. \n\
  \n\
  Args:                                    \n\
    global_edge_size (float): The value used to set the global_edge_size parameter. \n\
    surface_mesh_flag (int): The value used to set the surface_mesh_flag parameter. \n\
    volume_mesh_flag (int): The value used to set the volume_mesh_flag parameter. \n\
    mesh_wall_first (int): The value used to set the mesh_wall_first parameter. \n\
");

static PyObject *
MeshingTetGen_create_options(PyObject* self, PyObject* args, PyObject* kwargs )
{
  return CreateTetGenOptionsType(args, kwargs);
}

//---------------------------
// MeshingTetGen_set_options
//---------------------------
//
PyDoc_STRVAR(MeshingTetGen_set_options_doc,
  "set_options(options)  \n\ 
  \n\
  Set the TetGen mesh generation options. \n\
  \n\
  Args:                                    \n\
    options (meshing.TetGenOptions): A TetGenOptions options object containing option values. \n\
");

static PyObject *
MeshingTetGen_set_options(PyMeshingTetGenClass* self, PyObject* args )
{
  std::cout << "[MeshingTetGen_set_options] " << std::endl;
  std::cout << "[MeshingTetGen_set_options] ========== MeshingTetGen_set_options =========" << std::endl;
  auto api = SvPyUtilApiFunction("O!", PyRunTimeErr, __func__);
  PyObject* options;

  if (!PyArg_ParseTuple(args, api.format, &PyTetGenOptionsType, &options)) {
      return api.argsError();
  }

  auto mesher = self->super.mesher;

  // Set options that are not a list.
  //
  for (auto const& entry : TetGenOption::pyToSvNameMap) {
      auto pyName = entry.first;
      if (TetGenOption::ListOptions.count(std::string(pyName)) != 0) {
          continue;
      }

      // Check if an option can be correctly set for the mesh. 
      if (!MeshingTetGenCheckOption(self, pyName, api)) {
          return nullptr;
      }

      auto svName = entry.second;
      auto values = PyTetGenOptionsGetValues(options, pyName);
      int numValues = values.size();
      if (numValues == 0) { 
          continue;
      }

      std::cout << "[MeshingTetGen_set_options] name: " << svName << "  num values: " << numValues << "  values: ";
      for (auto const val : values) {
          std::cout << val << " "; 
      }
      std::cout << std::endl; 
 
      if (mesher->SetMeshOptions(svName, values.size(), values.data()) == SV_ERROR) {
        api.error("Error setting TetGen meshing '" + std::string(pyName) + "' option.");
        return nullptr;
      }
  }

  // Set options that are a list.
  //
  // For example local_edge_size is a list of dicts.
  //
  std::cout << "[MeshingTetGen_set_options] --------- process lists --------- " << std::endl;
  for (auto const& entry : TetGenOption::pyToSvNameMap) {
      auto pyName = entry.first;
      if (TetGenOption::ListOptions.count(std::string(pyName)) == 0) {
          continue;
      }
      auto svName = entry.second;
      auto valuesList = PyTetGenOptionsGetListValues(options, pyName);
      int numListValues = valuesList.size();
      std::cout << "[MeshingTetGen_set_options] list name: " << svName << "  num values: " << numListValues << std::endl;
      if (numListValues == 0) { 
          continue;
      }
      for (auto& values : valuesList) { 
          std::cout << "[MeshingTetGen_set_options] name: " << svName << "  num values: " << values.size() << "  values: ";
          for (auto const val : values) {
              std::cout << val << " "; 
          }
          std::cout << std::endl; 
          if (mesher->SetMeshOptions(svName, values.size(), values.data()) == SV_ERROR) {
            api.error("Error setting TetGen meshing '" + std::string(pyName) + "' option.");
            return nullptr;
          }
      }
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
  {"create_options", (PyCFunction)MeshingTetGen_create_options, METH_VARARGS|METH_KEYWORDS, MeshingTetGen_create_options_doc},
  {"set_options", (PyCFunction)MeshingTetGen_set_options, METH_VARARGS, MeshingTetGen_set_options_doc},
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

