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

// The functions defined here implement the SV Python API meshing module TetGen mesh generator class. 
//
// The class name is 'MeshingTetGen'.

//----------------------
// PyMeshingTetGenClass
//----------------------
// Define the PyMeshingTetGenClass class.
//
typedef struct {
  PyMeshingMesherClass super;
} PyMeshingTetGenClass;

/////////////////////////////////////////////////////////////////
//              C l a s s   F u n c t i o n s                  //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the PyMeshingTetGenClass class. 

//-------------------------
// PolyDataSolid_available
//-------------------------
//
static PyObject *
MeshingTetGen_available(PyObject* self, PyObject* args )
{
  return Py_BuildValue("s", "The TetGen mesh generator is available");
}

//------------------------------
// MeshingTetGen_create_options
//------------------------------
//
static PyObject *
//static PyMeshingTetGenOptionsClass *
MeshingTetGen_create_options(PyObject* self, PyObject* args )
{
  return CreateTetGenOptionsType();
}

//---------------------------
// MeshingTetGen_set_options
//---------------------------
//
static PyObject *
MeshingTetGen_set_options(PyObject* self, PyObject* args )
{
  std::cout << "[MeshingTetGen_set_options] " << std::endl;
  std::cout << "[MeshingTetGen_set_options] ========== MeshingTetGen_set_options =========" << std::endl;
  auto api = SvPyUtilApiFunction("O!", PyRunTimeErr, __func__);
  PyObject* options;

  if (!PyArg_ParseTuple(args, api.format, &PyTetGenOptionsType, &options)) {
      return api.argsError();
  }

  double global_edge_size = PyTetGenOptionsGetDouble(options, TetGenOption::PY_GLOBAL_EDGE_SIZE);

  std::cout << "[MeshingTetGen_set_options] global_edge_size: " << global_edge_size << std::endl;

  Py_RETURN_NONE;
}

////////////////////////////////////////////////////////
//           C l a s s    D e f i n i t i o n         //
////////////////////////////////////////////////////////

static char* MESHING_TETGEN_CLASS = "TetGen";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* MESHING_TETGEN_MODULE_CLASS = "meshing.TetGen";

PyDoc_STRVAR(PyMeshingTetGenClass_doc, "TetGen mesh generator class methods.");

//------------------------
// PyMeshingTetGenMethods
//------------------------
//
static PyMethodDef PyMeshingTetGenMethods[] = {
  {"available", (PyCFunction)MeshingTetGen_available, METH_VARARGS, NULL},
  {"create_options", (PyCFunction)MeshingTetGen_create_options, METH_VARARGS, NULL},
  {"set_options", (PyCFunction)MeshingTetGen_set_options, METH_VARARGS, NULL},
  {NULL, NULL}
};

//---------------------
// PyMeshingTetGenInit 
//---------------------
// This is the __init__() method for the MeshGenerator class. 
//
// This function is used to initialize an object after it is created.
//
static int 
PyMeshingTetGenInit(PyMeshingTetGenClass* self, PyObject* args, PyObject *kwds)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, "MeshGenerator");
  static int numObjs = 1;
  std::cout << "[PyMeshingTetGenClassInit] New PyMeshingTetGenClass object: " << numObjs << std::endl;
  self->super.mesher = new cvTetGenMeshObject();
  numObjs += 1;
  return 0;
}

//--------------------
// PyMeshingTetGenNew 
//--------------------
//
static PyObject *
PyMeshingTetGenNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyMeshingTetGenNew] PyMeshingTetGenNew " << std::endl;
  auto self = (PyMeshingMesherClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->super.id = 2;
  }
  return (PyObject*)self;
}

//------------------------
// PyMeshingTetGenDealloc 
//------------------------
//
static void
PyMeshingTetGenDealloc(PyMeshingTetGenClass* self)
{
  std::cout << "[PyMeshingTetGenDealloc] Free PyMeshingTetGen" << std::endl;
  delete self->super.mesher;
  Py_TYPE(self)->tp_free(self);
}

//--------------------------
// PyMeshingTetGenClassType 
//--------------------------
// Define the Python type object that stores PolyDataSolidClass data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
PyTypeObject PyMeshingTetGenClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  .tp_name = MESHING_TETGEN_MODULE_CLASS,
  .tp_basicsize = sizeof(PyMeshingTetGenClass)
};

//----------------------------
// SetMeshingTetGenTypeFields 
//----------------------------
// Set the Python type object fields that stores TetGen mesher data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
void
SetMeshingTetGenTypeFields(PyTypeObject& mesherType)
 {
  // Doc string for this type.
  mesherType.tp_doc = PyMeshingTetGenClass_doc;

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  mesherType.tp_new = PyMeshingTetGenNew;
  //.tp_new = PyType_GenericNew,

  // Subclass to PyMeshingMesherClassType.
  mesherType.tp_base = &PyMeshingMesherClassType; 

  mesherType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  mesherType.tp_init = (initproc)PyMeshingTetGenInit;
  mesherType.tp_dealloc = (destructor)PyMeshingTetGenDealloc;
  mesherType.tp_methods = PyMeshingTetGenMethods;
};

