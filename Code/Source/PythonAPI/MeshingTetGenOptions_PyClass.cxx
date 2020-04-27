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
// Some options store a list of values. Lists are implemented using the Python C API PyMemberDef struct.
// List options are then able to be set using Python list operations (e.g. append). The disadvantage 
// of this is that values can't be checked when they are appended.
//
// To help prevent errors I've created methods to create values for list options. For example, the 
// SphereRefinement() method creates a dict that can be appended to the options.sphere_refinement list
//
//     value = options.SphereRefinement(edge_size=0.3, radius=3.74711, center=[3.496379, 3.752667, 3.794])
//     options.sphere_refinement.append( value )
//
// [TODO:DaveP] Implement TetGenOptions in Python.
//
#ifndef PYAPI_MESHING_TETGEN_OPTIONS_H
#define PYAPI_MESHING_TETGEN_OPTIONS_H 

#include <regex>
#include <string>
#include <structmember.h>

PyObject * CreateTetGenOptionsType(PyObject* args, PyObject* kwargs);

//---------------------
// MeshingOptionsClass 
//---------------------
// Define the MeshingOptionsClass. 
//
// The follows options are set in cvTetGenMeshObject::SetMeshOptions():
//
//   add_hole: [x,y,z]
//   add_subdomain: { 'coordinate':[x,y,z], 'region_size':int } 
//   allow_multiple_regions: Boolean
//   boundary_layer_direction: int
//   check: set option to true without value
//   coarsen_percent: value / 100.0
//   diagnose: set option to true without value 
//   epsilon: not sure what range is valid 
//   global_edge_size: 
//   hausd: 
//   mesh_wall_first: set option to true without value 
//   new_region_boundary_layer: set option to true without value 
//   no_bisect: set option to true without value 
//   no_merge: set option to true without value 
//   optimization: int, not sure what valid range is.
//   quality_ratio:  
//   quiet: set option to true without value 
//   start_with_volume: set option to true without value 
//   surface_mesh_flag: Boolean 
//   use_mmg: Boolean 
//   verbose: set option to true without value 
//   volume_mesh_flag: Boolean 
//
// These options are used to generate meshing size function data arrays
// and are not seen by TetGen.
//
//   local_edge_size: list({'face_id':int, 'edge_size':double})
//   local_edge_size_on: Boolean. 
//
//   radius_meshing_centerlines: vtkPolyData object 
//   radius_meshing_on:  Boolean
//   radius_meshing_scale: float
//
//   sphere_refinement: list({ 'edge_size':float, 'radius':float,  'center':[float, float, float] }
//   sphere_refinement_on: Boolean 
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

  // Generate meshing size function data. 
  //
  PyObject* local_edge_size;
  int local_edge_size_on;
  PyObject* radius_meshing_centerlines;
  int radius_meshing_on; 
  double radius_meshing_scale;
  PyObject* sphere_refinement;
  int sphere_refinement_on;

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
  char* LocalEdgeSizeOn = "local_edge_size_on";
  char* MeshWallFirst = "mesh_wall_first";
  char* NewRegionBoundaryLayer = "new_region_boundary_layer";
  char* NoBisect = "no_bisect";
  char* NoMerge = "no_merge";
  char* Optimization = "optimization";
  char* QualityRatio = "quality_ratio";
  char* Quiet = "quiet";
  char* RadiusMeshingScale = "radius_meshing_scale";
  char* RadiusMeshingCenterlines = "radius_meshing_centerlines";
  char* RadiusMeshingOn = "radius_meshing_on";
  char* SphereRefinement = "sphere_refinement";
  char* SphereRefinementOn = "sphere_refinement_on";
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

  // SphereRefinement parameter names. 
  //
  std::string SphereRefinement_Type = "dictionary ";
  std::string SphereRefinement_Format = "{ 'edge_size':float, 'radius':float,  'center':[float, float, float] }";
  std::string SphereRefinement_Desc = SphereRefinement_Type + SphereRefinement_Format;
  // Use char* for these because they are used in the Python C API functions.
  char* SphereRefinement_EdgeSizeParam = "edge_size";
  char* SphereRefinement_RadiusParam = "radius";
  char* SphereRefinement_CenterParam = "center";

  // Create a map beteen Python and SV names. 
  //
  // The SV names are used to set mesh options by calling 
  // cvTetGenMeshObject::SetMeshOptions().
  //
  // Don't include LocalEdgeSize here because we want to control 
  // generate meshing size function data arrays, which would be done 
  // in cvTetGenMeshObject::SetMeshOptions(), in the API.
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
      //{std::string(LocalEdgeSize), "LocalEdgeSize"},
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

  // Create a set of options that can be a list.
  //
  // This is used when setting options.
  std::set<std::string> ListOptions { LocalEdgeSize };

  // Create a map between .msh file option names.
  //
  // Some of the options in the .msh file don't have an 'option' before it.
  // Look in sv4guiMeshTetGen::ParseCommand() to see which parameters are 
  // progrmatically set to be options.
  //
  std::map<std::string,std::string> MshFileOptionNamesMap = {
    { "surface", pyToSvNameMap[SurfaceMeshFlag] },
    { "volume", pyToSvNameMap[VolumeMeshFlag] }
  };

  // Command names used for mesh size data arrays found in a .msh file.
  std::string CommandFunctionBasedMeshing("functionBasedMeshing");
  std::string CommandLocalSize("localSize");
  std::string CommandUseCenterlineRadius("useCenterlineRadius");
  std::string CommandSphereRefinement("sphereRefinement");


//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

bool
LocalEdgeSizeIsOn(PyObject* optionsObj)
{
  auto options = (PyMeshingTetGenOptionsClass*)optionsObj;
  return (options->local_edge_size_on == 1);
}

bool
RadiusMeshingIsOn(PyObject* optionsObj)
{
  auto options = (PyMeshingTetGenOptionsClass*)optionsObj;
  return (options->radius_meshing_on == 1);
}

bool
SphereRefinementIsOn(PyObject* optionsObj)
{
  auto options = (PyMeshingTetGenOptionsClass*)optionsObj;
  return (options->sphere_refinement_on == 1);
}

}; // namespace TetGenOption

//--------------------------
// CreateLocalEdgeSizeValue
//--------------------------
// Create a PyObject dict for LocalEdgeSize option.
//
PyObject * 
CreateLocalEdgeSizeValue(SvPyUtilApiFunction& api, int faceID, double edgeSize) 
{
  if (edgeSize <= 0) {
      api.error("The '" + std::string(TetGenOption::LocalEdgeSize_EdgeSizeParam) + "' must be > 0.");
      return nullptr;
  }
  
  if (faceID <= 0) {
      api.error("The '" + std::string(TetGenOption::LocalEdgeSize_FaceIDParam) + "' must be > 0.");
      return nullptr;
  }
  
  // Create a local edge size dict. 
  auto value = Py_BuildValue("{s:i, s:d}", TetGenOption::LocalEdgeSize_FaceIDParam, faceID, TetGenOption::LocalEdgeSize_EdgeSizeParam, edgeSize);
  Py_INCREF(value);

  return value;
}

//------------------------
// GetLocalEdgeSizeValues
//------------------------
// Get the parameter values for the LocalEdgeSize option. 
//
bool
GetLocalEdgeSizeValues(PyObject* obj, int& faceID, double& edgeSize) 
{
  std::cout << "[GetLocalEdgeSizeValues] ========== GetLocalEdgeSizeValues ==========" << std::endl;
  static std::string errorMsg = "The local_edge_size parameter must be a " + TetGenOption::LocalEdgeSize_Desc; 

  // Check the LocalEdgeSize_FaceIDParam key.
  //
  PyObject* faceIDItem = PyDict_GetItemString(obj, TetGenOption::LocalEdgeSize_FaceIDParam);
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

  // Check the LocalEdgeSize_EdgeSizeParam key.
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
      PyErr_SetString(PyExc_ValueError, "The edge size parameter must be > 0.");
      return false;
  }

  return true;
}

//------------------------
// GetRadiusMeshingValues
//------------------------
//
void
GetRadiusMeshingValues(PyObject* optionsObj, double* scale, vtkPolyData** centerlines) 
{
  auto options = (PyMeshingTetGenOptionsClass*)optionsObj;
  *scale = options->radius_meshing_scale;

  if (options->radius_meshing_centerlines == Py_None) {
      *centerlines = nullptr;
  } else {
      *centerlines = (vtkPolyData*)vtkPythonUtil::GetPointerFromObject(options->radius_meshing_centerlines, "vtkPolyData");
  }
}

//-----------------------------
// CreateSphereRefinementValue
//-----------------------------
// Create a value for the SphereRefinement option. 
//
PyObject *
CreateSphereRefinementValue(SvPyUtilApiFunction& api, double edgeSize, double radius, double center[3])
{
  static std::string errorMsg = "The sphere_refinement parameter must be a " + TetGenOption::SphereRefinement_Desc; 
  if (edgeSize <= 0) {
      api.error("The '" + std::string(TetGenOption::SphereRefinement_EdgeSizeParam) + "' must be > 0.");
      return nullptr;
  }

  if (radius <= 0) {
      api.error("The '" + std::string(TetGenOption::SphereRefinement_RadiusParam) + "' must be > 0.");
      return nullptr;
  }

  // Create a local edge size dict. 
  //
  // [TODO:DaveP] Should I explicitly build the center list? Memory leak here?
  //
  //auto value = Py_BuildValue("{s:d, s:d, s:[d,d,d]}", TetGenOption::SphereRefinement_EdgeSizeParam, edgeSize, 
  auto value = Py_BuildValue("{s:d, s:d, s:[ddd]}", TetGenOption::SphereRefinement_EdgeSizeParam, edgeSize, 
      TetGenOption::SphereRefinement_RadiusParam, radius, TetGenOption::SphereRefinement_CenterParam, center[0], center[1], center[2]);
  Py_INCREF(value);
  return value;
}

//---------------------------
// GetSphereRefinementValues
//---------------------------
// Get the values for the SphereRefinement parameter. 
//
// The Python object should be a dict with the format
//
//    { 'edge_size':double, 'radius':double,  'center':[double, double, double] } 
//
bool
GetSphereRefinementValues(PyObject* obj, double& edgeSize, double& radius, std::vector<double>& center) 
{
  //std::cout << "[GetSphereRefinementValues] ========== GetSphereRefinementValues ==========" << std::endl;
  // Get the edge size parameter value.
  //
  // Check the SphereRefinement_SizeParam key.
  PyObject* sizeItem = PyDict_GetItemString(obj, TetGenOption::SphereRefinement_EdgeSizeParam);
  if (sizeItem == nullptr) {
      PyErr_SetString(PyExc_ValueError, "No edge size parameter.");
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
  //std::cout << "[GetSphereRefinementValues] edgeSize: " << edgeSize << std::endl;

  // Get the radius parameter value.
  //
  // Check the SphereRefinement_RadiusParam key.
  PyObject* radiusItem = PyDict_GetItemString(obj, TetGenOption::SphereRefinement_RadiusParam);
  if (radiusItem == nullptr) {
      PyErr_SetString(PyExc_ValueError, "No radius parameter.");
      return false;
  }
  
  radius = PyFloat_AsDouble(radiusItem);
  if (PyErr_Occurred()) {
      return false;
  }
  if (radius <= 0) {
      PyErr_SetString(PyExc_ValueError, "The radius parameter must be > 0.");
      return false;
  }   
  //std::cout << "[GetSphereRefinementValues] radius: " << radius << std::endl;

  // Get the center parameter value.
  //
  // Check the SphereRefinement_CenterParam key.
  PyObject* centerItem = PyDict_GetItemString(obj, TetGenOption::SphereRefinement_CenterParam);
  if (centerItem == nullptr) {
      return false;
  }

  if (!PyList_Check(centerItem)) {
      PyErr_SetString(PyExc_ValueError, "The center parameter must be a list of three floats.");
      return false;
  }

  center.clear();
  auto num = PyList_Size(centerItem);
  if (num != 3) { 
      PyErr_SetString(PyExc_ValueError, "The center parameter must be a list of three floats.");
      return false;
  }
  std::cout << "[GetSphereRefinementValues] num: " << num << std::endl;

  for (int i = 0; i < num; i++) {
      auto item = PyList_GetItem(centerItem, i);
      auto coord = PyFloat_AsDouble(item);
      //std::cout << "[GetSphereRefinementValues] coord: " << coord << std::endl;
      if (PyErr_Occurred()) {
          PyErr_SetString(PyExc_ValueError, "The center parameter must be a list of three floats.");
          return false;
      }
      center.push_back(coord);
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
      int faceID;
      double edgeSize;
      GetLocalEdgeSizeValues(obj, faceID, edgeSize);
      values.push_back((double)faceID);
      values.push_back(edgeSize);
  }

  Py_DECREF(obj);
  return values;
}

//------------------------------
// PyTetGenOptionsGetListValues 
//------------------------------
// Get a list of attribute values from the MeshingOptions object.
//
// Return a vector of vector of doubles to mimic how SV processes options.
//
static std::vector<std::vector<double>>
PyTetGenOptionsGetListValues(PyObject* meshingOptions, std::string name)
{
  //std::cout << "========== PyTetGenOptionsGetListValues ==========" << std::endl;
  //std::cout << "[PyTetGenOptionsGetListValues] name: " << name << std::endl;
  std::vector<std::vector<double>> listValues;
  auto obj = PyObject_GetAttrString(meshingOptions, name.c_str());
  if (obj == Py_None) {
      //std::cout << "[PyTetGenOptionsGetListValues] obj == Py_None " << std::endl;
      return listValues;
  }

  if (!PyList_Check(obj)) {
      //std::cout << "[PyTetGenOptionsGetListValues] ERROR: not a list " << std::endl;
      return listValues;
  }

  auto num = PyList_Size(obj);
  //std::cout << "[PyTetGenOptionsGetListValues] num: " << num << std::endl;
  for (int i = 0; i < num; i++) {
      auto item = PyList_GetItem(obj, i);
      if (name == TetGenOption::LocalEdgeSize) {
          int faceID;
          double edgeSize;
          GetLocalEdgeSizeValues(item, faceID, edgeSize);
          //std::cout << "[PyTetGenOptionsGetListValues] faceID: " << faceID << "  edgSize: " << edgeSize << std::endl;
          std::vector<double> values = { static_cast<double>(faceID), edgeSize };
          listValues.push_back(values);
      }
      /* [TODO:DaveP] I don't think we need this
      else if (name == TetGenOption::SphereRefinement) {
          double edgeSize;
          double radius;
          std::vector<double> center;
          PyTetGenOptionsGetSphereRefinementValues(item, edgeSize, radius, center);
          std::vector<double> values = { edgeSize, radius, center[0], center[1], center[2] };
          listValues.push_back(values);
     
      }
      */
  }

  return listValues;
}

//---------------------------------
// PyTetGenOptionsAddLocalEdgeSize
//---------------------------------
// Add a local (face) edge size option read from a .msh file.
//
// The face ID is a string but must be mapped to an int.
//
//   <command content="localSize wall_aorta 0.5" />
//
void
PyTetGenOptionsAddLocalEdgeSize(PyMeshingTetGenOptionsClass* options, std::vector<std::string>& vals, std::map<std::string,int>& faceMap)
{
  std::cout << "================ PyTetGenOptionsAddLocalEdgeSize ================" << std::endl;
  std::cout << "[PyTetGenOptionsAddLocalEdgeSize] vals[0]: " << vals[0] << std::endl;
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  std::cout << "[PyTetGenOptionsAddLocalEdgeSize] face mape: " << std::endl;
  for (auto const& item : faceMap) {
      std::cout << "[PyTetGenOptionsAddLocalEdgeSize]   face: " << item.first << std::endl;
  }
  // Map the string face name to an int ID.
  int faceID = faceMap[vals[0]];
  auto edgeSize = std::stod(vals[1]);
  auto value = CreateLocalEdgeSizeValue(api, faceID, edgeSize);

  // Create a new list or add the edge size to an existing list. 
  if (options->local_edge_size == Py_None) { 
      auto edgeList = PyList_New(1);
      PyList_SetItem(edgeList, 0, value);
      options->local_edge_size = edgeList; 
  } else {
      PyList_Append(options->local_edge_size, value);
  }
}

//------------------------------------
// PyTetGenOptionsAddSphereRefinement
//------------------------------------
// Add a sphere refinement option read from a .msh file.
//
//   <command content="sphereRefinement 0.1 3.74711 4.31023 0.963407 -14.3689" /> 
//
void
PyTetGenOptionsAddSphereRefinement(PyMeshingTetGenOptionsClass* options, std::vector<std::string>& values)
{
  std::cout << "================ PyTetGenOptionsAddLocalEdgeSize ================" << std::endl;
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto edgeSize = std::stod(values[0]);
  auto radius = std::stod(values[1]);
  double center[3] = { std::stod(values[2]), std::stod(values[3]), std::stod(values[4]) };
  auto value = CreateSphereRefinementValue(api, edgeSize, radius, center);

  // Create a new list or add the edge size to an existing list. 
  if (options->sphere_refinement == Py_None) {
      auto sphereRefList = PyList_New(1);
      PyList_SetItem(sphereRefList, 0, value);
      options->sphere_refinement = sphereRefList;
  } else {
      PyList_Append(options->sphere_refinement, value);
  }
}

//-----------------------------------
// PyTetGenOptionsAddMeshSizeOptions
//-----------------------------------
// Add options that are not processed in cvTetGenMeshObject::SetMeshOptions() and
// used generate the mesh size data arrays.
//
void 
PyTetGenOptionsAddMeshSizeOptions(PyMeshingTetGenOptionsClass* options, std::map<std::string,int>& faceMap, 
    std::map<std::string,std::vector<std::string>>& meshSizeOptions) 
{ 
  std::cout << "================ PyTetGenOptionsAddMeshSizeOptions ================" << std::endl;
  using namespace TetGenOption;

  for (auto const& entry : meshSizeOptions) {
      auto name = entry.first;
      auto values = entry.second;

      // Process radius based meshing.
      //
      //    <command content="useCenterlineRadius" />
      //    <command content="functionBasedMeshing 0.4 DistanceToCenterlines" />
      //
      if (name == CommandUseCenterlineRadius) {
          std::vector<std::string> functionParams;
          try {
              functionParams = meshSizeOptions.at(CommandFunctionBasedMeshing);
          } catch (const std::out_of_range& except) {
             std::cout << "ERROR: No " << CommandFunctionBasedMeshing << " found in .msh file." << std::endl;
             return;
          }
          options->radius_meshing_on = 1; 
          options->radius_meshing_scale = std::stod(functionParams[0]);
          std::cout << "[PyTetGenOptionsAddMeshSizeOptions] Set " << name << "  scale: " << options->radius_meshing_scale << std::endl;

      // Process local edge size.
      //
      //   <command content="localSize wall_aorta 0.2" />
      //
      } else if (name == CommandLocalSize) {
          PyTetGenOptionsAddLocalEdgeSize(options, values, faceMap);
          std::cout << "[PyTetGenOptionsAddMeshSizeOptions] Set " << name << std::endl;

      // Process sphere refinement.
      //
      //   <command content="sphereRefinement 0.1 3.74711 4.31023 0.963407 -14.3689" />
      //
      } else if (name == CommandSphereRefinement) {
          PyTetGenOptionsAddSphereRefinement(options, values);
      }
  }
}

//-------------------------------
// PyTetGenOptionsCreateFromList
//-------------------------------
// Create an TetGen options object from a list of commands read from 
// an SV Meshes .msh file.
//
// The list is obtained from a mesh .msh file. For example
//
//  <command_history>
//    <command content="option surface 1" />
//    <command content="option volume 1" />
//    <command content="option UseMMG 1" />
//    <command content="option GlobalEdgeSize 0.20" />
// </command_history>
//
// Some of the commands have an 'option' prefix designating them 
// as options processed in cvTetGenMeshObject::SetMeshOptions(). 
//
// The 'setWalls' option is used as a flag to set the
// set the mesh wall IDs using SetWalls().
//
// In SV the commads are parsed in sv4guiMeshTetGen::ParseCommand().
//
// Note: The options need to be processed after the solid model is loaded
// because the 'setWalls' option.
//
// Some entries in the .msh file are options used to create mesh size data 
// arrays used for meshing based on a size set for each element
//
//    <command content="useCenterlineRadius" />
//    <command content="functionBasedMeshing 0.4 DistanceToCenterlines" />
//    <command content="localSize wall_aorta 0.2" />
//    <command content="sphereRefinement 0.1 3.74711 4.31023 0.963407 -14.3689" />
//
// These commands, except for localSize, are not processed in cvTetGenMeshObject::SetMeshOptions().  
// We will use process these options (including localSize) separately and generate the mesh 
// size data arrays within the API (useCenterlineRadius must be generated first).
//
void 
PyTetGenOptionsCreateFromList(cvMeshObject* mesher, std::vector<std::string>& optionList, std::map<std::string,int>& faceMap, PyObject** optionsReturn)
{
  std::cout << "================ PyTetGenOptionsCreateFromList ================" << std::endl;
  std::cout << "[PyTetGenOptionsCreateFromList] faceMap: " << faceMap.size() << std::endl;

  // Define a map to set the values in PyMeshingTetGenOptionsClass.
  //
  // Note that some of the option names in the .msh file are not the
  // same as those used for TetGen options (e.g. surface = SurfaceMeshFlag).
  //
  // We need to have 'faceMap' to map string face IDs in the .msh file to ints 
  // used internally.
  //
  using namespace TetGenOption;
  using OptType = PyMeshingTetGenOptionsClass*;
  using ArgType = std::vector<std::string>&; 
  using MapType = std::map<std::string,int>&;
  using SetValueMapType  = std::map<std::string, std::function<void(OptType, ArgType, MapType)>>;
  SetValueMapType SetValueMap = {
    {pyToSvNameMap[AllowMultipleRegions], [](OptType opt, ArgType vals, MapType fmap) -> void { opt->allow_multiple_regions = std::stoi(vals[0]); }},
    {pyToSvNameMap[GlobalEdgeSize], [](OptType opt, ArgType vals, MapType fmap) -> void { opt->global_edge_size = std::stof(vals[0]); }},
    //{pyToSvNameMap[LocalEdgeSize], [](OptType opt, ArgType vals, MapType fmap) -> void { PyTetGenOptionsAddLocalEdgeSize(opt,vals,fmap); }},
    {pyToSvNameMap[NoBisect], [](OptType opt, ArgType vals, MapType fmap) -> void { opt->no_bisect = Py_BuildValue("i", 1); }},
    {pyToSvNameMap[Optimization], [](OptType opt, ArgType vals, MapType fmap) -> void { opt->optimization = std::stoi(vals[0]); }},
    {pyToSvNameMap[QualityRatio], [](OptType opt, ArgType vals, MapType fmap) -> void { opt->quality_ratio = std::stod(vals[0]); }},
    {pyToSvNameMap[SurfaceMeshFlag], [](OptType opt, ArgType vals, MapType fmap) -> void { opt->surface_mesh_flag = std::stoi(vals[0]); }},
    {pyToSvNameMap[UseMMG], [](OptType opt, ArgType vals, MapType fmap) -> void { opt->use_mmg = std::stoi(vals[0]); }},
    {pyToSvNameMap[VolumeMeshFlag], [](OptType opt, ArgType vals, MapType fmap) -> void { opt->volume_mesh_flag = std::stoi(vals[0]); }},
  };

  // Create an options object.
  PyObject *args = Py_BuildValue("()");
  PyObject* kwargs = Py_BuildValue("{}"); 
  auto optionsObj = CreateTetGenOptionsType(args, kwargs);
  auto options = (PyMeshingTetGenOptionsClass*)optionsObj;

  // Set option values given in the option list.
  std::map<std::string,std::vector<std::string>> meshSizeOptions;
  std::cout << "[PyTetGenOptionsCreateFromList] List: " << std::endl;
  for (auto const& option : optionList) {
      std::regex regex{R"([\s,]+)"}; // split on space and comma
      std::sregex_token_iterator it{option.begin(), option.end(), regex, -1};
      std::vector<std::string> tokens{it, {}};
      bool isOption = false;
      if (tokens[0] == "option") {
          tokens.erase(tokens.begin());
          isOption = true;
      }
      auto name = tokens[0];

      // Map .msh file option names. 
      if (MshFileOptionNamesMap.count(name)) {
          name = MshFileOptionNamesMap[name];
          isOption = true;
      }

      // Set values to be processed in cvTetGenMeshObject::SetMeshOptions().
      if (isOption) { 
          tokens.erase(tokens.begin());
          std::cout << "[PyTetGenOptionsCreateFromList]   option  '" << name << "'" << std::endl;
          try {
              SetValueMap[name](options, tokens, faceMap);
          } catch (const std::bad_function_call& except) {
              std::cout << "[PyTetGenOptionsCreateFromList]       Unknown name: " << name << std::endl;
          }

      // Save commands not processed in cvTetGenMeshObject::SetMeshOptions().
      } else {
          std::cout << "[PyTetGenOptionsCreateFromList]   parameter '" << option << "'" << std::endl;
          tokens.erase(tokens.begin());
          //auto tetGenMesher = dynamic_cast<cvTetGenMeshObject*>(mesher);
          //MeshingTetGenSetParameter(tetGenMesher, name, tokens);
          meshSizeOptions[name] = tokens;
      }
  }

  // Process commands not processed in cvTetGenMeshObject::SetMeshOptions().
  if (meshSizeOptions.size() != 0) { 
      PyTetGenOptionsAddMeshSizeOptions(options, faceMap, meshSizeOptions);
  }

  *optionsReturn = optionsObj;
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
PyDoc_STRVAR(PyTetGenOptions_AddSubdomain_doc,
  "AddSubdomain(coordinate, region_size)  \n\ 
  \n\
  Create a value for the add_subdomain option. \n\
  \n\
  Args:  \n\
    coordinate ([float,float,float]): The 3D coordinate for the subdomain. \n\
    region_size(int): The size of the region.  \n\
");

static PyObject *
PyTetGenOptions_AddSubdomain(PyMeshingTetGenOptionsClass* self, PyObject* args, PyObject* kwargs)
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

//-------------------------------
// PyTetGenOptions_LocalEdgeSize
//-------------------------------
//
PyDoc_STRVAR(PyTetGenOptions_LocalEdgeSize_doc,
  "LocalEdgeSize(face_id, size)  \n\ 
  \n\
  Create a value for the local_edge_size option. \n\
  \n\
  Args:  \n\
    face_id (int): The ID of the face to set the edge size for.  \n\
    size (double): The edge size for the face.  \n\
");

static PyObject *
PyTetGenOptions_LocalEdgeSize(PyMeshingTetGenOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("id", PyRunTimeErr, __func__);
  static char *keywords[] = { TetGenOption::LocalEdgeSize_FaceIDParam, TetGenOption::LocalEdgeSize_EdgeSizeParam, NULL }; 
  int faceID = 0;
  double edgeSize = 0.0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &faceID, &edgeSize)) {
      return api.argsError();
  }

  auto value = CreateLocalEdgeSizeValue(api, faceID, edgeSize);
  if (value == nullptr) { 
      return nullptr;
  }

  return value; 
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
  PyDict_SetItemString(values, TetGenOption::LocalEdgeSizeOn, PyBool_FromLong(self->local_edge_size_on));

  PyDict_SetItemString(values, TetGenOption::MeshWallFirst, self->mesh_wall_first);

  PyDict_SetItemString(values, TetGenOption::NewRegionBoundaryLayer, self->new_region_boundary_layer);
  PyDict_SetItemString(values, TetGenOption::NoBisect, self->no_bisect);
  PyDict_SetItemString(values, TetGenOption::NoMerge, self->no_merge);

  PyDict_SetItemString(values, TetGenOption::Optimization, Py_BuildValue("i", self->optimization));

  PyDict_SetItemString(values, TetGenOption::QualityRatio, Py_BuildValue("d", self->quality_ratio));
  PyDict_SetItemString(values, TetGenOption::Quiet, self->quiet);

  if (self->radius_meshing_centerlines != Py_None) {
      PyTypeObject* type = self->radius_meshing_centerlines->ob_type;
      PyDict_SetItemString(values, TetGenOption::RadiusMeshingCenterlines, Py_BuildValue("s", type->tp_name));
  } else {
      PyDict_SetItemString(values, TetGenOption::RadiusMeshingCenterlines, self->radius_meshing_centerlines);
  }

  PyDict_SetItemString(values, TetGenOption::RadiusMeshingOn, PyBool_FromLong(self->radius_meshing_on));
  PyDict_SetItemString(values, TetGenOption::RadiusMeshingScale, Py_BuildValue("d", self->radius_meshing_scale));

  PyDict_SetItemString(values, TetGenOption::SphereRefinement, self->sphere_refinement);
  PyDict_SetItemString(values, TetGenOption::SphereRefinementOn, PyBool_FromLong(self->sphere_refinement_on));
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

  self->local_edge_size = PyList_New(0);
  Py_INCREF(self->local_edge_size);
  self->local_edge_size_on = 0;

  self->radius_meshing_centerlines = Py_BuildValue("");
  self->radius_meshing_on = 0;
  self->radius_meshing_scale = 0.1;

  self->sphere_refinement = PyList_New(0);
  // [TODO:DaveP] do i need to incr?
  Py_INCREF(self->sphere_refinement);
  self->sphere_refinement_on = 0;

  Py_RETURN_NONE;
}

//----------------------------------
// PyTetGenOptions_SphereRefinement
//----------------------------------
//
PyDoc_STRVAR(PyTetGenOptions_SphereRefinement_doc,
  "SphereRefinement(edge_size, radius, center)  \n\ 
  \n\
  Create a sphere refinement value. \n\
  \n\
  Args:  \n\
    edge_size (float): The edge size.  \n\
    radius: (float): The sphere radius.  \n\
    center: (list[float,float,float]): The sphere center.  \n\
");

static PyObject *
PyTetGenOptions_SphereRefinement(PyMeshingTetGenOptionsClass* self, PyObject* args, PyObject* kwargs)
{
  auto api = SvPyUtilApiFunction("ddO", PyRunTimeErr, __func__);
  static char *keywords[] = { TetGenOption::SphereRefinement_EdgeSizeParam, TetGenOption::SphereRefinement_RadiusParam, 
    TetGenOption::SphereRefinement_CenterParam, NULL }; 
  double edgeSize = 0.0;
  double radius = 0.0;
  PyObject* centerArg;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &edgeSize, &radius, &centerArg)) {
      return api.argsError();
  }

  // Get center.
  auto num = PyList_Size(centerArg);
  if (num != 3) {
      api.error("The '" + std::string(TetGenOption::SphereRefinement_CenterParam) + "' must be a list of three floats.");
      return nullptr;
  }
  double center[3];
  for (int i = 0; i < num; i++) {
      auto item = PyList_GetItem(centerArg, i);
      auto coord = PyFloat_AsDouble(item);
      if (PyErr_Occurred()) {
          api.error("The '" + std::string(TetGenOption::SphereRefinement_CenterParam) + "' must be a list of three floats.");
          return nullptr;
      }
      center[i] = coord;
  }

  auto value = CreateSphereRefinementValue(api, edgeSize, radius, center);
  if (value == nullptr) { 
      return nullptr;
  }
  return value; 
}

//------------------------
// PyTetGenOptionsMethods 
//------------------------
//
static PyMethodDef PyTetGenOptionsMethods[] = {
  {"AddSubdomain", (PyCFunction)PyTetGenOptions_AddSubdomain, METH_VARARGS|METH_KEYWORDS, PyTetGenOptions_AddSubdomain_doc},
  {"get_values", (PyCFunction)PyTetGenOptions_get_values, METH_NOARGS, PyTetGenOptions_get_values_doc},
  {"LocalEdgeSize", (PyCFunction)PyTetGenOptions_LocalEdgeSize, METH_VARARGS|METH_KEYWORDS, PyTetGenOptions_LocalEdgeSize_doc},
  {"SphereRefinement", (PyCFunction)PyTetGenOptions_SphereRefinement, METH_VARARGS|METH_KEYWORDS, PyTetGenOptions_SphereRefinement_doc},
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
// SphereRefinement: Implement as a list here, getter/setter does not work, crashes SV.
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

    {TetGenOption::LocalEdgeSize, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, local_edge_size), 0, "local_edge_size"},
    {TetGenOption::LocalEdgeSizeOn, T_BOOL, offsetof(PyMeshingTetGenOptionsClass, local_edge_size_on), 0, "local_edge_size_on"},

    {TetGenOption::MeshWallFirst, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, mesh_wall_first), 0, "mesh_wall_first"},

    {TetGenOption::NewRegionBoundaryLayer, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, new_region_boundary_layer), 0, "new_region_boundary_layer"},
    {TetGenOption::NoBisect, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, no_bisect), 0, "no_bisect"},
    {TetGenOption::NoMerge, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, no_merge), 0, "no_merge"},

    {TetGenOption::Optimization, T_INT, offsetof(PyMeshingTetGenOptionsClass, optimization), 0, "Optimization"},
    {TetGenOption::QualityRatio, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, quality_ratio), 0, "quality_ratio"},
    {TetGenOption::Quiet, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, quiet), 0, "Quiet"},

    {TetGenOption::RadiusMeshingOn, T_BOOL, offsetof(PyMeshingTetGenOptionsClass, radius_meshing_on), 0, "radius_meshing_on"},

    {TetGenOption::SphereRefinementOn, T_BOOL, offsetof(PyMeshingTetGenOptionsClass, sphere_refinement_on), 0, "sphere_refinement_on"},
    {TetGenOption::SphereRefinement, T_OBJECT_EX, offsetof(PyMeshingTetGenOptionsClass, sphere_refinement), 0, "sphere_refinement"},

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
// [TODO:DaveP] Get/Set does not work for lists, causes SV to crash.
/*
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
  int faceID;
  double edgeSize;
  if (!GetLocalEdgeSizeValues(value, faceID, edgeSize)) {
      return -1;
  }

  // Create a list of dicts.
  //
  auto edgeList = PyList_New(1);
  PyList_SetItem(edgeList, 0, value);
  self->local_edge_size = edgeList;
  Py_INCREF(value);

  return 0;
}
*/

//------------------------------------------------
// PyTetGenOptions_get_radius_meshing_centerlines 
//------------------------------------------------
//
static PyObject*
PyTetGenOptions_get_radius_meshing_centerlines(PyMeshingTetGenOptionsClass* self, void* closure)
{
  return self->radius_meshing_centerlines; 
}

static int
PyTetGenOptions_set_radius_meshing_centerlines(PyMeshingTetGenOptionsClass* self, PyObject* object, void* closure)
{
  static std::string errorMsg = "The radius_meshing_centerlines option must be a vtkPolyData object.";

  if (!PyVTKObject_Check(object)) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return -1;
  }

  auto polydata = (vtkPolyData*)vtkPythonUtil::GetPointerFromObject(object, "vtkPolyData");
  if (polydata == nullptr) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return -1;
  }

  // Check that the polydata contains centerlines data.
  auto ids = polydata->GetCellData()->GetArray("CenterlineIds");
  if (ids == nullptr) {
      std::string msg = "The radius_meshing_centerlines option does not conatain centerline data.";
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return -1;
  }

  self->radius_meshing_centerlines = object;
  Py_INCREF(object);
  return 0;
}

//------------------------------------------
// PyTetGenOptions_get_radius_meshing_scale
//------------------------------------------
//
static PyObject*
PyTetGenOptions_get_radius_meshing_scale(PyMeshingTetGenOptionsClass* self, void* closure)
{
  return  Py_BuildValue("d", self->radius_meshing_scale);
}

static int
PyTetGenOptions_set_radius_meshing_scale(PyMeshingTetGenOptionsClass* self, PyObject* value, void* closure)
{
  static std::string errorMsg = "The radius_meshing_scale option must be a float > 0.0.";

  double scale = PyFloat_AsDouble(value);
  if (PyErr_Occurred()) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return -1;
  }

  if (scale <= 0.0) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return -1;
  }

  // [TODO:DaveP] maybe check that this is a good value, should near global_edge_size ?
  self->radius_meshing_scale = scale;
  return 0;
}

//---------------------------------------
// PyTetGenOptions_get_sphere_refinement 
//---------------------------------------
//
// [TODO:DaveP] Get/Set does not work for lists, causes SV to crash.
/*
static PyObject*
PyTetGenOptions_get_sphere_refinement(PyMeshingTetGenOptionsClass* self, void* closure)
{
  return self->sphere_refinement;
}

static int
PyTetGenOptions_set_sphere_refinement(PyMeshingTetGenOptionsClass* self, PyObject* valuesList, void* closure)
{
  std::cout << "========== PyTetGenOptions_set_sphere_refinement ==========  " << std::endl;
  static std::string errorMsg = "The sphere_refinement parameter must be a list of " + TetGenOption::SphereRefinement_Desc; 

  if (!PyList_Check(valuesList)) {
      PyErr_SetString(PyExc_ValueError, errorMsg.c_str());
      return -1;
  }

  int numValues = PyList_Size(valuesList);
  if (numValues == 0) { 
      PyErr_SetString(PyExc_ValueError, "The refinement values list argument is empty.");
      return -1;
  }
  std::cout << "[PyTetGenOptions_set_sphere_refinement] numValues: " << numValues << std::endl;

  // Check the refinement values.
  for (int i = 0; i < numValues; i++) {
      auto item = PyList_GetItem(valuesList, i);
      double edgeSize;
      double radius;
      std::vector<double> vcenter;
      std::cout << "[PyTetGenOptions_set_sphere_refinement] item: " << item << std::endl;
      if (!GetSphereRefinementValues(item, edgeSize, radius, vcenter)) {
          return -1;
      }
  }

  // Create a list of dicts.
  //
  auto refList = PyList_New(numValues);
  for (int i = 0; i < numValues; i++) {
      auto item = PyList_GetItem(valuesList, i);
      PyList_SetItem(refList, i, Py_BuildValue("O",item));
  }
  self->sphere_refinement = refList; 
  Py_INCREF(refList);
  return 0;
}
*/

//------------------------
// PyTetGenOptionsGetSets
//------------------------
//
PyGetSetDef PyTetGenOptionsGetSets[] = {
    { TetGenOption::AddHole, (getter)PyTetGenOptions_get_add_hole, (setter)PyTetGenOptions_set_add_hole, NULL,  NULL },
    { TetGenOption::AddSubDomain, (getter)PyTetGenOptions_get_add_subdomain, (setter)PyTetGenOptions_set_add_subdomain, NULL,  NULL },
    { TetGenOption::RadiusMeshingScale, (getter)PyTetGenOptions_get_radius_meshing_scale, (setter)PyTetGenOptions_set_radius_meshing_scale, NULL,  NULL },
    // [TODO:DaveP] Get/Set does not work for lists, causes SV to crash.
    //{ TetGenOption::LocalEdgeSize, (getter)PyTetGenOptions_get_local_edge_size, (setter)PyTetGenOptions_set_local_edge_size, NULL,  NULL },
    { TetGenOption::RadiusMeshingCenterlines, (getter)PyTetGenOptions_get_radius_meshing_centerlines, (setter)PyTetGenOptions_set_radius_meshing_centerlines, NULL,  NULL },
    { TetGenOption::RadiusMeshingScale, (getter)PyTetGenOptions_get_radius_meshing_scale, (setter)PyTetGenOptions_set_radius_meshing_scale, NULL,  NULL },
    // [TODO:DaveP] Get/Set does not work for lists, causes SV to crash.
    //{ TetGenOption::SphereRefinement, (getter)PyTetGenOptions_get_sphere_refinement, (setter)PyTetGenOptions_set_sphere_refinement, NULL,  NULL },
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
  auto api = SvPyUtilApiFunction("|dO!O!O!", PyRunTimeErr, __func__);
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
  std::cout << "[PyTetGenOptionsDealloc] Done. " << std::endl;
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
PyObject *
CreateTetGenOptionsType(PyObject* args, PyObject* kwargs)
{
  return PyObject_Call((PyObject*)&PyTetGenOptionsType, args, kwargs);
}

#endif

