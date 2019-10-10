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

// The functions defined here implement the SV Python API 'solid' Module. 
//
// The module name is 'solid'. The module defines a 'SolidModel' class used
// to store solid modeling data. The 'SolidModel' class cannot be imported and must
// be used prefixed by the module name. For example
//
//     model = solid.SolidModel()
//
// A Python exception sv.solid.SolidModelException is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.solid.SolidModelException: 
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_solid_init_py.h"
#include "sv_SolidModel.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"
#include "sv_vtk_utils.h"
#include "sv_PolyData.h"
#include "sv_PolyDataSolid.h"
#include "sv_sys_geom.h"
#include "sv_PyUtils.h"

#include "sv_FactoryRegistrar.h"
#include <map>

// Needed for Windows.
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"
#include "Python.h"
#include <structmember.h>
#include "vtkPythonUtil.h"
#if PYTHON_MAJOR_VERSION == 3
#include "PyVTKObject.h"
#elif PYTHON_MAJOR_VERSION == 2
#include "PyVTKClass.h"
#endif

#include "sv_occt_init_py.h"
#include "sv_polydatasolid_init_py.h"

// Define a map between solid model kernel name and enum type.
static std::map<std::string,SolidModel_KernelT> kernelNameTypeMap =
{
    {"Discrete", SM_KT_DISCRETE},
    {"MeshSimSolid", SM_KT_MESHSIMSOLID},
    {"OpenCASCADE", SM_KT_OCCT},
    {"Parasolid", SM_KT_PARASOLID},
    {"PolyData", SM_KT_POLYDATA}
};

typedef struct
{
  PyObject_HEAD
  cvSolidModel* geom;
}pySolidModel;

static void pySolidModel_dealloc(pySolidModel* self)
{
  Py_XDECREF(self->geom);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

// createSolidModelType() references pySolidModelType and is used before 
// it is defined so we need to define its prototype here.
pySolidModel * createSolidModelType();

//---------------
// CheckGeometry
//---------------
// Check if the solid model object has geometry.
//
// This is really used to set the error message 
// in a single place. 
//
static cvSolidModel *
CheckGeometry(SvPyUtilApiFunction& api, pySolidModel *self)
{
  auto geom = self->geom;
  if (geom == NULL) {
      api.error("The solid model object does not have geometry.");
      return nullptr;
  }

  return geom;
}


//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//-------------------------
// Solid_RegistrarsListCmd
//-------------------------
// This routine is used for debugging the registrar/factory system.
//
PyDoc_STRVAR(Solid_list_registrars_doc,
" list_registrars()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
Solid_list_registrars(PyObject* self, PyObject* args)
{
  cvFactoryRegistrar* pySolidModelRegistrar =(cvFactoryRegistrar *) PySys_GetObject("solidModelRegistrar");
  char result[255];
  PyObject* pyList=PyList_New(6);
  sprintf( result, "Solid model registrar ptr -> %p\n", pySolidModelRegistrar );
  PyList_SetItem(pyList,0,PyString_FromFormat(result));
  for (int i = 0; i < CV_MAX_FACTORY_METHOD_PTRS; i++) {
    sprintf( result, "GetFactoryMethodPtr(%i) = %p\n",
      i, (pySolidModelRegistrar->GetFactoryMethodPtr(i)));
    PyList_SetItem(pyList,i+1,PyString_FromFormat(result));
  }
  return pyList;
}

// -----------------
// Solid_GetModelCmd
// -----------------

PyObject* Solid_GetModelCmd( pySolidModel* self, PyObject* args)
{
  char *objName=NULL;
  RepositoryDataT type;
  cvRepositoryData *rd;
  cvSolidModel *geom;

  if (!PyArg_ParseTuple(args,"s", &objName))
  {
    PyErr_SetString(PyRunTimeErr, "Could not import 1 char: objName");
    
  }

  // Do work of command:

  // Retrieve source object:
  rd = gRepository->GetObject( objName );
  char r[2048];
  if ( rd == NULL )
  {
    r[0] = '\0';
    sprintf(r, "couldn't find object %s", objName);
    PyErr_SetString(PyRunTimeErr,r);
    
  }

  type = rd->GetType();

  if ( type != SOLID_MODEL_T )
  {
    r[0] = '\0';
    sprintf(r, "%s not a model object", objName);
    PyErr_SetString(PyRunTimeErr,r);
    
  }
  
  geom = dynamic_cast<cvSolidModel*> (rd);
  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK; 
  
}

// ----------------
// Solid_PolyPtsCmd
// ----------------
//
PyDoc_STRVAR(Solid_polygon_points_doc,
  "polygon_points(kernel)  \n\ 
   \n\
   Set the computational kernel used to segment image data.       \n\
   \n\
   Args:\n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Solid_polygon_points(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);

  char *srcName;
  char *dstName;
  cvRepositoryData *pd;
  RepositoryDataT type;
  cvSolidModel *geom;

  if (!PyArg_ParseTuple(args,"ss",&srcName,&dstName)) {
    PyErr_SetString(PyRunTimeErr,"Could not import two chars.");
    
  }
  // Do work of command:

  // Make sure the specified dst object does not exist:
  if ( gRepository->Exists( dstName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Retrieve cvPolyData source:
  pd = gRepository->GetObject( srcName );
  if ( pd == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object " );
    
  }
  type = pd->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvPolyData" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr,"geom is NULL");
  }

  // Create the polygon solid:
  if ( geom->MakePoly2dPts( (cvPolyData *)pd ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "polygon solid creation error" );
    
  }

  // Register the solid:
  if ( !( gRepository->Register( dstName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
  }

  // Make a new Tcl command:
  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);

  return SV_PYTHON_OK;
}


// -------------
// Solid_PolyCmd
// -------------

PyObject* Solid_PolyCmd( pySolidModel* self, PyObject* args)
{
  char *srcName;
  char *dstName;
  cvRepositoryData *pd;
  RepositoryDataT type;
  cvSolidModel *geom;
  if(!PyArg_ParseTuple(args,"ss",&srcName,&dstName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two chars.");
    
  }

  // Do work of command:

  // Make sure the specified dst object does not exist:
  if ( gRepository->Exists( dstName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  // Retrieve cvPolyData source:
  pd = gRepository->GetObject( srcName );
  if ( pd == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  type = pd->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvPolyData" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  // Create the polygon solid:
  if ( geom->MakePoly2d( (cvPolyData *)pd ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "polygon solid creation error" );
    
  }

  // Register the solid:
  if ( !( gRepository->Register( dstName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
  }

  // Make a new Tcl command:

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// ---------------
// Solid_CircleCmd
// ---------------

// % Circle -result /some/obj/name -r <radius> -x <x_ctr> -y <y_ctr>
PyObject* Solid_CircleCmd(pySolidModel* self, PyObject* args)
{
  char *objName;
  double radius;
  double ctr[2];
  cvSolidModel* geom;
  if(!PyArg_ParseTuple(args,"sddd",&objName,&radius,&(ctr[0]),&(ctr[1])))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char and three doubles");
    
  }
  // Do work of command:

  if ( radius <= 0.0 ) {
    PyErr_SetString(PyRunTimeErr,"radius must be positive");
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr,"object already exists" );
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeCircle( radius, ctr ) != SV_OK ) {
     PyErr_SetString(PyRunTimeErr, "circle solid creation error");
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
  }
  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);

  return SV_PYTHON_OK;
}

// ---------------
// Solid_SphereCmd
// ---------------
PyObject* Solid_SphereCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  PyObject* ctrList;
  double ctr[3];
  double r;
  cvSolidModel* geom;
if(!PyArg_ParseTuple(args,"sdO",&objName,&r,&ctrList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char, one double and one list");
    
  }

  if (PyList_Size(ctrList)!=3)
  {
    PyErr_SetString(PyRunTimeErr,"sphere requires a 3D center coordinate");
    
  }
  for(int i=0;i<PyList_Size(ctrList);i++)
  {
    ctr[i]=PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }
  // Do work of command:

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }
  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }
  if ( geom->MakeSphere( r, ctr ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "sphere solid creation error");
    delete geom;
    
  }
  // Register the new solid:
  if ( !( gRepository->Register( objName, geom) ) ) {
     PyErr_SetString(PyRunTimeErr,"error registering obj in repository" );
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  //self->name=objName;
  //std::cout<<"self "<<self->geom<<"geom "<<geom<<std::endl;
 // cvRepositoryData* geom2=gRepository->GetObject(self->name);
  //cvPolyData* PD2=(self->geom)->GetPolyData(0,1.0);
 // return Py_BuildValue("s",geom->GetName());
  return SV_PYTHON_OK;
}
// ----------------
// Solid_EllipseCmd
// ----------------

PyObject* Solid_EllipseCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  double xr, yr;
  double ctr[2];
  cvSolidModel *geom;

  if(!PyArg_ParseTuple(args,"sdddd",&objName,&xr,&yr,&(ctr[0]),&(ctr[1])))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char and four doubles.");
    
  }

  // Do work of command:

  if ( ( xr <= 0.0 ) || ( yr <= 0.0 ) ) {
    PyErr_SetString(PyRunTimeErr, "radii must be positive");
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeEllipse( xr, yr, ctr ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "ellipse solid creation error" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// --------------
// Solid_Box2dCmd
// --------------

// % Box2d -result /some/obj/name -h <double> -w <double> \
//       -xctr <double> -yctr <double>

PyObject* Solid_Box2dCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  double boxDims[2];
  double ctr[2];
  cvSolidModel *geom;
  if(!PyArg_ParseTuple(args,"sdddd",&objName,&(boxDims[0]),&(boxDims[1])
                       ,&(ctr[0]),&(ctr[1])))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char and four doubles.");
    
  }
  // Do work of command:

  if ( ( boxDims[0] <= 0.0 ) || ( boxDims[1] <= 0.0 ) ) {
    PyErr_SetString(PyRunTimeErr, "height and width must be positive");
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeBox2d( boxDims, ctr ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "box solid creation error");
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// --------------
// Solid_Box3dCmd
// --------------

PyObject* Solid_Box3dCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  double dims[3];
  double ctr[3];
  PyObject* dimList;
  PyObject* ctrList;
  cvSolidModel *geom;
  if(!PyArg_ParseTuple(args,"sOO",&objName,&dimList,&ctrList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char and two doubles.");
    
  }

  if (PyList_Size(dimList)>3||PyList_Size(ctrList)>3)
  {
     PyErr_SetString(PyRunTimeErr,"error in list dimension");
     
  }
  for (int i=0;i<PyList_Size(dimList);i++)
  {
    dims[i]=PyFloat_AsDouble(PyList_GetItem(dimList,i));
  }
  for (int i=0;i<PyList_Size(ctrList);i++)
  {
    ctr[i]=PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }
  // Do work of command:

  if ( ( dims[0] <= 0.0 ) || ( dims[1] <= 0.0 ) || ( dims[2] <= 0.0 ) ) {
    PyErr_SetString(PyRunTimeErr, "all dims must be positive" );
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exist" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeBox3d( dims, ctr ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "box solid creation error" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// ------------------
// Solid_EllipsoidCmd
// ------------------

PyObject* Solid_EllipsoidCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  double ctr[3];
  double r[3];
  PyObject* rList;
  PyObject* ctrList;
  cvSolidModel *geom;
  if(!PyArg_ParseTuple(args,"sOO",&objName,&rList,&ctrList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char and two doubles.");
    
  }
  // Do work of command:

  if (PyList_Size(ctrList)!=3)
  {
     PyErr_SetString(PyRunTimeErr,"ellipsoid requires a 3D center coordinate");
     
  }
  if (PyList_Size(rList)!=3)
  {
     PyErr_SetString(PyRunTimeErr,"ellipsoid requires a 3D radius vector.");
    
  }

  for (int i=0;i<3;i++)
  {
    r[i]=PyFloat_AsDouble(PyList_GetItem(rList,i));
    ctr[i]=PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }
  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeEllipsoid( r, ctr ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "sphere solid creation error" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// -----------------
// Solid_CylinderCmd
// -----------------

PyObject* Solid_CylinderCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  double ctr[3];
  double axis[3];
  double r, l;
  int nctr, naxis=0;
  PyObject* ctrList;
  PyObject* axisList;
  cvSolidModel *geom;

  if(!PyArg_ParseTuple(args,"sddOO",&objName,&r,&l,&ctrList,&axisList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char, two doubles and two lists");
    
  }
  // Do work of command:

  if (PyList_Size(ctrList)!=3)
  {
     PyErr_SetString(PyRunTimeErr,"cylinder requires a 3D center coordinate");
     
  }
  if (PyList_Size(axisList)!=3)
  {
     PyErr_SetString(PyRunTimeErr,"ellipsoid requires a 3D axis vector.");
    
  }

  for (int i=0;i<3;i++)
  {
    axis[i]=PyFloat_AsDouble(PyList_GetItem(axisList,i));
    ctr[i]=PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }
  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeCylinder( r, l, ctr, axis ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "cylinder solid creation error" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// ----------------------
// Solid_TruncatedConeCmd
// ----------------------

PyObject* Solid_TruncatedConeCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  double pt[3];
  double dir[3];
  double r1, r2;
  PyObject* ptList;
  PyObject* dirList;
  cvSolidModel *geom;

  if(!PyArg_ParseTuple(args,"sddOO",&objName,&r1,&r2,&ptList,&dirList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char,two doubles and two lists.");
    
  }
  // Do work of command:

  if (PyList_Size(ptList)!=3)
  {
     PyErr_SetString(PyRunTimeErr,"truncatedCone requires a 3D coordinate");
     
  }
  if (PyList_Size(dirList)!=3)
  {
     PyErr_SetString(PyRunTimeErr,"truncatedCone requires a 3D direction vector.");
    
  }

  for (int i=0;i<3;i++)
  {
    pt[i]=PyFloat_AsDouble(PyList_GetItem(ptList,i));
    dir[i]=PyFloat_AsDouble(PyList_GetItem(dirList,i));
  }
  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeTruncatedCone( pt, dir, r1, r2 ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "cylinder solid creation error" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    
  }
  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// --------------
// Solid_TorusCmd
// --------------

PyObject* Solid_TorusCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  double ctr[3];
  double axis[3];
  PyObject* ctrList;
  PyObject* axisList;
  double rmaj, rmin;
  int nctr, naxis=0;
  cvSolidModel *geom;
  if(!PyArg_ParseTuple(args,"sddOO",&objName,&rmaj,&rmin,&ctrList,&axisList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one char,two doubles and two lists.");
    
  }

  // Do work of command:
  if (PyList_Size(ctrList)!=3)
  {
     PyErr_SetString(PyRunTimeErr,"torus requires a 3D center coordinate");
     
  }
  if (PyList_Size(axisList)!=3)
  {
     PyErr_SetString(PyRunTimeErr,"ellipsoid requires a 3D axis vector.");
    
  }

  for (int i=0;i<3;i++)
  {
    axis[i]=PyFloat_AsDouble(PyList_GetItem(axisList,i));
    ctr[i]=PyFloat_AsDouble(PyList_GetItem(ctrList,i));
  }
  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeTorus( rmaj, rmin, ctr, axis ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "torus solid creation error" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// --------------------
// Solid_Poly3dSolidCmd
// --------------------

PyObject* Solid_Poly3dSolidCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  char *srcName;
  char *facetMethodName;
  char *facetStr;
  SolidModel_FacetT facetMethod;
  cvRepositoryData *pd;
  RepositoryDataT type;
  cvSolidModel *geom;
  double angle = 0.0;

  if(!PyArg_ParseTuple(args,"sssd",&objName,&srcName,&facetMethodName,&angle))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import three char and one double.");
    
  }

  facetMethod = SolidModel_FacetT_StrToEnum( facetMethodName );
  if ( facetMethod == SM_Facet_Invalid ) {
    facetStr = SolidModel_FacetT_EnumToStr( SM_Facet_Invalid );
    PyErr_SetString(PyRunTimeErr, facetStr);
    
  }

  // Do work of command:

  // Retrieve cvPolyData source:
  pd = gRepository->GetObject( srcName );
  if ( pd == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object " );
    
  }
  type = pd->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvPolyData" );
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->SetPoly3dFacetMethod( facetMethod ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error selecting facet method ");
    delete geom;
    
  }
  if ( geom->MakePoly3dSolid( (cvPolyData*)pd , angle ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "polygonal solid creation error" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// ----------------------
// Solid_Poly3dSurfaceCmd
// ----------------------

PyObject* Solid_Poly3dSurfaceCmd( pySolidModel* self, PyObject* args)
{
  char *objName;
  char *srcName;
  char *facetMethodName;
  char *facetStr;
  SolidModel_FacetT facetMethod;
  cvRepositoryData *pd;
  RepositoryDataT type;
  cvSolidModel *geom;

  if(!PyArg_ParseTuple(args,"sss",&objName,&srcName,&facetMethodName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import three chars.");
    
  }

  facetMethod = SolidModel_FacetT_StrToEnum( facetMethodName );
  if ( facetMethod == SM_Facet_Invalid ) {
    facetStr = SolidModel_FacetT_EnumToStr( SM_Facet_Invalid );
    PyErr_SetString(PyRunTimeErr, facetStr );
    
  }

  // Do work of command:

  // Retrieve cvPolyData source:
  pd = gRepository->GetObject( srcName );
  if ( pd == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  type = pd->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvPolyData");
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object  already exists" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->SetPoly3dFacetMethod( facetMethod ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error selecting facet method " );
    delete geom;
    
  }
  if ( geom->MakePoly3dSurface( (cvPolyData*)pd ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "solid polygonal surface creation error");
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( objName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
  }


  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// -----------------
// Solid_ExtrudeZCmd
// -----------------

PyObject* Solid_ExtrudeZCmd( pySolidModel* self, PyObject* args)
{
  char *srcName;
  char *dstName;
  double dist;
  cvRepositoryData *src;
  RepositoryDataT type;
  cvSolidModel *geom;
  if(!PyArg_ParseTuple(args,"ssd",&srcName,&dstName,&dist))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two chars and one double.");
    
  }
  // Do work of command:

  // Retrieve cvSolidModel source:
  src = gRepository->GetObject( srcName );
  if ( src == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  type = src->GetType();
  if ( type != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvSolidModel" );
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( dstName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->ExtrudeZ( (cvSolidModel *)src, dist ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error in solid extrusion" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( dstName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// ----------------
// Solid_ExtrudeCmd
// ----------------

PyObject* Solid_ExtrudeCmd( pySolidModel* self, PyObject* args)
{
  char *srcName;
  char *dstName;
  double pt1[3],pt2[3];
  PyObject* pt1List;
  PyObject* pt2List;
  double **dist;
  cvRepositoryData *src;
  RepositoryDataT type;
  cvSolidModel *geom;

  if(!PyArg_ParseTuple(args,"ssOO",&srcName,&dstName,&pt1List,&pt2List))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two strings and two lists");
    
  }

  // Do work of command:

  if (PyList_Size(pt1List)>3||PyList_Size(pt2List)>3)
  {
     PyErr_SetString(PyRunTimeErr,"error in list dimension ");
     
  }

  for (int i=0;i<PyList_Size(pt1List);i++)
  {
    pt1[i]=PyFloat_AsDouble(PyList_GetItem(pt1List,i));
  }
  for (int i=0;i<PyList_Size(pt2List);i++)
  {
    pt2[i]=PyFloat_AsDouble(PyList_GetItem(pt2List,i));
  }
  // Retrieve cvSolidModel source:
  src = gRepository->GetObject( srcName );
  if ( src == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  type = src->GetType();
  if ( type != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvSolidModel" );
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( dstName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  dist = new double*[2];
  dist[0] = &pt1[0];
  dist[1] = &pt2[0];

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    delete dist;
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->Extrude( (cvSolidModel *)src, dist ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error in solid extrusion" );
    delete geom;
    delete dist;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( dstName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    delete dist;
    
  }

  delete dist;
  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// ----------------------------
// Solid_MakeApproxCurveLoopCmd
// ----------------------------

PyObject* Solid_MakeApproxCurveLoopCmd( pySolidModel* self, PyObject* args)
{
  char *srcName;
  char *dstName;
  double tol;
  cvRepositoryData *src;
  RepositoryDataT type;
  cvSolidModel *geom;
  int closed = 1;

  if(!PyArg_ParseTuple(args,"ssdi",&srcName,&dstName,&tol,&closed))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two chars, one double and one int.");
    
  }

  // Do work of command:

  // Retrieve cvPolyData source:
  src = gRepository->GetObject( srcName );
  if ( src == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  type = src->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvPolyData" );
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( dstName ) ) {
    PyErr_SetString(PyRunTimeErr, "object  already exists");
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeApproxCurveLoop( (cvPolyData *)src, tol, closed ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error in curve loop construction" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( dstName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// ----------------------------
// Solid_MakeInterpCurveLoopCmd
// ----------------------------

PyObject* Solid_MakeInterpCurveLoopCmd( pySolidModel* self, PyObject* args)
{
  char *srcName;
  char *dstName;

  cvRepositoryData *src;
  RepositoryDataT type;
  cvSolidModel *geom;
  int closed = 1;

  if(!PyArg_ParseTuple(args,"ss|i",&srcName,&dstName,&closed))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two chars and one int");
    
  }
  // Do work of command:

  // Retrieve cvPolyData source:
  src = gRepository->GetObject( srcName );
  if ( src == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  type = src->GetType();
  if ( type != POLY_DATA_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvPolyData");
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( dstName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->MakeInterpCurveLoop( (cvPolyData *)src, closed ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error in curve loop construction" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( dstName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// -----------------------
// Solid_MakeLoftedSurfCmd
// -----------------------

PyObject* Solid_MakeLoftedSurfCmd( pySolidModel* self, PyObject* args)
{
  char *dstName;
  cvRepositoryData *src;
  RepositoryDataT type;
  int numSrcs;
  PyObject* srcList;
  cvSolidModel **srcs;
  cvSolidModel *geom;
  int continuity=0;
  int partype=0;
  int smoothing=0;
  double w1=0.4,w2=0.2,w3=0.4;
  int i;

  if(!PyArg_ParseTuple(args,"Os|iidddi",&srcList,&dstName,&continuity,&partype,&w1,&w2,&w3,&smoothing))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one list, one string or optional three ints and three doubles.");
    
  }
  // Do work of command:

  numSrcs = PyList_Size(srcList);

  if ( numSrcs < 2 ) {
    PyErr_SetString(PyRunTimeErr, "need >= 2 curve cvSolidModel's to loft surface");
    
  }

  // Foreach src obj, check that it is in the repository and of the
  // correct type (i.e. cvSolidModel).  Also build up the array of
  // cvSolidModel*'s to pass to cvSolidModel::MakeLoftedSurf.

  srcs = new cvSolidModel * [numSrcs];

  for ( i = 0; i < numSrcs; i++ ) {
    src = gRepository->GetObject( PyString_AsString(PyList_GetItem(srcList,i)) );
    if ( src == NULL ) {
      PyErr_SetString(PyRunTimeErr,"Couldn't find object ");
      delete [] srcs;
      
    }
    type = src->GetType();
    if ( type != SOLID_MODEL_T ) {
	    PyErr_SetString(PyRunTimeErr,"src not of type cvSolidModel");
      delete [] srcs;
      
    }
    srcs[i] = (cvSolidModel *) src;
  }

  // We're done with the src object names:

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( dstName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    delete [] srcs;
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    delete [] srcs;
    
  }

  if ( geom->MakeLoftedSurf( srcs, numSrcs , dstName,
	continuity,partype,w1,w2,w3,smoothing) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error in curve loop construction" );
    delete [] srcs;
    delete geom;
    
  }

  // We're done with the srcs array:
  delete [] srcs;

  // Register the new solid:
  if ( !( gRepository->Register( dstName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// -----------------------
// Solid_CapSurfToSolidCmd
// -----------------------

PyObject* Solid_CapSurfToSolidCmd( pySolidModel* self, PyObject* args)
{
  char *srcName;
  char *dstName;
  cvRepositoryData *src;
  RepositoryDataT type;
  cvSolidModel *geom;

  if(!PyArg_ParseTuple(args,"ss",&srcName,&dstName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two chars");
    
  }

  // Do work of command:

  src = gRepository->GetObject( srcName );
  if ( src == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }

  type = src->GetType();
  if ( type != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvSolidModel" );
    
  }

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( dstName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->CapSurfToSolid( (cvSolidModel *)src ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "error in cap / bound operation" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( dstName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
  }


  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// -------------------
// Solid_ReadNativeCmd
// -------------------

PyObject* Solid_ReadNativeCmd( pySolidModel* self, PyObject* args)
{
  char *objName, *fileName;

  if(!PyArg_ParseTuple(args,"ss",&objName,&fileName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two chars");
    
  }
  // Do work of command:

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists" );
    
  }

  // Instantiate the new solid:
  cvSolidModel *geom;
  if (cvSolidModel::gCurrentKernel == SM_KT_PARASOLID ||
      cvSolidModel::gCurrentKernel == SM_KT_DISCRETE ||
      cvSolidModel::gCurrentKernel == SM_KT_POLYDATA ||
      cvSolidModel::gCurrentKernel == SM_KT_OCCT ||
      cvSolidModel::gCurrentKernel == SM_KT_MESHSIMSOLID) {

	  geom = cvSolidModel::pyDefaultInstantiateSolidModel();

	  if ( geom == NULL ) {
	   PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
	    
	  }

	  if ( geom->ReadNative( fileName ) != SV_OK ) {
	    PyErr_SetString(PyRunTimeErr, "file read error" );
	    delete geom;
	    
	  }

	  // Register the new solid:
	  if ( !( gRepository->Register( objName, geom ) ) ) {
	    PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
	    delete geom;
	    
	  }

  }

  else {
    fprintf( stdout, "current kernel is not valid (%i)\n",cvSolidModel::gCurrentKernel);
    PyErr_SetString(PyRunTimeErr, "current kernel is not valid" );}
  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// -------------
// Solid_CopyCmd
// -------------

PyObject* Solid_CopyCmd( pySolidModel* self, PyObject* args)
{
  char *srcName;
  char *dstName;
  cvRepositoryData *srcGeom;
  RepositoryDataT src_t;
  cvSolidModel *dstGeom;

  if(!PyArg_ParseTuple(args,"ss",&srcName,&dstName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two chars");
    
  }

  // Do work of command:

  // Retrieve source:
  srcGeom = gRepository->GetObject( srcName );
  if ( srcGeom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  src_t = gRepository->GetType( srcName );
  if ( src_t != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvSolidModel" );
    
  }

  // Make sure the specified destination object does not exist:
  if ( gRepository->Exists( dstName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  // Instantiate the new solid:
  dstGeom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( dstGeom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }
  if ( dstGeom->Copy( *((cvSolidModel *)srcGeom) ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "cvSolidModel copy error" );
    delete dstGeom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( dstName, dstGeom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete dstGeom;
    
  }

  Py_INCREF(dstGeom);
  self->geom=dstGeom;
  Py_DECREF(dstGeom);
  return SV_PYTHON_OK;
}


// ------------------
// Solid_IntersectCmd
// ------------------

PyObject* Solid_IntersectCmd( pySolidModel* self, PyObject* args)
{
  char *resultName;
  char *smpName=NULL;
  char *smpStr;
  char *aName;
  char *bName;
  SolidModel_SimplifyT smp = SM_Simplify_All;  // DEFAULT ARG VALUE
  RepositoryDataT aType, bType;
  cvRepositoryData *gmA;
  cvRepositoryData *gmB;
  cvSolidModel *geom;
  if(!PyArg_ParseTuple(args,"sss|s",&resultName,&aName,&bName,&smpName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import three chars or one optional char smpName");
    
  }


  // Parse the simplification flag if given:
  if (smpName ) {
    smp = SolidModel_SimplifyT_StrToEnum( smpName );
    if ( smp == SM_Simplify_Invalid ) {
      smpStr = SolidModel_SimplifyT_EnumToStr( SM_Simplify_Invalid );
      PyErr_SetString(PyRunTimeErr, smpStr );
      
    }
  }

  // Do work of command:

  // Retrieve cvSolidModel operands:
  gmA = gRepository->GetObject( aName );
  if ( gmA == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  aType = gRepository->GetType( aName );
  if ( aType != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvSolidModel");
    
  }

  gmB = gRepository->GetObject( bName );
  if ( gmB == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  bType = gRepository->GetType( bName );
  if ( bType != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object  not of type cvSolidModel" );
    
  }

  // Instantiate the new solid:
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

  if ( geom->Intersect( (cvSolidModel*)gmA, (cvSolidModel*)gmB, smp ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "intersection error" );
    delete geom;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( resultName, geom ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// --------------
// Solid_UnionCmd
// --------------

PyObject* Solid_UnionCmd( pySolidModel* self, PyObject* args)
{
  char *resultName;
  char *smpName=NULL;
  char *smpStr;
  char *aName;
  char *bName;
  SolidModel_SimplifyT smp = SM_Simplify_All;  // DEFAULT ARG VALUE
  RepositoryDataT aType, bType;
  cvRepositoryData *gmA;
  cvRepositoryData *gmB;
  cvSolidModel *result;

  if(!PyArg_ParseTuple(args,"sss|s",&resultName,&aName,&bName,&smpName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import three chars or one optional char smpName");
    
  }

  // Parse the simplification flag if given:
  if (smpName) {
    smp = SolidModel_SimplifyT_StrToEnum( smpName );
    if ( smp == SM_Simplify_Invalid ) {
      smpStr = SolidModel_SimplifyT_EnumToStr( SM_Simplify_Invalid );
      PyErr_SetString(PyRunTimeErr, smpStr );
      
    }
  }

  // Do work of command:
  // Retrieve cvSolidModel operands:
  gmA = gRepository->GetObject( aName );
  if ( gmA == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  aType = gRepository->GetType( aName );
  if ( aType != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvSolidModel");
    
  }

  gmB = gRepository->GetObject( bName );
  if ( gmB == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  bType = gRepository->GetType( bName );
  if ( bType != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvSolidModel");
    
  }

  // Instantiate the new solid:
  result = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( result == NULL ) {
    PyErr_SetString(PyRunTimeErr,"result is NULL");
  }
  if ( result->Union( (cvSolidModel*)gmA, (cvSolidModel*)gmB, smp ) != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "union error" );
    delete result;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( resultName, result ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete result;
    
  }

  Py_INCREF(result);
  self->geom=result;
  Py_DECREF(result);
  return SV_PYTHON_OK;
}


// -----------------
// Solid_SubtractCmd
// -----------------

PyObject* Solid_SubtractCmd( pySolidModel* self, PyObject* args)
{
  char *resultName;
  char *smpName=NULL;
  char *smpStr;
  char *aName;
  char *bName;
  SolidModel_SimplifyT smp = SM_Simplify_All;  // DEFAULT ARG VALUE
  RepositoryDataT aType, bType;
  cvRepositoryData *gmA;
  cvRepositoryData *gmB;
  cvSolidModel *result;

  if(!PyArg_ParseTuple(args,"sss|s",&resultName,&aName,&bName,&smpName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import three chars or one optional char smpName");
    
  }

  // Parse the simplification flag if given:
  if (smpName) {
    smp = SolidModel_SimplifyT_StrToEnum( smpName );
    if ( smp == SM_Simplify_Invalid ) {
      smpStr = SolidModel_SimplifyT_EnumToStr( SM_Simplify_Invalid );
      PyErr_SetString(PyRunTimeErr, smpStr );
      
    }
  }

  // Do work of command:

  // Retrieve cvSolidModel operands:
  gmA = gRepository->GetObject( aName );
  if ( gmA == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  aType = gRepository->GetType( aName );
  if ( aType != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvSolidModel");
    
  }

  gmB = gRepository->GetObject( bName );
  if ( gmB == NULL ) {
    PyErr_SetString(PyRunTimeErr, "couldn't find object ");
    
  }
  bType = gRepository->GetType( bName );
  if ( bType != SOLID_MODEL_T ) {
    PyErr_SetString(PyRunTimeErr, "object not of type cvSolidModel" );
    
  }

  // Instantiate the new solid:
  result = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( result == NULL ) {
    PyErr_SetString(PyRunTimeErr,"geom is NULL");
  }

  if ( result->Subtract( (cvSolidModel*)gmA, (cvSolidModel*)gmB, smp )
       != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "subtract error" );
    delete result;
    
  }

  // Register the new solid:
  if ( !( gRepository->Register( resultName, result ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete result;
    
  }

  Py_INCREF(result);
  self->geom=result;
  Py_DECREF(result);
  return SV_PYTHON_OK;
}

// ---------------
// Solid_ObjectCmd
// ---------------
PyObject* Solid_ObjectCmd(pySolidModel* self,PyObject* args )
{
  if (PyTuple_Size(args)== 0 ) {
    //PrintMethods();
  }
  return SV_PYTHON_OK;
}

// -----------
// DeleteSolid
// -----------
// This is the deletion call-back for cvSolidModel object commands.

PyObject* DeleteSolid( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = self->geom;

  gRepository->UnRegister( geom->GetName() );
  Py_INCREF(Py_None);
  return Py_None;
}

//------------------
//Solid_NewObjectCmd
//------------------
PyObject* Solid_NewObjectCmd(pySolidModel* self,PyObject *args )
{
  char *objName, *fileName;
  cvSolidModel* geom;
  if(!PyArg_ParseTuple(args,"s",&objName))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import char objName");
    
  }
  // Do work of command:
  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( objName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exist");
    
  }
  // Instantiate the new solid:

  if (cvSolidModel::gCurrentKernel == SM_KT_PARASOLID ||
      cvSolidModel::gCurrentKernel == SM_KT_DISCRETE ||
      cvSolidModel::gCurrentKernel == SM_KT_POLYDATA ||
      cvSolidModel::gCurrentKernel == SM_KT_OCCT ||
      cvSolidModel::gCurrentKernel == SM_KT_MESHSIMSOLID) {
  geom = cvSolidModel::pyDefaultInstantiateSolidModel();
  if ( geom == NULL ) {
    PyErr_SetString(PyRunTimeErr, "Error creating solid object" );
    
  }

   // Register the new solid:
   if ( !( gRepository->Register( objName, geom ) ) )
   {
     PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    
   }
  }
  else {
    fprintf( stdout, "current kernel is not valid (%i)\n",cvSolidModel::gCurrentKernel);
    //PyErr_SetString(PyRunTimeErr, "current kernel is not valid", TCL_STATIC );
    PyErr_SetString(PyRunTimeErr,"kernel is invalid");
  }
  // Allocate new object in python
  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//------------------
// Solid_set_kernel  
//------------------
//
PyDoc_STRVAR(Solid_set_kernel_doc,
" set_kernel(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Solid_set_kernel(PyObject* self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *kernelArg;

  if (!PyArg_ParseTuple(args, api.format, &kernelArg)) {
      return api.argsError();
  }

  SolidModel_KernelT kernel;

  try {
        kernel = kernelNameTypeMap.at(std::string(kernelArg));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown solid modeling kernel '" + std::string(kernelArg) + "'." +
        " Valid solid modeling kernel names are: Discrete, MeshSimSolid, OpenCASCADE, Parasolid or PolyData.";
      api.error(msg);
      return nullptr;
  }

  cvSolidModel::gCurrentKernel = kernel;
  return Py_BuildValue("s", kernelArg);
}

//------------------
// Solid_get_kernel 
//------------------
//
PyDoc_STRVAR(Solid_get_kernel_doc,
" get_kernel()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
Solid_get_kernel(PyObject* self, PyObject* args)
{
  char *kernelName;
  kernelName = SolidModel_KernelT_EnumToStr(cvSolidModel::gCurrentKernel);
  return Py_BuildValue("s",kernelName);
}


/////////////////////////////////////////////////////////////////
//          M o d u l e  C l a s s  F u n c t i o n s          //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the SolidModel class. 

PyDoc_STRVAR(SolidModel_get_class_name_doc,
" SolidModel_get_class_name()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_class_name(pySolidModel* self, PyObject* args)
{
  return Py_BuildValue("s","SolidModel");
}

//------------------------
// SolidModel_find_extent
//------------------------
//
PyDoc_STRVAR(SolidModel_find_extent_doc,
" find_extent()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
SolidModel_find_extent(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto geom =self->geom;

  double extent;
  auto status = geom->FindExtent(&extent);

  if (status != SV_OK) {
      api.error("Error finding extent");
      return nullptr;
  }

  Py_BuildValue("d",extent);
  return SV_PYTHON_OK;
}

//---------------------------
// SolidModel_find_centroid
//---------------------------
PyDoc_STRVAR(SolidModel_find_centroid_doc,
" find_centroid()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *  
SolidModel_find_centroid(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  double centroid[3];
  int tdim;

  cvSolidModel *geom = self->geom;

  if (geom->GetSpatialDim(&tdim) != SV_OK) {
      api.error("Unable to get the spatial dimension of the solid model.");
      return nullptr;
  }

  if ((tdim != 2) && (tdim != 3)) {
      api.error("The spatial dimension " + std::to_string(tdim) + " is not supported.");
      return nullptr;
  }

  if (geom->FindCentroid(centroid) != SV_OK) {
      api.error("Error finding centroid of the solid model.");
      return nullptr;
  }

  PyObject* tmpPy = PyList_New(3);
  PyList_SetItem(tmpPy,0,PyFloat_FromDouble(centroid[0]));
  PyList_SetItem(tmpPy,1,PyFloat_FromDouble(centroid[1]));

  if (tdim == 3) {
      PyList_SetItem(tmpPy,2,PyFloat_FromDouble(centroid[2]));
  }

  return tmpPy;
}

//--------------------------------------
// SolidModel_get_topological_dimension
//--------------------------------------
//
PyDoc_STRVAR(SolidModel_get_topological_dimension_doc,
" get_topological_dimension()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_get_topological_dimension( pySolidModel *self ,PyObject* args  )
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto geom = self->geom;
  int tdim;

  if (geom->GetTopoDim(&tdim) != SV_OK) {
      api.error("Error getting the topological dimension of the solid model.");
      return nullptr;
  }

  return Py_BuildValue("d",tdim);
}

// ----------------------
// Solid_GetSpatialDimMtd
// ----------------------
PyDoc_STRVAR(SolidModel_get_spatial_dimension_doc,
" get_spatial_dimension()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *  
SolidModel_get_spatial_dimension(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto geom = self->geom;
  int sdim;

  if (geom->GetSpatialDim(&sdim) != SV_OK) {
      api.error("Error getting the spatial dimension of the solid model.");
      return nullptr;
  }

  return Py_BuildValue("d",sdim);
}

// -------------------
// Solid_ClassifyPtMtd
// -------------------
PyDoc_STRVAR(SolidModel_classify_point_doc,
" classify_point(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_classify_point(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("dd|di", PyRunTimeErr, __func__);
  double x, y;
  double z = std::numeric_limits<double>::infinity();
  int v = 0;
  int ans;
  int status;
  int tdim, sdim;

  if (!PyArg_ParseTuple(args, api.format,&x,&y,&z,&v)) {
      return api.argsError();
  }

  auto geom = self->geom;
  geom->GetTopoDim(&tdim);
  geom->GetSpatialDim(&sdim);

  if (!std::isinf(z)) {
      status = geom->ClassifyPt(x, y, z, v, &ans);
  } else {
      if ((tdim == 2) && (sdim == 2)) {
          status = geom->ClassifyPt( x, y, v, &ans);
      } else {
          api.error("the solid model must have a topological and spatial dimension of two.");
          return nullptr;
      }
  }

  if (status != SV_OK) {
      api.error("Error classifying a point for the solid model.");
      return nullptr;
  }

  return Py_BuildValue("d",ans);
}

// -----------------
// Solid_DistanceMtd
// -----------------
PyDoc_STRVAR(SolidModel_distance_doc,
" distance(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_distance(pySolidModel *self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Od", PyRunTimeErr, __func__);
  double pos[3];
  int npos;
  double upperLimit, dist;
  int sdim;
  int status;
  PyObject *posList;

  if (!PyArg_ParseTuple(args, api.format, &posList, &upperLimit)) {
      return api.argsError();
  }

  auto geom = self->geom;

  if (PyList_Size(posList) > 3) {
      api.error("The position argument is not between 1 and 3.");
      return nullptr;
  }

  npos = PyList_Size(posList);
  for (int i=0;i<npos;i++) {
      pos[i] = PyFloat_AsDouble(PyList_GetItem(posList,i));
  }

  // Check validity of given pos.
  if (geom->GetSpatialDim(&sdim) != SV_OK ) {
      api.error("Error getting the spatial dimension of the solid model.");
      return nullptr;
  }

  if ((sdim == 3) && (npos != 3)) {
      api.error("The postion argument is not a 3D point. A 3D solid model requires a 3D point.");
      return nullptr;
  } else if ((sdim == 2) && (npos != 2)) {
     PyErr_SetString(PyRunTimeErr,"objects in 2 spatial dims require a 2D position");
      api.error("The postion argument is not a 2D point. A 2D solid model requires a 2D point.");
      return nullptr;
  }

  status = geom->Distance( pos, upperLimit, &dist );

  if (geom->Distance( pos, upperLimit, &dist) != SV_OK) {
      api.error("Error computing the distance to the solid model.");
      return nullptr;
  }

  return Py_BuildValue("d",dist);
}

//----------------------
// SolidModel_translate 
//----------------------
//
PyDoc_STRVAR(SolidModel_translate_doc,
" translate(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_translate(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__);
  PyObject*  vecList;
  double vec[3];
  int nvec;
  int status;

  if (!PyArg_ParseTuple(args, api.format, &vecList)) {
      return api.argsError();
  }

  if (PyList_Size(vecList) > 3) {
      api.error("The translation vector argument is > 3.");
      return nullptr;
  }

  // [TODO:DaveP] do a better check here.
  nvec = PyList_Size(vecList);
  for(int i=0;i<nvec;i++) {
    vec[i] = PyFloat_AsDouble(PyList_GetItem(vecList,i));
  }

  auto geom = self->geom;

  if (geom->Translate(vec, nvec) != SV_OK) {
      api.error("Error translating the solid model.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}


// ---------------
// Solid_RotateMtd
// ---------------

PyDoc_STRVAR(SolidModel_rotate_doc,
" rotate(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_rotate( pySolidModel *self ,PyObject* args  )
{
  auto api = SvPyUtilApiFunction("Od", PyRunTimeErr, __func__);
  PyObject*  axisList;
  double rad;

  if (!PyArg_ParseTuple(args, api.format, &axisList, &rad)) {
      return api.argsError();
  }
  auto naxis = PyList_Size(axisList);
  if (naxis > 3) {
      api.error("The rotation axis argument is > 3.");
      return nullptr;
  }

  double axis[3];
  for(int i=0;i<naxis;i++) {
    axis[i]=PyFloat_AsDouble(PyList_GetItem(axisList,i));
  }

  auto geom = self->geom;

  if (geom->Rotate(axis, naxis, rad) != SV_OK) {
      api.error("Error rotating the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}


//------------------
// SolidModel_scale
//------------------
//
PyDoc_STRVAR(SolidModel_scale_doc,
" scale(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_scale(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__);
  double factor;

  if (!PyArg_ParseTuple(args, api.format, &factor)) {
      return api.argsError();
  }

  auto geom = self->geom;

  if (geom->Scale(factor) != SV_OK) {
      api.error("Error scaling the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------
// Solid_ReflectMtd
//----------------
//
PyDoc_STRVAR(SolidModel_reflect_doc,
" reflect(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_reflect(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("OO", PyRunTimeErr, __func__);
  PyObject* posList;
  PyObject* nrmList;
  int status;

  if (!PyArg_ParseTuple(args, api.format, &posList, &nrmList)) {
      return api.argsError();
  }

  // [TODO:DaveP] do more checking here.
  //   Can posList be < 3?
  //
  if (PyList_Size(posList) > 3) {
      api.error("The position argument is > 3.");
      return nullptr;
  }

  if (PyList_Size(nrmList) > 3) {
      api.error("The normal argument is > 3.");
      return nullptr;
  }

  double pos[3];
  for(int i=0;i<PyList_Size(posList);i++) {
    pos[i]=PyFloat_AsDouble(PyList_GetItem(posList,i));
  }

  double nrm[3];
  for(int i=0;i<PyList_Size(nrmList);i++) {
    nrm[i]=PyFloat_AsDouble(PyList_GetItem(nrmList,i));
  }

  auto geom = self->geom;

  if (geom->Reflect(pos, nrm) != SV_OK) {
      api.error("Error reflecting the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-----------------
// Solid_Apply4x4Mtd
//-----------------
//
PyDoc_STRVAR(SolidModel_apply4x4_doc,
" apply4x4(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_apply4x4(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__);
  PyObject* matList;
  PyObject* rowList;

  if (!PyArg_ParseTuple(args, api.format, &matList)) {
      return api.argsError();
  }

  if (PyList_Size(matList) != 4) {
      api.error("The matrix argument is not a 4x4 matrix.");
      return nullptr;
  }

  // Extract the 4x4 matrix.
  //
  double mat[4][4];
  for (int i=0;i<PyList_Size(matList);i++) {
      rowList = PyList_GetItem(matList,i);
      if (PyList_Size(rowList) != 4) {
          api.error("The matrix argument is not a 4x4 matrix.");
          return nullptr;
      }
      for (int j=0;j<PyList_Size(rowList);j++) {
          mat[i][j] = PyFloat_AsDouble(PyList_GetItem(rowList,j));
      }
  }

  auto geom = self->geom;
  if (geom->Apply4x4(mat) != SV_OK) {
      api.error("Error applyonig a 4x4 matrix to the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------------
// SolidModel_print
//------------------
//
PyDoc_STRVAR(SolidModel_print_doc,
" print(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *  
SolidModel_print(pySolidModel *self ,PyObject* args)
{
  auto geom = self->geom;
  geom->Print();
  return SV_PYTHON_OK;
}

// --------------
// Solid_CheckMtd
// --------------
PyDoc_STRVAR(SolidModel_check_doc,
" check()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *  
SolidModel_check(pySolidModel *self ,PyObject* args)
{
  auto geom = self->geom;
  int nerr;
  geom->Check( &nerr );
  return Py_BuildValue("i",nerr);
}

//-------------------------
// SolidModel_write_native
//-------------------------
//
PyDoc_STRVAR(SolidModel_write_native_doc,
" write_native(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_write_native(pySolidModel *self ,PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__);
  char *fn;
  int status;
  int file_version = 0;

  if (!PyArg_ParseTuple(args, api.format, &fn, &file_version)) {
      return api.argsError();
  }

  auto geom = self->geom;
  if (geom->WriteNative(file_version, fn) != SV_OK) {
      api.error("Error writing the solid model to the file '" + std::string(fn) + "' using version '" + std::to_string(file_version)+"'."); 
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//-------------------------------
// SolidModel_write_vtk_polydata
//-------------------------------
//
PyDoc_STRVAR(SolidModel_write_vtk_polydata_doc,
" write_vtk_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_write_vtk_polydata(pySolidModel *self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fn;

  if (!PyArg_ParseTuple(args, api.format, &fn)) {
      return api.argsError();
  }

  auto geom = self->geom;
  if (geom->WriteVtkPolyData(fn) != SV_OK) {
      api.error("Error writing the solid model to the file '" + std::string(fn) + "'.");
      return nullptr;
  }
    
  return SV_PYTHON_OK;
}

//---------------------------
// SolidModel_write_geom_sim
//---------------------------
//
PyDoc_STRVAR(SolidModel_write_geom_sim_doc,
" write_geom_sim(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_write_geom_sim(pySolidModel *self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *fn;

  if (!PyArg_ParseTuple(args, api.format, &fn)) {
      return api.argsError();
  }

  auto geom = self->geom;
  if (geom->WriteGeomSim(fn) != SV_OK) {
      api.error("Error writing the solid model to the file '" + std::string(fn) + "'.");
      return nullptr;
  } 

  return SV_PYTHON_OK;
}

//-------------------------
// SolidModel_get_polydata
//-------------------------
//
PyDoc_STRVAR(SolidModel_get_polydata_doc,
" get_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
SolidModel_get_polydata(pySolidModel *self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s|d", PyRunTimeErr, __func__);
  char *resultName;
  double max_dist = -1.0;

  if (!PyArg_ParseTuple(args, api.format, &resultName, &max_dist)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  int useMaxDist = 0;
  if (max_dist > 0) {
      useMaxDist = 1;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The repository object '" + std::string(resultName) + "' already exists.");
      return nullptr;
  }

  // Get the cvPolyData:
  auto pd = geom->GetPolyData(useMaxDist, max_dist);
  if (pd == NULL) {
      api.error("Could not get polydata for the solid model.");
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
      delete pd;
      api.error("Could not add the polydata to the repository.");
      return nullptr;
  }

  Py_INCREF(geom);
  self->geom=geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK;
}

//----------------------
// Solid_SetVtkPolyDataMtd
//----------------------
//
PyDoc_STRVAR(SolidModel_set_vtk_polydata_doc,
" set_vtk_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_set_vtk_polydata(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  auto obj = gRepository->GetObject(objName);
  if (obj == nullptr) {
      api.error("The solid model '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  // Do work of command:
  auto type = gRepository->GetType( objName );
  if (type != POLY_DATA_T) {
      api.error("The solid model '" + std::string(objName)+"' is not of type polydata.");
      return nullptr;
  }

  auto pd = ((cvPolyData *)obj)->GetVtkPolyData();
  if (!geom->SetVtkPolyDataObject(pd)) {
      api.error("Error setting vtk polydata.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}


//------------------------------
// SolidModel_get_face_polydata
//------------------------------
//
PyDoc_STRVAR(SolidModel_get_face_polydata_doc,
" set_vtk_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_face_polydata(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("si|d", PyRunTimeErr, __func__);
  char *resultName;
  int faceid;
  double max_dist = -1.0;

  if(!PyArg_ParseTuple(args,api.format,&resultName,&faceid,&max_dist)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  int useMaxDist = 0;
  if (max_dist > 0) {
      useMaxDist = 1;
  }

  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The solid model '" + std::string(resultName) + "' is already in the repository.");
      return nullptr;
  }

  // Get the cvPolyData:
  auto pd = geom->GetFacePolyData(faceid,useMaxDist,max_dist);
  if (pd == NULL) {
      api.error("Error getting polydata for the solid model face ID '" + std::to_string(faceid) + "'.");
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
    delete pd;
    api.error("Error adding the polydata '" + std::string(resultName) + "' to the repository.");
    return nullptr;
  }

  return SV_PYTHON_OK;
}

//----------------------------
// SolidModel_get_face_normal 
//---------------------------
//
PyDoc_STRVAR(SolidModel_get_face_normal_doc,
" get_face_normal(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_face_normal(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("idd", PyRunTimeErr, __func__);
  int faceid;
  double u,v;

  if (!PyArg_ParseTuple(args,api.format,&faceid,&u,&v)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  double normal[3];

  if (geom->GetFaceNormal(faceid,u,v,normal) == SV_ERROR ) {
      api.error("Error getting the face normal for the solid model face ID '" + std::to_string(faceid) + "'.");
      return nullptr;
  }

  return Py_BuildValue("ddd",normal[0],normal[1],normal[2]);
}


// ---------------------------
// Solid_GetDiscontinuitiesMtd
// ---------------------------

PyDoc_STRVAR(SolidModel_get_discontinuities_doc,
" get_discontinuities(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
SolidModel_get_discontinuities(pySolidModel* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *resultName;

  if(!PyArg_ParseTuple(args,api.format,&resultName)) {
      return api.argsError();
  }

  auto geom = CheckGeometry(api, self);
  if (geom == nullptr) {
      return nullptr;
  }

  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(resultName)) {
      api.error("The solid model '" + std::string(resultName) + "' is already in the repository.");
      return nullptr;
  }

  // Get the discontinuities as polydata.
  auto pd = geom->GetDiscontinuities();
  if (pd == NULL) {
      api.error("Error getting discontinuities for the solid model.");
      return nullptr;
  }

  // Register the result:
  if (!gRepository->Register(resultName, pd)) {
      delete pd;
      api.error("Error adding the discontinuities polydata '" + std::string(resultName) + "' to the repository.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}


// -----------------------------------
// Solid_GetAxialIsoparametricCurveMtd
// -----------------------------------

PyDoc_STRVAR(SolidModel_get_discontinuities_doc,
" get_discontinuities(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static pySolidModel * 
SolidModel_get_axial_isoparametric_curve(pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  char *resultName;
  double prm;
  cvSolidModel *curve;

  if(!PyArg_ParseTuple(args,"sd",&resultName,&prm))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one string and one double");
    
  }


  // Do work of command:

  // Make sure the specified result object does not exist:
  if ( gRepository->Exists( resultName ) ) {
    PyErr_SetString(PyRunTimeErr, "object already exists");
    
  }

  // Get the isoparametric curve on the given surface at the given
  // parameter value:
  if ( ( prm < 0.0 ) || ( prm > 1.0 ) ) {
    PyErr_SetString(PyRunTimeErr, "parameter value must be between 0.0 and 1.0");
    
  }
  curve = geom->GetAxialIsoparametricCurve( prm );
  if ( curve == NULL ) {
    PyErr_SetString(PyRunTimeErr, "error getting isoparametric curve for");
    
  }

  // Register the result:
  if ( !( gRepository->Register( resultName, curve ) ) ) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete curve;
    
  }

  PyErr_SetString(PyRunTimeErr, curve->GetName());

  Py_INCREF(curve);
  pySolidModel* newCurve;
  newCurve = createSolidModelType();
  //newCurve = PyObject_New(pySolidModel, &pySolidModelType);
  newCurve->geom=curve;
  Py_DECREF(curve);
  return newCurve;
}

//-------------------------
// SolidModel_GetKernel
//-------------------------
//
static PyObject * 
Solid_GetKernelMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  SolidModel_KernelT kernelType;
  char *kernelName;

  // Do work of command:
  kernelType = geom->GetKernelT();
  kernelName = SolidModel_KernelT_EnumToStr( kernelType );


  if ( kernelType == SM_KT_INVALID ) {
    PyErr_SetString(PyRunTimeErr, kernelName);
    
  } else {
    return Py_BuildValue("s",kernelName);
  }
}


// ---------------------
// Solid_GetLabelKeysMtd
// ---------------------

static PyObject* Solid_GetLabelKeysMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  int numKeys, i;
  char **keys;

  // Do work of command:
  geom->GetLabelKeys( &numKeys, &keys );
  PyObject* keyList=PyList_New(numKeys);
  for (i = 0; i < numKeys; i++) {
    PyList_SetItem(keyList, i, PyString_FromString(keys[i]));
  }
  delete [] keys;

  return keyList;
}


// -----------------
// Solid_GetLabelMtd
// -----------------

static PyObject* Solid_GetLabelMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  char *key, *value;

  if(!PyArg_ParseTuple(args,"s",&key))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one string");
    
  }


  // Do work of command:

  if ( ! geom->GetLabel( key, &value ) ) {
    PyErr_SetString(PyRunTimeErr, "key not found" );
    
  }
  return Py_BuildValue("s",value);
}


// -----------------
// Solid_SetLabelMtd
// -----------------

static PyObject* Solid_SetLabelMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  char *key, *value;

  if(!PyArg_ParseTuple(args,"ss",&key,&value))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two strings");
    
  }


  // Do work of command:

  if ( ! geom->SetLabel( key, value ) ) {
    if ( geom->IsLabelPresent( key ) ) {
      PyErr_SetString(PyRunTimeErr, "key already in use");
      
    } else {
      PyErr_SetString(PyRunTimeErr, "error setting label" );
      
    }
  }

  return SV_PYTHON_OK;
}


// -------------------
// Solid_ClearLabelMtd
// -------------------

static PyObject* Solid_ClearLabelMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  char *key;

  if(!PyArg_ParseTuple(args,"s",&key))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one string");
    
  }

  // Do work of command:

  if ( ! geom->IsLabelPresent( key ) ) {
    PyErr_SetString(PyRunTimeErr, "key not found");
    
  }

  geom->ClearLabel( key );

  return SV_PYTHON_OK;
}


// -------------------
// Solid_GetFaceIdsMtd
// -------------------

static PyObject* Solid_GetFaceIdsMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  int numFaces;
  int *faces;
  char facestring[256];
  PyObject* faceList;
  int status = geom->GetFaceIds( &numFaces, &faces);
  if ( status == SV_OK ) {
    if (numFaces == 0)
    {
      Py_INCREF(Py_None);

      return Py_None;
    }
    faceList=PyList_New(numFaces);
    for (int i = 0; i < numFaces; i++) {
	  sprintf(facestring, "%i", faces[i]);
      PyList_SetItem(faceList,i,PyString_FromFormat(facestring));
	  facestring[0]='\n';
    }
    delete faces;
    return faceList;
  } else {
    PyErr_SetString(PyRunTimeErr, "GetFaceIds: error on object ");
    
  }
}

// -------------------
// Solid_GetBoundaryFacesMtd
// -------------------
//
static PyObject*  Solid_GetBoundaryFacesMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  double angle = 0.0;

  if(!PyArg_ParseTuple(args,"d",&angle))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one double");
    
  }


  int status = geom->GetBoundaryFaces(angle);
  if ( status == SV_OK ) {
    return SV_PYTHON_OK;
  } else {
    PyErr_SetString(PyRunTimeErr, "GetBoundaryFaces: error on object ");
    
  }
}

// ---------------------
// Solid_GetRegionIdsMtd
// ---------------------

static PyObject* Solid_GetRegionIdsMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  int numRegions;
  int *regions;
  char regionstring[256];
  PyObject* regionList;

  int status = geom->GetRegionIds( &numRegions, &regions);
  if ( status == SV_OK ) {
    if (numRegions == 0)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }
    regionList=PyList_New(numRegions);
    for (int i = 0; i < numRegions; i++) {
	  sprintf(regionstring, "%i", regions[i]);
      PyList_SetItem(regionList,i,PyString_FromFormat(regionstring));
	  regionstring[0]='\n';
    }
    delete regions;
    return regionList;
  } else {
    PyErr_SetString(PyRunTimeErr, "GetRegionIds: error on object ");
    
  }
}


// --------------------
// Solid_GetFaceAttrMtd
// --------------------

static PyObject* Solid_GetFaceAttrMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  char *key, *value;
  int faceid;

  if(!PyArg_ParseTuple(args,"si",&key, &faceid))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one string and one int");
    
  }

  // Do work of command:

  if ( ! geom->GetFaceAttribute( key, faceid, &value ) ) {
    PyErr_SetString(PyRunTimeErr, "attribute not found");
    
  }


  return Py_BuildValue("s",value);
}


// --------------------
// Solid_SetFaceAttrMtd
// --------------------

static PyObject* Solid_SetFaceAttrMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  char *key, *value;
  int faceid;

  if(!PyArg_ParseTuple(args,"ssi",&key,&value,&faceid))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two strings and one int");
    
  }

  // Do work of command:

  if ( ! geom->SetFaceAttribute( key, faceid, value ) ) {
    PyErr_SetString(PyRunTimeErr, "attribute could not be set");
    
  }

  return SV_PYTHON_OK;
}


// ----------------------
// Solid_GetRegionAttrMtd
// ----------------------

static PyObject* Solid_GetRegionAttrMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  char *key, *value;
  int regionid;

  if(!PyArg_ParseTuple(args,"si",&key, &regionid))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one string and one int");
    
  }


  // Do work of command:

  if ( ! geom->GetRegionAttribute( key, regionid, &value ) ) {
    PyErr_SetString(PyRunTimeErr, "attribute not found");
    
  }


  return Py_BuildValue("s",value);
}


// ----------------------
// Solid_SetRegionAttrMtd
// ----------------------

static PyObject* Solid_SetRegionAttrMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  char *key, *value;
  int regionid;

  if(!PyArg_ParseTuple(args,"ssi",&key,&value,&regionid))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two strings and one int");
    
  }


  // Do work of command:

  if ( ! geom->SetRegionAttribute( key, regionid, value ) ) {
    PyErr_SetString(PyRunTimeErr, "attribute could not be set" );
    
  }

  return SV_PYTHON_OK;
}


// --------------------
// Solid_DeleteFacesMtd
// --------------------

static PyObject* Solid_DeleteFacesMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  int status;
  PyObject* faceList;
  if(!PyArg_ParseTuple(args,"O",&faceList))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one list");
    
  }

  if (PyList_Size(faceList) == 0) {
      return SV_PYTHON_OK;
  }

  int nfaces = 0;
  int *faces = new int[PyList_Size(faceList)];

  for (int i=0;i<PyList_Size(faceList);i++)
  {
    faces[i]=PyLong_AsLong(PyList_GetItem(faceList,i));
  }
  // Do work of command:

  status = geom->DeleteFaces( nfaces, faces );

  delete [] faces;

  if ( status != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "DeleteFaces: error on object");
    
  }

  return SV_PYTHON_OK;
}

// --------------------
// Solid_DeleteRegionMtd
// --------------------

static PyObject* Solid_DeleteRegionMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  int regionid;
  int status;

  if(!PyArg_ParseTuple(args,"i",&regionid))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one int");
    
  }
  // Do work of command:

  status = geom->DeleteRegion( regionid );

  if ( status != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "DeleteRegion: error on object");
    
  }

  return SV_PYTHON_OK;
}


// ------------------------
// Solid_CreateEdgeBlendMtd
// ------------------------

static PyObject* Solid_CreateEdgeBlendMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  int status;
  int faceA;
  int faceB;
  int filletshape=0;
  double radius;

  if(!PyArg_ParseTuple(args,"iid|i",&faceA,&faceB,&radius,&filletshape))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two ints, one double or one optional int");
    
  }

  status = geom->CreateEdgeBlend( faceA, faceB, radius, filletshape );

  if ( status != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "CreateEdgeBlend: error on object ");
    
  }

  return SV_PYTHON_OK;
}

static PyObject* Solid_CombineFacesMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  int status;
  int faceid1;
  int faceid2;

  if(!PyArg_ParseTuple(args,"ii",&faceid1,&faceid2))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import two ints");
    
  }


  status = geom->CombineFaces( faceid1, faceid2);

  if ( status != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "Combine Faces: Error");
    
  }

  return SV_PYTHON_OK;
}

static PyObject* Solid_RemeshFaceMtd( pySolidModel* self, PyObject* args)
{
  cvSolidModel *geom = (self->geom);
  int status;
  double size;
  PyObject* excludeList;
  if(!PyArg_ParseTuple(args,"Od",&excludeList,&size))
  {
    PyErr_SetString(PyRunTimeErr,"Could not import one list and one double");
    
  }

  if (PyList_Size(excludeList) == 0) {
      return SV_PYTHON_OK;
  }

  int nfaces = 0;
  int *faces = new int[PyList_Size(excludeList)];

  for (int i=0;i<PyList_Size(excludeList);i++)
  {
    faces[i]=PyLong_AsLong(PyList_GetItem(excludeList,i));
  }

  status = geom->RemeshFace( nfaces, faces, size);

  delete [] faces;

  if ( status != SV_OK ) {
    PyErr_SetString(PyRunTimeErr, "Remesh Face: Error");
    
  }

  return SV_PYTHON_OK;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////


//All functions listed and initiated as pySolid_methods declared here
/*
static PyMemberDef pySolidModel_members[]={
{NULL}
};
*/

//------------------------------------------------
// Define API function names for SolidModel class 
//------------------------------------------------

static PyMethodDef pySolidModel_methods[] = {

  { "Apply4x4",
      (PyCFunction)SolidModel_apply4x4,
      METH_VARARGS,
      SolidModel_apply4x4_doc
  },

  { "check", 
      (PyCFunction)SolidModel_check,
      METH_VARARGS,
      SolidModel_check_doc
  },

  { "classify_point",
      (PyCFunction)SolidModel_classify_point,
      METH_VARARGS,   
      SolidModel_classify_point_doc
  },

  { "distance",
      (PyCFunction)SolidModel_distance,
      METH_VARARGS,
      SolidModel_distance_doc
   },

  { "find_centroid",
      (PyCFunction)SolidModel_find_centroid,
      METH_VARARGS,
      SolidModel_find_centroid_doc
   },

  { "find_extent", 
      (PyCFunction)SolidModel_find_extent,
      METH_VARARGS,
      SolidModel_find_extent_doc
   },

  { "GetAxialIsoparametricCurve",
      (PyCFunction)Solid_GetAxialIsoparametricCurveMtd,
      METH_VARARGS,
      NULL
  },

  { "get_class_name", 
      (PyCFunction)SolidModel_get_class_name,
      METH_NOARGS,
      SolidModel_get_class_name_doc
  },

  { "get_discontinuities",
      (PyCFunction)SolidModel_get_discontinuities,
      METH_VARARGS,
      SolidModel_get_discontinuities_doc
  },

  { "get_face_normal",
      (PyCFunction)SolidModel_get_face_normal,
      METH_VARARGS,     
      SolidModel_get_face_normal_doc
  },

  { "get_face_polydata",
      (PyCFunction)SolidModel_get_face_polydata,
      METH_VARARGS,
      SolidModel_get_face_polydata_doc
  },

  { "get_polydata",
      (PyCFunction)SolidModel_get_polydata,
      METH_VARARGS,
      SolidModel_get_polydata_doc
  },

  { "get_spatial_dimension",
      (PyCFunction)SolidModel_get_spatial_dimension,
      METH_VARARGS,
      SolidModel_get_spatial_dimension_doc
  },

  { "get_topological_dimension",
      (PyCFunction)SolidModel_get_topological_dimension,
      METH_VARARGS,     
      SolidModel_get_topological_dimension_doc
   },

  { "print", 
      (PyCFunction)SolidModel_print,
      METH_VARARGS,
      SolidModel_print_doc
   },

  { "reflect",
      (PyCFunction)SolidModel_reflect,
      METH_VARARGS,
      SolidModel_reflect_doc
  },

  { "rotate",
      (PyCFunction)SolidModel_rotate,
      METH_VARARGS,
      SolidModel_rotate_doc
  },

  { "scale",
      (PyCFunction)SolidModel_scale,
      METH_VARARGS,
      SolidModel_scale_doc
  },

  { "set_vtk_polydata",
      (PyCFunction)SolidModel_set_vtk_polydata,
      METH_VARARGS,
      SolidModel_set_vtk_polydata_doc
  },

  { "translate", 
      (PyCFunction)SolidModel_translate,
      METH_VARARGS,
      SolidModel_translate_doc
  },

  { "write_geom_sim", 
      (PyCFunction)SolidModel_write_geom_sim,
      METH_VARARGS,
      SolidModel_write_geom_sim_doc
   },

  { "write_native",
      (PyCFunction)SolidModel_write_native,
      METH_VARARGS,
      SolidModel_write_native_doc
   },

  { "write_vtk_polydata",
      (PyCFunction)SolidModel_write_vtk_polydata,
      METH_VARARGS,
      SolidModel_write_vtk_polydata_doc
   },


// ============================================== //

  { "GetModel", 
      (PyCFunction)Solid_GetModelCmd, 
      METH_VARARGS, 
      NULL
  },

  { "Poly",(PyCFunction) Solid_PolyCmd,
		     METH_VARARGS,NULL},
  { "polygon_points", 
      (PyCFunction)Solid_polygon_points,
      METH_VARARGS,
      Solid_polygon_points_doc
  },

  { "Circle", (PyCFunction)Solid_CircleCmd,
		     METH_VARARGS,NULL},
  { "Ellipse", (PyCFunction)Solid_EllipseCmd,
		     METH_VARARGS,NULL},
  { "Box2d", (PyCFunction)Solid_Box2dCmd,
		     METH_VARARGS,NULL},
  { "Box3d", (PyCFunction)Solid_Box3dCmd,
		     METH_VARARGS,NULL},
  { "Sphere", (PyCFunction)Solid_SphereCmd,
		     METH_VARARGS,NULL},
  { "Ellipsoid", (PyCFunction)Solid_EllipsoidCmd,
		     METH_VARARGS,NULL},
  { "Cylinder", (PyCFunction)Solid_CylinderCmd,
		     METH_VARARGS,NULL},
  { "TruncatedCone", (PyCFunction)Solid_TruncatedConeCmd,
		     METH_VARARGS,NULL},
  { "Torus", (PyCFunction)Solid_TorusCmd,
		     METH_VARARGS,NULL},
  { "Poly3dSolid", (PyCFunction)Solid_Poly3dSolidCmd,
		     METH_VARARGS,NULL},
  { "Poly3dSurface", (PyCFunction)Solid_Poly3dSurfaceCmd,
		     METH_VARARGS,NULL},
  { "ExtrudeZ", (PyCFunction)Solid_ExtrudeZCmd,
		     METH_VARARGS,NULL},
  { "Extrude", (PyCFunction)Solid_ExtrudeCmd,
		     METH_VARARGS,NULL},
  { "MakeApproxCurveLoop",
		     (PyCFunction)Solid_MakeApproxCurveLoopCmd,
		     METH_VARARGS,NULL},
  { "MakeInterpCurveLoop",
		     (PyCFunction)Solid_MakeInterpCurveLoopCmd,
		     METH_VARARGS,NULL},
  { "MakeLoftedSurf", (PyCFunction)Solid_MakeLoftedSurfCmd,
		     METH_VARARGS,NULL},
  { "CapSurfToSolid", (PyCFunction)Solid_CapSurfToSolidCmd,
		     METH_VARARGS,NULL},
  { "Intersect", (PyCFunction)Solid_IntersectCmd,
		     METH_VARARGS,NULL},
  { "Union", (PyCFunction)Solid_UnionCmd,
		     METH_VARARGS,NULL},
  { "Subtract", (PyCFunction)Solid_SubtractCmd,
		     METH_VARARGS,NULL},
  { "ReadNative", (PyCFunction)Solid_ReadNativeCmd,
		     METH_VARARGS,NULL},
  { "Copy", (PyCFunction)Solid_CopyCmd,
		     METH_VARARGS,NULL},
  { "NewObject", (PyCFunction)Solid_NewObjectCmd,
		     METH_VARARGS,NULL},

  { "DeleteFaces",(PyCFunction)Solid_DeleteFacesMtd,
		     METH_VARARGS,NULL},
  { "DeleteRegion",(PyCFunction)Solid_DeleteRegionMtd,
		     METH_VARARGS,NULL},
  { "CreateEdgeBlend",(PyCFunction)Solid_CreateEdgeBlendMtd,
		     METH_VARARGS,NULL},
  { "CombineFaces",(PyCFunction)Solid_CombineFacesMtd,
		     METH_VARARGS,NULL},
  { "RemeshFace",(PyCFunction)Solid_RemeshFaceMtd,
		     METH_VARARGS,NULL},


















  { "GetKernel",(PyCFunction)Solid_GetKernelMtd,
		     METH_VARARGS,NULL},
  { "GetLabelKeys",(PyCFunction)Solid_GetLabelKeysMtd,
		     METH_VARARGS,NULL},
  { "GetLabel", (PyCFunction)Solid_GetLabelMtd,
		     METH_VARARGS,NULL},
  { "SetLabel",(PyCFunction)Solid_SetLabelMtd,
		     METH_VARARGS,NULL},
  { "ClearLabel", (PyCFunction)Solid_ClearLabelMtd,
		     METH_VARARGS,NULL},
  { "GetFaceIds", (PyCFunction)Solid_GetFaceIdsMtd,
		     METH_NOARGS,NULL},
  { "GetBoundaryFaces",(PyCFunction)Solid_GetBoundaryFacesMtd,
		     METH_VARARGS,NULL},
  { "GetRegionIds",(PyCFunction)Solid_GetRegionIdsMtd,
		     METH_VARARGS,NULL},
  { "GetFaceAttr",(PyCFunction)Solid_GetFaceAttrMtd,
		     METH_VARARGS,NULL},
  { "SetFaceAttr",(PyCFunction)Solid_SetFaceAttrMtd,
		     METH_VARARGS,NULL},
  { "GetRegionAttr",(PyCFunction)Solid_GetRegionAttrMtd,
		     METH_VARARGS,NULL},
  { "SetRegionAttr", (PyCFunction)Solid_SetRegionAttrMtd,
		     METH_VARARGS,NULL},
  {NULL,NULL}
};

//-------------------
// pySolidModel_init
//-------------------
// This is the __init__() method for the SolidModel class. 
//
// This function is used to initialize an object after it is created.
//
static int 
pySolidModel_init(pySolidModel* self, PyObject* args)
{
  fprintf(stdout,"pySolid Model tp_init called\n");
  return SV_OK;
}

//------------------
// pySolidModelType 
//------------------
// This is the definition of the SolidModel class.
//
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject pySolidModelType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "solid.SolidModel",        /* tp_name */
  sizeof(pySolidModel),      /* tp_basicsize */
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
  "SolidModel  objects",           /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  pySolidModel_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)pySolidModel_init,                            /* tp_init */
  0,                         /* tp_alloc */
  0,                  /* tp_new */
};


//----------------------
// createSolidModelType 
//----------------------
pySolidModel * createSolidModelType()
{
  return PyObject_New(pySolidModel, &pySolidModelType);
}


//------------------
// pySolid_methods
//------------------
// Methods for the 'solid' module.
//
static PyMethodDef pySolid_methods[] = {

  {"list_registrars", 
      (PyCFunction)Solid_list_registrars,
      METH_NOARGS,
      Solid_list_registrars_doc
  },

  { "set_kernel", 
      (PyCFunction)Solid_set_kernel,
      METH_VARARGS, 
      NULL
  },

  { "get_kernel", 
      (PyCFunction)Solid_get_kernel,
      METH_NOARGS,
      Solid_get_kernel_doc
  },

  {NULL, NULL}
};

static PyTypeObject pycvFactoryRegistrarType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "solid.pycvFactoryRegistrar",             /* tp_name */
  sizeof(pycvFactoryRegistrar),             /* tp_basicsize */
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
  "cvFactoryRegistrar wrapper  ",           /* tp_doc */
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python interpreter 
// when the module is loaded.

static char* MODULE_NAME = "solid";
static char* SOLID_MODEL_CLASS_NAME = "SolidModel";
static char* MODULE_EXCEPTION_NAME = "solid.SolidModelException";
static char* SOLID_MODEL_EXCEPTION_NAME = "SolidModelException";

PyDoc_STRVAR(Solid_module_doc, "solid module functions");

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3

PyMODINIT_FUNC PyInit_pySolid(void);

int Solid_pyInit()
{ 
  PyInit_pySolid();
  return SV_OK;
}

// Size of per-interpreter state of the module.
// Set to -1 if the module keeps state in global variables. 
static int perInterpreterStateSize = -1;

// Always initialize this to PyModuleDef_HEAD_INIT.
static PyModuleDef_Base m_base = PyModuleDef_HEAD_INIT;

// Define the module definition struct which holds all information 
// needed to create a module object. 

static struct PyModuleDef pySolidmodule = {
   m_base,
   MODULE_NAME, 
   Solid_module_doc, 
   perInterpreterStateSize, 
   pySolid_methods
};

//-----------------
// PyInit_pySolid  
//-----------------
// The initialization function called by the Python interpreter 
// when the 'solid' module is loaded.
//
PyMODINIT_FUNC
PyInit_pySolid(void)
{
  if (gRepository == NULL) {
      gRepository=new cvRepository();
      fprintf(stdout,"New gRepository created from cv_solid_init\n");
  }

  // Set the default modeling kernel. 
  cvSolidModel::gCurrentKernel = SM_KT_INVALID;
  #ifdef SV_USE_PARASOLID
  cvSolidModel::gCurrentKernel = SM_KT_PARASOLID;
  #endif

  // Create the SolidModeling class definition.
  pySolidModelType.tp_new = PyType_GenericNew;
  pycvFactoryRegistrarType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&pySolidModelType) < 0) {
    fprintf(stdout,"Error in pySolidModelType");
    return SV_PYTHON_ERROR;
  }

  if (PyType_Ready(&pycvFactoryRegistrarType) < 0) {
    fprintf(stdout,"Error in pySolidModelType");
    return SV_PYTHON_ERROR;
  }

  // Create the 'solid' module. 
  auto module = PyModule_Create(&pySolidmodule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing pySolid");
    return SV_PYTHON_ERROR;
  }

  // Add solid.SolidModelException exception.
  //
  PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION_NAME, NULL, NULL);
  PyModule_AddObject(module, SOLID_MODEL_EXCEPTION_NAME, PyRunTimeErr);

  // Add the 'SolidModel' class.
  Py_INCREF(&pySolidModelType);
  PyModule_AddObject(module, SOLID_MODEL_CLASS_NAME, (PyObject *)&pySolidModelType);

  Py_INCREF(&pycvFactoryRegistrarType);
  PyModule_AddObject(module, "pyCvFactoryRegistrar", (PyObject *)&pycvFactoryRegistrarType);

  // [TODO:DaveP] What does this do?
  pycvFactoryRegistrar* tmp = PyObject_New(pycvFactoryRegistrar, &pycvFactoryRegistrarType);
  tmp->registrar = (cvFactoryRegistrar *)&cvSolidModel::gRegistrar;
  PySys_SetObject("solidModelRegistrar", (PyObject *)tmp);

  return module;
}

#endif


//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC initpySolid(void);
int Solid_pyInit()
{ 
  initpySolid();
  return SV_OK;
}

PyMODINIT_FUNC
initpySolid(void)
{
    // Initialize-gRepository
  if (gRepository ==NULL)
  {
    gRepository=new cvRepository();
    fprintf(stdout,"New gRepository created from cv_solid_init\n");
  }
  //Initialize-gCurrentKernel
  cvSolidModel::gCurrentKernel = SM_KT_INVALID;
  #ifdef SV_USE_PARASOLID
  cvSolidModel::gCurrentKernel = SM_KT_PARASOLID;
  #endif

  pySolidModelType.tp_new=PyType_GenericNew;
  pycvFactoryRegistrarType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&pySolidModelType)<0)
  {
    fprintf(stdout,"Error in pySolidModelType");
    return;
  }
  if (PyType_Ready(&pycvFactoryRegistrarType)<0)
  {
    fprintf(stdout,"Error in pySolidModelType");
    return;
  }
  //Init our defined functions
  PyObject *pythonC;
  pythonC = Py_InitModule("pySolid", pySolid_methods);
  if (pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pySolid");
    return;
  }

  PyRunTimeErr=PyErr_NewException("pySolid.error",NULL,NULL);
  PyModule_AddObject(pythonC, "error",PyRunTimeErr);
  Py_INCREF(&pySolidModelType);
  Py_INCREF(&pycvFactoryRegistrarType);
  PyModule_AddObject(pythonC, "pySolidModel", (PyObject *)&pySolidModelType);
  PyModule_AddObject(pythonC, "pyCvFactoryRegistrar", (PyObject *)&pycvFactoryRegistrarType);

  pycvFactoryRegistrar* tmp = PyObject_New(pycvFactoryRegistrar, &pycvFactoryRegistrarType);
  tmp->registrar = (cvFactoryRegistrar *)&cvSolidModel::gRegistrar;
  PySys_SetObject("solidModelRegistrar", (PyObject *)tmp);


}
#endif

