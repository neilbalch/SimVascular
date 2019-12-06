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

// The functions defined here implement the SV Python API spline polygon countour module. 
//
// The module name is 'spline_polygon_contour'. 

#include "SimVascular.h"
#include "sv_misc_utils.h"
#include "sv3_Contour.h"
#include "sv3_Contour_PyModule.h"
#include "sv3_SplinePolygonContour.h"
#include "sv3_SplinePolygonContour_init_py.h"
#include "sv_arg.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"

#include "Python.h"
#include "sv_PyUtils.h"
#include "sv2_globals.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//--------------------------------
// splinePolygonContour_available
//--------------------------------
//
PyDoc_STRVAR(splinePolygonContour_available_doc,
  "available(kernel)                                    \n\ 
                                                                 \n\
   Set the computational kernel used to segment image data.       \n\
                                                                 \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject *  
splinePolygonContour_available(PyObject* self, PyObject* args)
{
  return Py_BuildValue("s","polygonContour Available");
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "spline_polygon_contour";

PyDoc_STRVAR(SplinePolygonContour_doc, "spline_polygon_contour module functions");

//------------------------------
// splinePolygonContour_methods
//------------------------------
// Define the module methods.
//
PyMethodDef splinePolygonContour_methods[] = {

  {"available", splinePolygonContour_available, METH_NOARGS, splinePolygonContour_available_doc},

  {NULL, NULL}
};

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3

// Size of per-interpreter state of the module.
// Set to -1 if the module keeps state in global variables. 
static int perInterpreterStateSize = -1;

// Always initialize this to PyModuleDef_HEAD_INIT.
static PyModuleDef_Base m_base = PyModuleDef_HEAD_INIT;

// Define the module definition struct which holds all information 
// needed to create a module object. 
static struct PyModuleDef pySplinePolygonContourModule = {
   m_base, 
   MODULE_NAME, 
   SplinePolygonContour_doc,
   perInterpreterStateSize, 
   splinePolygonContour_methods
};

//-------------------------------
// PyInit_pySplinePolygonContour 
//-------------------------------
// The initialization function called by the Python interpreter when the module is loaded.
//
// This function is used in Application/SimVascular_Init_py.cxx.
//
PyMODINIT_FUNC
PyInit_pySplinePolygonContour()
{
  auto module = PyModule_Create(&pySplinePolygonContourModule);
  if(module == NULL) {
    fprintf(stdout,"Error in initializing pySplinePolygonContour");
    Py_RETURN_NONE;
  }
  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC
initpySplinePolygonContour()
{
  printf("  %-12s %s\n","","splinePolygonContour Enabled");

  // Associate the adapt registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  PyObject* pyGlobal = PySys_GetObject("ContourObjectRegistrar");
  pyContourFactoryRegistrar* tmp = (pyContourFactoryRegistrar *) pyGlobal;
  cvFactoryRegistrar* contourObjectRegistrar =tmp->registrar;

  if (contourObjectRegistrar != NULL) {
          // Register this particular factory method with the main app.
          contourObjectRegistrar->SetFactoryMethodPtr( cKERNEL_SPLINEPOLYGON,
      (FactoryMethodPtr) &CreateSplinePolygonContour );
  }
  else {
    return;
  }
      tmp->registrar = contourObjectRegistrar;
  PySys_SetObject("ContourObjectRegistrar",(PyObject*)tmp);

  PyObject* pythonC;
  pythonC = Py_InitModule("pySplinePolygonContour", splinePolygonContour_methods);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pySplinePolygonContour");
    return;
  }
}
#endif

