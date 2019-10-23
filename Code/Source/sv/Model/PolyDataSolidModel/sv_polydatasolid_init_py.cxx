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

// The functions defined here implement the SV Python API polydata solid module. 
//
// The module name is 'polydata_solid'.
//
/** @file sv_polydatasolid_init_py.cxx
 *  @brief Ipmlements function to register PolyDataSolid as a solid type
 *
 *  @author Adam Updegrove
 *  @author updega2@gmail.com
 *  @author UC Berkeley
 *  @author shaddenlab.berkeley.edu
 */

#include "SimVascular.h"
#include "SimVascular_python.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_solid_init.h"
#include "sv_solid_init_py.h"
#include "sv_polydatasolid_init_py.h"
#include "sv_polydatasolid_utils.h"
#include "sv_SolidModel.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"
#include "sv_vtk_utils.h"
#include "sv_PolyData.h"
#include "sv_PolyDataSolid.h"
#include "vtkPolyData.h"
#include "sv2_globals.h"
#include "Python.h"
#include "sv_FactoryRegistrar.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

cvPolyDataSolid* pyCreatePolyDataSolid()
{
  return new cvPolyDataSolid();
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//-------------------------
// PolyDataSolid_available
//-------------------------
//
static PyObject * 
PolyDataSolid_available(PyObject* self, PyObject* args )
{
  return Py_BuildValue("s","PolyData Solid Module Available");
}

//--------------------------
// PolyDataSolid_registrars
//--------------------------
//
static PyObject * 
PolyDataSolid_registrars(PyObject* self, PyObject* args )
{
  char result[2048];
  int k=0;
  PyObject *pyPtr=PyList_New(6);
  PyObject* pyGlobal = PySys_GetObject("solidModelRegistrar");
  pycvFactoryRegistrar* tmp = (pycvFactoryRegistrar *) pyGlobal;
  cvFactoryRegistrar* pySolidModelRegistrar =tmp->registrar;

  sprintf(result, "Solid model registrar ptr -> %p\n", pySolidModelRegistrar);
  fprintf(stdout,result);
  PyList_SetItem(pyPtr,0,PyBytes_FromFormat(result));

  for (int i = 0; i < 5; i++) {
      sprintf( result,"GetFactoryMethodPtr(%i) = %p\n",
      i, (pySolidModelRegistrar->GetFactoryMethodPtr(i)));
      fprintf(stdout,result);
      PyList_SetItem(pyPtr,i+1,PyBytes_FromFormat(result));
  }
  return pyPtr;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "solid_polydata";

PyMethodDef SolidPolydata_methods[] = {
  {"available", PolyDataSolid_available,METH_NOARGS,NULL},
  {"registrars", PolyDataSolid_registrars,METH_NOARGS,NULL},
  {NULL, NULL}
};

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3
static struct PyModuleDef pySolidPolydatamodule = {
   PyModuleDef_HEAD_INIT,
   MODULE_NAME, 
   "", /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   SolidPolydata_methods
};

PyMODINIT_FUNC
PyInit_pySolidPolydata()
{
  PyObject* pyGlobal = PySys_GetObject("solidModelRegistrar");
  pycvFactoryRegistrar* tmp = (pycvFactoryRegistrar *) pyGlobal;
  cvFactoryRegistrar* pySolidModelRegistrar =tmp->registrar;

  if (pySolidModelRegistrar != NULL) {
          // Register this particular factory method with the main app.
          pySolidModelRegistrar->SetFactoryMethodPtr( SM_KT_POLYDATA,
      (FactoryMethodPtr) &pyCreatePolyDataSolid );
  }
  else {
    return SV_PYTHON_ERROR;

  }
  tmp->registrar = pySolidModelRegistrar;
  PySys_SetObject("solidModelRegistrar",(PyObject*)tmp);
  // Initialize parasolid_utils
  if (PlyDtaUtils_Init() != SV_OK) {
    return SV_PYTHON_ERROR;

  }

  PyObject *pythonC;

  pythonC = PyModule_Create(&pySolidPolydatamodule);

  if (pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pySolidolydata");
    return SV_PYTHON_ERROR;
  }

  return pythonC;
}

#endif


PyObject* Polydatasolid_pyInit()
{

  PyObject* pyGlobal = PySys_GetObject("solidModelRegistrar");
  pycvFactoryRegistrar* tmp = (pycvFactoryRegistrar *) pyGlobal;
  cvFactoryRegistrar* pySolidModelRegistrar =tmp->registrar;
  
  if (pySolidModelRegistrar != NULL) {
          // Register this particular factory method with the main app.
          pySolidModelRegistrar->SetFactoryMethodPtr( SM_KT_POLYDATA,
      (FactoryMethodPtr) &pyCreatePolyDataSolid );
  }
  else
    return SV_PYTHON_ERROR;
  tmp->registrar = pySolidModelRegistrar;
  PySys_SetObject("solidModelRegistrar",(PyObject*)tmp);
  // Initialize parasolid_utils
  if (PlyDtaUtils_Init() != SV_OK) {
    return SV_PYTHON_ERROR;
  }

  PyObject *pythonC;
#if PYTHON_MAJOR_VERSION == 2
  pythonC = Py_InitModule("pySolidPolydata", SolidPolydata_methods);
#elif PYTHON_MAJOR_VERSION == 3
  pythonC = PyModule_Create(&pySolidPolydatamodule);
#endif
  if (pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pySolid");
    return SV_PYTHON_ERROR;
  }
  return pythonC;
}

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC
initpySolidPolydata()
{

  PyObject* pyGlobal = PySys_GetObject("solidModelRegistrar");
  pycvFactoryRegistrar* tmp = (pycvFactoryRegistrar *) pyGlobal;
  cvFactoryRegistrar* pySolidModelRegistrar =tmp->registrar;
  if (pySolidModelRegistrar != NULL) {
          // Register this particular factory method with the main app.
          pySolidModelRegistrar->SetFactoryMethodPtr( SM_KT_POLYDATA,
      (FactoryMethodPtr) &pyCreatePolyDataSolid );
  }
  else {
    return ;
  }
  tmp->registrar = pySolidModelRegistrar;
  PySys_SetObject("solidModelRegistrar",(PyObject*)tmp);
  // Initialize parasolid_utils
  if (PlyDtaUtils_Init() != SV_OK) {
    return ;
  }

  PyObject *pythonC;
  pythonC = Py_InitModule("pySolidPolydata", SolidPolydata_methods);
  if (pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pySolid");
    return ;
  }
}
#endif

