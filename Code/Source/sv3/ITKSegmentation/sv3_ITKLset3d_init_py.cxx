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
// 
// The code here defines the Python API Itk 3D level set module. 
//
// The name of the module is 'itk_levelset'.
//
// [TODO:DaveP] I've not fully implemented this interface
// because I'm not sure to expose it or not.

/*
 * cv_itkls3d_init.cxx
 *
 *  Created on: Feb 12, 2014
 *      Author: Jameson Merkow
 */

#include "SimVascular.h"
#include "SimVascular_python.h"
#include "Python.h"
#include "sv3_ITKLSet_PYTHON_Macros.h"
#include "sv_PyUtils.h"

#include "sv2_LsetCore_init.h"
#include "sv3_ITKLevelSet.h"
#include "sv3_ITKLevelSetBase.h"
#include "sv2_LevelSetVelocity.h"
#include "sv_Repository.h"
#include "sv_SolidModel.h"
#include "sv_misc_utils.h"
#include "sv_arg.h"

#include "sv3_ITKLset_ITKUtils.h"
#include "sv2_globals.h"
#include "sv3_ITKLset3d_init_py.h"
#include "sv2_globals.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

PyObject *PyRunTimeErr3d;

typedef itk::Image<short,3> ImageType;

//--------------
// pyLevelSet3d
//--------------
// Stores data for the Python LevelSet3D class.
//

typedef struct
{
  PyObject_HEAD
  cvITKLevelSetBase<ImageType> *ls;
} pyLevelSet3d;

//////////////////////////////////////////////////////
//        U t i l i t y     F u n c t i o n s       //
//////////////////////////////////////////////////////

// NewName: Checks if the name is already used
static int NewName( CONST84 char *name )
{
  Tcl_HashEntry *entryPtr;
  entryPtr = Tcl_FindHashEntry( &gLsetCoreTable, name );
  if ( entryPtr != NULL ) {
    return 0;
  }
  return 1;
}

//-----------------------------
// itkls3d_new_levelset_object 
//-----------------------------
//
PyDoc_STRVAR(itkls3d_new_levelset_object_doc,
  "itkls3d_new_levelset_object(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static pyLevelSet3d * 
itkls3d_new_levelset_object( pyLevelSet3d* self, PyObject* args)
{
  CONST84 char *lsName;
  cvITKLevelSetBase<ImageType> *ls;
  Tcl_HashEntry *entryPtr;
  int newEntry = 0;

  // Check syntax:
  if(!PyArg_ParseTuple(args, "s",&lsName))
  {
    PyErr_SetString(PyRunTimeErr3d,"Could not import 1 char, lsname");
    return nullptr;
  }

  // Make sure this is a new object name:
  if ( !NewName( lsName ) ) {
    PyErr_SetString(PyRunTimeErr3d, "ITKLevelSetCore object already exists");
    return nullptr;
    
  }

  // Allocate new cvLevelSet object:
  ls = new cvITKLevelSetBase<ImageType>;
  if ( ls == NULL ) {
    PyErr_SetString(PyRunTimeErr3d,"error allocating object");
    return nullptr;
    
  }

  strcpy( ls->tclName_, lsName );
  entryPtr = Tcl_CreateHashEntry( &gLsetCoreTable, lsName, &newEntry );
  if ( !newEntry ) {
    PyErr_SetString(PyRunTimeErr3d, "error updating cvLevelSet hash table");
    delete ls;
    return nullptr;
    
  }
  Tcl_SetHashValue( entryPtr, (ClientData)ls );
    Py_INCREF(ls);
    self->ls=ls;
    Py_DECREF(ls);
    return self;
}

//--------------------------------
// itkls3d_delete_levelset_object 
//--------------------------------
// Deletion callback invoked when the Tcl object is deleted.  Delete
// Tcl hash table entry as well as the cvITKLevelSet object itself.
//
PyDoc_STRVAR(itkls3d_delete_levelset_object_doc,
  "delete_levelset_object(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls3d_delete_levelset_object( pyLevelSet3d* self, PyObject* args )
{
  cvITKLevelSetBase<ImageType> *ls = self->ls;
  Tcl_HashEntry *entryPtr;

  entryPtr = Tcl_FindHashEntry( &gLsetCoreTable, ls->tclName_ );
  if ( entryPtr == NULL ) {
    printf("Error looking up LsetCore object %s for deletion.\n",
        ls->tclName_);
  } else {
    Tcl_DeleteHashEntry( entryPtr );
  }
  delete ls;
  return SV_PYTHON_OK;
}

//
PyDoc_STRVAR(itkls3d_set_inputs_doc,
  "set_inputs(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls3d_set_inputs(pyLevelSet3d* self, PyObject* args)
{
  cvITKLevelSetBase<ImageType> *ls=self->ls;
  char *inputImageName;
  char *seedPdName;

  if(!PyArg_ParseTuple(args, "ss",&inputImageName,&seedPdName)) {
    PyErr_SetString(PyRunTimeErr3d,"Could not import 2 chars");
    return nullptr;
  }

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
      PyErr_SetString(PyRunTimeErr3d, temp );
    return nullptr;
      
    }
    printf("Found Object\n");
    // Make sure image is of type STRUCTURED_PTS_T:
    typeImg1 = inputImage->GetType();
    if ( typeImg1 != STRUCTURED_PTS_T ) {
      char temp[2048];
      sprintf(temp,"error: object ", inputImageName, "not of type StructuredPts", (char *)NULL);
      PyErr_SetString(PyRunTimeErr3d, temp );
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
      PyErr_SetString(PyRunTimeErr3d, temp );
    return nullptr;
      
    }
    printf("Found Object\n");
    // Make sure image is of type STRUCTURED_PTS_T:
    typeImg2 = seedPolyData->GetType();
    if ( typeImg2 != POLY_DATA_T) {
      char temp[2048];
      sprintf(temp,"error: object ", seedPdName, "not of type PolyData", (char *)NULL);
      PyErr_SetString(PyRunTimeErr3d, temp );
    return nullptr;
      
    }
  }

  ls->SetInputImage((cvStrPts*)inputImage);
  ls->SetSeed((cvPolyData*)seedPolyData);


  return SV_PYTHON_OK;
}


//
PyDoc_STRVAR(itkls3d_phase_one_levelset_doc,
  "phase_one_levelset(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls3d_phase_one_levelset(pyLevelSet3d* self, PyObject* args)
{
  cvITKLevelSetBase<ImageType> *ls=self->ls;
  double kc, expFactorRising,expFactorFalling, advScale;

  double sigmaFeat = -1, sigmaAdv = -1;

  if (!PyArg_ParseTuple(args,"ddd|dd",&kc,&expFactorRising,&expFactorFalling,
  &sigmaFeat,&sigmaAdv))
    {
    PyErr_SetString(PyRunTimeErr3d,"Could not import 5 doubles");
    return nullptr;
    
  }
  std::cout << "sigmaFeat " << sigmaFeat << std::endl;

  if(sigmaFeat >= 0)
    ls->SetSigmaFeature(sigmaFeat);
  if(sigmaAdv >= 0)
    ls->SetSigmaAdvection(sigmaAdv);

  ls->ComputePhaseOneLevelSet(kc, expFactorRising,expFactorFalling);

  return SV_PYTHON_OK;
}

//
PyDoc_STRVAR(itkls3d_phase_two_levelset_doc,
  "phase_two_levelset(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls3d_phase_two_levelset(pyLevelSet3d* self, PyObject* args)
{
  cvITKLevelSetBase<ImageType> *ls=self->ls;
  char *usage;
  double klow, kupp;
  double sigmaFeat = -1, sigmaAdv = -1;

  if (!PyArg_ParseTuple(args,"dd|dd",&klow,&kupp,
  &sigmaFeat,&sigmaAdv))
    {
    PyErr_SetString(PyRunTimeErr3d,"Could not import 4 doubles");
    return nullptr;
    
    }


  //std::cout << "Entering LS" << std::endl;

  if(sigmaFeat >= 0)
    ls->SetSigmaFeature(sigmaFeat);
  if(sigmaAdv >= 0)
    ls->SetSigmaAdvection(sigmaAdv);


  ls->ComputePhaseTwoLevelSet(kupp,klow);

  return SV_PYTHON_OK;
}


//
PyDoc_STRVAR(itkls3d_gac_levelset_doc,
  "gac_levelset(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls3d_gac_levelset(pyLevelSet3d* self, PyObject* args)
{
  cvITKLevelSetBase<ImageType> *ls=self->ls;
  char *usage;
  double sigma, expFactor,kappa,iso;
  if (!PyArg_ParseTuple(args,"ddd|d",&expFactor,&kappa,&iso,&sigma))
  {
    PyErr_SetString(PyRunTimeErr3d,"Could not import 4 doubles");
    return nullptr;
    
  }

  if(sigma >= 0)
    ls->SetSigmaFeature(sigma);
  ls->ComputeGACLevelSet(expFactor,kappa,iso);

  return SV_PYTHON_OK;
}


//
PyDoc_STRVAR(itkls3d_laplacian_levelset_doc,
  "laplacian_levelset(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls3d_laplacian_levelset(pyLevelSet3d* self, PyObject* args)
{
  cvITKLevelSetBase<ImageType> *ls=self->ls;
  char *usage;
  double sigma, expFactor,kappa,iso;
  std::cout << "Laplacian" << std::endl;
  if (!PyArg_ParseTuple(args,"ddd|d",&expFactor,&kappa,&iso,&sigma))
  {
    PyErr_SetString(PyRunTimeErr3d,"Could not import 4 doubles");
    return nullptr;
    
  }

  if(sigma >= 0)
    ls->SetSigmaFeature(sigma);
  ls->ComputeLaplacianLevelSet(expFactor,kappa,iso);


  return SV_PYTHON_OK;
}


// this is not exposed.
static PyObject* itkls3d_WriteFrontMtd( pyLevelSet3d* self, PyObject* args )
{
  cvITKLevelSetBase<ImageType> *ls=self->ls;
  ls->WriteFrontImages();
  return SV_PYTHON_OK;
}


//
PyDoc_STRVAR(itkls3d_copy_front_to_seed_doc,
  "copy_front_to_seed(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkls3d_copy_front_to_seed(pyLevelSet3d* self, PyObject* args)
{
  cvITKLevelSetBase<ImageType> *ls=self->ls;
  ls->CopyFrontToSeed();
  return SV_PYTHON_OK;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "itk_levelset3d";
static char* MODULE_EXCEPTION = "itk_levelset23.LevelSet3D";
static char* MODULE_EXCEPTION_OBJECT = "LevelSet3D";
static char* MODULE_LEVELSET2D_CLASS = "LevelSet3D";
static char* MODULE_LEVELSET2D_CLASS_NAME = "itk_levelset3d.LevelSet3D";

PyDoc_STRVAR(LevelSet3d_doc, "itk_levelset3d module functions.");


static int pyLevelSet3d_init(pyLevelSet3d* self, PyObject* args)
{
  fprintf(stdout,"pyLevelSet3d initialized.\n");
  return SV_OK;
}

//----------------------
// pyLevelSet3d_methods
//----------------------
//
PyMethodDef pyLevelSet3d_methods[] = {

    {"new_levelset_object", (PyCFunction)itkls3d_new_levelset_object, METH_VARARGS, itkls3d_new_levelset_object_doc},

    {"delete_levelset_object",(PyCFunction)itkls3d_delete_levelset_object, METH_NOARGS, itkls3d_delete_levelset_object_doc},

    {"set_inputs", (PyCFunction)itkls3d_set_inputs, METH_VARARGS, itkls3d_set_inputs_doc},

    {"phase_one_levelset", (PyCFunction)itkls3d_phase_one_levelset, METH_VARARGS, itkls3d_phase_one_levelset_doc},

    {"phase_two_levelset", (PyCFunction)itkls3d_phase_two_levelset, METH_VARARGS, itkls3d_phase_two_levelset_doc},

    {"gac_levelset", (PyCFunction)itkls3d_gac_levelset, METH_VARARGS, itkls3d_gac_levelset_doc},

    {"laplacian_levelset",(PyCFunction)itkls3d_laplacian_levelset, METH_VARARGS, itkls3d_laplacian_levelset_doc},

    //{"WriteFront", (PyCFunction)itkls3d_WriteFrontMtd, METH_VARARGS,NULL},

    {"copy_front_to_seed",(PyCFunction)itkls3d_copy_front_to_seed, METH_VARARGS, itkls3d_copy_front_to_seed_doc},

    {NULL, NULL,0,NULL},
};

PyMethodDef Itkls3d_methods[] = {
    {NULL, NULL,0,NULL},
};

//------------------
// pyLevelSet3dType
//------------------
//
static PyTypeObject pyLevelSet3dType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  MODULE_LEVELSET2D_CLASS_NAME,  /* tp_name */
  sizeof(pyLevelSet3d),             /* tp_basicsize */
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
  "LevelSet3D objects",           /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  pyLevelSet3d_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)pyLevelSet3d_init,       /* tp_init */
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
static struct PyModuleDef Itkls3dmodule = {
   m_base,
   MODULE_NAME,   
   LevelSet3d_doc, 
   perInterpreterStateSize, 
   Itkls3d_methods
};

#endif

//-------------
// Itkls3d_Init
//-------------
//
PyObject* Itkls3d_pyInit()
{
  pyLevelSet3dType.tp_new=PyType_GenericNew;

  if (PyType_Ready(&pyLevelSet3dType)<0) {
      fprintf(stdout,"Error in pyLevelSet3dType\n");
  }

  #if PYTHON_MAJOR_VERSION == 2
  auto pyItkls3D = Py_InitModule("Itkls3d",Itkls3d_methods);
  #elif PYTHON_MAJOR_VERSION == 3
  auto pyItkls3D = PyModule_Create(&Itkls3dmodule);
  #endif

  PyRunTimeErr3d = PyErr_NewException(MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(pyItkls3D, MODULE_EXCEPTION_OBJECT,PyRunTimeErr3d);
  Py_INCREF(&pyLevelSet3dType);
  PyModule_AddObject(pyItkls3D, MODULE_LEVELSET2D_CLASS, (PyObject*)&pyLevelSet3dType);

  return pyItkls3D;
}
