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

// The code here defines the Python API Itk 2D level set module. 
//
// The name of the module is 'itk_levelset'.
//
// An class named 'levelset2d.LevelSet2D' is defined.
//
// [TODO:DaveP] I've not fully implemented this interface
// because I'm not sure to expose it or not.
//
/*
 * cv_itkls2d_init.cxx
 *
 *  Created on: Feb 12, 2014
 *      Author: Jameson Merkow
 */

#include "SimVascular.h"
#include "SimVascular_python.h"
#include "sv3_ITKLSet_PYTHON_Macros.h"
#include "Python.h"
#include "sv_PyUtils.h"

#include "sv2_LsetCore_init.h"
#include "sv3_ITKLevelSet.h"
#include "sv3_ITKLevelSetBase.h"
#include "sv2_LevelSetVelocity.h"
#include "sv_Repository.h"
#include "sv_SolidModel.h"
#include "sv_misc_utils.h"
#include "sv_arg.h"
#include "sv2_globals.h"

#include "sv3_ITKLset_ITKUtils.h"
#include "sv3_ITKLset2d_init_py.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

typedef itk::Image<short,2> ImageType;

//------------
// pyLevelSet
//------------
// Stores data for the Python LevelSet2D class.
//
typedef struct
{
  PyObject_HEAD
  cvITKLevelSet* ls;
} pyLevelSet;

//////////////////////////////////////////////////////
//        U t i l i t y     F u n c t i o n s       //
//////////////////////////////////////////////////////

//---------
// NewName
//---------
// NewName: Checks if the name is already used
//
static int 
NewName( CONST84 char *name )
{
  Tcl_HashEntry *entryPtr;
  entryPtr = Tcl_FindHashEntry( &gLsetCoreTable, name );
  if ( entryPtr != NULL ) {
  	return 0;
  }
  return 1;
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 
 
//-----------------------------
// itkls2d_new_levelset_object 
//-----------------------------
//
PyDoc_STRVAR(itkls2d_new_levelset_object_doc,
  "new_levelset_object(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static pyLevelSet * 
itkls2d_new_levelset_object(pyLevelSet* self, PyObject* args )
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  CONST84 char *lsName;

  cvITKLevelSet *ls;
  Tcl_HashEntry *entryPtr;
  int newEntry = 0;

  if (!PyArg_ParseTuple(args, api.format, &lsName)) {
    api.argsError();
    return nullptr; 
  }

  // Make sure this is a new object name:
  if (!NewName(lsName)) {
    api.error("The level set object '"+std::string(lsName)+"' is already in the repository.");
    return nullptr;
  }

  // Allocate new cvLevelSet object:
  //
  // [TODO:DaveP] do we realy need to check this?
  //
  ls = new cvITKLevelSet;
  if (ls == NULL) {
    api.error("Error creating level set object '"+std::string(lsName)+"'.");
    return nullptr;
  }

  // [TODO:DaveP] what the heck is this? Tcl_CreateHashEntry?
  //
  strcpy(ls->tclName_, lsName);
  entryPtr = Tcl_CreateHashEntry( &gLsetCoreTable, lsName, &newEntry );
  if (!newEntry) {
  	PyErr_SetString(PyRunTimeErr, "error updating cvLevelSet hash table");
  	delete ls;
        api.error("Error adding '"+std::string(lsName)+"' to the repository.");
        return nullptr;
  }

  Tcl_SetHashValue( entryPtr, (ClientData)ls );

  Py_INCREF(ls);
  self->ls=ls;
  Py_DECREF(ls);
  return self;
}

// --------------
// DeleteLsetCore
// --------------
// Deletion callback invoked when the Tcl object is deleted.  Delete
// Tcl hash table entry as well as the cvITKLevelSet object itself.
//
PyDoc_STRVAR(itkls2d_delete_levelset_object_doc,
  "delete_levelset_object(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
itkls2d_delete_levelset_object(pyLevelSet* self, PyObject* args)
{
  cvITKLevelSet *ls = self->ls;
  Tcl_HashEntry *entryPtr;

  entryPtr = Tcl_FindHashEntry( &gLsetCoreTable, ls->tclName_ );
  if ( entryPtr == NULL ) {
      printf("Error looking up LsetCore object %s for deletion.\n", ls->tclName_);
      return nullptr;
  } else {
  	Tcl_DeleteHashEntry( entryPtr );
  }
  delete ls;
  return SV_PYTHON_OK;
}

//--------------------
// itkls2d_set_inputs
//--------------------
//
PyDoc_STRVAR(itkls2d_set_inputs_doc,
  "set_inputs(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls2d_set_inputs(pyLevelSet* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *inputImageName;
  char *seedPdName;

  if(!PyArg_ParseTuple(args, api.format, &inputImageName, &seedPdName)) {
    return api.argsError();
  }

  cvITKLevelSet *ls = self->ls;
  RepositoryDataT typeImg1;
  char *typeImg1Str;
  typeImg1 = gRepository->GetType( inputImageName );
  typeImg1Str = RepositoryDataT_EnumToStr( typeImg1 );
  cvRepositoryData *inputImage;
  if (inputImageName != NULL) {
  	// Look up given image object:
  	inputImage = gRepository->GetObject( inputImageName );
  	if ( inputImage == NULL ) {
  		char temp[2048];
  		sprintf(temp,"couldn't find object ", inputImageName, (char *)NULL );
  		PyErr_SetString(PyRunTimeErr, temp );
  		return nullptr;
  		
  	}
  	printf("Found Object\n");
  	// Make sure image is of type STRUCTURED_PTS_T:
  	typeImg1 = inputImage->GetType();
  	if ( typeImg1 != STRUCTURED_PTS_T ) {
  		char temp[2048];
  		sprintf(temp,"error: object ", inputImageName, "not of type StructuredPts", (char *)NULL);
  		PyErr_SetString(PyRunTimeErr, temp );
  		return nullptr;
  		
  	}
  }


  RepositoryDataT typeImg2;
  char *typeImg2Str;
  typeImg2 = gRepository->GetType( seedPdName );
  typeImg2Str = RepositoryDataT_EnumToStr( typeImg2 );
  cvRepositoryData *seedPolyData;

  if (seedPdName != NULL) {
  	// Look up given image object:
  	seedPolyData = gRepository->GetObject( seedPdName );
  	if ( seedPolyData == NULL ) {
  		char temp[2048];
  		sprintf(temp,"couldn't find object ", seedPdName, (char *)NULL );
  		PyErr_SetString(PyRunTimeErr, temp );
  		return nullptr;
  		
  	}
  	printf("Found Object\n");
  	// Make sure image is of type STRUCTURED_PTS_T:
  	typeImg2 = seedPolyData->GetType();
  	if ( typeImg2 != POLY_DATA_T) {
  		char temp[2048];
  		sprintf(temp,"error: object ", seedPdName, "not of type PolyData", (char *)NULL);
  		PyErr_SetString(PyRunTimeErr, temp );
  		return nullptr;
  	}
  }

  ls->SetInputImage((cvStrPts*)inputImage);
  ls->SetSeed((cvPolyData*)seedPolyData);


  return SV_PYTHON_OK;
}

//----------------------------
// itkls2d_phase_one_levelset
//----------------------------
//
PyDoc_STRVAR(itkls2d_phase_one_levelset_doc,
  "phase_one_levelset(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
itkls2d_phase_one_levelset( pyLevelSet* self, PyObject* args  )
{
  auto api = SvPyUtilApiFunction("ddd|dd", PyRunTimeErr, __func__);
  double kc; 
  double expFactorRising;
  double expFactorFalling;
  double sigmaFeat = -1;
  double advScale;

  double sigmaAdv = -1;

  if (!PyArg_ParseTuple(args, api.format,&kc,&expFactorRising,&expFactorFalling, &sigmaFeat,&sigmaAdv)) {
    return api.argsError();
  }

  cvITKLevelSet *ls=self->ls ;

  if(sigmaFeat >= 0)
  	ls->SetSigmaFeature(sigmaFeat);
  if(sigmaAdv >= 0)
  	ls->SetSigmaAdvection(sigmaAdv);

  ls->ComputePhaseOneLevelSet(kc, expFactorRising,expFactorFalling);

  return SV_PYTHON_OK;
}

//
PyDoc_STRVAR(itkls2d_phase_two_levelset_doc,
  "phase_two_levelset(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls2d_phase_two_levelset(pyLevelSet* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("dd|dd", PyRunTimeErr, __func__);
  double klow, kupp;
  double sigmaFeat = -1, sigmaAdv = -1;

  if (!PyArg_ParseTuple(args, api.format, &klow, &kupp, &sigmaFeat, &sigmaAdv)) {
    return api.argsError();
  }

  cvITKLevelSet *ls=self->ls;

  if(sigmaFeat >= 0)
  	ls->SetSigmaFeature(sigmaFeat);

  if(sigmaAdv >= 0)
  	ls->SetSigmaAdvection(sigmaAdv);

  ls->ComputePhaseTwoLevelSet(kupp,klow);

  return SV_PYTHON_OK;
}

//----------------------
// itkls2d_gac_levelset 
//----------------------
//
// [TODO:DaveP] this does not appear to be used.
//
PyDoc_STRVAR(itkls2d_gac_levelset_doc,
  "gac_levelset(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls2d_gac_levelset(pyLevelSet* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("d|d", PyRunTimeErr, __func__);
  char *usage;
  double sigma, expFactor;

  if (!PyArg_ParseTuple(args, api.format, &expFactor, &sigma)) {
    return api.argsError();
  }

  cvITKLevelSet *ls=self->ls;
  ls->ComputeGACLevelSet(expFactor);

  return SV_PYTHON_OK;
}


PyDoc_STRVAR(Mesh_new_object_doc,
  "new_object(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls2d_WriteFrontMtd(pyLevelSet* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssdd|s", PyRunTimeErr, __func__);
  cvITKLevelSet *ls=self->ls;
  ls->WriteFrontImages();
  return SV_PYTHON_OK;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "itk_levelset2d";
static char* MODULE_EXCEPTION = "itk_levelset2d.LevelSet2D";
static char* MODULE_EXCEPTION_OBJECT = "LevelSet2D";
static char* MODULE_LEVELSET2D_CLASS = "LevelSet2D";
static char* MODULE_LEVELSET2D_CLASS_NAME = "itk_levelset2d.LevelSet2D";

PyDoc_STRVAR(LevelSet_doc, "itk_levelset2d module functions.");

static int pyLevelSet_init(pyLevelSet* self, PyObject* args)
{
  fprintf(stdout,"pyLevelSet initialized.\n");
  return SV_OK;
}

PyMethodDef pyLevelSet_methods[] = {

    {"delete_levelset_object",(PyCFunction)itkls2d_delete_levelset_object, METH_NOARGS, itkls2d_delete_levelset_object_doc},

    {"gac_levelset", (PyCFunction)itkls2d_gac_levelset, METH_VARARGS, itkls2d_gac_levelset_doc},

    {"new_levelset_object", (PyCFunction)itkls2d_new_levelset_object, METH_VARARGS, itkls2d_new_levelset_object_doc},

    {"phase_one_levelset", (PyCFunction)itkls2d_phase_one_levelset, METH_VARARGS, itkls2d_phase_one_levelset_doc},

    {"phase_two_levelset", (PyCFunction)itkls2d_phase_two_levelset, METH_VARARGS, itkls2d_phase_two_levelset_doc},

    {"set_inputs", (PyCFunction)itkls2d_set_inputs, METH_VARARGS, itkls2d_set_inputs_doc},

    {NULL, NULL,0,NULL},
};

PyMethodDef Itkls2d_methods[] = {
    {NULL, NULL,0,NULL},
};

//----------------
// pyLevelSetType
//----------------
// Define the 'levelset.LevelSet' class.
//
static PyTypeObject pyLevelSetType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  MODULE_LEVELSET2D_CLASS_NAME, /* tp_name */
  sizeof(pyLevelSet),           /* tp_basicsize */
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
  Py_TPFLAGS_DEFAULT |
      Py_TPFLAGS_BASETYPE,   /* tp_flags */
  "LevelSet objects",           /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  pyLevelSet_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)pyLevelSet_init,       /* tp_init */
  0,                         /* tp_alloc */
  0,                  /* tp_new */
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
static struct PyModuleDef Itkls2dmodule = {
   m_base,
   MODULE_NAME, 
   LevelSet_doc, 
   perInterpreterStateSize, 
   Itkls2d_methods
};

#endif

//--------------
// Itkls2d_Init
//--------------
//
PyObject * 
Itkls2d_pyInit()
{
  pyLevelSetType.tp_new = PyType_GenericNew;

  if (PyType_Ready(&pyLevelSetType)<0) {
      fprintf(stdout,"Error in pyLevelSetType\n");
  }
    
  #if PYTHON_MAJOR_VERSION == 2
  auto module = = Py_InitModule("Itkls2d",Itkls2d_methods);
  #elif PYTHON_MAJOR_VERSION == 3
  auto module = PyModule_Create(&Itkls2dmodule);
  #endif

  PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, MODULE_EXCEPTION_OBJECT, PyRunTimeErr);
  Py_INCREF(&pyLevelSetType);

  // Add the LevelSet2D class.
  PyModule_AddObject(module, MODULE_LEVELSET2D_CLASS, (PyObject*)&pyLevelSetType);

  return module;
}



