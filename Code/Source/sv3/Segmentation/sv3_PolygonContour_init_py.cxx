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

// The functions defined here implement the SV Python API polygon countour module. 
//
// The module name is 'polygon_contour'. 
//
#include "SimVascular.h"
#include "sv_misc_utils.h"
#include "sv3_Contour.h"
#include "sv3_Contour_init_py.h"
#include "sv3_PolygonContour.h"
#include "sv3_PolygonContour_init_py.h"
#include "sv_arg.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"

#include "Python.h"
#include "sv2_globals.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif
//
using sv3::ContourPolygon;

ContourPolygon* CreatePolygonContour()
{
  return new ContourPolygon();
}

//--------------------------
// polygonContour_available
//--------------------------
//
static PyObject *
polygonContour_available(PyObject* self, PyObject* args)
{
  return Py_BuildValue("s","polygonContour Available");
}

//---------------------------
// polygonContour_registrars
//---------------------------
//
static PyObject * 
polygonContour_registrars(PyObject* self, PyObject* args)
{
  PyObject* pyGlobal = PySys_GetObject("ContourObjectRegistrar");
  pyContourFactoryRegistrar* tmp = (pyContourFactoryRegistrar *) pyGlobal;
  cvFactoryRegistrar* contourObjectRegistrar =tmp->registrar;

  char result[255];
  PyObject* pyPtr=PyList_New(8);
  sprintf( result, "Contour object registrar ptr -> %p\n", contourObjectRegistrar );
  PyList_SetItem(pyPtr,0,PyBytes_FromFormat(result));

  for (int i = 0; i < 7; i++) {
      sprintf( result,"GetFactoryMethodPtr(%i) = %p\n",
      i, (contourObjectRegistrar->GetFactoryMethodPtr(i)));
      fprintf(stdout,result);
      PyList_SetItem(pyPtr,i+1,PyBytes_FromFormat(result));
  }
  return pyPtr;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "polygon_contour";

PyDoc_STRVAR(Contour_doc, "polygon_contour module functions");

PyMethodDef polygonContour_methods[] = {
  {"available", polygonContour_available,METH_NOARGS,NULL},
  {"registrars", polygonContour_registrars,METH_NOARGS,NULL},
  {NULL, NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3
static struct PyModuleDef pyPolygonContourModule = {
   PyModuleDef_HEAD_INIT,
   MODULE_NAME,   
   "", /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   polygonContour_methods
};

PyMODINIT_FUNC
PyInit_pyPolygonContour()
{
  printf("  %-12s %s\n","","polygonContour Enabled");

  // Associate the adapt registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  PyObject* pyGlobal = PySys_GetObject("ContourObjectRegistrar");
  pyContourFactoryRegistrar* tmp = (pyContourFactoryRegistrar *) pyGlobal;
  cvFactoryRegistrar* contourObjectRegistrar =tmp->registrar;

  if (contourObjectRegistrar != NULL) {
          // Register this particular factory method with the main app.
          contourObjectRegistrar->SetFactoryMethodPtr( cKERNEL_POLYGON,
      (FactoryMethodPtr) &CreatePolygonContour );
  }
  else {
    Py_RETURN_NONE;
  }
      tmp->registrar = contourObjectRegistrar;
  PySys_SetObject("ContourObjectRegistrar",(PyObject*)tmp);

  PyObject* pythonC;
  pythonC = PyModule_Create(&pyPolygonContourModule);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyPolygonContour\n");
    Py_RETURN_NONE;
  }
  return pythonC;
}
#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC
initpyPolygonContour()
{
  printf("  %-12s %s\n","","polygonContour Enabled");

  // Associate the adapt registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  PyObject* pyGlobal = PySys_GetObject("ContourObjectRegistrar");
  pyContourFactoryRegistrar* tmp = (pyContourFactoryRegistrar *) pyGlobal;
  cvFactoryRegistrar* contourObjectRegistrar =tmp->registrar;

  if (contourObjectRegistrar != NULL) {
          // Register this particular factory method with the main app.
          contourObjectRegistrar->SetFactoryMethodPtr( cKERNEL_POLYGON,
      (FactoryMethodPtr) &CreatePolygonContour );
  }
  else {
    return;
  }
      tmp->registrar = contourObjectRegistrar;
  PySys_SetObject("ContourObjectRegistrar",(PyObject*)tmp);

  PyObject* pythonC;
  pythonC = Py_InitModule("pyPolygonContour", polygonContour_methods);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyPolygonContour\n");
    return;
  }
}
#endif

