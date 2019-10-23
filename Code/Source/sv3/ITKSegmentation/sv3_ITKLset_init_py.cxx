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

// The code here defines the Python API Itk level set module. 
//
// The name of the module is 'itk_levelset'.

#include "SimVascular.h"

#include "sv3_ITKLset_init_py.h"
#include "sv3_ITKUtils_init_py.h"
#include "sv3_ITKLset2d_init_py.h"
#include "sv3_ITKLset3d_init_py.h"
#include "Python.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "itk_levelset";
static char* MODULE_EXCEPTION = "itk_levelset.ItkLevelSetException";
static char* MODULE_EXCEPTION_OBJECT = "ItkLevelSetException";

PyDoc_STRVAR(ItkLevelSet_doc, "itk_levelset module functions");

// No module methods.
//
PyMethodDef pyItkls_methods[] = {
    {NULL, NULL,0,NULL},
};

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3

PyMODINIT_FUNC PyInit_pyItkls(void);

// Size of per-interpreter state of the module.
// Set to -1 if the module keeps state in global variables. 
static int perInterpreterStateSize = -1;

// Always initialize this to PyModuleDef_HEAD_INIT.
static PyModuleDef_Base m_base = PyModuleDef_HEAD_INIT;

// Define the module definition struct which holds all information 
// needed to create a module object. 
static struct PyModuleDef pyItklsmodule = {
   m_base,
   MODULE_NAME, 
   ItkLevelSet_doc,
   perInterpreterStateSize,  
   pyItkls_methods
};

//----------------
// Itklset_pyInit
//----------------
// [TODO:DaveP] what is this function used for?
//
int Itklset_pyInit()
{
  PyInit_pyItkls();
  return SV_OK;
}

//----------------
// PyInit_pyItkls 
//----------------
// The initialization function called by the Python interpreter 
// when the module is loaded.
//
// This function is used in Application/SimVascular_Init_py.cxx.
//
PyMODINIT_FUNC
PyInit_pyItkls(void)
{
    // Create the module.
    auto module = PyModule_Create(&pyItklsmodule);

    // Add itk_levelset.ItkLevelSetException exception.
    PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION, NULL, NULL);
    Py_INCREF(PyRunTimeErr);
    PyModule_AddObject(module, MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

    // Add sub-modules?
    //
    PyObject* pyItkls2D = Itkls2d_pyInit();
    Py_INCREF(pyItkls2D);
    PyModule_AddObject(module, "Itkls2d", pyItkls2D);

    PyObject* pyItkls3D = Itkls3d_pyInit();
    Py_INCREF(pyItkls3D);
    PyModule_AddObject(module, "Itkls3d", pyItkls3D);

    PyObject* pyItkUtils=Itkutils_pyInit();
    Py_INCREF(pyItkUtils);
    PyModule_AddObject(module,"Itkutils",pyItkUtils);

    return module;
}
#endif


//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC initpyItkls(void);

int Itklset_pyInit()
{
  initpyItkls();
  return SV_OK;
}

//-------------
// initpyItkls
//-------------
//
PyMODINIT_FUNC
initpyItkls(void)
{
    PyObject* pyItklsm;
    pyItklsm=Py_InitModule("pyItkls",pyItkls_methods);

    PyRunTimeErr = PyErr_NewException("pyItkls.error",NULL,NULL);
    Py_INCREF(PyRunTimeErr);
    PyModule_AddObject(pyItklsm,"error",PyRunTimeErr);

    PyObject* pyItkls2D=Itkls2d_pyInit();
    Py_INCREF(pyItkls2D);
    PyModule_AddObject(pyItklsm,"Itkls2d",pyItkls2D);

    PyObject* pyItkls3D=Itkls3d_pyInit();
    Py_INCREF(pyItkls3D);
    PyModule_AddObject(pyItklsm,"Itkls3d",pyItkls3D);

    PyObject* pyItkUtils=Itkutils_pyInit();
    Py_INCREF(pyItkUtils);
    PyModule_AddObject(pyItklsm,"Itkutils",pyItkUtils);
}

#endif

