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

// Define the Python 'meshing.TetGenOptions' class that encapsulates the parameters
// used for generating a mesh using TetGen. Options are stored as Python class attributes 
// and are set directly in the object created from that class.
//
//     options = sv.meshing.TetGenOptions(global_edge_size=0.1, surface_mesh_flag=True, volume_mesh_flag=True)
//     options.global_edge_size = 0.1
//
// Once options parameters have been set they are used to set the TetGen mesher options using
//
//    mesher.set_options(options)
//
// SV uses string literals to process options one at a time using 
//
//    int cvTetGenMeshObject::SetMeshOptions(char *flags, int numValues, double *values)
//
// The processing of Boolean options is not consistent, some options are set to true
// without using a value
//
//      else if(!strncmp(flags,"Verbose",7)) {
//          meshoptions_.verbose=1;
//
// To reproduce this behavior some options are defined as a PyObject* and initially set to 'None'.
// Options with a 'None' value are not sent to SetMeshOption().
//
#ifndef PYAPI_MESHING_TETGEN_OPTIONS_H
#define PYAPI_MESHING_TETGEN_OPTIONS_H 

#include <string>
#include <structmember.h>

//---------------------
// MeshingOptionsClass 
//---------------------
// Define the MeshingOptionsClass. 
//
// add_hole: [x,y,z]
// add_subdomain: { 'coordinate':[x,y,z], 'region_size':int } 
// allow_multiple_regions: bool
// boundary_layer_direction: int
// check: set option to true without value
// coarsen_percent: value / 100.0
// diagnose: set option to true without value 
// epsilon: not sure what range is valid 
// global_edge_size: 
// hausd: 
// local_edge_size: {'face_id':int, 'edge_size':double}
// mesh_wall_first: set option to true without value 
// new_region_boundary_layer: set option to true without value 
// no_bisect: set option to true without value 
// no_merge: set option to true without value 
// optimization: int, not sure what valid range is.
// quality_ratio:  
// quiet: set option to true without value 
// start_with_volume: set option to true without value 
// surface_mesh_flag: Boolean 
// use_mmg: Boolean 
// verbose: set option to true without value 
// volume_mesh_flag: Boolean 
//
typedef struct {
  PyObject_HEAD
  PyObject* add_hole;
  PyObject* add_subdomain;
  int allow_multiple_regions;
  int boundary_layer_direction;
  PyObject* check;
  double coarsen_percent;
  PyObject* diagnose;
  double epsilon;
  double global_edge_size;
  double hausd;
  PyObject* local_edge_size;
  PyObject* mesh_wall_first;
  PyObject* new_region_boundary_layer;
  PyObject* no_bisect;
  PyObject* no_merge;
  int optimization;
  double quality_ratio;
  PyObject* quiet;
  PyObject* start_with_volume;
  int surface_mesh_flag;
  int use_mmg;
  PyObject* verbose;
  int volume_mesh_flag;
} PyMeshingTetGenOptionsClass;

//--------------
// TetGenOption
//--------------
// PyMeshingTetGenOptionsClass attribute names.
//
namespace TetGenOption {
  char* AddHole = "add_hole";                      
  char* AddSubDomain = "add_subdomain";            
  char* AllowMultipleRegions = "allow_multiple_regions";           
  char* BoundaryLayerDirection = "boundary_layer_direction";     
  char* Check = "check";
  char* CoarsenPercent = "coarsen_percent";
  char* Diagnose = "diagnose";
  char* Epsilon = "epsilon";
  char* GlobalEdgeSize = "global_edge_size";
  char* Hausd = "hausd";
  char* LocalEdgeSize = "local_edge_size";
  char* MeshWallFirst = "mesh_wall_first";
  char* NewRegionBoundaryLayer = "new_region_boundary_layer";
  char* NoBisect = "no_bisect";
  char* NoMerge = "no_merge";
  char* Optimization = "optimization";
  char* QualityRatio = "quality_ratio";
  char* Quiet = "quiet";
  char* StartWithVolume = "start_with_volume";
  char* SurfaceMeshFlag = "surface_mesh_flag";
  char* UseMMG = "use_mmg";
  char* Verbose = "verbose";
  char* VolumeMeshFlag = "volume_mesh_flag";

  // Parameter names for the 'add_subdomain' option.
  //
  std::string AddSubDomain_Type = "dictionary "; 
  std::string AddSubDomain_Format = "{ 'coordinate':[x,y,z], 'region_size':int }";            
  std::string AddSubDomain_Desc = AddSubDomain_Type + AddSubDomain_Format; 
  // Use char* for these because they are used in the Python C API functions.
  char* AddSubDomain_CoordinateParam = "coordinate";            
  char* AddSubDomain_RegionSizeParam = "region_size";            

  // Parameter names for the 'local_edge_size' option.
  //
  std::string LocalEdgeSize_Type = "dictionary "; 
  std::string LocalEdgeSize_Format = "{ 'face_id':int, 'edge_size':double }";            
  std::string LocalEdgeSize_Desc = LocalEdgeSize_Type + LocalEdgeSize_Format; 
  // Use char* for these because they are used in the Python C API functions.
  char* LocalEdgeSize_FaceIDParam = "face_id";            
  char* LocalEdgeSize_EdgeSizeParam = "edge_size";            

  // Create a map beteen Python and SV names. The SV names are needed when
  // setting mesh options.
  //
  std::map<std::string,char*> pyToSvNameMap = {
      {std::string(AddHole), "AddHole"}, 
      {std::string(AddSubDomain), "AddSubDomain"},
      {std::string(AllowMultipleRegions), "AllowMultipleRegions"},
      {std::string(BoundaryLayerDirection), "BoundaryLayerDirection"},
      {std::string(Check), "Check"},
      {std::string(CoarsenPercent), "CoarsenPercent"},
      {std::string(Diagnose), "Diagnose"},
      {std::string(Epsilon), "Epsilon"},
      {std::string(GlobalEdgeSize), "GlobalEdgeSize"},
      {std::string(Hausd), "Hausd"},
      {std::string(LocalEdgeSize), "LocalEdgeSize"},
      {std::string(MeshWallFirst), "MeshWallFirst"},
      {std::string(NewRegionBoundaryLayer), "NewRegionBoundaryLayer"},
      {std::string(NoBisect), "NoBisect"},
      {std::string(NoMerge), "NoMerge"},
      {std::string(Optimization), "Optimization"},
      {std::string(QualityRatio), "QualityRatio"},
      {std::string(Quiet), "Quiet"},
      {std::string(StartWithVolume), "StartWithVolume"},
      {std::string(SurfaceMeshFlag), "SurfaceMeshFlag"},
      {std::string(UseMMG), "UseMMG"},
      {std::string(Verbose), "Verbose"},
      {std::string(VolumeMeshFlag), "VolumeMeshFlag"}
   };

};

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//---------------------------------------
// PyTetGenOptionsGetLocalEdgeSizeValues
//---------------------------------------
// Get the parameter values for the LocalEdgeSize option. 
//
bool
PyTetGenOptionsGetLocalEdgeSizeValues(PyObject* obj, int& regionID, double& edgeSize) 
{
  static std::string errorMsg = "The local_edge_size parameter must be a " + TetGenOption::LocalEdgeSize_Desc; 

  // Check the LocalEdgeSize_RegionIDParam key.
  //
  PyObject* regionIDItem = PyDict_GetItemString(obj, TetGenOption::LocalEdgeSize_FaceIDParam);
  if (regionIDItem == nullptr) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return false;
  }
  regionID = PyLong_AsLong(regionIDItem);
  if (PyErr_Occurred()) {
      return false;
  }
  if (regionID <= 0) {
      PyErr_SetString(PyExc_ValueError, "The region ID paramter must be > 0.");
      return false;
  }

  // Check the LocalEdgeSize_SizeParam key.
  //
  PyObject* sizeItem = PyDict_GetItemString(obj, TetGenOption::LocalEdgeSize_EdgeSizeParam);
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

  return true;
}

//--------------------------
// PyTetGenOptionsGetValues
//--------------------------
// Get attribute values from the MeshingOptions object.
//
// Return a vector of doubles to mimic how SV processes options.
//
static std::vector<double> 
PyTetGenOptionsGetValues(PyObject* meshingOptions, std::string name)
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
  } else if (name == TetGenOption::LocalEdgeSize) {
      int regionID;
      double edgeSize;
      PyTetGenOptionsGetLocalEdgeSizeValues(obj, regionID, edgeSize);
      values.push_back((double)regionID);
      values.push_back(edgeSize);
  }

  Py_DECREF(obj);
  return values;
}

////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//
// Methods for the TetGenOptions class.

//------------------------------------------------
// PyTetGenOptions_create_add_subdomain_parameter
//------------------------------------------------
// [TODO:DaveP] figure out what the parameters are.
//
PyDoc_STRVAR(PyTetGenOptions_create_add_subdomain_parameter_doc,
  " create_add_subdomain_parameter(coordinate, region_size)  \n\ 
  \n\
  Create a parameter for the add_subdomain option. \n\
  \n\
  Args:  \n\
    coordinate ([float,float,float]): The 3D coordinate for the subdomain. \n\
    region_size(int): The size of the region.  \n\
");

static PyObject *
PyTetGenOptions_create_add_subdomain_parameter(PyMeshingTetGenOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("O!i", PyRunTimeErr, __func__);
  static char *keywords[] = { TetGenOption::AddSubDomain_CoordinateParam, TetGenOption::AddSubDomain_RegionSizeParam, NULL }; 
  PyObject* coordArg = nullptr;
  int regionSize = 0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &PyList_Type, &coordArg, &regionSize)) {
      return api.argsError();
  }

  if (regionSize <= 0) {
      api.error("The '" + std::string(TetGenOption::AddSubDomain_RegionSizeParam) + "' must be > 0.");
      return nullptr;
  }

  // Check AddSubDomain_CoordinateParam argument.
  //
  std::string errorMsg = "The '" + std::string(TetGenOption::AddSubDomain_CoordinateParam) + "' parameter must be a list of three floats.";
  int num = PyList_Size(coordArg);
  if (num != 3) {
      api.error(errorMsg);
      return nullptr;
  }

  std::vector<double> coord;
  for (int i = 0; i < num; i++) {
      auto item = PyList_GetItem(coordArg, i);
      auto value = PyFloat_AsDouble(item);
      coord.push_back(value);
  }

  if (PyErr_Occurred()) {
      api.error(errorMsg);
      return nullptr;
  }

  // Create and return parameter.
  auto coordList = Py_BuildValue("[d,d,d]",  coord[0], coord[1], coord[2]);
  return Py_BuildValue("{s:O,s:i}", TetGenOption::AddSubDomain_CoordinateParam, coordList, 
                                    TetGenOption::AddSubDomain_RegionSizeParam, regionSize);
}

//--------------------------------------------------
// PyTetGenOptions_create_local_edge_size_parameter
//--------------------------------------------------
// [TODO:DaveP] figure out what the parameters are.
//
PyDoc_STRVAR(PyTetGenOptions_create_local_edge_size_parameter_doc,
  " create_local_edge_size(region_id, size)  \n\ 
  \n\
  Create a parameter for the local_edge_size option. \n\
  \n\
  Args:  \n\
    region_id (int): The ID of the region.  \n\
    size (double): The edge size for the face.  \n\
");

static PyObject *
PyTetGenOptions_create_local_edge_size_parameter(PyMeshingTetGenOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("id", PyRunTimeErr, __func__);
  static char *keywords[] = { TetGenOption::LocalEdgeSize_FaceIDParam, TetGenOption::LocalEdgeSize_EdgeSizeParam, NULL }; 
  int faceID = 0;
  double edgeSize = 0.0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &faceID, &edgeSize)) {
      return api.argsError();
  }

  if (edgeSize <= 0) {
      api.error("The '" + std::string(TetGenOption::LocalEdgeSize_EdgeSizeParam) + "' must be > 0.");
      return nullptr;
  }

  if (faceID <= 0) {
      api.error("The '" + std::string(TetGenOption::LocalEdgeSize_FaceIDParam) + "' must be > 0.");
      return nullptr;
  }

  // Create and return parameter.
  return Py_BuildValue("{s:i, s:d}", TetGenOption::LocalEdgeSize_FaceIDParam, faceID, 
                                     TetGenOption::LocalEdgeSize_EdgeSizeParam, edgeSize);
}

//----------------------------
// PyTetGenOptions_get_values
//----------------------------
//
PyDoc_STRVAR(PyTetGenOptions_get_values_doc,
" get_values()  \n\ 
  \n\
  Get the names and values of TetGen mesh generation options. \n\
  \n\
  Args:  \n\
");

static PyObject *
PyTetGenOptions_get_values(PyMeshingTetGenOptionsClass* self, PyObject* args)
{
  PyObject* values = PyDict_New();

  PyDict_SetItemString(values, TetGenOption::AddHole, self->add_hole);
  PyDict_SetItemString(values, TetGenOption::AddSubDomain, self->add_subdomain);
  PyDict_SetItemString(values, TetGenOption::AllowMultipleRegions, PyBool_FromLong(self->allow_multiple_regions));
  PyDict_SetItemString(values, TetGenOption::BoundaryLayerDirection, Py_BuildValue("i", self->boundary_layer_direction));
  PyDict_SetItemString(values, TetGenOption::Check, self->check);
  PyDict_SetItemString(values, TetGenOption::CoarsenPercent, Py_BuildValue("d", self->coarsen_percent));
  PyDict_SetItemString(values, TetGenOption::Diagnose, self->diagnose);
  PyDict_SetItemString(values, TetGenOption::Epsilon, Py_BuildValue("d", self->epsilon));
  PyDict_SetItemString(values, TetGenOption::GlobalEdgeSize, Py_BuildValue("d", self->global_edge_size));
  PyDict_SetItemString(values, TetGenOption::Hausd, Py_BuildValue("d", self->hausd));
  PyDict_SetItemString(values, TetGenOption::LocalEdgeSize, self->local_edge_size);
  PyDict_SetItemString(values, TetGenOption::MeshWallFirst, self->mesh_wall_first);
  PyDict_SetItemString(values, TetGenOption::NewRegionBoundaryLayer, self->new_region_boundary_layer);
  PyDict_SetItemString(values, TetGenOption::NoBisect, self->no_bisect);
  PyDict_SetItemString(values, TetGenOption::NoMerge, self->no_merge);
  PyDict_SetItemString(values, TetGenOption::Optimization, Py_BuildValue("i", self->optimization));
  PyDict_SetItemString(values, TetGenOption::QualityRatio, Py_BuildValue("d", self->quality_ratio));
  PyDict_SetItemString(values, TetGenOption::Quiet, self->quiet);
  PyDict_SetItemString(values, TetGenOption::StartWithVolume, self->start_with_volume);

  PyDict_SetItemString(values, TetGenOption::SurfaceMeshFlag, PyBool_FromLong(self->surface_mesh_flag));

  PyDict_SetItemString(values, TetGenOption::UseMMG, Py_BuildValue("i", self->use_mmg));
  PyDict_SetItemString(values, TetGenOption::Verbose, self->verbose);
  PyDict_SetItemString(values, TetGenOption::VolumeMeshFlag, PyBool_FromLong(self->volume_mesh_flag));

  return values;
}

//------------------------------
// PyTetGenOptions_set_defaults
//------------------------------
// Set the default options parameter values.
//
static PyObject *
PyTetGenOptions_set_defaults(PyMeshingTetGenOptionsClass* self)
{
  self->add_hole = Py_BuildValue("");
  self->add_subdomain = Py_BuildValue("");
  self->allow_multiple_regions = 0;
  self->boundary_layer_direction = 0;
  self->check = Py_BuildValue("");
  self->coarsen_percent = 0;
  self->diagnose = Py_BuildValue("");
  self->epsilon = 0;
  self->global_edge_size = 0;
  self->hausd = 0;
  self->local_edge_size = Py_BuildValue(""); 
  self->mesh_wall_first = Py_BuildValue("");
  self->new_region_boundary_layer = Py_BuildValue("");
  self->no_bisect = Py_BuildValue("");
  self->no_merge = Py_BuildValue("");
  self->optimization = 0;
  self->quality_ratio = 0;
  self->quiet = Py_BuildValue("");
  self->start_with_volume = Py_BuildValue("");
  self->surface_mesh_flag = 0;
  self->use_mmg = 0;
  self->verbose = Py_BuildValue("");
  self->volume_mesh_flag = 0;

  Py_RETURN_NONE;
}

//------------------------
// PyTetGenOptionsMethods 
//------------------------
//
static PyMethodDef PyTetGenOptionsMethods[] = {
  {"create_add_subdomain_parameter", (PyCFunction)PyTetGenOptions_create_add_subdomain_parameter, METH_VARARGS|METH_KEYWORDS, PyTetGenOptions_create_add_subdomain_parameter_doc},
  {"create_local_edge_size_parameter", (PyCFunction)PyTetGenOptions_create_local_edge_size_parameter, METH_VARARGS|METH_KEYWORDS, PyTetGenOptions_create_local_edge_size_parameter_doc},
  {"get_values", (PyCFunction)PyTetGenOptions_get_values, METH_NOARGS, PyTetGenOptions_get_values_doc},
  {NULL, NULL}
};

////////////////////////////////////////////////////////
//          C l a s s    M e m b e r s                //
////////////////////////////////////////////////////////
//
// Define the PyMeshingTetGenOptionsClass attribute names.
//
// The attributes can be set/get directly in from the MeshingOptions object.
//
static PyMemberDef PyTetGenOptionsMembers[] = {
    {TetGenOption::AllowMultipleRegions, T_BOOL, offsetof(PyMeshingTetGenOptionsClass, allow_multiple_regions), 0, "allow_multiple_regions"},
    {TetGenOption::BoundaryLayerDirection, T_INT, offsetof(PyMeshingTetGenOptionsClass, boundary_layer_direction), 0, "boundary_layer_direction"},
    {TetGenOption::Check, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, check), 0, "check"},
    {TetGenOption::CoarsenPercent, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, coarsen_percent), 0, "coarsen_percent"},
    {TetGenOption::Diagnose, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, diagnose), 0, "Diagnose"},
    {TetGenOption::Epsilon, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, epsilon), 0, "Epsilon"},
    {TetGenOption::GlobalEdgeSize, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, global_edge_size), 0, "global_edge_size"},
    {TetGenOption::Hausd, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, hausd), 0, "Hausd"},
    //{TetGenOption::LocalEdgeSize, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, local_edge_size), 0, "local_edge_size"},
    {TetGenOption::MeshWallFirst, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, mesh_wall_first), 0, "mesh_wall_first"},
    {TetGenOption::NewRegionBoundaryLayer, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, new_region_boundary_layer), 0, "new_region_boundary_layer"},
    {TetGenOption::NoBisect, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, no_bisect), 0, "no_bisect"},
    {TetGenOption::NoMerge, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, no_merge), 0, "no_merge"},
    {TetGenOption::Optimization, T_INT, offsetof(PyMeshingTetGenOptionsClass, optimization), 0, "Optimization"},
    {TetGenOption::QualityRatio, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, quality_ratio), 0, "quality_ratio"},
    {TetGenOption::Quiet, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, quiet), 0, "Quiet"},
    {TetGenOption::StartWithVolume, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, start_with_volume), 0, "start_with_volume"},

    {TetGenOption::SurfaceMeshFlag, T_BOOL, offsetof(PyMeshingTetGenOptionsClass, surface_mesh_flag), 0, "surface_mesh_flag"},

    {TetGenOption::UseMMG, T_INT, offsetof(PyMeshingTetGenOptionsClass, use_mmg), 0, "use_mmg"},
    {TetGenOption::Verbose, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, verbose), 0, "Verbose"},
    {TetGenOption::VolumeMeshFlag, T_BOOL, offsetof(PyMeshingTetGenOptionsClass, volume_mesh_flag), 0, "volume_mesh_flag"},
    {NULL}  
};

////////////////////////////////////////////////////////
//          C l a s s    G e t / S e t                //
////////////////////////////////////////////////////////
//
// Define setters/getters for certain options.

//------------------------------
// PyTetGenOptions_get_add_hole 
//------------------------------
//
static PyObject*
PyTetGenOptions_get_add_hole(PyMeshingTetGenOptionsClass* self, void* closure)
{
  return self->add_hole;
}

static int 
PyTetGenOptions_set_add_hole(PyMeshingTetGenOptionsClass* self, PyObject* value, void* closure)
{
  if (!PyList_Check(value)) {
      PyErr_SetString(PyExc_ValueError, "The add_hole parameter must be a list of three floats.");
      return -1;
  }

  int num = PyList_Size(value);
  if (num != 3) {
      PyErr_SetString(PyExc_ValueError, "The add_hole parameter must be a list of three floats.");
      return -1;
  }

  std::vector<double> values;
  for (int i = 0; i < num; i++) {
      auto item = PyList_GetItem(value, i);
      auto value = PyFloat_AsDouble(item);
      values.push_back(value);
  }

  if (PyErr_Occurred()) {
      return -1;
  }

  self->add_hole = Py_BuildValue("[d,d,d]", values[0], values[1], values[2]);
  return 0;
}

//-----------------------------------
// PyTetGenOptions_get_add_subdomain 
//-----------------------------------
//
static PyObject*
PyTetGenOptions_get_add_subdomain(PyMeshingTetGenOptionsClass* self, void* closure)
{
  return self->add_subdomain;
}

static int
PyTetGenOptions_set_add_subdomain(PyMeshingTetGenOptionsClass* self, PyObject* value, void* closure)
{
  static std::string errorMsg = "The add_subdomain parameter must be a " + TetGenOption::AddSubDomain_Desc; 
  std::cout << "[PyTetGenOptions_set_add_subdomain]  " << std::endl;
  if (!PyDict_Check(value)) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return -1;
  }

  int num = PyDict_Size(value);
  std::cout << "[PyTetGenOptions_set_add_subdomain]  num: " << num << std::endl;
  if (num != 2) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str()); 
      return -1;
  }

  // Check the AddSubDomain_CoordinateParam key.
  //
  PyObject* coord = PyDict_GetItemString(value, TetGenOption::AddSubDomain_CoordinateParam);
  if (coord == nullptr) { 
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str()); 
      return -1;
  }

  int numCoord = PyList_Size(coord);
  if (numCoord != 3) {
      PyErr_SetString(PyExc_ValueError, "The add_subdomain 'coordinate' parameter must be a list of three floats.");
      return -1;
  }
  
  for (int i = 0; i < numCoord; i++) {
      auto item = PyList_GetItem(coord, i);
      auto value = PyFloat_AsDouble(item);
  }
  
  if (PyErr_Occurred()) {
      return -1;
  }

  // Check the AddSubDomain_RegionSizeParam key.
  //
  PyObject* regionSize = PyDict_GetItemString(value, TetGenOption::AddSubDomain_RegionSizeParam);
  if (regionSize == nullptr) { 
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str()); 
      return -1;
  }

  auto regionValue = PyLong_AsLong(regionSize);
  if (PyErr_Occurred()) {
      return -1;
  }

  self->add_subdomain = value;
  Py_INCREF(value);

  return 0;
}

//-------------------------------------
// PyTetGenOptions_get_local_edge_size 
//-------------------------------------
//
static PyObject*
PyTetGenOptions_get_local_edge_size(PyMeshingTetGenOptionsClass* self, void* closure)
{
  return self->local_edge_size;
}

static int
PyTetGenOptions_set_local_edge_size(PyMeshingTetGenOptionsClass* self, PyObject* value, void* closure)
{
  static std::string errorMsg = "The local_edge_size parameter must be a " + TetGenOption::LocalEdgeSize_Desc; 
  std::cout << "[PyTetGenOptions_set_local_edge_size]  " << std::endl;
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
  int regionID;
  double edgeSize;
  if (!PyTetGenOptionsGetLocalEdgeSizeValues(value, regionID, edgeSize)) {
      return -1;
  }

  self->local_edge_size = value;
  Py_INCREF(value);

  return 0;
}

//------------------------
// PyTetGenOptionsGetSets
//------------------------
//
PyGetSetDef PyTetGenOptionsGetSets[] = {
    { TetGenOption::AddHole, (getter)PyTetGenOptions_get_add_hole, (setter)PyTetGenOptions_set_add_hole, NULL,  NULL },
    { TetGenOption::AddSubDomain, (getter)PyTetGenOptions_get_add_subdomain, (setter)PyTetGenOptions_set_add_subdomain, NULL,  NULL },
    { TetGenOption::LocalEdgeSize, (getter)PyTetGenOptions_get_local_edge_size, (setter)PyTetGenOptions_set_local_edge_size, NULL,  NULL },
    {NULL}
};


////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MESHING_TETGEN_OPTIONS_CLASS = "TetGenOptions";
static char* MESHING_TETGEN_OPTIONS_MODULE_CLASS = "meshing.TetGenOptions";

PyDoc_STRVAR(TetGenOptionsClass_doc, "TetGen meshing options class functions");

//---------------------
// PyTetGenOptionsType 
//---------------------
// Define the Python type object that implements the meshing.MeshingOptions class. 
//
static PyTypeObject PyTetGenOptionsType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = MESHING_TETGEN_OPTIONS_MODULE_CLASS,
  .tp_basicsize = sizeof(PyMeshingTetGenOptionsClass)
};

//----------------------
// PyTetGenOptions_init
//----------------------
// This is the __init__() method for the meshing.MeshingOptions class. 
//
// This function is used to initialize an object after it is created.
//
// Arguments:
//
static int 
PyTetGenOptionsInit(PyMeshingTetGenOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  static int numObjs = 1;
  std::cout << "[PyTetGenOptionsInit] New MeshingOptions object: " << numObjs << std::endl;
  auto api = SvPyUtilApiFunction("d|O!O!O!", PyRunTimeErr, __func__);
  static char *keywords[] = { TetGenOption::GlobalEdgeSize, TetGenOption::SurfaceMeshFlag, TetGenOption::VolumeMeshFlag, 
                              TetGenOption::MeshWallFirst, NULL};
  double global_edge_size = 0.0;
  PyObject* surface_mesh_flag = NULL;
  PyObject* volume_mesh_flag = NULL;
  PyObject* mesh_wall_first = NULL;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &global_edge_size, &PyBool_Type, &surface_mesh_flag, 
        &PyBool_Type, &volume_mesh_flag, &PyBool_Type, &mesh_wall_first)) {
      api.argsError();
      return -1;
  }

  // Set the default option values.
  PyTetGenOptions_set_defaults(self);

  // Set the values that may have been passed in.
  //
  self->global_edge_size = global_edge_size;
  if (surface_mesh_flag) {
       self->surface_mesh_flag = PyObject_IsTrue(surface_mesh_flag);
  }
  if (volume_mesh_flag) {
      self->volume_mesh_flag = PyObject_IsTrue(volume_mesh_flag);
  }

  // If mesh_wall_first is defined and True then set self->mesh_wall_first 
  // to be a Py_True PyObject. Need to use a PyObject for it because SV sets 
  // this option to true if it is defined.
  //
  if (mesh_wall_first && PyObject_IsTrue(mesh_wall_first)) { 
      Py_INCREF(Py_True);
      self->mesh_wall_first = Py_True;
      std::cout << "[PyTetGenOptionsInit] mesh_wall_first is True: " << std::endl;
  }

  return 0;
}

//--------------------
// PyTetGenOptionsNew 
//--------------------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyTetGenOptionsNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyTetGenOptionsNew] PyTetGenOptionsNew " << std::endl;
  auto self = (PyMeshingTetGenOptionsClass*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyTetGenOptionsNew] ERROR: Can't allocate type." << std::endl;
      return nullptr; 
  }
  return (PyObject *) self;
}

//-------------------------
// PyTetGenOptionsDealloc 
//-------------------------
//
static void
PyTetGenOptionsDealloc(PyMeshingTetGenOptionsClass* self)
{
  std::cout << "[PyTetGenOptionsDealloc] Free PyTetGenOptions" << std::endl;
  Py_TYPE(self)->tp_free(self);
}

//----------------------------
// SetTetGenOptionsTypeFields 
//----------------------------
// Set the Python type object fields that stores loft option data. 
//
static void
SetTetGenOptionsTypeFields(PyTypeObject& meshingOpts)
 {
  meshingOpts.tp_doc = TetGenOptionsClass_doc; 
  meshingOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  meshingOpts.tp_dict = PyDict_New();
  meshingOpts.tp_new = PyTetGenOptionsNew;
  meshingOpts.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  meshingOpts.tp_init = (initproc)PyTetGenOptionsInit;
  meshingOpts.tp_dealloc = (destructor)PyTetGenOptionsDealloc;
  meshingOpts.tp_methods = PyTetGenOptionsMethods;
  meshingOpts.tp_members = PyTetGenOptionsMembers;
  meshingOpts.tp_getset = PyTetGenOptionsGetSets;
};

//------------------------
// SetMeshingOptionsTypes
//------------------------
// Set the  loft optinnames in the MeshingOptionsType dictionary.
//
// These are for read only attibutes.
//
static void
SetTetGenOptionsClassTypes(PyTypeObject& meshingOptsType)
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
// CreateTetGenOptionsType 
//-------------------------
//
/*
static PyMeshingTetGenOptionsClass *
CreateTetGenOptionsType()
{
  return PyObject_New(PyMeshingTetGenOptionsClass, &PyTetGenOptionsType);
}
*/

static PyObject *
CreateTetGenOptionsType(PyObject* args, PyObject* kwargs)
{
  return PyObject_Call((PyObject*)&PyTetGenOptionsType, args, kwargs);
}


#endif

