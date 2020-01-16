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

// Define the Python 'meshing.TetGenOptions' class that encapsulates the paramters
// used for generating a mesh using TetGen.
//
#ifndef PYAPI_MESHING_TETGEN_OPTIONS_H
#define PYAPI_MESHING_TETGEN_OPTIONS_H 

#include <iostream>
#include <math.h>
#include <string>
#include <structmember.h>

//---------------------
// MeshingOptionsClass 
//---------------------
// Define the MeshingOptionsClass. 
//
typedef struct {
  PyObject_HEAD
  int add_hole;
  int add_subdomain;
  int allow_multiple_regions;
  int boundary_layer_direction;
  int check;
  double coarsen_percent;
  int diagnose;
  double epsilon;
  double global_edge_size;
  double hausd;
  int local_edge_size[2];
  int mesh_wall_first;
  int new_region_boundary_layer;
  int no_bisect;
  int no_merge;
  int optimization;
  double quality_ratio;
  int quiet;
  int start_with_volume;
  int surface_mesh_flag;
  int use_mmg;
  int verbose;
  int volume_mesh_flag;
  std::map<std::string,double> values;
} PyMeshingTetGenOptionsClass;

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

  std::map<std::string,std::string> pyToSvNameMap = {
      {std::string(AddHole), "AddHole"}, 
      {std::string(AddSubDomain), "AddSubDomain"},
      {std::string(AllowMultipleRegions), "AllowMultipleRegions"},
      {std::string(BoundaryLayerDirection), "boundary_layer_direction"},
      {std::string(Check), "check"},
      {std::string(CoarsenPercent), "coarsen_percent"},
      {std::string(Diagnose), "diagnose"},
      {std::string(Epsilon), "epsilon"},
      {std::string(GlobalEdgeSize), "global_edge_size"},
      {std::string(Hausd), "hausd"},
      {std::string(LocalEdgeSize), "local_edge_size"},
      {std::string(MeshWallFirst), "mesh_wall_first"},
      {std::string(NewRegionBoundaryLayer), "new_region_boundary_layer"},
      {std::string(NoBisect), "no_bisect"},
      {std::string(NoMerge), "no_merge"},
      {std::string(Optimization), "optimization"},
      {std::string(QualityRatio), "quality_ratio"},
      {std::string(Quiet), "quiet"},
      {std::string(StartWithVolume), "start_with_volume"},
      {std::string(SurfaceMeshFlag), "surface_mesh_flag"},
      {std::string(UseMMG), "use_mmg"},
      {std::string(Verbose), "verbose"},
      {std::string(VolumeMeshFlag), "volume_mesh_flag"}
  };

};

//-----------------------
// PyTetGenOptionsGetInt
//-----------------------
// Get an integer or boolean atttibute from the MeshingOptions object.
//
static int
PyTetGenOptionsGetInt(PyObject* meshingOptions, std::string name)
{
  auto obj = PyObject_GetAttrString(meshingOptions, name.c_str());
  auto value = PyInt_AsLong(obj);
  Py_DECREF(obj);
  return value;
}

//--------------------------
// PyTetGenOptionsGetDouble 
//--------------------------
// Get a double attribute from the MeshingOptions object.
//
static double 
PyTetGenOptionsGetDouble(PyObject* meshingOptions, std::string name)
{
  auto obj = PyObject_GetAttrString(meshingOptions, name.c_str());
  auto value = PyFloat_AsDouble(obj);
  Py_DECREF(obj);
  return value;
}

////////////////////////////////////////////////////////
//          C l a s s    M e t h o d s                //
////////////////////////////////////////////////////////
//

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

  PyDict_SetItemString(values, TetGenOption::AddHole, Py_BuildValue("i", self->add_hole));
  PyDict_SetItemString(values, TetGenOption::AddSubDomain, Py_BuildValue("i", self->add_subdomain));
  PyDict_SetItemString(values, TetGenOption::AllowMultipleRegions, Py_BuildValue("i", self->allow_multiple_regions));
  PyDict_SetItemString(values, TetGenOption::BoundaryLayerDirection, Py_BuildValue("i", self->boundary_layer_direction));
  PyDict_SetItemString(values, TetGenOption::Check, Py_BuildValue("i", self->check));
  PyDict_SetItemString(values, TetGenOption::CoarsenPercent, Py_BuildValue("d", self->coarsen_percent));
  PyDict_SetItemString(values, TetGenOption::Diagnose, Py_BuildValue("i", self->diagnose));
  PyDict_SetItemString(values, TetGenOption::Epsilon, Py_BuildValue("d", self->epsilon));
  PyDict_SetItemString(values, TetGenOption::GlobalEdgeSize, Py_BuildValue("d", self->global_edge_size));
  PyDict_SetItemString(values, TetGenOption::Hausd, Py_BuildValue("d", self->hausd));
  PyDict_SetItemString(values, TetGenOption::LocalEdgeSize, Py_BuildValue("(i,i)", self->local_edge_size[0], self->local_edge_size[1]));
  PyDict_SetItemString(values, TetGenOption::MeshWallFirst, Py_BuildValue("i", self->mesh_wall_first));
  PyDict_SetItemString(values, TetGenOption::NewRegionBoundaryLayer, Py_BuildValue("i", self->new_region_boundary_layer));
  PyDict_SetItemString(values, TetGenOption::NoBisect, Py_BuildValue("i", self->no_bisect));
  PyDict_SetItemString(values, TetGenOption::NoMerge, Py_BuildValue("i", self->no_merge));
  PyDict_SetItemString(values, TetGenOption::Optimization, Py_BuildValue("i", self->optimization));
  PyDict_SetItemString(values, TetGenOption::QualityRatio, Py_BuildValue("d", self->quality_ratio));
  PyDict_SetItemString(values, TetGenOption::Quiet, Py_BuildValue("i", self->quiet));
  PyDict_SetItemString(values, TetGenOption::StartWithVolume, Py_BuildValue("i", self->start_with_volume));
  PyDict_SetItemString(values, TetGenOption::SurfaceMeshFlag, Py_BuildValue("i", self->surface_mesh_flag));
  PyDict_SetItemString(values, TetGenOption::UseMMG, Py_BuildValue("i", self->use_mmg));
  PyDict_SetItemString(values, TetGenOption::Verbose, Py_BuildValue("i", self->verbose));
  PyDict_SetItemString(values, TetGenOption::VolumeMeshFlag, Py_BuildValue("i", self->volume_mesh_flag));

  return values;
}

//------------------------------
// PyTetGenOptions_set_defaults
//------------------------------
//
static PyObject *
PyTetGenOptions_set_defaults(PyMeshingTetGenOptionsClass* self)
{
  self->add_hole = 0;
  self->add_subdomain = 0;
  self->allow_multiple_regions = 0;
  self->boundary_layer_direction = 0;
  self->check = 0;
  self->coarsen_percent = 0;
  self->diagnose = 0;
  self->epsilon = 0;
  self->global_edge_size = 0;
  self->hausd = 0;
  self->local_edge_size[0] = 0;
  self->local_edge_size[1] = 0;
  self->mesh_wall_first = 0;
  self->new_region_boundary_layer = 0;
  self->no_bisect = 0;
  self->no_merge = 0;
  self->optimization = 0;
  self->quality_ratio = 0;
  self->quiet = 0;
  self->start_with_volume = 0;
  self->surface_mesh_flag = 0;
  self->use_mmg = 0;
  self->verbose = 0;
  self->volume_mesh_flag = 0;

  Py_RETURN_NONE;
}

//------------------------
// PyTetGenOptionsMethods 
//------------------------
//
static PyMethodDef PyTetGenOptionsMethods[] = {
  {"get_values", (PyCFunction)PyTetGenOptions_get_values, METH_NOARGS, PyTetGenOptions_get_values_doc},
  {NULL, NULL}
};

///////////////////////////////////////////////////////
//          C l a s s    M e m b e r s                //
////////////////////////////////////////////////////////
//
// Define the PyMeshingTetGenOptionsClass attribute names.
//
// The attributes can be set/get directly in from the MeshingOptions object.
//
static PyMemberDef PyTetGenOptionsMembers[] = {
    {TetGenOption::AddHole, T_INT, offsetof(PyMeshingTetGenOptionsClass, add_hole), 0, "add_hole"},
    {TetGenOption::AddSubDomain, T_INT, offsetof(PyMeshingTetGenOptionsClass, add_subdomain), 0, "add_subdomain"},
    {TetGenOption::AllowMultipleRegions, T_INT, offsetof(PyMeshingTetGenOptionsClass, allow_multiple_regions), 0, "allow_multiple_regions"},
    {TetGenOption::BoundaryLayerDirection, T_INT, offsetof(PyMeshingTetGenOptionsClass, boundary_layer_direction), 0, "boundary_layer_direction"},
    {TetGenOption::Check, T_INT, offsetof(PyMeshingTetGenOptionsClass, check), 0, "check"},
    {TetGenOption::CoarsenPercent, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, coarsen_percent), 0, "coarsen_percent"},
    {TetGenOption::Diagnose, T_INT, offsetof(PyMeshingTetGenOptionsClass, diagnose), 0, "Diagnose"},
    {TetGenOption::Epsilon, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, epsilon), 0, "Epsilon"},
    {TetGenOption::GlobalEdgeSize, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, global_edge_size), 0, "global_edge_size"},
    {TetGenOption::Hausd, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, hausd), 0, "Hausd"},

    // [TODO] What should this be? Object?
    {TetGenOption::LocalEdgeSize, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, local_edge_size), 0, "local_edge_size"},

    {TetGenOption::MeshWallFirst, T_INT, offsetof(PyMeshingTetGenOptionsClass, mesh_wall_first), 0, "mesh_wall_first"},
    {TetGenOption::NewRegionBoundaryLayer, T_INT, offsetof(PyMeshingTetGenOptionsClass, new_region_boundary_layer), 0, "new_region_boundary_layer"},
    {TetGenOption::NoBisect, T_INT, offsetof(PyMeshingTetGenOptionsClass, no_bisect), 0, "no_bisect"},
    {TetGenOption::NoMerge, T_INT, offsetof(PyMeshingTetGenOptionsClass, no_merge), 0, "no_merge"},
    {TetGenOption::Optimization, T_INT, offsetof(PyMeshingTetGenOptionsClass, optimization), 0, "Optimization"},
    {TetGenOption::QualityRatio, T_DOUBLE, offsetof(PyMeshingTetGenOptionsClass, quality_ratio), 0, "quality_ratio"},
    {TetGenOption::Quiet, T_INT, offsetof(PyMeshingTetGenOptionsClass, quiet), 0, "Quiet"},
    {TetGenOption::StartWithVolume, T_INT, offsetof(PyMeshingTetGenOptionsClass, start_with_volume), 0, "start_with_volume"},
    {TetGenOption::SurfaceMeshFlag, T_INT, offsetof(PyMeshingTetGenOptionsClass, surface_mesh_flag), 0, "surface_mesh_flag"},
    {TetGenOption::UseMMG, T_INT, offsetof(PyMeshingTetGenOptionsClass, use_mmg), 0, "use_mmg"},
    {TetGenOption::Verbose, T_INT, offsetof(PyMeshingTetGenOptionsClass, verbose), 0, "Verbose"},
    {TetGenOption::VolumeMeshFlag, T_INT, offsetof(PyMeshingTetGenOptionsClass, volume_mesh_flag), 0, "volume_mesh_flag"},
    {NULL}  
};

////////////////////////////////////////////////////////
//          C l a s s    G e t / S e t                //
////////////////////////////////////////////////////////
//

//---------------------------------------
// PyTetGenOptions_get_global_edge_size 
//---------------------------------------
//
/*
static PyObject*
PyTetGenOptions_get_global_edge_size(PyMeshingTetGenOptionsClass* self, void* closure)
{
    return PyFloat_FromDouble(self->global_edge_size);
}

static double
PyTetGenOptions_set_global_edge_size(PyMeshingTetGenOptionsClass* self, PyObject* valueArg, void* closure)
{
  std::cout << "[PyTetGenOptions_set_global_edge_size] " << std::endl;
  double value = PyFloat_AsDouble(valueArg);
  std::cout << "[PyTetGenOptions_set_global_edge_size] value: " << value << std::endl;
  if (PyErr_Occurred()) {
      return -1;
  }

  if (value < 0) {
      PyErr_SetString(PyExc_ValueError, "global_edge_size must be positive");
  }

  self->values[TetGenOption::GLOBAL_EDGE_SIZE] = value;
  self->global_edge_size = value;
  return 0;
}

//---------------------------------------
// PyTetGenOptions_get_surface_mesh_flag 
//---------------------------------------
//
static PyObject*
PyTetGenOptions_get_surface_mesh_flag(PyMeshingTetGenOptionsClass* self, void* closure)
{
    return PyFloat_FromDouble(self->surface_mesh_flag);
}

static int 
PyTetGenOptions_set_surface_mesh_flag(PyMeshingTetGenOptionsClass* self, PyObject* valueArg, void* closure)
{
  double value = PyFloat_AsDouble(valueArg);
  std::cout << "[PyTetGenOptions_set_global_edge_size] value: " << value << std::endl;
  if (PyErr_Occurred()) {
      return -1;
  }

  if (value < 0) {
      PyErr_SetString(PyExc_ValueError, "global_edge_size must be positive");
  }

  self->values[TetGenOption::SURFACE_MESH_FLAG] = value;
  self->surface_mesh_flag = value;
  return 0;
}

PyGetSetDef PyTetGenOptionsGetSets[] = {
    { TetGenOption::GlobalEdgeSize, (getter)PyTetGenOptions_get_global_edge_size, (setter)PyTetGenOptions_set_global_edge_size, NULL,  NULL },
    { TetGenOption::SurfaceMeshFlag, (getter)PyTetGenOptions_get_surface_mesh_flag, (setter)PyTetGenOptions_set_surface_mesh_flag, NULL,  NULL },
    { TetGenOption::VolumeMeshFlag, (getter)PyTetGenOptions_get_volume_mesh_flag, (setter)PyTetGenOptions_set_volume_mesh_flag, NULL,  NULL },
    {NULL}
};
*/


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
  auto api = SvPyUtilApiFunction("d|iii", PyRunTimeErr, __func__);
  static char *keywords[] = { TetGenOption::GlobalEdgeSize, TetGenOption::SurfaceMeshFlag, TetGenOption::VolumeMeshFlag, 
                              TetGenOption::MeshWallFirst, NULL};
  double global_edge_size = 0.0;
  int surface_mesh_flag = 1;
  int volume_mesh_flag = 1;
  int mesh_wall_first = 1;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &global_edge_size, &surface_mesh_flag, &volume_mesh_flag, 
          &mesh_wall_first)) {
      api.argsError();
      return -1;
  }

  // Set the default option values.
  PyTetGenOptions_set_defaults(self);

  // Set the values that may have been passed in.
  self->global_edge_size = global_edge_size;
  self->mesh_wall_first = mesh_wall_first;
  self->surface_mesh_flag = surface_mesh_flag;
  self->volume_mesh_flag = volume_mesh_flag;

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
  //meshingOpts.tp_getset = PyTetGenOptionsGetSets;
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

