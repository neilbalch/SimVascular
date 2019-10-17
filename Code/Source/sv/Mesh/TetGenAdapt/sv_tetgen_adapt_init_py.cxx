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

// The functions defined here implement the SV Python API TetGen adapt module. 
//
// [TODO:DaveP] what do these functions do?
//

/** @file sv_tetgen_adapt_init_py.cxx
 *  @brief Ipmlements functions to register TetGenAdapt as an adaptor type
 *
 *  @author Adam Updegrove
 *  @author updega2@gmail.com
 *  @author UC Berkeley
 *  @author shaddenlab.berkeley.edu
 */

#include "SimVascular.h"
#include "SimVascular_python.h"
#include "sv_misc_utils.h"
#include "sv_tetgen_adapt_init_py.h"
#include "sv_adapt_init_py.h"
#include "sv_TetGenAdapt.h"
//#include "sv_adapt_utils.h"
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

//---------------------
// pyCreateTetGenAdapt
//---------------------
//
cvTetGenAdapt * 
pyCreateTetGenAdapt()
{
    return new cvTetGenAdapt();
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

static PyObject *  
TetGenAdapt_AvailableCmd(PyObject* self, PyObject* args)
{
  return Py_BuildValue("s","TetGen Adaption Available");
}

//--------------------------------
// TetGenAdapt_RegistrarsListCmd
//--------------------------------
//
static PyObject * 
TetGenAdapt_RegistrarsListCmd(PyObject* self, PyObject* args)
{
  PyObject* pyGlobal = PySys_GetObject("AdaptObjectRegistrar");
  pyAdaptObjectRegistrar* tmp = (pyAdaptObjectRegistrar *) pyGlobal;
  cvFactoryRegistrar* adaptObjectRegistrar =tmp->registrar;

  char result[255];
  PyObject* pyPtr=PyList_New(6);
  sprintf( result, "Adapt object registrar ptr -> %p\n", adaptObjectRegistrar );
  PyList_SetItem(pyPtr,0,PyBytes_FromFormat(result));

  for (int i = 0; i < 5; i++) {
      sprintf( result,"GetFactoryMethodPtr(%i) = %p\n",
      i, (adaptObjectRegistrar->GetFactoryMethodPtr(i)));
      fprintf(stdout,result);
      PyList_SetItem(pyPtr,i+1,PyBytes_FromFormat(result));
  }
  return pyPtr;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

PyMethodDef TetGenAdapt_methods[] = {

  {"Available", TetGenAdapt_AvailableCmd,METH_NOARGS,NULL},

  {"Registrars", TetGenAdapt_RegistrarsListCmd,METH_NOARGS,NULL},

  {NULL, NULL}
};


//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MODULE_NAME = "tetgen_adapt";

PyDoc_STRVAR(TetgenAdapt_doc, "tetgen_adapt module functions");

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

static struct PyModuleDef pyTetGenAdaptmodule = {
   m_base,
   MODULE_NAME, 
   TetgenAdapt_doc, 
   perInterpreterStateSize, 
   TetGenAdapt_methods
};

PyMODINIT_FUNC
PyInit_pyTetGenAdapt()
{
  printf("  %-12s %s\n","","TetGen Adaption Enabled");

  // Associate the adapt registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  PyObject* pyGlobal = PySys_GetObject("AdaptRegistrar");
  pyAdaptObjectRegistrar* tmp = (pyAdaptObjectRegistrar *) pyGlobal;
  cvFactoryRegistrar* adaptObjectRegistrar =tmp->registrar;

  if (adaptObjectRegistrar != NULL) {
          // Register this particular factory method with the main app.
          adaptObjectRegistrar->SetFactoryMethodPtr( KERNEL_TETGEN,
      (FactoryMethodPtr) &pyCreateTetGenAdapt );
  }
  else {
    return SV_PYTHON_ERROR;
  }
  tmp->registrar = adaptObjectRegistrar;
  PySys_SetObject("AdaptModelRegistrar",(PyObject*)tmp);

  PyObject* pythonC;
  pythonC = PyModule_Create(&pyTetGenAdaptmodule);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyTetGenAdapt\n");
    return SV_PYTHON_ERROR;

  }

  return pythonC;
}

#endif


//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC
initpyTetGenAdapt()
{
  printf("  %-12s %s\n","","TetGen Adaption Enabled");

  // Associate the adapt registrar with the python interpreter so it can be
  // retrieved by the DLLs.

  PyObject* pyGlobal = PySys_GetObject("AdaptObjectRegistrar");
  pyAdaptObjectRegistrar* tmp = (pyAdaptObjectRegistrar *) pyGlobal;
  cvFactoryRegistrar* adaptObjectRegistrar =tmp->registrar;

  if (adaptObjectRegistrar != NULL) {
          // Register this particular factory method with the main app.
          adaptObjectRegistrar->SetFactoryMethodPtr( KERNEL_TETGEN,
      (FactoryMethodPtr) &pyCreateTetGenAdapt );
  }
  else {
    return;

  }
  
  tmp->registrar = adaptObjectRegistrar;
  PySys_SetObject("AdaptModelRegistrar",(PyObject*)tmp);

  PyObject* pythonC;
  pythonC = Py_InitModule("pyTetGenAdapt", TetGenAdapt_methods);

  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyTetGenAdapt\n");
    return;

  }

}

#endif

//-----------------
// Tetgenmesh_Init
//-----------------
//
// [TODO:DaveP] where is this used?
//
PyObject * 
Tetgenadapt_pyInit()
{
  printf("  %-12s %s\n","","TetGen Adaption Enabled");

  // Associate the adapt registrar with the python interpreter
  //
  PyObject* pyGlobal = PySys_GetObject("AdaptObjectRegistrar");
  pyAdaptObjectRegistrar* tmp = (pyAdaptObjectRegistrar *) pyGlobal;
  cvFactoryRegistrar* adaptObjectRegistrar = tmp->registrar;
  
  // Register this particular factory method with the main app.
  if (adaptObjectRegistrar != NULL) {
      adaptObjectRegistrar->SetFactoryMethodPtr( KERNEL_TETGEN, (FactoryMethodPtr)&pyCreateTetGenAdapt);
  } else {
    return SV_PYTHON_ERROR;
  }
  
  tmp->registrar = adaptObjectRegistrar;
  PySys_SetObject("AdaptModelRegistrar",(PyObject*)tmp);

  PyObject* pythonC;
  #if PYTHON_MAJOR_VERSION == 2
  pythonC = Py_InitModule("pyTetGenAdapt", TetGenAdapt_methods);
  #elif PYTHON_MAJOR_VERSION == 3
  pythonC = PyModule_Create(&pyTetGenAdaptmodule);
  #endif

  if (pythonC == NULL) {
    fprintf(stdout,"Error in initializing pyTetGenAdapt\n");
    return SV_PYTHON_ERROR;
  }

  return pythonC;
}
