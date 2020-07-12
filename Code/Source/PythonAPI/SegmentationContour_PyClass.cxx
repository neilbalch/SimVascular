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

// The functions defined here implement the SV Python API contour segmentation class. 
//
// The class name is 'segmentation.Contour'.
//

//----------------------
// PyContourSegmentation 
//----------------------
// Define the Contour class.
//
typedef struct {
  PySegmentation super;
} PyContourSegmentation;

//////////////////////////////////////////////////////
//        U t i l i t y     F u n c t i o n s       //
//////////////////////////////////////////////////////

//------------------------------
// PyContourCopySegmentationData
//------------------------------
//
void PyContourCopySegmentationData(sv3::Contour* contour, sv4guiContour* sv4Contour)
{
  PySegmentationCopySv4ContourData(sv4Contour, contour);
}

//////////////////////////////////////////////////////
//          C l a s s    M e t h o d s              //
//////////////////////////////////////////////////////
//
// Python API functions. 

////////////////////////////////////////////////////////
//          C l a s s    D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* SEGMENTATION_CONTOUR_CLASS = "Contour";
static char* SEGMENTATION_CONTOUR_MODULE_CLASS = "segmentation.Contour";

PyDoc_STRVAR(PyContourSegmentationClass_doc, 
   "Contour(points)  \n\
   \n\
   The ContourSegmentation class provides an interface for creating a contour segmentation. \n\
   A contur segmentation is defined by a set of 3D points. \n\
   \n\
   Args: \n\
     points(list([float,float,float]): The list of 3D points defining the contour. \n\
   \n\
");

//------------------------------
// PyContourSegmentationMethods 
//------------------------------
//
static PyMethodDef PyContourSegmentationMethods[] = {
  {NULL, NULL}
};

//---------------------------
// PyContourSegmentationInit 
//---------------------------
// This is the __init__() method for the ContourSegmentation class. 
//
// This function is used to initialize an object after it is created.
//
// A 'radius' and 'normal or 'frame' arguments are required.
//
static int
PyContourSegmentationInit(PyContourSegmentation* self, PyObject* args, PyObject *kwargs)
{
  std::cout << "[PyContourSegmentationInit] ========== Init Contour Segmentation object ========== " << std::endl;
  std::cout << "[PyContourSegmentationInit] kwargs: " << kwargs << std::endl;

  auto api = PyUtilApiFunction("|O!", PyRunTimeErr, "ContourSegmentation");
  static char *keywords[] = {"points", NULL};
  PyObject* pointsArg = nullptr;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &PyList_Type, &pointsArg)) {
      return -1;
  }

  // If keyword args have been given.
  //
  if (kwargs != nullptr) {
  }

  // Create the Contour object.
  self->super.contour = new sv3::Contour();
  self->super.CopySv4ContourData = PyContourCopySegmentationData; 

  // Set contour data if it has been given.
  //
  if (kwargs != nullptr) {
  }

  std::cout << "[PyContourSegmentationInit] Done " << std::endl;
  return 0;
}

//--------------------------
// PyContourSegmentationNew 
//--------------------------
//
static PyObject *
PyContourSegmentationNew(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{ 
  std::cout << "[PyContourSegmentationNew] New ContourSegmentation " << std::endl;
  auto self = (PyContourSegmentation*)type->tp_alloc(type, 0);
  if (self == NULL) {
      std::cout << "[PyContourSegmentationNew] ERROR: alloc failed." << std::endl;
      return nullptr;
  }
  return (PyObject*)self;
}

//------------------------------
// PyContourSegmentationDealloc 
//------------------------------
//
static void
PyContourSegmentationDealloc(PyContourSegmentation* self)
{ 
  std::cout << "[PyContourSegmentationDealloc] **** Free PyContourSegmentation ****" << std::endl;
  delete self->super.contour;
  Py_TYPE(self)->tp_free(self);
}

//--------------------------------------
// Define the PyContourSegmentationType 
//--------------------------------------
// Define the Python type object that stores Segmentation data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static PyTypeObject PyContourSegmentationType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  .tp_name = SEGMENTATION_CONTOUR_MODULE_CLASS, 
  .tp_basicsize = sizeof(PyContourSegmentation)
};

//----------------------------------
// SetContourSegmentationTypeFields 
//----------------------------------
// Set the Python type object fields that stores Segmentation data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetContourSegmentationTypeFields(PyTypeObject& segType)
 {
  // Doc string for this type.
  segType.tp_doc = PyContourSegmentationClass_doc; 

  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  segType.tp_new = PyContourSegmentationNew;
  //.tp_new = PyType_GenericNew,

  segType.tp_base = &PySegmentationType;

  segType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  segType.tp_init = (initproc)PyContourSegmentationInit;
  segType.tp_dealloc = (destructor)PyContourSegmentationDealloc;
  segType.tp_methods = PyContourSegmentationMethods;
};


