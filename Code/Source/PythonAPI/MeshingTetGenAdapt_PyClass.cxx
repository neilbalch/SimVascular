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
// The class name is 'meshing.TetGenAdaptive'.

#include "sv_TetGenAdapt.h"

//---------------------------
// PyMeshingTetGenAdaptClass 
//---------------------------
// Define the PyMeshingTetGenAdaptClass class.
//
typedef struct {
  PyMeshingAdaptiveClass super;
} PyTetGenAdaptClass;

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//---------------------
// pyCreateTetGenAdapt
//---------------------
//
cvTetGenAdapt * 
pyCreateTetGenAdapt()
{
    return new cvTetGenAdapt();
}

/////////////////////////////////////////////////////////////////
//              C l a s s   F u n c t i o n s                  //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the PyMeshingTetGenAdaptClass class. 

static PyObject *  
TetGenAdapt_AvailableCmd(PyObject* self, PyObject* args)
{
  return Py_BuildValue("s","TetGen Adaption Available");
}

//------------------------------
// MeshingTetGen_create_options
//------------------------------
//
PyDoc_STRVAR(TetGenAdapt_create_options_doc,
  "create_options()  \n\ 
  \n\
  Create a TetGenAdaptiveOptions object. \n\
  \n\
  Args:                                    \n\
    global_edge_size (float): The value used to set the global_edge_size parameter. \n\
    surface_mesh_flag (int): The value used to set the surface_mesh_flag parameter. \n\
    volume_mesh_flag (int): The value used to set the volume_mesh_flag parameter. \n\
    mesh_wall_first (int): The value used to set the mesh_wall_first parameter. \n\
");

static PyObject *
TetGenAdapt_create_options(PyObject* self, PyObject* args, PyObject* kwargs )
{
  return CreateTetGenAdaptOptType(args, kwargs);
}

////////////////////////////////////////////////////////
//           C l a s s    D e f i n i t i o n         //
////////////////////////////////////////////////////////

static char* MESHING_TETGEN_ADAPTIVE_CLASS = "TetGenAdaptive";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* MESHING_TETGEN_ADAPTIVE_MODULE_CLASS = "meshing.TetGenAdaptive";

PyDoc_STRVAR(PyTetGenAdaptClass_doc, "TetGen adaptive mesh generator class methods.");

//--------------------
// TetGenAdaptMethods
//--------------------
//
PyMethodDef PyTetGenAdaptMethods[] = {

  {"Available", TetGenAdapt_AvailableCmd,METH_NOARGS,NULL},

  {"create_options", (PyCFunction)TetGenAdapt_create_options, METH_VARARGS|METH_KEYWORDS, TetGenAdapt_create_options_doc},

  {NULL, NULL}
};

//-------------------
// PyTetGenAdaptInit 
//-------------------
// This is the __init__() method for the MeshGenerator class. 
//
// This function is used to initialize an object after it is created.
//
static int 
PyTetGenAdaptInit(PyTetGenAdaptClass* self, PyObject* args, PyObject *kwds)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, "TetGen adaptive mesh generator");
  static int numObjs = 1;
  std::cout << "[PyTetGenAdaptClassInit] New PyTetGenAdaptClass object: " << numObjs << std::endl;
  self->super.adaptive_mesher = new cvTetGenAdapt();
  numObjs += 1;
  return 0;
}

//------------------
// PyTetGenAdaptNew 
//------------------
//
static PyObject *
PyTetGenAdaptNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyTetGenAdaptNew] PyTetGenAdaptNew " << std::endl;
  auto self = (PyMeshingAdaptiveClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->super.id = 2;
  }
  return (PyObject*)self;
}

//----------------------
// PyTetGenAdaptDealloc 
//----------------------
//
static void
PyTetGenAdaptDealloc(PyTetGenAdaptClass* self)
{
  std::cout << "[PyTetGenAdaptDealloc] Free PyTetGenAdapt" << std::endl;
  delete self->super.adaptive_mesher;
  Py_TYPE(self)->tp_free(self);
}

//------------------------
// PyTetGenAdaptClassType 
//------------------------
// Define the Python type object that stores TetGen adaptive meshing data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
PyTypeObject PyTetGenAdaptClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  .tp_name = MESHING_TETGEN_ADAPTIVE_MODULE_CLASS,
  .tp_basicsize = sizeof(PyTetGenAdaptClass)
};

//--------------------------
// SetTetGenAdaptTypeFields 
//--------------------------
// Set the Python type object fields that stores TetGen adaptive mesher data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
void
SetTetGenAdaptTypeFields(PyTypeObject& mesherType)
 {
  // Doc string for this type.
  mesherType.tp_doc = PyTetGenAdaptClass_doc;

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  mesherType.tp_new = PyTetGenAdaptNew;
  //.tp_new = PyType_GenericNew,

  // Subclass to PyMeshingMesherClassType.
  mesherType.tp_base = &PyMeshingMesherClassType; 

  mesherType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  mesherType.tp_init = (initproc)PyTetGenAdaptInit;
  mesherType.tp_dealloc = (destructor)PyTetGenAdaptDealloc;
  mesherType.tp_methods = PyTetGenAdaptMethods;
};


