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

// The functions defined here implement the SV Python API Contour Module. 
//
// The module name is 'pyContour' but is imported as 'Contour' so functions
// are called as follows 
//
//   Contour.Create(...)
//
//
#include "SimVascular.h"
#include "SimVascular_python.h"
#include "sv3_PathElement.h"
#include "sv3_Contour.h"
#include "sv3_LevelSetContour.h"
#include "sv3_Contour_init_py.h"
#include "sv3_PyUtil.h"
#include "sv3_SegmentationUtils.h"
#include "vtkPythonUtil.h"

#include <stdio.h>
#include <string.h>
#include <array>
#include <map>
#include <iostream>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
#include "sv_PolyData.h"
#include "vtkSmartPointer.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"

using sv3::Contour;
using sv3::PathElement;

// Exception type used by PyErr_SetString() to set the for the error indicator.
PyObject* PyRunTimeErr;

// createContourType() references pyContourType and is used before 
// it is defined so we need to define its prototype here.
pyContour* createContourType();

// Define a map between contour kernel type to enum.
static std::map<std::string,cKernelType> kernelNameTypeMap = 
{ 
    {"Circle", cKERNEL_CIRCLE},
    {"Ellipse", cKERNEL_ELLIPSE},
    {"LevelSet", cKERNEL_LEVELSET},
    {"Polygon", cKERNEL_POLYGON},
    {"SplinePolygon", cKERNEL_SPLINEPOLYGON},
    {"Threshold", cKERNEL_THRESHOLD}
};

//////////////////////////////////////////////////////////////////
//        C o n t o u r  M o d u l e  F u n c t i o n s         //
//////////////////////////////////////////////////////////////////
//
// Python API functions. 

//--------------------------
// Contour_SetContourKernel
//--------------------------
//
// [TODO:DaveP] SetContourKernel is a bit obscure. How about
//   SetContourMethod?
//
PyDoc_STRVAR(Contour_set_contour_kernel_doc, 
  "set_contour_kernel(kernel)                                    \n\ 
                                                                 \n\
   Set the computational kernel used to segment image data.       \n\
                                                                 \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Contour_set_contour_kernel(PyObject* self, PyObject *args)
{
    char *kernelName;
    cKernelType kernel;
    std::string functionName = Sv3PyUtilGetFunctionName(__func__);
    std::string msgp = Sv3PyUtilGetMsgPrefix(functionName); 
    std::string format = "s:" + functionName; 

    if (!PyArg_ParseTuple(args, format.c_str(), &kernelName)) {
        return nullptr;
    }

    try {
        kernel = kernelNameTypeMap.at(std::string(kernelName));
    } catch (const std::out_of_range& except) {
        auto msg = msgp + "Unknown kernel type '" + kernelName + "'."; 
        msg += " Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold.";
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }

    Contour::gCurrentKernel = kernel;
    return Py_BuildValue("s", kernelName);
}

//--------------------
// Contour_new_object
//--------------------
//
PyDoc_STRVAR(Contour_new_object_doc,
  "Contour_new_object(name, path)                                \n\ 
                                                                 \n\
   Create a Contour object at a given position along an existing path. \n\
                                                                 \n\
   Args:                                                         \n\
     name (str): The name of the Contour object to create.       \n\
     path (str): The name of the Path object the contour is defined on. \n\
     index (int): The index into the path points array to position the contour. 0 <= Index <= N-1, N = number of path points.\n\
");

static PyObject * 
Contour_new_object(pyContour* self, PyObject* args)
{
    char *contourName = nullptr;
    char *pathName = nullptr;
    int index = 0;
    std::string functionName = Sv3PyUtilGetFunctionName(__func__);
    std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
    std::string format = "ssi:" + functionName;

    if (!PyArg_ParseTuple(args, format.c_str(), &contourName, &pathName, &index)) {
        return nullptr;
    }
  
    // Check that the new Contour object does not already exist.
    if (gRepository->Exists(contourName)) {
        auto msg = msgp + "The Contour object '" + contourName + "' is already in the repository."; 
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }

    // Get Path object.
    auto rd = gRepository->GetObject(pathName);
    if (rd == nullptr) {
        auto msg = msgp + "The Path object '" + pathName + "' is not in the repository."; 
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }
    
    // Check that the Path is a path.
    auto type = rd->GetType();
    if (type != PATH_T) {
        auto msg = msgp + "'" + pathName + "' is not a Path object.";
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }
    
    auto path = dynamic_cast<PathElement*> (rd);
    int numPathPts = path->GetPathPointNumber();

    if (index >= numPathPts) {
        auto msg = msgp + "Index is larger than the number of path points " + std::to_string(numPathPts) + "."; 
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    } else if (index < 0) {
        auto msg = msgp + "Index must be larger than 0."; 
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }

    // Create a new Contour object.
    // [TODO:DaveP] We should add a name to the Contour object?
    Contour *geom = sv3::Contour::DefaultInstantiateContourObject(Contour::gCurrentKernel, path->GetPathPoint(index));
    
    if (geom == NULL) {
        auto msg = msgp + "Failed to create Contour object.";
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }

    // Add contour to the repository.
    if (!gRepository->Register(contourName, geom)) {
        delete geom;
        auto msg = msgp + "Error adding the Contour object '" + contourName + "' to the repository.";
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }
    
    Py_INCREF(geom);
    self->geom = geom;
    Py_DECREF(geom);
    return Py_None;
}

//-------------------
// Contour_set_image
//-------------------
//
PyDoc_STRVAR(Contour_set_image_doc,
  "Contour_set_image(image)  \n\ 
                             \n\
   Set the image data for a Contour object. \n\
                                            \n\
   Args:                                    \n\
     image (vtkImageData): A VTK image object.  \n\
");

static PyObject* 
Contour_set_image(pyContour* self, PyObject* args)
{
    char *pathName = nullptr;
    vtkImageData *vtkObj;
    int index = 0;
    RepositoryDataT type;
    cvRepositoryData *rd;
    PyObject *vtkName; 
    std::string functionName = Sv3PyUtilGetFunctionName(__func__);
    std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
    std::string format = "O:" + functionName;

    if (!PyArg_ParseTuple(args, format.c_str(), &vtkName)) {
        return nullptr;
    }
  
    // Check the Contour object has data. 
    Contour* contour = self->geom;
    if (contour == nullptr) {
        auto msg = msgp + "The Contour object does not have geometry."; 
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }
  
    // Look up the named vtk object:
    vtkObj = (vtkImageData *)vtkPythonUtil::GetPointerFromObject(vtkName, "vtkImageData");
    if (vtkObj == nullptr) {
        auto msg = msgp + "The vtkImageData object does not exist."; 
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }

    // [TODO:DaveP] What does this do?
    vtkImageData* slice = sv3::SegmentationUtils::GetSlicevtkImage(contour->GetPathPoint(),vtkObj, 5.0);
    contour->SetVtkImageSlice(slice);
    
    Py_INCREF(contour);
    self->geom=contour;
    Py_DECREF(contour);
    return Py_None;
}

//--------------------
// Contour_get_object
//--------------------
//
// [TODO:DaveP] This sets the 'geom' data member of the pyContour struct for
// this object. Bad!
//
PyDoc_STRVAR(Contour_get_object_doc,
  "Contour_get_object(name)  \n\ 
                             \n\
   Set the image data for a Contour object. \n\
                                            \n\
   Args:                                    \n\
     name (str): The name of the Contour object. \n\
");

static PyObject* 
Contour_get_object(pyContour* self, PyObject* args)
{
    char *objName = NULL;
    RepositoryDataT type;
    Contour *contour;

    std::string functionName = Sv3PyUtilGetFunctionName(__func__);
    std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
    std::string format = "s:" + functionName;

    if (!PyArg_ParseTuple(args, format.c_str(), &objName)) {
        return nullptr;
    }
    
    // Get the Contour object from the repository. 
    auto rd = gRepository->GetObject(objName);
    if (rd == nullptr) {
        auto msg = msgp + "The Contour object '" + objName + "' is not in the repository."; 
        PyErr_SetString(PyRunTimeErr, msg.c_str()); 
        return nullptr;
    }

    // Check its type.
    type = rd->GetType();
    if (type != CONTOUR_T) {
        auto msg = msgp + "'" + objName + "' is not a Contour object.";
        PyErr_SetString(PyRunTimeErr, msg.c_str());
        return nullptr;
    }

    contour = dynamic_cast<Contour*>(rd);
    Py_INCREF(contour);
    self->geom = contour;
    Py_DECREF(contour);
    return Py_None;
}

//----------------------------
// Contour_set_control_points 
//----------------------------
//
PyDoc_STRVAR(Contour_set_control_points_doc,
  "Contour_set_control_points(control_points)  \n\ 
                             \n\
   Set the control points for a Contour object. \n\
                                            \n\
   Args:                                    \n\
     control_points (list[]): The list of control points to set for the Contour object. The number of control points needed depends on the Contour kernel set for this object.\n\
");

static PyObject* 
Contour_set_control_points(pyContour* self, PyObject* args)
{
    PyObject *ptList = nullptr;
    std::string functionName = Sv3PyUtilGetFunctionName(__func__);
    std::string msgp = Sv3PyUtilGetMsgPrefix(functionName);
    std::string format = "O:" + functionName;

    if (!PyArg_ParseTuple(args, format.c_str(), &ptList)) {
        return nullptr;
    }

  // Do work of command:
    int numPts = PyList_Size(ptList);
    
    if (Contour::gCurrentKernel==cKERNEL_CIRCLE)
    {
        if(numPts!=2)
        {
            PyErr_SetString(PyRunTimeErr, "Circle contour requires two points, center and boundary");
            
        }
    }
    else if (Contour::gCurrentKernel==cKERNEL_ELLIPSE)
    {
        if(numPts!=3)
        {
            PyErr_SetString(PyRunTimeErr, "Ellipse contour requires three points, center and two boudaries");
            
        }
    }
    else if (Contour::gCurrentKernel==cKERNEL_POLYGON)
    {
        if(numPts<3)
        {
            PyErr_SetString(PyRunTimeErr, "Polygon contour requires at least three points");
            
        }
    }        
    
    Contour* contour = self->geom;
    if ( contour==NULL ) {
        PyErr_SetString(PyRunTimeErr, "Contour object not created");
        
    }
    
    std::vector<std::array<double,3> > pts(numPts);
    for (int i = 0; i<numPts; i++)
    {
        PyObject* tmpList = PyList_GetItem(ptList,i);
        if(PyList_Size(tmpList)!=3)
        {
            PyErr_SetString(PyRunTimeErr, "The length of double list must be 3");
            
        }
        for (int j = 0;j<3;j++)
        {
            pts[i][j] = PyFloat_AsDouble(PyList_GetItem(tmpList,j));
        }
    }
    contour->SetControlPoints(pts);
    return Py_None;
}

PyObject* Contour_SetControlPointsByRadiusCmd(pyContour* self, PyObject* args)
{
    PyObject *center;
    double radius = -1.;
    if (Contour::gCurrentKernel==cKERNEL_CIRCLE)
    {
        if (!PyArg_ParseTuple(args,"Od", &center, &radius ))
        {
            PyErr_SetString(PyRunTimeErr, "Could not import one list and one double\
            , center and radius");
            
        }
    }
    else
    {
        PyErr_SetString(PyRunTimeErr, "Kernel method does not support this function");
        
    }
  // Do work of command:
    double ctr[3];
    if(PyList_Size(center)!=3)
    {
        PyErr_SetString(PyRunTimeErr, "The length of double list must be 3");
        
    }
    for (int i = 0;i<PyList_Size(center);i++)
    {
        ctr[i] = PyFloat_AsDouble(PyList_GetItem(center,i));
    }
    
    Contour* contour = self->geom;
    if ( contour==NULL ) {
        PyErr_SetString(PyRunTimeErr, "Contour object not created");
        
    }
    if (radius>0.)
    {
        contour->SetControlPointByRadius(radius,ctr);
    }
    else
    {
        PyErr_SetString(PyRunTimeErr, "Must provide either a point on the \
            circle or a positive radius value");
        

    }
    return Py_None;
}


// --------------------
// Contour_CreateCmd
// --------------------
PyObject* Contour_CreateCmd( pyContour* self, PyObject* args)
{
    Contour* contour = self->geom;
    if ( contour==NULL ) {
        PyErr_SetString(PyRunTimeErr, "Contour object not created");
        
    }
    if (Contour::gCurrentKernel==cKERNEL_LEVELSET)
    {
        sv3::levelSetContour::svLSParam paras;
        contour->SetLevelSetParas(&paras);
        //std::cout<<"paras: "<<paras.radius<<std::endl;
    }
    
    contour->CreateContourPoints();
    if(contour->GetContourPointNumber()==0)
    {
        PyErr_SetString(PyRunTimeErr, "Error creating contour points");
        
    }
    
    Py_INCREF(contour);
    self->geom=contour;
    Py_DECREF(contour);
    return Py_None;
    
}

// --------------------
// Contour_GetAreaCmd
// --------------------
PyObject* Contour_GetAreaCmd( pyContour* self, PyObject* args)
{
    Contour* contour = self->geom;
    if ( contour==NULL ) {
        PyErr_SetString(PyRunTimeErr, "Contour object not created");
        
    }
    double area = contour->GetArea();
    return Py_BuildValue("d",area);    
}

// --------------------
// Contour_GetPerimeterCmd
// --------------------
PyObject* Contour_GetPerimeterCmd( pyContour* self, PyObject* args)
{
    Contour* contour = self->geom;
    if ( contour==NULL ) {
        PyErr_SetString(PyRunTimeErr, "Contour object not created");
        
    }
    double perimeter = contour->GetPerimeter();
    return Py_BuildValue("d",perimeter);    
}

// --------------------
// Contour_GetPerimeterCmd
// --------------------
PyObject* Contour_GetCenterPointCmd( pyContour* self, PyObject* args)
{
    Contour* contour = self->geom;
    if ( contour==NULL ) {
        PyErr_SetString(PyRunTimeErr, "Contour object not created");
        
    }
    std::array<double,3> center = contour->GetCenterPoint();
    char output[1024];
    output[0] = '\0';
    sprintf(output,"(%.4f,%.4f,%.4f)",center[0],center[1],center[2]);
    return Py_BuildValue("s",output);    
}

// ----------------------------
// Contour_SetThresholdValueCmd
// ----------------------------
PyObject* Contour_SetThresholdValueCmd(pyContour* self, PyObject* args)
{
    double threshold = 0.;
    if (!PyArg_ParseTuple(args,"d", &threshold))
    {
        PyErr_SetString(PyRunTimeErr, "Could not import double, threshold");
        
    }
    
    if (Contour::gCurrentKernel!=cKERNEL_THRESHOLD)
    {
        PyErr_SetString(PyRunTimeErr, "Contour type is not threshold");
        
    }
    
    Contour* contour = self->geom;
    if ( contour==NULL ) {
        PyErr_SetString(PyRunTimeErr, "Contour object not created");
        
    }
    contour->SetThresholdValue(threshold);
    return Py_None;
}
    
//-----------------------------
// Contour_CreateSmoothContour
//-----------------------------
//
pyContour* Contour_CreateSmoothContour(pyContour* self, PyObject* args)
{
    int fourierNumber = 0;
    char* contourName;
    if (!PyArg_ParseTuple(args,"is", &fourierNumber,&contourName))
    {
        PyErr_SetString(PyRunTimeErr, "Could not import int and one char, fourierNumber, contourName");
        
    }
    
    Contour* contour = self->geom;
    if ( contour==NULL ) {
        PyErr_SetString(PyRunTimeErr, "Contour object not created");
        
    }
    
    Contour *newContour = sv3::Contour::DefaultInstantiateContourObject(Contour::gCurrentKernel, contour->GetPathPoint());
    
    newContour= contour->CreateSmoothedContour(fourierNumber);
    
    // Register the contour:
    if ( !( gRepository->Register( contourName, newContour ) ) ) {
        PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
        delete newContour;
        
    }
        
    Py_INCREF(newContour);
    pyContour* pyNewCt;
    pyNewCt = createContourType();
    pyNewCt->geom = newContour;
    Py_DECREF(newContour);
    return pyNewCt;
}

//========================
// Contour_GetPolyDataCmd
//=========================
PyObject* Contour_GetPolyDataCmd(pyContour* self, PyObject* args)
{
    char* dstName=NULL;
    if (!PyArg_ParseTuple(args,"s", &dstName))
    {
        PyErr_SetString(PyRunTimeErr, "Could not import char, dstName");
        
    }
    
    // Make sure the specified result object does not exist:
    if ( gRepository->Exists( dstName ) ) {
        PyErr_SetString(PyRunTimeErr, "object already exists");
        
    }
  
    Contour* geom = self->geom;
    if ( geom==NULL ) {
        PyErr_SetString(PyRunTimeErr, "Contour object not created");
        
    }
    vtkSmartPointer<vtkPolyData> vtkpd = geom->CreateVtkPolyDataFromContour();
    
    cvPolyData* pd = new cvPolyData(vtkpd);
    
    if (pd==NULL)
    {
        PyErr_SetString(PyRunTimeErr, "Could not get polydata from object");
        
    }
    
      // Register the result:
    if ( !( gRepository->Register( dstName, pd ) ) ) {
      PyErr_SetString(PyRunTimeErr, "error registering obj in repository" );
      delete pd;
      
    }
    
    return Py_None;
}

//////////////////////////////////////////////////////////////////
//       C o n t o u r   M o d u l e  D e f i n i t i o n       //
//////////////////////////////////////////////////////////////////

//----------------------------
// Define API function names
//----------------------------
    
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC  initpyContour();
#endif
#if PYTHON_MAJOR_VERSION == 3
PyMODINIT_FUNC  PyInit_pyContour();
#endif

int Contour_pyInit()
{
#if PYTHON_MAJOR_VERSION == 2
  initpyContour();
#endif
#if PYTHON_MAJOR_VERSION == 3
  PyInit_pyContour();
#endif
  return SV_OK;
}

//----------------------------
// Define API function names
//----------------------------

static PyMethodDef pyContour_methods[] = {

  {"new_object", 
     (PyCFunction)Contour_new_object,
     METH_VARARGS,
     Contour_new_object_doc
   },

  {"GetObject", 
      (PyCFunction)Contour_get_object,
      METH_VARARGS,
      Contour_get_object_doc
  },

  {"Create", (PyCFunction)Contour_CreateCmd,METH_NOARGS,NULL},

  {"Area", (PyCFunction)Contour_GetAreaCmd,METH_NOARGS,NULL},

  {"Perimeter", (PyCFunction)Contour_GetPerimeterCmd,METH_NOARGS,NULL},

  {"Center", (PyCFunction)Contour_GetCenterPointCmd, METH_NOARGS,NULL},

  {"set_control_points", 
      (PyCFunction)Contour_set_control_points, 
      METH_VARARGS, 
      NULL
  },

  {"SetCtrlPtsByRadius", (PyCFunction)Contour_SetControlPointsByRadiusCmd, METH_VARARGS, NULL},

  {"SetThresholdValue", (PyCFunction)Contour_SetThresholdValueCmd, METH_VARARGS, NULL},

  {"CreateSmoothCt", (PyCFunction)Contour_CreateSmoothContour, METH_VARARGS, NULL},

  {"set_image", 
      (PyCFunction)Contour_set_image, 
      METH_VARARGS,
      Contour_set_image_doc},

  {"GetPolyData", (PyCFunction)Contour_GetPolyDataCmd, METH_VARARGS,NULL},
  {NULL,NULL}
};

//----------------
// pyContour_init
//----------------
// This is the __init__() method for the Contour class. 
//
// This function is used to initialize an object after it is created.
//
static int pyContour_init(pyContour* self, PyObject* args)
{
  fprintf(stdout, "Contour object type initialized.\n");
  return SV_OK;
}

//--------------------------------------
// Define the pyContourType type object
//--------------------------------------
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject pyContourType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "contour.Contour",         /* tp_name */
  sizeof(pyContour),         /* tp_basicsize */
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
  Py_TPFLAGS_DEFAULT |       /* tp_flags */
  Py_TPFLAGS_BASETYPE,   
  "Contour  objects",        /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  pyContour_methods,         /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)pyContour_init,  /* tp_init */
  0,                         /* tp_alloc */
  0,                         /* tp_new */
};

//-------------------
// createContourType
//-------------------
pyContour* createContourType()
{
  return PyObject_New(pyContour, &pyContourType);
}

static PyTypeObject pyContourFactoryRegistrarType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "pyContour.pyContourFactoryRegistrar",             /* tp_name */
  sizeof(pyContourFactoryRegistrar),             /* tp_basicsize */
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
  "pyContourFactoryRegistrar wrapper  ",           /* tp_doc */
};

// Define methods operating on the Contour Module level.
//
static PyMethodDef pyContourModule_methods[] =
{
    {"set_contour_kernel", 
       (PyCFunction)Contour_set_contour_kernel, 
       METH_VARARGS,
       Contour_set_contour_kernel_doc
    },

    {NULL,NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MODULE_NAME = "contour";

PyDoc_STRVAR(Contour_doc,
  "Contour functions");

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 3                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 3

// Size of per-interpreter state of the module.
// Set to -1 if the module keeps state in global variables. 
static int perInterpreterStateSize = -1;

// Always initialize this to PyModuleDef_HEAD_INIT.
PyModuleDef_Base m_base = PyModuleDef_HEAD_INIT;

// Define the module definition struct which holds all information 
// needed to create a module object. 
static struct PyModuleDef pyContourModule = {
   m_base,
   MODULE_NAME,   
   Contour_doc, /* module documentation, may be NULL */
   perInterpreterStateSize,  
   pyContourModule_methods
};

//------------------
// PyInit_pyContour 
//------------------
// The initialization function called by the Python interpreter when the module is loaded.
//
// [TODO:Davep] The global 'gRepository' is created here, as it is in all other modules init
//     function. Why is this not create in main()?
//
PyMODINIT_FUNC PyInit_pyContour()
{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL) {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from sv3_Contour_init\n");
  }

  Contour::gCurrentKernel = cKERNEL_INVALID;
  //if (PySys_SetObject("ContourObjectRegistrar",(PyObject*)&Contour::gRegistrar)<0)
  //{
  //  fprintf(stdout,"Unable to create ContourObjectRegistrar");
  //  return;
  //}
  // Initialize
  pyContourType.tp_new = PyType_GenericNew;
  pyContourFactoryRegistrarType.tp_new = PyType_GenericNew;

  if (PyType_Ready(&pyContourType)<0) {
    fprintf(stdout,"Error in pyContourType\n");
    return SV_PYTHON_ERROR;
  }

  if (PyType_Ready(&pyContourFactoryRegistrarType)<0) {
    fprintf(stdout,"Error in pyContourFactoryRegistrarType\n");
    return SV_PYTHON_ERROR;
  }
  PyObject* pythonC;
  pythonC = PyModule_Create(&pyContourModule);
  if(pythonC==NULL) {
    fprintf(stdout,"Error in initializing pyContour\n");
    return SV_PYTHON_ERROR;
  }
  PyRunTimeErr = PyErr_NewException("pyContour.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  Py_INCREF(&pyContourType);
  Py_INCREF(&pyContourFactoryRegistrarType);

  // Add the 'Contour' object.
  PyModule_AddObject(pythonC,"Contour", (PyObject*)&pyContourType);
  PyModule_AddObject(pythonC, "pyContourFactoryRegistrar", (PyObject *)&pyContourFactoryRegistrarType);
  
  pyContourFactoryRegistrar* tmp = PyObject_New(pyContourFactoryRegistrar, &pyContourFactoryRegistrarType);
  tmp->registrar = (cvFactoryRegistrar *)&Contour::gRegistrar;
  PySys_SetObject("ContourObjectRegistrar", (PyObject *)tmp);

  return pythonC;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

//----------------
//initpyContour
//----------------
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyContour()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from sv3_Contour_init\n");
  }

  Contour::gCurrentKernel = cKERNEL_INVALID;
  //if (PySys_SetObject("ContourObjectRegistrar",(PyObject*)&Contour::gRegistrar)<0)
  //{
  //  fprintf(stdout,"Unable to create ContourObjectRegistrar");
  //  return;
  //}
  // Initialize
  pyContourType.tp_new=PyType_GenericNew;
  pyContourFactoryRegistrarType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&pyContourType)<0)
  {
    fprintf(stdout,"Error in pyContourType\n");
    return;
  }
  if (PyType_Ready(&pyContourFactoryRegistrarType)<0)
  {
    fprintf(stdout,"Error in pyContourFactoryRegistrarType\n");
    return;
  }
  PyObject* pythonC;
  pythonC = Py_InitModule(MODULE_NAME, pyContourModule_methods);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyContour\n");
    return;
  }
  PyRunTimeErr = PyErr_NewException("pyContour.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  Py_INCREF(&pyContourType);
  Py_INCREF(&pyContourFactoryRegistrarType);
  PyModule_AddObject(pythonC,"pyContour",(PyObject*)&pyContourType);
  PyModule_AddObject(pythonC, "pyContourFactoryRegistrar", (PyObject *)&pyContourFactoryRegistrarType);
  
  pyContourFactoryRegistrar* tmp = PyObject_New(pyContourFactoryRegistrar, &pyContourFactoryRegistrarType);
  tmp->registrar = (cvFactoryRegistrar *)&Contour::gRegistrar;
  PySys_SetObject("ContourObjectRegistrar", (PyObject *)tmp);
  
  return ;

}
#endif

#if PYTHON_MAJOR_VERSION == 3

#endif
