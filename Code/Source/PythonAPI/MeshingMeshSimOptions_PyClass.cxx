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

// Define the Python 'meshing.MeshSimOptions' class that encapsulates the parameters
// used for generating a mesh using MeshSim. Options are stored as Python class attributes 
// and are set directly in the object created from that class.
//
//     options = sv.meshing.MeshSimOptions(global_edge_size=0.1)
//     options.global_edge_size = 0.1
//
// Once options parameters have been set they are used to set the MeshSim mesher options using
//
//    mesher.set_options(options)
//
// SV uses string literals to process options one at a time using 
//
//    int cvMeshSimMeshObject::SetMeshOptions(char *flags, int numValues, double *values)
//
#ifndef PYAPI_MESHING_MESHSIM_OPTIONS_H
#define PYAPI_MESHING_MESHSIM_OPTIONS_H 

#include <string>
#include <structmember.h>

//------------------------------
// PyMeshingMeshSimOptionsClass 
//------------------------------
// Define the PyMeshingMeshSimOptionsClass data. 
//
typedef struct {
  PyObject_HEAD
  PyObject* global_edge_size;
  PyObject* local_edge_size;
  int surface_mesh_flag;
  int volume_mesh_flag;
} PyMeshingMeshSimOptionsClass;

//--------------
// MeshSimOption
//--------------
// PyMeshingMeshSimOptionsClass attribute names.
//
// [TODO:DaveP] Maybe change some of these names to be more descriptive.
//
namespace MeshSimOption {
  char* GlobalEdgeSize = "global_edge_size";
  char* LocalEdgeSize = "local_edge_size";
  char* SurfaceMeshFlag = "surface_mesh_flag";
  char* VolumeMeshFlag = "volume_mesh_flag";

  // Parameter names for the 'global_edge_size' option.
  //
  std::string GlobalEdgeSize_Type = "dictionary ";
  std::string GlobalEdgeSize_Format = "{ 'edge_size':double, 'absolute':bool }";
  std::string GlobalEdgeSize_Desc = GlobalEdgeSize_Type + GlobalEdgeSize_Format;
  // Use char* for these because they are used in the Python C API functions.
  char* GlobalEdgeSize_SizeParam = "edge_size";
  char* GlobalEdgeSize_AbsoluteParam = "absolute";

  // Parameter names for the 'local_edge_size' option.
  //
  std::string LocalEdgeSize_Type = "dictionary ";
  std::string LocalEdgeSize_Format = "{ 'face_id':int, 'edge_size':double, 'absolute':bool }";
  std::string LocalEdgeSize_Desc = LocalEdgeSize_Type + LocalEdgeSize_Format;
  // Use char* for these because they are used in the Python C API functions.
  char* LocalEdgeSize_FaceIDParam = "face_id";
  char* LocalEdgeSize_EdgeSizeParam = "edge_size";
  char* LocalEdgeSize_AbsoluteParam = "absolute";

  // Create a map beteen Python and SV names. The SV names are needed when
  // setting mesh options.
  //
  std::map<std::string,char*> pyToSvNameMap = {
      {std::string(GlobalEdgeSize), "GlobalEdgeSize"},
      {std::string(LocalEdgeSize), "LocalEdgeSize"},
      {std::string(SurfaceMeshFlag), "SurfaceMeshFlag"},
      {std::string(VolumeMeshFlag), "VolumeMeshFlag"}
   };

};

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//----------------------------------------
// PyMeshSimOptionsGetLocalEdgeSizeValues
//----------------------------------------
// Get the parameter values for the LocalEdgeSize option. 
//
bool
PyMeshSimOptionsGetLocalEdgeSizeValues(PyObject* obj, int& faceID, double& edgeSize, double& absoluteFlag) 
{
  static std::string errorMsg = "The local_edge_size parameter must be a " + MeshSimOption::LocalEdgeSize_Desc; 

  // Check the LocalEdgeSize_RegionIDParam key.
  //
  PyObject* faceIDItem = PyDict_GetItemString(obj, MeshSimOption::LocalEdgeSize_FaceIDParam);
  if (faceIDItem == nullptr) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return false;
  }
  faceID = PyLong_AsLong(faceIDItem);
  if (PyErr_Occurred()) {
      return false;
  }
  if (faceID <= 0) {
      PyErr_SetString(PyExc_ValueError, "The face ID paramter must be > 0.");
      return false;
  }

  // Check the LocalEdgeSize_SizeParam key.
  //
  PyObject* sizeItem = PyDict_GetItemString(obj, MeshSimOption::LocalEdgeSize_EdgeSizeParam);
  if (sizeItem == nullptr) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return false;
  }

  edgeSize = PyFloat_AsDouble(sizeItem);
  if (PyErr_Occurred()) {
      return false;
  }
  if (edgeSize <= 0) {
      PyErr_SetString(PyExc_ValueError, "The size parameter must be > 0.");
      return false;
  }

  // Check the GlobalEdgeSize_AbsoluteParam key.
  //
  PyObject* absoluteItem = PyDict_GetItemString(obj, MeshSimOption::LocalEdgeSize_AbsoluteParam);
  if (absoluteItem == nullptr) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return false;
  }

  if (absoluteItem == Py_True) {
      absoluteFlag = 1.0;
  } else {
      absoluteFlag = 0.0;
  }

  return true;
}

//-----------------------------------------
// PyMeshSimOptionsGetGlobalEdgeSizeValues
//-----------------------------------------
// Get the parameter values for the GlobalEdgeSize option. 
//
bool
PyMeshSimOptionsGetGlobalEdgeSizeValues(PyObject* obj, double& edgeSize, double& absoluteFlag) 
{
  static std::string errorMsg = "The global_edge_size parameter must be a " + MeshSimOption::GlobalEdgeSize_Desc; 

  // Check the GlobalEdgeSize_AbsoluteParam key.
  //
  PyObject* absoluteItem = PyDict_GetItemString(obj, MeshSimOption::GlobalEdgeSize_AbsoluteParam);
  if (absoluteItem == nullptr) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return false;
  }

  if (absoluteItem == Py_True) { 
      absoluteFlag = 1.0; 
  } else {
      absoluteFlag = 0.0; 
  }

  // Check the GlobalEdgeSize_SizeParam key.
  //
  PyObject* sizeItem = PyDict_GetItemString(obj, MeshSimOption::GlobalEdgeSize_SizeParam);
  if (sizeItem == nullptr) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return false;
  }

  edgeSize = PyFloat_AsDouble(sizeItem);
  if (PyErr_Occurred()) {
      return false;
  }
  if (edgeSize <= 0) {
      PyErr_SetString(PyExc_ValueError, "The edge size parameter must be > 0.");
      return false;
  }

  return true;
}

//-----------------------------------------
// PyMeshSimOptionsCreateLocalEdgeSizeDict
//-----------------------------------------
//
static PyObject *
PyMeshSimOptionsCreateLocalEdgeSizeDict(SvPyUtilApiFunction& api, int faceID, double edgeSize, int absolute)
{
  if (edgeSize <= 0.0) {
      api.error("The '" + std::string(MeshSimOption::LocalEdgeSize_EdgeSizeParam) + "' must be > 0.");
      return nullptr;
  }

  if (faceID <= 0) {
      api.error("The '" + std::string(MeshSimOption::LocalEdgeSize_FaceIDParam) + "' must be > 0.");
      return nullptr;
  }

  auto value = Py_BuildValue("{s:i, s:d, s:O}", MeshSimOption::LocalEdgeSize_FaceIDParam, faceID,
                                                MeshSimOption::LocalEdgeSize_EdgeSizeParam, edgeSize,
                                                MeshSimOption::LocalEdgeSize_AbsoluteParam, PyBool_FromLong(absolute));

  return value;
}

//---------------------------
// PyMeshSimOptionsGetValues
//---------------------------
// Get attribute values from the MeshingOptions object.
//
// Return a vector of doubles to mimic how SV processes options.
//
// Parameter values must be stored in the same order as they 
// are processed in sv/MeshSimMeshObject/cvMeshSimMeshObject.cxx.
//
static std::vector<double> 
PyMeshSimOptionsGetValues(PyObject* meshingOptions, std::string name)
{
  std::vector<double> values;
  auto obj = PyObject_GetAttrString(meshingOptions, name.c_str());
  if (obj == Py_None) {
      return values;
  } 

  if (PyFloat_Check(obj)) {
      auto value = PyFloat_AsDouble(obj);
      values.push_back(value);
  } else if (PyInt_Check(obj)) {
      auto value = PyLong_AsDouble(obj);
      values.push_back(value);
  } else if (PyTuple_Check(obj)) {
      int num = PyTuple_Size(obj);
      for (int i = 0; i < num; i++) {
          auto item = PyTuple_GetItem(obj, i);
          auto value = PyFloat_AsDouble(item);
          values.push_back(value);
      }
  } else if (name == MeshSimOption::GlobalEdgeSize) {
      double edgeSize;
      double absolute;
      PyMeshSimOptionsGetGlobalEdgeSizeValues(obj, edgeSize, absolute);
      values.push_back(absolute);
      values.push_back(edgeSize);
  } else if (name == MeshSimOption::LocalEdgeSize) {
      int faceID;
      double edgeSize;
      double absolute;
      PyMeshSimOptionsGetLocalEdgeSizeValues(obj, faceID, edgeSize, absolute);
      values.push_back((double)faceID);
      values.push_back(absolute);
      values.push_back(edgeSize);
  }

  Py_DECREF(obj);
  return values;
}

////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//
// Methods for the MeshSimOptions class.

//--------------------------------------
// PyMeshSimOptions_add_local_edge_size
//--------------------------------------
//
PyDoc_STRVAR(PyMeshSimOptions_add_local_edge_size_doc,
  "add_local_edge_size(face_id, edge_size)  \n\ 
  \n\
  Create a parameter for the local_edge_size option. \n\
  \n\
  Args:  \n\
    face_id (int): The ID of a solid model face.  \n\
    edge_size (double): The edge size for the face.  \n\
");

static PyObject *
PyMeshSimOptions_add_local_edge_size(PyMeshingMeshSimOptionsClass* self, PyObject* args, PyObject* kwargs) 
{
  auto api = SvPyUtilApiFunction("id|O!", PyRunTimeErr, __func__);
  static char *keywords[] = { MeshSimOption::LocalEdgeSize_FaceIDParam, MeshSimOption::LocalEdgeSize_EdgeSizeParam, 
                              MeshSimOption::LocalEdgeSize_AbsoluteParam, NULL };
  int faceID = 0;
  double edgeSize = 0.0;
  PyObject* absoluteArg;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &faceID, &edgeSize, &PyBool_Type, &absoluteArg)) {
      return api.argsError();
  }

  int absolute = 1;
  if (absoluteArg == Py_False) {
      absolute = 0;
  }

  auto value = PyMeshSimOptionsCreateLocalEdgeSizeDict(api, faceID, edgeSize, absolute);
  if (value == nullptr) {
      return nullptr;
  }

  PyList_Append(self->local_edge_size, value);
  Py_INCREF(value);
  Py_RETURN_NONE;
}

//--------------------------------------------------
// PyMeshSimOptions_create_local_edge_size_parameter
//--------------------------------------------------
// [TODO:DaveP] figure out what the parameters are.
//
PyDoc_STRVAR(PyMeshSimOptions_create_local_edge_size_parameter_doc,
  " create_local_edge_size(region_id, size)  \n\ 
  \n\
  Create a parameter for the local_edge_size option. \n\
  \n\
  Args:  \n\
    region_id (int): The ID of the region.  \n\
    size (double): The edge size for the face.  \n\
");

static PyObject *
PyMeshSimOptions_create_local_edge_size_parameter(PyMeshingMeshSimOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("id|O!", PyRunTimeErr, __func__);
  static char *keywords[] = { MeshSimOption::LocalEdgeSize_FaceIDParam, MeshSimOption::LocalEdgeSize_EdgeSizeParam, 
                              MeshSimOption::LocalEdgeSize_AbsoluteParam, NULL };
  int faceID = 0;
  double edgeSize = 0.0;
  PyObject* absoluteArg;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &faceID, &edgeSize, &PyBool_Type, &absoluteArg)) {
      return api.argsError();
  }

  int absolute = 1;
  if (absoluteArg == Py_False) {
      absolute = 0;
  }

  auto value = PyMeshSimOptionsCreateLocalEdgeSizeDict(api, faceID, edgeSize, absolute);
  if (value == nullptr) {
      return nullptr;
  }

  return value; 
}

//----------------------------
// PyMeshSimOptions_get_values
//----------------------------
//
PyDoc_STRVAR(PyMeshSimOptions_get_values_doc,
" get_values()  \n\ 
  \n\
  Get the names and values of MeshSim mesh generation options. \n\
  \n\
  Args:  \n\
");

static PyObject *
PyMeshSimOptions_get_values(PyMeshingMeshSimOptionsClass* self, PyObject* args)
{
  PyObject* values = PyDict_New();

  PyDict_SetItemString(values, MeshSimOption::GlobalEdgeSize, self->global_edge_size); 
  PyDict_SetItemString(values, MeshSimOption::LocalEdgeSize, self->local_edge_size);
  PyDict_SetItemString(values, MeshSimOption::SurfaceMeshFlag, PyBool_FromLong(self->surface_mesh_flag));
  PyDict_SetItemString(values, MeshSimOption::VolumeMeshFlag, PyBool_FromLong(self->volume_mesh_flag));

  return values;
}

//-------------------------------
// PyMeshSimOptions_set_defaults
//-------------------------------
// Set the default options parameter values.
//
static PyObject *
PyMeshSimOptions_set_defaults(PyMeshingMeshSimOptionsClass* self)
{
  self->global_edge_size = Py_BuildValue(""); 
  self->local_edge_size = Py_BuildValue("[]"); 
  self->surface_mesh_flag = 0;
  self->volume_mesh_flag = 0;

  Py_RETURN_NONE;
}

//--------------------------------------
// PyMeshSimOptions_set_local_edge_size
//--------------------------------------
//
PyDoc_STRVAR(PyMeshSimOptions_set_local_edge_size_doc,
  "set_local_edge_size(face_id, edge_size)  \n\ 
  \n\
  Create a parameter for the local_edge_size option. \n\
  \n\
  Args:  \n\
    face_id (int): The ID of a solid model face.  \n\
    edge_size (double): The edge size for the face.  \n\
");

static PyObject *
PyMeshSimOptions_set_local_edge_size(PyMeshingMeshSimOptionsClass* self, PyObject* args, PyObject* kwargs) 
{
  auto api = SvPyUtilApiFunction("id|O!", PyRunTimeErr, __func__);
  static char *keywords[] = { MeshSimOption::LocalEdgeSize_FaceIDParam, MeshSimOption::LocalEdgeSize_EdgeSizeParam, 
                              MeshSimOption::LocalEdgeSize_AbsoluteParam, NULL };
  int faceID = 0;
  double edgeSize = 0.0;
  PyObject* absoluteArg;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &faceID, &edgeSize, &PyBool_Type, &absoluteArg)) {
      return api.argsError();
  }

  int absolute = 1;
  if (absoluteArg == Py_False) {
      absolute = 0;
  }

  auto value = PyMeshSimOptionsCreateLocalEdgeSizeDict(api, faceID, edgeSize, absolute);
  if (value == nullptr) {
      return nullptr;
  }

  // [TODO:DaveP] need to release memory here.
  auto edgeSizeList = PyList_New(1);
  PyList_SetItem(edgeSizeList, 0, value);
  Py_INCREF(value);
  self->local_edge_size = edgeSizeList; 

  Py_RETURN_NONE;
}

//------------------------
// PyMeshSimOptionsMethods 
//------------------------
//
static PyMethodDef PyMeshSimOptionsMethods[] = {
  {"add_local_edge_size", (PyCFunction)PyMeshSimOptions_add_local_edge_size, METH_VARARGS|METH_KEYWORDS, PyMeshSimOptions_add_local_edge_size_doc},
  {"create_local_edge_size_parameter", (PyCFunction)PyMeshSimOptions_create_local_edge_size_parameter, METH_VARARGS|METH_KEYWORDS, PyMeshSimOptions_create_local_edge_size_parameter_doc},
  {"get_values", (PyCFunction)PyMeshSimOptions_get_values, METH_NOARGS, PyMeshSimOptions_get_values_doc},
  {"set_local_edge_size", (PyCFunction)PyMeshSimOptions_set_local_edge_size, METH_VARARGS|METH_KEYWORDS, PyMeshSimOptions_set_local_edge_size_doc},
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    M e m b e r s                //
////////////////////////////////////////////////////////
//
// Define the PyMeshingMeshSimOptionsClass attribute names.
//
// The attributes can be set/get directly in from the MeshingOptions object.
//
static PyMemberDef PyMeshSimOptionsMembers[] = {

    {MeshSimOption::LocalEdgeSize, T_OBJECT_EX, offsetof(PyMeshingMeshSimOptionsClass, local_edge_size) , 0, "local edge size"},

    {MeshSimOption::SurfaceMeshFlag, T_BOOL, offsetof(PyMeshingMeshSimOptionsClass, surface_mesh_flag), 0, "surface_mesh_flag"},

    {MeshSimOption::SurfaceMeshFlag, T_BOOL, offsetof(PyMeshingMeshSimOptionsClass, surface_mesh_flag), 0, "surface_mesh_flag"},

    {MeshSimOption::VolumeMeshFlag, T_BOOL, offsetof(PyMeshingMeshSimOptionsClass, volume_mesh_flag), 0, "volume_mesh_flag"},
    {NULL}  
};

////////////////////////////////////////////////////////
//          C l a s s    G e t / S e t                //
////////////////////////////////////////////////////////
//
// Define setters/getters for certain options.

//--------------------------------------
// PyMeshSimOptions_get_global_edge_size 
//--------------------------------------
//
static PyObject*
PyMeshSimOptions_get_global_edge_size(PyMeshingMeshSimOptionsClass* self, void* closure)
{
  return self->global_edge_size;
}

static int
PyMeshSimOptions_set_global_edge_size(PyMeshingMeshSimOptionsClass* self, PyObject* value, void* closure)
{
  static std::string errorMsg = "The global_edge_size parameter must be a " + MeshSimOption::GlobalEdgeSize_Desc; 
  std::cout << "[PyMeshSimOptions_set_global_edge_size]  " << std::endl;
  if (!PyDict_Check(value)) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return -1;
  }

  int num = PyDict_Size(value);
  if (num != 2) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str()); 
      return -1;
  }

  // Check that the option is vaild. 
  //
  double absoluteEdgeSize;
  double realativeEdgeSize;
  if (!PyMeshSimOptionsGetGlobalEdgeSizeValues(value, absoluteEdgeSize, realativeEdgeSize)) {
      return -1;
  }

  self->global_edge_size = value;
  Py_INCREF(value);
  return 0;
}

//--------------------------------------
// PyMeshSimOptions_get_local_edge_size 
//--------------------------------------
// The 'local_edge_size' option is a list of dicts.
//
static PyObject*
PyMeshSimOptions_local_edge_size_getter(PyMeshingMeshSimOptionsClass* self, void* closure)
{
  return self->local_edge_size;
}

static int
PyMeshSimOptions_local_edge_size_setter(PyMeshingMeshSimOptionsClass* self, PyObject* listArg, void* closure)
{
  static std::string errorMsg = "The local_edge_size parameter must be a list of " + MeshSimOption::LocalEdgeSize_Desc; 
  std::cout << "[PyMeshSimOptions_set_local_edge_size]  ---------- local_edge_size_setter ---------- " << std::endl;
  if (!PyList_Check(listArg)) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return -1;
  }

  int listSize = PyList_Size(listArg);
  if (listSize == 0) {
      PyErr_SetString(PyExc_ValueError, "The local_edge_size parameter list is empty.");
      return 0;
  }

  auto local_edge_size = self->local_edge_size;
  int currentListSize = PyList_Size(local_edge_size);
  std::cout << "[PyMeshSimOptions_set_local_edge_size]  currentListSize: " << currentListSize << std::endl;

  // Check for a valid list of edge sizes.
  for (int i = 0; i < listSize; i++) {
      auto value = PyList_GetItem(listArg,i);
      int faceID;
      double edgeSize;
      double absolute;
      if (!PyMeshSimOptionsGetLocalEdgeSizeValues(value, faceID, edgeSize, absolute)) {
          return -1;
      }
      std::cout << "[PyMeshSimOptions_set_local_edge_size]  faceID: " << faceID << "  edgeSize: " << edgeSize << "  absolute: " << absolute << std::endl;
  }

  // Set the list of edge sizes.
  for (int i = 0; i < listSize; i++) {
      auto value = PyList_GetItem(listArg,i);
      PyList_Insert(local_edge_size, i, value);
      Py_INCREF(value);
  }

  return 0;
}

//-------------------------
// PyMeshSimOptionsGetSets
//-------------------------
//
PyGetSetDef PyMeshSimOptionsGetSets[] = {
    //{ MeshSimOption::LocalEdgeSize, (getter)PyMeshSimOptions_local_edge_size_getter, (setter)PyMeshSimOptions_local_edge_size_setter, NULL,  NULL },
    { MeshSimOption::GlobalEdgeSize, (getter)PyMeshSimOptions_get_global_edge_size, (setter)PyMeshSimOptions_set_global_edge_size, NULL,  NULL },
    {NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MESHING_MESHSIM_OPTIONS_CLASS = "MeshSimOptions";
static char* MESHING_MESHSIM_OPTIONS_MODULE_CLASS = "meshing.MeshSimOptions";

PyDoc_STRVAR(MeshSimOptionsClass_doc, "MeshSim meshing options class functions");

//----------------------
// PyMeshSimOptionsType 
//----------------------
// Define the Python type object that implements the meshing.MeshingOptions class. 
//
static PyTypeObject PyMeshSimOptionsType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = MESHING_MESHSIM_OPTIONS_MODULE_CLASS,
  .tp_basicsize = sizeof(PyMeshingMeshSimOptionsClass)
};

//-----------------------
// PyMeshSimOptions_init
//-----------------------
// This is the __init__() method for the meshing.MeshingOptions class. 
//
// This function is used to initialize an object after it is created.
//
static int 
PyMeshSimOptionsInit(PyMeshingMeshSimOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  static int numObjs = 1;
  std::cout << "[PyMeshSimOptionsInit] New MeshingOptions object: " << numObjs << std::endl;
  auto api = SvPyUtilApiFunction("O!|O!O!", PyRunTimeErr, __func__);
  static char *keywords[] = { MeshSimOption::GlobalEdgeSize, MeshSimOption::SurfaceMeshFlag, MeshSimOption::VolumeMeshFlag, NULL};
  PyObject* global_edge_size = nullptr;
  PyObject* surface_mesh_flag = nullptr;
  PyObject* volume_mesh_flag = nullptr;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &PyDict_Type, &global_edge_size, 
        &PyBool_Type, &surface_mesh_flag, &PyBool_Type, &volume_mesh_flag)) {
      api.argsError();
      return -1;
  }

  // Set the default option values.
  PyMeshSimOptions_set_defaults(self);

  // Check global edge size parameter.
  double edgeSize;
  double absolute;
  if (!PyMeshSimOptionsGetGlobalEdgeSizeValues(global_edge_size, edgeSize, absolute)) {
      return -1;
  }

  // Set the values that may have been passed in.
  //
  self->global_edge_size = global_edge_size;
  if (surface_mesh_flag) {
       self->surface_mesh_flag = PyObject_IsTrue(surface_mesh_flag);
  }
  if (volume_mesh_flag) {
      self->volume_mesh_flag = PyObject_IsTrue(volume_mesh_flag);
  }

  return 0;
}

//--------------------
// PyMeshSimOptionsNew 
//--------------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyMeshSimOptionsNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyMeshSimOptionsNew] PyMeshSimOptionsNew " << std::endl;
  auto self = (PyMeshingMeshSimOptionsClass*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyMeshSimOptionsNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }
  return (PyObject *) self;
}

//-------------------------
// PyMeshSimOptionsDealloc 
//-------------------------
//
static void
PyMeshSimOptionsDealloc(PyMeshingMeshSimOptionsClass* self)
{
  std::cout << "[PyMeshSimOptionsDealloc] Free PyMeshSimOptions" << std::endl;
  Py_TYPE(self)->tp_free(self);
}

//----------------------------
// SetMeshSimOptionsTypeFields 
//----------------------------
// Set the Python type object fields that stores loft option data. 
//
static void
SetMeshSimOptionsTypeFields(PyTypeObject& meshingOpts)
 {
  meshingOpts.tp_doc = MeshSimOptionsClass_doc; 
  meshingOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  meshingOpts.tp_dict = PyDict_New();
  meshingOpts.tp_new = PyMeshSimOptionsNew;
  meshingOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  meshingOpts.tp_init = (initproc)PyMeshSimOptionsInit;
  meshingOpts.tp_dealloc = (destructor)PyMeshSimOptionsDealloc;
  meshingOpts.tp_methods = PyMeshSimOptionsMethods;
  meshingOpts.tp_members = PyMeshSimOptionsMembers;
  meshingOpts.tp_getset = PyMeshSimOptionsGetSets;
};

//------------------------
// SetMeshingOptionsTypes
//------------------------
// Set the  loft optinnames in the MeshingOptionsType dictionary.
//
// These are for read only attibutes.
//
static void
SetMeshSimOptionsClassTypes(PyTypeObject& meshingOptsType)
{
/*
  std::cout << "=============== SetMeshingOptionsClassTypes ==========" << std::endl;

  //PyDict_SetItemString(meshingOptsType.tp_dict, "num_pts", PyLong_AsLong(10));

  PyObject *o = PyLong_FromLong(1);
  PyDict_SetItemString(meshingOptsType.tp_dict, "num_pts", o);

  //PyDict_SetItem(meshingOptsType.tp_dict, "num_pts", o);

  std::cout << "[SetMeshingOptionsClassTypes] Done! " << std::endl;
*/
 
};

//-------------------------
// CreateMeshSimOptionsType 
//-------------------------
//
static PyObject *
CreateMeshSimOptionsType(PyObject* args, PyObject* kwargs)
{
  return PyObject_Call((PyObject*)&PyMeshSimOptionsType, args, kwargs);
}


#endif

