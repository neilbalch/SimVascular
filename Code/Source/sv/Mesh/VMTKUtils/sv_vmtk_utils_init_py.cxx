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

// The functions defined here implement the SV Python API VMTK utils module. 
//
// The module name is 'vmtk_utils'. 
//

#include "SimVascular.h"
#include "SimVascular_python.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
#include "sv_PolyData.h"
#include "sv_vmtk_utils_init.h"
#include "sv_vmtk_utils.h"
#include "sv_SolidModel.h"
#include "sv_vtk_utils.h"
#include "sv_PyUtils.h"
#include "Python.h"
#include "vtkSmartPointer.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

//-------------------
// GetRepositoryData 
//-------------------
// Get repository data of the given type. 
//
static cvRepositoryData *
GetRepositoryData(SvPyUtilApiFunction& api, char* name, RepositoryDataT dataType)
{
  auto data = gRepository->GetObject(name);
  if (data == NULL) {
      api.error("'"+std::string(name)+"' is not in the repository.");
      return nullptr;
  }
  if (gRepository->GetType(name) != dataType) { 
      auto typeStr = std::string(RepositoryDataT_EnumToStr(dataType));
      api.error("'" + std::string(name) + "' does not have type '" + typeStr + "'.");
      return nullptr;
  }

  return data; 
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

#ifdef SV_USE_VMTK

//------------------
// Geom_centerlines 
//------------------
//
PyDoc_STRVAR(Geom_centerlines_doc,
  " Geom_centerlines(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Geom_centerlines(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sOOss", PyRunTimeErr, __func__);
  char *geomName;
  PyObject* sourceList;
  PyObject* targetList;
  char *linesName;
  char *voronoiName;
 
  char *usage;
  cvRepositoryData *linesDst = NULL;
  cvRepositoryData *voronoiDst = NULL;
  RepositoryDataT type;

  if (!PyArg_ParseTuple(args, api.format, &geomName,&sourceList,&targetList, &linesName, &voronoiName)) {
      return api.argsError();
  }

  auto geomSrc = GetRepositoryData(api, geomName, POLY_DATA_T);
  if (geomSrc == nullptr) {
    return nullptr; 
  }

  // Make sure the specified dst object does not exist:
  if (gRepository->Exists(linesName)) {
    api.error("The object '"+std::string(linesName)+"' is already in the repository.");
    return nullptr; 
  }

  if (gRepository->Exists(voronoiName)) {
    api.error("The object '"+std::string(voronoiName)+"' is already in the repository.");
    return nullptr; 
  }

  // [TODO:DaveP] should this be an error?
  //
  int nsources = PyList_Size(sourceList);
  int ntargets = PyList_Size(targetList);
  if (nsources==0||ntargets==0) {
    return SV_PYTHON_OK;
  }

  // Get the source and target IDs?
  //
  std::vector<int> sources;
  std::vector<int> targets;

  for (int i=0;i<nsources;i++) {
    sources.push_back(PyLong_AsLong(PyList_GetItem(sourceList,i)));
  }

  for (int j=0;j<ntargets;j++) {
    targets.push_back(PyLong_AsLong(PyList_GetItem(targetList,j)));
  }

  if (sys_geom_centerlines(geomSrc, sources.data(), nsources, targets.data(), ntargets, &linesDst, &voronoiDst) 
    != SV_OK ) {
    api.error("Error creating centerlines.");
    return nullptr; 
  }

  if (!gRepository->Register(linesName, linesDst)) {
      delete linesDst;
      delete voronoiDst;
      api.error("Error adding the lines data '" + std::string(linesName) + "' to the repository.");
      return nullptr;
  }

  if (!gRepository->Register(voronoiName, voronoiDst)) {
      delete linesDst;
      delete voronoiDst;
      api.error("Error adding the voronoi data '" + std::string(voronoiName) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",linesDst->GetName());
}

//------------------
//Geom_GroupPolyDataCmd
//------------------


PyObject* Geom_GroupPolyDataCmd( PyObject* self, PyObject* args)
{
  char *usage;
  char *geomName;
  char *linesName;
  char *groupedName;
  cvRepositoryData *geomSrc;
  cvRepositoryData *linesSrc;
  cvRepositoryData *groupedDst = NULL;
  RepositoryDataT type;

  if (!PyArg_ParseTuple(args,"sss",&geomName,
	&linesName, &groupedName))
  {
    PyErr_SetString(PyRunTimeErr,
	"Could not import three chars, geomName,linesName, groupedName");
    
  }
  // Retrieve source object:
  geomSrc = gRepository->GetObject( geomName );
  if ( geomSrc == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }

  type = geomSrc->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj not of type cvPolyData");
    
  }

  // Retrieve source object:
  linesSrc = gRepository->GetObject( linesName );
  if ( linesSrc == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }

  type = linesSrc->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj not of type cvPolyData");
    
  }

  // Do work of command:

  if ( sys_geom_grouppolydata( (cvPolyData*)geomSrc, (cvPolyData*)linesSrc, (cvPolyData**)(&groupedDst) )
       != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error getting grouped polydata" );
    
  }

  if ( !( gRepository->Register( groupedName, groupedDst ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete groupedDst;
    
  }


  return Py_BuildValue("s",groupedDst->GetName()) ;
}

PyObject* Geom_DistanceToCenterlinesCmd( PyObject* self, PyObject* args)
{
  char *usage;
  char *geomName;
  char *linesName;
  char *distanceName;
  cvRepositoryData *geomSrc;
  cvRepositoryData *linesSrc;
  cvRepositoryData *distanceDst = NULL;
  RepositoryDataT type;
  if (!PyArg_ParseTuple(args,"sss",&geomName,
	&linesName, &distanceName))
  {
    PyErr_SetString(PyRunTimeErr,
	"Could not import three chars, geomName,linesName, distanceName");
    
  }

  // Retrieve source object:
  geomSrc = gRepository->GetObject( geomName );
  if ( geomSrc == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object" );
    
  }

  type = geomSrc->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj not of type cvPolyData");
    
  }

  // Retrieve source object:
  linesSrc = gRepository->GetObject( linesName );
  if ( linesSrc == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }

  type = linesSrc->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj not of type cvPolyData");
    
  }

  // Do work of command:

  if ( sys_geom_distancetocenterlines( (cvPolyData*)geomSrc, (cvPolyData*)linesSrc, (cvPolyData**)(&distanceDst) )
       != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error getting distance to centerlines" );
    
  }

  if ( !( gRepository->Register( distanceName, distanceDst ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete distanceDst;
    
  }


  return Py_BuildValue("s", distanceDst->GetName()) ;
}

PyObject* Geom_SeparateCenterlinesCmd( PyObject* self, PyObject* args)
{
  char *usage;
  char *linesName;
  char *separateName;
  cvRepositoryData *linesSrc;
  cvRepositoryData *separateDst = NULL;
  RepositoryDataT type;

  if (!PyArg_ParseTuple(args,"ss",&linesName, &separateName))
  {
    PyErr_SetString(PyRunTimeErr,
	"Could not import two chars,linesName, separateName");
    
  }
  // Retrieve source object:
  linesSrc = gRepository->GetObject( linesName );
  if ( linesSrc == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object " );
    
  }

  type = linesSrc->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj not of type cvPolyData");
    
  }

  // Do work of command:

  if ( sys_geom_separatecenterlines( (cvPolyData*)linesSrc, (cvPolyData**)(&separateDst) )
       != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error grouping centerlines" );
    
  }

  if ( !( gRepository->Register( separateName, separateDst ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete separateDst;
    
  }

  return Py_BuildValue("s",separateDst->GetName()) ;
}

PyObject* Geom_MergeCenterlinesCmd( PyObject* self, PyObject* args)
{
  char *usage;
  char *linesName;
  char *mergeName;
  int mergeblanked = 1;
  cvRepositoryData *linesSrc;
  cvRepositoryData *mergeDst = NULL;
  RepositoryDataT type;

  if (!PyArg_ParseTuple(args,"ssi",&linesName,
	&mergeName, &mergeblanked))
  {
    PyErr_SetString(PyRunTimeErr,
	"Could not import two chars and one int, linesName,mergeName, mergeblanked");
    
  }
  // Retrieve source object:
  linesSrc = gRepository->GetObject( linesName );
  if ( linesSrc == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object " );
    
  }

  type = linesSrc->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj not of type cvPolyData");
    
  }

  // Do work of command:

  if ( sys_geom_mergecenterlines( (cvPolyData*)linesSrc, mergeblanked, (cvPolyData**)(&mergeDst) )
       != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error merging centerlines" );
    
  }

  if ( !( gRepository->Register( mergeName, mergeDst ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete mergeDst;
    
  }


  return Py_BuildValue("s", mergeDst->GetName());
}


PyObject* Geom_CapCmd( PyObject* self, PyObject* args)
{
  int numIds;
  int *ids;
  int captype;
  char *usage;
  char *cappedName;
  char *geomName;
  char idstring[256];
  cvRepositoryData *geomSrc;
  cvRepositoryData *cappedDst = NULL;
  RepositoryDataT type;

  if (!PyArg_ParseTuple(args,"sss",&geomName,
	&cappedName, &captype))
  {
    PyErr_SetString(PyRunTimeErr,
	"Could not import three chars, geomName,cappedName, captype");
    
  }
  // Retrieve source object:
  geomSrc = gRepository->GetObject( geomName );
  if ( geomSrc == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object " );
    
  }

  type = geomSrc->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj not of type cvPolyData");
    
  }

  // Make sure the specified dst object does not exist:
  if ( gRepository->Exists( cappedName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Do work of command:

  if ( sys_geom_cap( (cvPolyData*)geomSrc, (cvPolyData**)(&cappedDst), &numIds,&ids,captype )
       != SV_OK ) {
    PyErr_SetString(PyRunTimeErr,"error capping model" );
    
  }

  if ( !( gRepository->Register( cappedName, cappedDst ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete cappedDst;
    
  }

//  Tcl_SetResult( interp, cappedDst->GetName() );

  if (numIds == 0)
  {
    PyErr_SetString(PyRunTimeErr, "No Ids Found" );
    
  }
  PyObject* pyList=PyList_New(numIds);
  for (int i = 0; i < numIds; i++) {
	sprintf(idstring, "%i", ids[i]);
    PyList_SetItem(pyList,i,PyBytes_FromFormat(idstring));
	idstring[0]='\n';
  }
  delete [] ids;

  return pyList;
}

PyObject* Geom_CapWIdsCmd( PyObject* self, PyObject* args)
{
  int fillId;
  char *usage;
  char *cappedName;
  char *geomName;
  int num_filled = 0;
  int filltype = 0;
  cvRepositoryData *geomSrc;
  cvRepositoryData *cappedDst = NULL;
  RepositoryDataT type;

  if (!PyArg_ParseTuple(args,"ssii",&geomName,
	&cappedName, &fillId,&filltype))
  {
    PyErr_SetString(PyRunTimeErr,
	"Could not import two chars and two ints, geomName,cappedName, fillId,filltype");
    
  }
  // Retrieve source object:
  geomSrc = gRepository->GetObject( geomName );
  if ( geomSrc == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    return nullptr;
  }

  type = geomSrc->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr,"obj not of type cvPolyData");
    return nullptr;
    
  }

  // Make sure the specified dst object does not exist:
  if ( gRepository->Exists( cappedName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    return nullptr;
    
  }

  // Do work of command:

  if ( sys_geom_cap_with_ids( (cvPolyData*)geomSrc, (cvPolyData**)(&cappedDst)
	,fillId,num_filled,filltype)
       != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error capping model" );
    return nullptr;
    
  }

  if ( !( gRepository->Register( cappedName, cappedDst ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete cappedDst;
    return nullptr;
    
  }

  return Py_BuildValue("i",num_filled);
}

PyObject* Geom_MapAndCorrectIdsCmd( PyObject* self, PyObject* args)
{
  char *usage;
  char *originalName;
  char *newName;
  char *resultName;
  char *originalArray;
  char *newArray;
  cvRepositoryData *geomSrc;
  cvRepositoryData *geomNew;
  cvRepositoryData *geomDst = NULL;
  RepositoryDataT type;

  if (!PyArg_ParseTuple(args,"sssss",&originalName,
	&newName, &resultName,&originalArray,&newArray))
  {
    PyErr_SetString(PyRunTimeErr,
	"Could not import five chars, originalName,newName, resultName"
	"originalArray, newArray");
    
  }
  // Retrieve source object:
  geomSrc = gRepository->GetObject( originalName );
  if ( geomSrc == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object " );
    
  }

  // Retrieve source object:
  geomNew = gRepository->GetObject( newName );
  if ( geomNew == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }

  type = geomSrc->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj not of type cvPolyData");
    
  }

  type = geomNew->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "obj not of type cvPolyData");
    
  }

  // Make sure the specified dst object does not exist:
  if ( gRepository->Exists( resultName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  // Do work of command:

  if ( sys_geom_mapandcorrectids( (cvPolyData*)geomSrc, (cvPolyData*)geomNew, (cvPolyData**)(&geomDst), originalArray,newArray )
       != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error correcting ids" );
    
  }

  if ( !( gRepository->Register( resultName, geomDst ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geomDst;
    
  }


  return Py_BuildValue("s",geomDst->GetName()) ;
}
#endif


////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyVMTKUtils();
#elif PYTHON_MAJOR_VERSION == 3
PyMODINIT_FUNC PyInit_pyVMTKUtils();
#endif

//----------------
// Vmtkutils_Init
//----------------
//
int Vmtkutils_pyInit()
{
#if PYTHON_MAJOR_VERSION == 2
  initpyVMTKUtils();
#elif PYTHON_MAJOR_VERSION == 3
  PyInit_pyVMTKUtils();
#endif
  return SV_OK;
}

//-------------------
// VMTKUtils_methods
//-------------------
//
PyMethodDef VMTKUtils_methods[]=
{
#ifdef SV_USE_VMTK

  { "centerlines", Geom_centerlines, METH_VARARGS, Geom_centerlines_doc},

  { "Grouppolydata", Geom_GroupPolyDataCmd, METH_VARARGS,NULL},

  { "Distancetocenterlines", Geom_DistanceToCenterlinesCmd, METH_VARARGS,NULL},

  { "Separatecenterlines", Geom_SeparateCenterlinesCmd, METH_VARARGS,NULL},

  { "Mergecenterlines", Geom_MergeCenterlinesCmd, METH_VARARGS,NULL},

  { "Cap", Geom_CapCmd, METH_VARARGS,NULL},

  { "Cap_with_ids", Geom_CapWIdsCmd, METH_VARARGS,NULL},

  { "Mapandcorrectids", Geom_MapAndCorrectIdsCmd, METH_VARARGS,NULL},

#endif

  {NULL,NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python interpreter 
// when the module is loaded.

static char* MODULE_NAME = "vmtk_utils";

PyDoc_STRVAR(VmtkUtils_doc, "vmtk_utils module functions");

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3

static struct PyModuleDef pyVMTKUtilsmodule = {
   PyModuleDef_HEAD_INIT,
   MODULE_NAME, 
   VmtkUtils_doc, 
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   VMTKUtils_methods
};

PyMODINIT_FUNC 
PyInit_pyVMTKUtils()
{
  auto module = PyModule_Create(&pyVMTKUtilsmodule);
  if (module == NULL) {
    fprintf(stdout,"Error initializing pyVMTKUtils.\n");
    return SV_PYTHON_ERROR;
  }

  PyRunTimeErr = PyErr_NewException("pyVMTKUtils.error",NULL,NULL);
  PyModule_AddObject(module,"error",PyRunTimeErr);

  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

//------------------
//initpyVMTKUtils
//------------------
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyVMTKUtils()
{
  PyObject* pythonC;
  pythonC=Py_InitModule("pyVMTKUtils",VMTKUtils_methods);

  if (pythonC==NULL)
  {
    fprintf(stdout,"Error initializing pyVMTKUtils.\n");
    return;

  }
  PyRunTimeErr=PyErr_NewException("pyVMTKUtils.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  return;
}
#endif

