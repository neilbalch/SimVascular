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

// The functions defined here implement the SV Python API polydata solid class. 
//
// The class name is 'PolyDataSolid'.
//

//-----------------
// PyPolyDataSolid 
//-----------------
// Define the PolyDataSolid class (type).
//
typedef struct {
  PySolidModelClass super;
} PyPolyDataSolid;

cvPolyDataSolid* pyCreatePolyDataSolid()
{
  return new cvPolyDataSolid();
}

//////////////////////////////////////////////////////
//          C l a s s    M e t h o d s              //
//////////////////////////////////////////////////////
// PolyDataSolid class methods. 

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
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* SOLID_POLYDATA_CLASS = "PolyData";
static char* SOLID_POLYDATA_MODULE_CLASS = "solid.PolyData";

PyDoc_STRVAR(PyPolyDataSolidClass_doc, "polydata solid model functions");

PyMethodDef PyPolyDataSolidMethods[] = {
  {"available", PolyDataSolid_available,METH_NOARGS,NULL},
  {"registrars", PolyDataSolid_registrars,METH_NOARGS,NULL},
  {NULL, NULL}
};

//---------------------
// PyPolyDataSolidInit 
//---------------------
// This is the __init__() method for the PolyDataSolid class. 
//
// This function is used to initialize an object after it is created.
//
static int
PyPolyDataSolidInit(PyPolyDataSolid* self, PyObject* args, PyObject *kwds)
{ 
  static int numObjs = 1;
  std::cout << "[PyPolyDataSolidInit] New PolyDataSolid object: " << numObjs << std::endl;
  self->super.solidModel = new cvPolyDataSolid();
  numObjs += 1;
  return 0;
}

//--------------------
// PyPolyDataSolidNew 
//--------------------
//
static PyObject *
PyPolyDataSolidNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyPolyDataSolidNew] PyPolyDataSolidNew " << std::endl;
  auto self = (PyPolyDataSolid*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->super.id = 2;
  }
  return (PyObject *) self;
}

//------------------------
// PyPolyDataSolidDealloc 
//------------------------
//
static void
PyPolyDataSolidDealloc(PyPolyDataSolid* self)
{
  std::cout << "[PyPolyDataSolidDealloc] Free PyPolyDataSolid" << std::endl;
  delete self->super.solidModel;
  Py_TYPE(self)->tp_free(self);
}

//--------------------------
// PyPolyDataSolidClassType 
//--------------------------
// Define the Python type object that stores PolyDataSolidClass data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
PyTypeObject PyPolyDataSolidClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  .tp_name = SOLID_POLYDATA_MODULE_CLASS,
  .tp_basicsize = sizeof(PyPolyDataSolid)
};

//----------------------------
// SetPolyDataSolidTypeFields
//----------------------------
// Set the Python type object fields that stores PolyDataSolid data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
void
SetPolyDataSolidTypeFields(PyTypeObject& solidType)
 {
  // Doc string for this type.
  solidType.tp_doc = PyPolyDataSolidClass_doc; 

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  solidType.tp_new = PyPolyDataSolidNew;
  //.tp_new = PyType_GenericNew,

  // Subclass to PyPolyDataSolid.
  solidType.tp_base = &PySolidModelClassType;

  solidType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  solidType.tp_init = (initproc)PyPolyDataSolidInit;
  solidType.tp_dealloc = (destructor)PyPolyDataSolidDealloc;
  solidType.tp_methods = PyPolyDataSolidMethods;
};


