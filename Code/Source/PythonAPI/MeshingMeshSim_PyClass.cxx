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

// The functions defined here implement the SV Python API meshing module MeshSim mesh generator class. 
//
// The class name is 'meshing.MeshSim'.

//----------------------
// PyMeshingMeshSimClass
//----------------------
// Define the PyMeshingMeshSimClass class.
//
typedef struct {
  PyMeshingMesherClass super;
} PyMeshingMeshSimClass;

CreateMesherObjectFunction PyCreateMeshSimObject = nullptr;

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////
//              C l a s s   F u n c t i o n s                  //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the PyMeshingMeshSimClass class. 

////////////////////////////////////////////////////////
//           C l a s s    D e f i n i t i o n         //
////////////////////////////////////////////////////////

static char* MESHING_MESHSIM_CLASS = "MeshSim";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* MESHING_MESHSIM_MODULE_CLASS = "meshing.MeshSim";

PyDoc_STRVAR(PyMeshingMeshSimClass_doc, "MeshSim mesh generator class methods.");

//-------------------------
// PyMeshingMeshSimMethods
//-------------------------
//
static PyMethodDef PyMeshingMeshSimMethods[] = {
  {NULL, NULL}
};

//----------------------
// PyMeshingMeshSimInit 
//----------------------
// This is the __init__() method for the MeshGenerator class. 
//
// This function is used to initialize an object after it is created.
//
static int 
PyMeshingMeshSimInit(PyMeshingMeshSimClass* self, PyObject* args, PyObject *kwds)
{
  std::cout << "[PyMeshingMeshSimInit] New MeshSim object: " << std::endl;
/* [TODO:DaveP] what goes here?
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, "MeshGenerator");
  static int numObjs = 1;
  std::cout << "[PyMeshingMeshSimClassInit] New PyMeshingMeshSimClass object: " << numObjs << std::endl;
  self->super.mesher = new cvMeshSimMeshObject();
*/
  return 0;
}

//---------------------
// PyMeshingMeshSimNew 
//---------------------
//
static PyObject *
PyMeshingMeshSimNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyMeshingMeshSimNew] PyMeshingMeshSimNew " << std::endl;
  auto self = (PyMeshingMesherClass*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->super.id = 2;
  }
  return (PyObject*)self;
}

//-------------------------
// PyMeshingMeshSimDealloc 
//-------------------------
//
static void
PyMeshingMeshSimDealloc(PyMeshingMeshSimClass* self)
{
  std::cout << "[PyMeshingMeshSimDealloc] Free PyMeshingMeshSim" << std::endl;
  delete self->super.mesher;
  Py_TYPE(self)->tp_free(self);
}

//---------------------------
// PyMeshingMeshSimClassType 
//---------------------------
// Define the Python type object that stores PolyDataSolidClass data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
PyTypeObject PyMeshingMeshSimClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  .tp_name = MESHING_MESHSIM_MODULE_CLASS,
  .tp_basicsize = sizeof(PyMeshingMeshSimClass)
};

//-----------------------------
// SetMeshingMeshSimTypeFields 
//-----------------------------
// Set the Python type object fields that stores MeshSim mesher data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
void
SetMeshingMeshSimTypeFields(PyTypeObject& mesherType)
 {
  // Doc string for this type.
  mesherType.tp_doc = PyMeshingMeshSimClass_doc;

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  mesherType.tp_new = PyMeshingMeshSimNew;
  //.tp_new = PyType_GenericNew,

  // Subclass to PyMeshingMesherClassType.
  mesherType.tp_base = &PyMeshingMesherClassType; 

  mesherType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  mesherType.tp_init = (initproc)PyMeshingMeshSimInit;
  mesherType.tp_dealloc = (destructor)PyMeshingMeshSimDealloc;
  mesherType.tp_methods = PyMeshingMeshSimMethods;
};

//-------------------
// PyAPI_InitMeshSim 
//-------------------
// Setup creating MeshSim mesh generation objects.
//
// This is called from the MeshSim plugin Python API code.
//
void
PyAPI_InitMeshSim(CreateMesherObjectFunction create_object)
{
  // Set the function to create Parasolid modeling objects.
  PyCreateMeshSimObject = create_object;

  // Add a method to create a Parasolid modeling object.
  CvMesherCtorMap[cvMeshObject::KERNEL_MESHSIM] = []()-> cvMeshObject*{ return PyCreateMeshSimObject(); };
}

