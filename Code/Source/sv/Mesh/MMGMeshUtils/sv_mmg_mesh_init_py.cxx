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

// The functions defined here implement the SV Python API mesh_util Module. 
//
// [TODO:DaveP] I'm not sure what this module does.
//
//   Something to do with MMG?
//
#include "SimVascular.h"
#include "SimVascular_python.h"
#include "sv_misc_utils.h"
#include "sv_mmg_mesh_init.h"
#include "sv_arg.h"
#include "sv_PyUtils.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_PolyData.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"

#include "sv_mmg_mesh_utils.h"
#include "Python.h"

// Needed for Windows.
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
PyObject* PyRunTimeErr;

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//------------
// MMG_remesh
//------------
//
static PyObject * 
MMG_remesh(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss|ddddd", PyRunTimeErr, __func__);
  char *srcName, *dstName;
  double hmin = 0.1;
  double hmax = 0.1;
  double angle = 45.0;
  double hgrad = 1.1;
  double hausd = 0.01;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &dstName, &hmin, &hmax, &angle, &hgrad, &hausd)) {
   return api.argsError();
  }

  // Check that the source Polydata object is in the
  // repository and that it is the correct type.
  //
  auto src = gRepository->GetObject(srcName);
  if (src == NULL) {
      api.error("The Mesh object '" + std::string(srcName) + "' is not the repository.");
      return nullptr;
  }

  auto type = src->GetType();
  if (type != POLY_DATA_T) {
      api.error("'" + std::string(srcName) + "' is not a polydata object.");
      return nullptr;
  }

  // Check that the new Mesh object does not already exist.
  //
  // [TODO:DaveP] Why does it matter that it is already in the repository? 
  //   Can't it just  be overwritten?
  //
  if (gRepository->Exists(dstName)) {
      api.error("The Mesh object '" + std::string(dstName) + "' is already in the repository.");
      return nullptr;
  }

  // Get the surface polydata.
  auto surfPolydata = ((cvPolyData*)src)->GetVtkPolyData();
  surfPolydata->BuildLinks();

  // Try to remesh the surface polydata.
  //
  int useSizingFunction = 0;
  int numAddedRefines = 0;
  vtkDoubleArray *meshSizingFunction = NULL;

  if (MMGUtils_SurfaceRemeshing(surfPolydata, hmin, hmax, hausd, angle, hgrad, useSizingFunction, meshSizingFunction, numAddedRefines) != SV_OK) {
      api.error("Error remeshing object '" + std::string(srcName) + "'.");
      return nullptr;
  }

  auto dst = new cvPolyData(surfPolydata);
  if (dst == NULL) {
      api.error("Error creating polydata from the remeshed object '" + std::string(dstName) + "'.");
      return nullptr;
  }

  if (!gRepository->Register(dstName, dst)) {
      delete dst;
      api.error("Error adding the remeshed object '" + std::string(dstName) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",dst->GetName());
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

PyMethodDef Mmgmesh_methods[] =
{
  {"remesh", MMG_remesh, METH_VARARGS, NULL },
  {NULL,NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MODULE_NAME = "mesh_util";

PyDoc_STRVAR(MeshUtil_doc, "mesh_util functions");

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3

static struct PyModuleDef pyMeshUtilmodule = {
   PyModuleDef_HEAD_INIT,
   MODULE_NAME,  
   MeshUtil_doc,
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   Mmgmesh_methods
};

// ----------
// Mmgmesh_Init
// ----------

PyObject* Mmgmesh_pyInit()
{
  auto module = PyModule_Create(&pyMeshUtilmodule);

  if (module == NULL) {
    fprintf(stdout,"Error in initializing pyMeshUtil");
    return SV_PYTHON_ERROR;
  }

  // Add mesh_util.MeshUtilException exception.
  PyRunTimeErr = PyErr_NewException("mesh_util.MeshUtilException", NULL, NULL);
  PyModule_AddObject(module, "MeshUtilException", PyRunTimeErr);

  return module;
}

PyMODINIT_FUNC PyInit_pyMeshUtil()
{
  PyObject *pythonC;
  pythonC = PyModule_Create(&pyMeshUtilmodule);
  if (pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyMeshUtil");
     return SV_PYTHON_ERROR;
  }
  PyRunTimeErr=PyErr_NewException("pyMeshUtil.error",NULL,NULL);
  PyModule_AddObject(pythonC, "error",PyRunTimeErr);

  return pythonC;

}
#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC initpyMeshUtil()
{
  PyObject *pythonC;
  pythonC = Py_InitModule("pyMeshUtil", Mmgmesh_methods);

  if (pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyMeshUtil");
    return;

  }
  PyRunTimeErr=PyErr_NewException("pyMeshUtil.error",NULL,NULL);
  return;

}

PyObject* Mmgmesh_pyInit()
{ 
  PyObject *pythonC;
  pythonC = Py_InitModule("pyMeshUtil", Mmgmesh_methods);

  if (pythonC==NULL)
  { 
    fprintf(stdout,"Error in initializing pyMeshUtil");
    return SV_PYTHON_ERROR;
  }
  PyRunTimeErr=PyErr_NewException("pyMeshUtil.error",NULL,NULL);
  PyModule_AddObject(pythonC, "error",PyRunTimeErr);
  return pythonC;
}

#endif

