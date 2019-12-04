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

// The functions defined here implement the SV Python API contour module
// and Contour class. 
//
// The module name is 'contour'. A class named 'Contour' is defined and 
// used to store contour data. The 'Contour' class is a base class for the
// contour cirlce, ellipse, level set, polygon and threshold types.
//
// The 'Contour' class cannot be imported and must be used prefixed by the 
// module name. For example
//
//     contour = contour.Countour()
//
// A Python exception sv.contour.ContourError is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.contour.ContourError:
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include "sv3_Contour.h"
#include "sv3_CircleContour.h"
#include "sv3_LevelSetContour.h"
#include "sv3_PolygonContour.h"
#include "sv3_SplinePolygonContour.h"
#include "sv3_ThresholdContour.h"
#include "sv3_Contour_PyModule.h"
#include "sv3_ContourKernel_PyClass.h"

#include "sv3_PathElement.h"

#include "sv3_SegmentationUtils.h"
#include "vtkPythonUtil.h"
#include "sv_PyUtils.h"

#include <stdio.h>
#include <string.h>
#include <array>
#include <map>
#include <functional> 
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
static PyObject * PyRunTimeErr;

// Prototypes for creating SV and Python contour objects. 
static PyContour * PyCreateContourType();
static PyObject * PyCreateContour(cKernelType contourType);

//----------------
// ContourCtorMap
//----------------
// Define an object factory for creating objects for Contour derived classes.
//
using ContourCtorMapType = std::map<cKernelType, std::function<Contour*()>>;
ContourCtorMapType ContourCtorMap = {
    {cKernelType::cKERNEL_CIRCLE, []() -> Contour* { return new sv3::circleContour(); } },
    //{cKernelType::cKERNEL_ELLIPSE, []() -> Contour* { return new sv3::circleContour(); } },
    {cKernelType::cKERNEL_LEVELSET, []() -> Contour* { return new sv3::levelSetContour(); } },
    {cKernelType::cKERNEL_POLYGON, []() -> Contour* { return new sv3::ContourPolygon(); } },
    {cKernelType::cKERNEL_SPLINEPOLYGON, []() -> Contour* { return new sv3::ContourSplinePolygon(); } },
    {cKernelType::cKERNEL_THRESHOLD, []() -> Contour* { return new sv3::thresholdContour(); } },
};

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//---------------------
// CreateContourObject
//---------------------
// Create an SV Contour derived object.
//
static Contour * 
CreateContourObject(cKernelType contourType, PathElement::PathPoint pathPoint)
{
  Contour* contour = NULL;

  try {
      contour = ContourCtorMap[contourType]();
  } catch (const std::bad_function_call& except) {
      return nullptr;
  }

  contour->SetPathPoint(pathPoint);
  return contour;
}

//////////////////////////////////////////////////////
//          C l a s s   M e t h o d s               //
//////////////////////////////////////////////////////
//
// Python 'Contour' class methods.

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
    auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);

    if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
        return api.argsError();
    }

    try {
        kernel = kernelNameEnumMap.at(std::string(kernelName));
    } catch (const std::out_of_range& except) {
        auto msg = "Unknown kernel type '" + std::string(kernelName) + "'." + 
          " Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold.";
        api.error(msg); 
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
  "Contour_new_object(name, path) \n\ 
   \n\
   Create a contour at a given position along an existing path. \n\
   \n\
   Args: \n\
     name (str): The name of the contour to create.       \n\
     path (str): The name of the Path object the contour is defined on. \n\
     index (int): The index into the path points array to position the contour. 0 <= Index <= N-1, N = number of path points.\n\
");

static PyObject * 
Contour_new_object(PyContour* self, PyObject* args)
{
    char *contourName = nullptr;
    char *pathName = nullptr;
    int index = 0;
    auto api = SvPyUtilApiFunction("ssi", PyRunTimeErr, __func__);

    if (!PyArg_ParseTuple(args, api.format, &contourName, &pathName, &index)) {
        return api.argsError();
    }
  
    // Check that the new Contour object does not already exist.
    if (gRepository->Exists(contourName)) {
        api.error("The Contour object '" + std::string(contourName) + "' is already in the repository."); 
        return nullptr;
    }

    // Get Path object.
    auto rd = gRepository->GetObject(pathName);
    if (rd == nullptr) {
        api.error("The Path object '" + std::string(pathName) + "' is not in the repository."); 
        return nullptr;
    }
    
    // Check that the Path is a path.
    auto type = rd->GetType();
    if (type != PATH_T) {
        api.error("'" + std::string(pathName) + "' is not a Path object.");
        return nullptr;
    }
    
    auto path = dynamic_cast<PathElement*>(rd);
    if (path == nullptr) {
        api.error("Path element is null.");
        return nullptr;
    } 
    int numPathPts = path->GetPathPointNumber();

    if (index >= numPathPts) {
        api.error("Index is larger than the number of path points " + std::to_string(numPathPts) + "."); 
        return nullptr;
    } else if (index < 0) {
        api.error("Index must be larger than 0."); 
        return nullptr;
    }

    // Create a new Contour object.
    std::cout << "####### Create a new Contour object" << std::endl;
    //Contour *geom = sv3::Contour::DefaultInstantiateContourObject(Contour::gCurrentKernel, path->GetPathPoint(index));
    auto contour = CreateContourObject(Contour::gCurrentKernel, path->GetPathPoint(index)); 
    
    if (contour == NULL) {
        api.error("Failed to create Contour object.");
        return nullptr;
    }

    // Add contour to the repository.
    if (!gRepository->Register(contourName, contour)) {
        delete contour;
        api.error("Error adding the Contour object '" + std::string(contourName) + "' to the repository.");
        return nullptr;
    }
    
    Py_INCREF(contour);
    self->contour = contour;
    Py_DECREF(contour);
    return Py_None;
}

//-------------------
// Contour_set_image
//-------------------
//
PyDoc_STRVAR(Contour_set_image_doc,
  "Contour_set_image(image)  \n\ 
   \n\
   Set the image data for a contour. \n\
   \n\
   Args: \n\
     image (vtkImageData): A VTK image object.  \n\
");

static PyObject* 
Contour_set_image(PyContour* self, PyObject* args)
{
    PyObject *vtkName; 
    auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__);

    if (!PyArg_ParseTuple(args, api.format, &vtkName)) {
        return api.argsError();
    }
  
    // Check the Contour object has data. 
    Contour* contour = self->contour;
    if (contour == nullptr) {
        api.error("The Contour object does not have geometry."); 
        return nullptr;
    }
  
    // Look up the named vtk object:
    auto vtkObj = (vtkImageData *)vtkPythonUtil::GetPointerFromObject(vtkName, "vtkImageData");
    if (vtkObj == nullptr) {
        api.error("The vtkImageData object does not exist."); 
        return nullptr;
    }

    // [TODO:DaveP] What does this do?
    vtkImageData* slice = sv3::SegmentationUtils::GetSlicevtkImage(contour->GetPathPoint(),vtkObj, 5.0);
    contour->SetVtkImageSlice(slice);
    
    Py_INCREF(contour);
    self->contour=contour;
    Py_DECREF(contour);
    return Py_None;
}

//--------------------
// Contour_get_object
//--------------------
//
// [TODO:DaveP] This sets the 'geom' data member of the PyContour struct for
// this object. Bad!
//
PyDoc_STRVAR(Contour_get_object_doc,
  "Contour_get_object(name)  \n\ 
   \n\
   Set the image data for a contour. \n\
   \n\
   Args: \n\
     name (str): The name of the Contour object. \n\
");

static PyObject* 
Contour_get_object(PyContour* self, PyObject* args)
{
    char *objName = NULL;
    auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);

    if (!PyArg_ParseTuple(args, api.format, &objName)) {
        return api.argsError();
    }
    
    // Get the Contour object from the repository. 
    auto rd = gRepository->GetObject(objName);
    if (rd == nullptr) {
        api.error("The Contour object '" + std::string(objName) + "' is not in the repository."); 
        return nullptr;
    }

    // Check its type.
    auto type = rd->GetType();
    if (type != CONTOUR_T) {
        api.error("'" + std::string(objName) + "' is not a Contour object.");
        return nullptr;
    }

    auto contour = dynamic_cast<Contour*>(rd);
    Py_INCREF(contour);
    self->contour = contour;
    Py_DECREF(contour);
    return Py_None;
}

//----------------------------
// Contour_set_control_points 
//----------------------------
//
// Use try-catch block for error handling.
//
// [TODO:DaveP] I'm not sure we need this function, think it should be
// defined for each Contour object type.
//
PyDoc_STRVAR(Contour_set_control_points_doc,
  "Contour.set_control_points(control_points)  \n\ 
   \n\
   Set the control points for a contour. \n\
   \n\
   Args: \n\
     control_points (list[]): The list of control points to set for the contour. The number of control points needed depends on the Contour kernel set for this object.\n\
");

static PyObject* 
Contour_set_control_points(PyContour* self, PyObject* args)
{
    PyObject *controlPoints = nullptr;
    auto api = SvPyUtilApiFunction("O", PyRunTimeErr, __func__);

    if (!PyArg_ParseTuple(args, api.format, &controlPoints)) {
        return api.argsError();
    }

    try {

    // Check control points data.
    //
    if (!PyList_Check(controlPoints)) {
        throw std::runtime_error("Control points argument is not a Python list.");
    }

    int numPts = PyList_Size(controlPoints);
    for (int i = 0; i < numPts; i++) {
        PyObject* pt = PyList_GetItem(controlPoints,i);
        if ((PyList_Size(pt) != 3) || !PyList_Check(pt)) {
            throw std::runtime_error("Control points argument data at " + std::to_string(i) + 
              " in the list is not a 3D point (three float values).");
        }
        for (int j = 0; j < 3; j++) {
            if (!PyFloat_Check(PyList_GetItem(pt,j))) { 
                throw std::runtime_error("Control points argument data at " + std::to_string(i) + 
                  " in the list is not a 3D point (three float values).");
            }
        }
    }
    
    // Check that the number of control is consistant 
    // with the kernel type.
    //
    // [TODO:DaveP] The kernel should be set in the object.
    //
    if ((Contour::gCurrentKernel == cKERNEL_CIRCLE) && (numPts != 2)) {
        throw std::runtime_error("Circle contour requires two points: a center and a point on its boundary.");

    } else if ((Contour::gCurrentKernel == cKERNEL_ELLIPSE) && (numPts != 3)) {
        throw std::runtime_error("Ellipse contour requires three points: a center and two points on its boundary.");

    } else if ((Contour::gCurrentKernel == cKERNEL_POLYGON) && (numPts < 3)) {
        throw std::runtime_error("Polygon contour requires at least three points");
    }        
    
    Contour* contour = self->contour;
    if (contour == NULL ) {
        throw std::runtime_error("Geometry has not been created for the contour.");
    }
    
    // Copy control points to contour object.
    std::vector<std::array<double,3> > pts(numPts);
    for (int i = 0; i < numPts; i++) {
        PyObject* tmpList = PyList_GetItem(controlPoints,i);
        for (int j = 0; j < 3; j++) {
            pts[i][j] = PyFloat_AsDouble(PyList_GetItem(tmpList,j));
        }
    }
    contour->SetControlPoints(pts);
    return Py_None;

   } catch (std::exception &e) {
       api.error(e.what());
       return nullptr;
  }

}

//--------------------------------------
// Contour_set_control_points_by_radius
//--------------------------------------
//
// [TODO:DaveP] I think this should be removed, have it only defined 
// for the CircelContour type.
//
PyDoc_STRVAR(Contour_set_control_points_by_radius_doc,
  "Contour.set_control_points_by_radius(control_points)  \n\ 
   \n\
   Set the control points for a Circle Contour with a center point and radius. \n\
   \n\
   Args: \n\
     center ([x,y,z]): The list of three floats defining the center of the Circle Contour.   \n\ 
     radius (float)): The radius of the Circle Contour.   \n\ 
");

static PyObject* 
Contour_set_control_points_by_radius(PyContour* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("Od", PyRunTimeErr, __func__);

    if (Contour::gCurrentKernel != cKERNEL_CIRCLE) {
        api.error("Contour kernel is not set to 'Circle'");
        return nullptr;
    }

    PyObject *center;
    double radius = 0.0;
    if (!PyArg_ParseTuple(args, api.format, &center, &radius )) {
        return api.argsError();
    }

    double ctr[3];
    if (PyList_Size(center) != 3) {
        api.error("Center argument is not a 3D point (three float values).");
        return nullptr;
    }

    for (int i = 0; i < PyList_Size(center); i++) {
        if (!PyFloat_Check(PyList_GetItem(center,i))) { 
            api.error("Center argument is not a 3D point (three float values).");
            return nullptr;
        }
        ctr[i] = PyFloat_AsDouble(PyList_GetItem(center,i));
    }
    
    auto contour = self->contour;
    if (contour == NULL) {
        api.error("No geometry has been created for the contour.");
        return nullptr;
    }

    if (radius <= 0.0) {
        api.error("Radius argument must be > 0.0.");
        return nullptr;
    }

    contour->SetControlPointByRadius(radius,ctr);
    return Py_None;
}


//------------------
// Contour_get_area
//------------------
//
PyDoc_STRVAR(Contour_get_area_doc,
  "Contour.area()  \n\ 
   \n\
   Get the area of the contour. \n\
   \n\
   Args:                                    \n\
     None \n\
   \n\
   Returns: Area (float) of the contour. \n\
");

static PyObject* 
Contour_get_area(PyContour* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
    auto contour = self->contour;
    if (contour == NULL) {
        api.error("No geometry has been created for the contour.");
        return nullptr;
        
    }
    double area = contour->GetArea();
    return Py_BuildValue("d",area);    
}

//-----------------------
// Contour_get_perimeter
//-----------------------
//
PyDoc_STRVAR(Contour_get_perimeter_doc,
  "Contour.perimeter()  \n\ 
   \n\
   Get the length of the contour perimeter. \n\
   \n\
   Args:                                    \n\
     None \n\
   \n\
   Returns: Length (float) of the contour perimeter. \n\
");

PyObject* Contour_get_perimeter(PyContour* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
    auto contour = self->contour;
    if (contour == NULL) {
        api.error("No geometry has been created for the Contour.");
        return nullptr;
    }
    double perimeter = contour->GetPerimeter();
    return Py_BuildValue("d",perimeter);    
}

//--------------------
// Contour_get_center
//--------------------
//
// [TODO:DaveP] This should return a list of three floats.
PyDoc_STRVAR(Contour_get_center_doc,
  "Contour.center()  \n\ 
   \n\
   Get the center of the contour. \n\
   \n\
   Args:                                    \n\
     None \n\
   \n\
   Returns: Center (string(x,y,z)) of the contour. \n\
");

PyObject* Contour_get_center(PyContour* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
    auto contour = self->contour;
    if (contour == NULL ) {
        api.error("No geometry has been created for the Contour.");
        return nullptr;
    }

    std::array<double,3> center = contour->GetCenterPoint();
    char output[1024];
    output[0] = '\0';
    sprintf(output,"(%.4f,%.4f,%.4f)",center[0],center[1],center[2]);
    return Py_BuildValue("s",output);    
}

//-----------------------------
// Contour_set_threshold_value
//-----------------------------
//
PyDoc_STRVAR(Contour_set_threshold_value_doc,
  "Contour.set_threshold_value()  \n\ 
   \n\
   Set the threshold value for a Threshold Contour. \n\
   \n\
   Args: \n\
     threshold (float): Threshold value. \n\
");

PyObject* Contour_set_threshold_value(PyContour* self, PyObject* args)
{
    double threshold = 0.0;
    auto api = SvPyUtilApiFunction("d", PyRunTimeErr, __func__);

    if (!PyArg_ParseTuple(args, api.format, &threshold)) {
        return api.argsError();
    }
    
    if (Contour::gCurrentKernel != cKERNEL_THRESHOLD) {
        api.error("Contour kernel is not set to 'Threshold'");
        return nullptr;
    }
    
    Contour* contour = self->contour;
    if (contour == NULL) {
        api.error("No geometry has been created for the contour.");
        return nullptr;
    }

    contour->SetThresholdValue(threshold);
    return Py_None;
}
    
//-------------------------------
// Contour_create_smooth_contour
//-------------------------------
//
PyDoc_STRVAR(Contour_create_smooth_contour_doc,
  "Contour.create_smooth_contour()  \n\ 
   \n\
   Create a smoothed contour. \n\
   \n\
   Args:                                    \n\
     num_modes (int): Number of Fourier modes.\n\
     name (str): Name of the new smoothed contour. \n\
");

static PyContour* 
Contour_create_smooth_contour(PyContour* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("is", PyRunTimeErr, __func__);
    int fourierNumber = 0;
    char* contourName;

    if (!PyArg_ParseTuple(args, api.format, &fourierNumber, &contourName)) {
        api.argsError();
        return nullptr;
    }
    
    Contour* contour = self->contour;
    if (contour == NULL) {
        api.error("No geometry has been created for the Contour.");
        return nullptr;
    }
    
    auto newContour = CreateContourObject(Contour::gCurrentKernel, contour->GetPathPoint()); 
    //Contour *newContour = sv3::Contour::DefaultInstantiateContourObject(Contour::gCurrentKernel, contour->GetPathPoint());
    newContour= contour->CreateSmoothedContour(fourierNumber);
    
    if ( !( gRepository->Register(contourName, newContour))) {
        delete newContour;
        api.error("Could not add the new contour into the repository."); 
        return nullptr;
    }
        
    Py_INCREF(newContour);
    PyContour* pyNewCt;
    pyNewCt = PyCreateContourType();
    pyNewCt->contour = newContour;
    Py_DECREF(newContour);
    return pyNewCt;
}

//----------------------
// Contour_get_polydata
//----------------------
//
// [TODO:DaveP] This function name is confusing, does not get
// anything: puts contour geometry into the Python repository.
//
//   Do we want to have an explicit operation for this
//
//       poly_data = contour.get_polydata()
//       repository.add_polydata(poly_data)
//
PyDoc_STRVAR(Contour_get_polydata_doc,
  "Contour.get_polydata(name)  \n\ 
   \n\
   Add the contour geometry to the repository. \n\
   \n\
   Args:                                    \n\
     name (str): Name in the repository to store the geometry. \n\
");

static PyObject* 
Contour_get_polydata(PyContour* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
    char* dstName = NULL;
    if (!PyArg_ParseTuple(args, api.format, &dstName)) {
        return api.argsError();
    }
    
    // Check that the repository object does not already exist.
    if (gRepository->Exists(dstName)) {
        api.error("The repository object '" + std::string(dstName) + "' already exists."); 
        return nullptr;
    }
  
    auto geom = self->contour;
    if (geom == NULL) {
        api.error("The contour does not have geometry."); 
        return nullptr;
    }

    // Get the VTK polydata.
    vtkSmartPointer<vtkPolyData> vtkpd = geom->CreateVtkPolyDataFromContour();
    cvPolyData* pd = new cvPolyData(vtkpd);
    
    if (pd == NULL) {
        api.error("Could not get polydata for the contour.");
        return nullptr;
    }
    
    if ( !( gRepository->Register( dstName, pd ) ) ) {
        api.error("Could not add the polydata to the repository.");
        delete pd;
        return nullptr;
    }
    
    return Py_None;
}

//----------------
// Contour_create 
//----------------
//
static PyObject *
Contour_create(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* kernelName = nullptr;

  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  cKernelType contourType;

  try {
      contourType = kernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." +
          " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  std::cout << "[Contour_create] Kernel name: " << kernelName << std::endl;
  auto cont = PyCreateContour(contourType);
  Py_INCREF(cont);
  return cont;
}

////////////////////////////////////////////////////////
//           C l a s s   D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* CONTOUR_CLASS = "Contour";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* CONTOUR_MODULE_CLASS = "contour.Contour";

PyDoc_STRVAR(ContourModule_doc, "contour module functions");

//-------------------
// ContourObjectInit
//-------------------
// This is the __init__() method for the Contour class. 
//
// This function is used to initialize an object after it is created.
//
static int
PyContourInit(PyContour* self, PyObject* args, PyObject *kwds)
{
  static int numObjs = 1;
  std::cout << "[PyContourInit] New Contour object: " << numObjs << std::endl;
  char* kernelName = nullptr;
  if (!PyArg_ParseTuple(args, "s", &kernelName)) {
      return -1;
  }

  std::cout << "[ContourObjectInit] Kernel name: " << kernelName << std::endl;
  self->contour = new Contour();
  numObjs += 1;
  return 0;
}

//------------------
// PyContourMethods 
//------------------
// Define the methods for the Python 'Contour' class.
//
static PyMethodDef PyContourMethods[] = {

  { "area", (PyCFunction)Contour_get_area, METH_NOARGS, Contour_get_area_doc },

  {"center", (PyCFunction)Contour_get_center, METH_NOARGS, Contour_get_center_doc }, 

  {"create_smooth_contour", (PyCFunction)Contour_create_smooth_contour, METH_VARARGS, Contour_create_smooth_contour_doc },

  {"get_object", (PyCFunction)Contour_get_object, METH_VARARGS, Contour_get_object_doc },

  {"get_polydata", (PyCFunction)Contour_get_polydata, METH_VARARGS, Contour_get_polydata_doc },

  { "new_object", (PyCFunction)Contour_new_object, METH_VARARGS, Contour_new_object_doc },

  {"perimeter", (PyCFunction)Contour_get_perimeter, METH_NOARGS, Contour_get_perimeter_doc },

  {"set_control_points", (PyCFunction)Contour_set_control_points, METH_VARARGS, Contour_set_control_points_doc },

  {"set_control_points_by_radius", (PyCFunction)Contour_set_control_points_by_radius, METH_VARARGS, Contour_set_control_points_by_radius_doc },

  {"set_image", (PyCFunction)Contour_set_image, METH_VARARGS, Contour_set_image_doc },

  {"set_threshold_value", (PyCFunction)Contour_set_threshold_value, METH_VARARGS, Contour_set_threshold_value_doc },

  {NULL,NULL}
};

//--------------------
// PyContourClassType 
//--------------------
// Define the Python type object for the Python 'Contour' class. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static PyTypeObject PyContourClassType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = CONTOUR_CLASS,
  .tp_basicsize = sizeof(PyContour)
};

//---------------
// PyContourtNew
//---------------
//
static PyObject *
PyContourNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyContourNew] PyContourNew " << std::endl;
  auto self = (PyContour*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->id = 1;
  }

  return (PyObject *) self;
}

//-----------------
// PyContourDelete
//-----------------
//
static void
PyContourDealloc(PyContour* self)
{
  std::cout << "[PyContourDealloc] Free PyContour" << std::endl;
  delete self->contour;
  Py_TYPE(self)->tp_free(self);
}

//----------------------
// SetContourTypeFields 
//----------------------
// Set the Python type object fields that stores Contour data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetContourTypeFields(PyTypeObject& contourType)
{
  // Doc string for this type.
  contourType.tp_doc = "Contour  objects";
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  contourType.tp_new = PyContourNew;
  //contourType.tp_new = PyType_GenericNew,
  contourType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  contourType.tp_init = (initproc)PyContourInit;
  contourType.tp_dealloc = (destructor)PyContourDealloc;
  contourType.tp_methods = PyContourMethods;
};

//---------------------
// PyCreateContourType
//---------------------
// Create a Python PyContour object.
//
static PyContour * 
PyCreateContourType()
{
  return PyObject_New(PyContour, &PyContourClassType);
}

//////////////////////////////////////////////////////
//          M o d u l e  M e t h o d s              //
//////////////////////////////////////////////////////
//
// Define the 'contour' module methods.

//----------------
// Contour_create
//----------------
//
PyDoc_STRVAR(Contour_create_doc,
  "Contour_create()  \n\ 
   \n\
   Set the control points for the contour. \n\
   \n\
   Args:                                    \n\
     None \n\
");

static PyObject* 
Contour_create(PyContour* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* kernelName = nullptr;

  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  // Get the contour type.
  //
  cKernelType contourType;
  try {
      contourType = kernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." + " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  std::cout << "[Contour_create] Kernel name: " << kernelName << std::endl;
  auto cont = PyCreateContour(contourType);
  Py_INCREF(cont);
  return cont;


/*

    auto contour = self->contour;
    if (contour == NULL) {
        api.error("No geometry has been created for the Contour.");
        return nullptr;
    }

    // Set default level set parameters.
    if (Contour::gCurrentKernel == cKERNEL_LEVELSET) {
        sv3::levelSetContour::svLSParam paras;
        contour->SetLevelSetParas(&paras);
    }
    
    contour->CreateContourPoints();

    if (contour->GetContourPointNumber() == 0) {
        api.error("Error creating contour points.");
        return nullptr;
    }
    
    Py_INCREF(contour);
    self->contour = contour;
    Py_DECREF(contour);
    return Py_None;
*/
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* CONTOUR_MODULE = "contour";

// Contour module exception.
static char* CONTOUR_MODULE_EXCEPTION = "contour.ContourError";
static char* CONTOUR_MODULE_EXCEPTION_OBJECT = "ContourError";

//------------------------
// PyContourModuleMethods
//------------------------
// Define the methods for the Python 'contour' module.
//
static PyMethodDef PyContourModuleMethods[] =
{
    {"create", (PyCFunction)Contour_create, METH_VARARGS, "Create a Contour object."},

    {"set_contour_kernel", (PyCFunction)Contour_set_contour_kernel, METH_VARARGS, Contour_set_contour_kernel_doc },

    {NULL,NULL}
};

// Include derived Contour classes.
#include "sv3_CircleContour_PyClass.h"

//------------------
// PyContourCtorMap
//------------------
// Define an object factory for Python Contour derived classes.
//
using PyContourCtorMapType = std::map<cKernelType, std::function<PyObject*()>>;
PyContourCtorMapType PyContourCtorMap = {
    {cKernelType::cKERNEL_CIRCLE, []() -> PyObject* { return PyObject_CallObject((PyObject*)&PyCircleContourType, NULL); } },
};

//-----------------
// PyCreateContour
//-----------------
// Create a Python contour object for the given kernel type.
// 
static PyObject *
PyCreateContour(cKernelType contourType)
{
  PyObject* contour;

  try {
      contour = PyContourCtorMap[contourType]();
  } catch (const std::bad_function_call& except) {
      return nullptr;
  }

  return contour;
}

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

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
static struct PyModuleDef PyContourModule = {
   m_base,
   CONTOUR_MODULE,   
   ContourModule_doc, 
   perInterpreterStateSize,  
   PyContourModuleMethods
};

//------------------
// PyInit_PyContour 
//------------------
// The initialization function called by the Python interpreter 
// when the module is loaded.
//
PyMODINIT_FUNC PyInit_PyContour()
{
  std::cout << "========== load contour module ==========" << std::endl;

  // Initialize the Contour class type.
  SetContourTypeFields(PyContourClassType);
  if (PyType_Ready(&PyContourClassType) < 0) {
    fprintf(stdout,"Error in PyContourClassType\n");
    return SV_PYTHON_ERROR;
  }

  // Initialize the circle class type.
  SetCircleContourTypeFields(PyCircleContourType);
  if (PyType_Ready(&PyCircleContourType) < 0) {
      std::cout << "Error creating CircleContour type" << std::endl;
      return nullptr;
  }

  // Initialize the kernel class type.
  SetContourKernelTypeFields(ContourKernelType);
  if (PyType_Ready(&ContourKernelType) < 0) {
      std::cout << "Error creating ContourKernel type" << std::endl;
      return nullptr;
  }

  // Create the contour module.
  auto module = PyModule_Create(&PyContourModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing PyContour\n");
    return SV_PYTHON_ERROR;
  }

  // Add contour.ContourException exception.
  PyRunTimeErr = PyErr_NewException(CONTOUR_MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, CONTOUR_MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  // Add the 'Contour' object.
  Py_INCREF(&PyContourClassType);
  PyModule_AddObject(module, CONTOUR_CLASS, (PyObject*)&PyContourClassType);

  // Add the 'Circle' class.
  Py_INCREF(&PyCircleContourType);
  PyModule_AddObject(module, CONTOUR_CIRCLE_CLASS, (PyObject*)&PyCircleContourType);

  // Add the 'Kernel' class.
  Py_INCREF(&ContourKernelType);
  PyModule_AddObject(module, CONTOUR_KERNEL_CLASS, (PyObject*)&ContourKernelType);

  // Set the kernel names in the ContourKernelType dictionary.
  SetContourKernelTypes(ContourKernelType);

  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

//----------------
//initPyContour
//----------------
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initPyContour()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from sv3_Contour_init\n");
  }

  // Set the global contour kernel.
  //
  // [TODO:DaveP] yuk!
  //
  Contour::gCurrentKernel = cKERNEL_INVALID;

  // Create a Contour class.
  PyContourClassType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&PyContourClassType)<0) {
    fprintf(stdout,"Error in PyContourClassType\n");
    return;
  }

  auto module = Py_InitModule(CONTOUR_MODULE, PyContourModule_methods);
  if(module == NULL) {
    fprintf(stdout,"Error in initializing PyContour\n");
    return;
  }
  PyRunTimeErr = PyErr_NewException("PyContour.error",NULL,NULL);
  PyModule_AddObject(module,"error",PyRunTimeErr);

  Py_INCREF(&PyContourClassType);
  PyModule_AddObject(pythonC,"PyContour",(PyObject*)&PyContourClassType);

  // Add the 'kernel' object.
  Py_INCREF(&ContourKernelType);
  PyModule_AddObject(module, "kernel", (PyObject*)&ContourKernelType);

  SetContourKernelTypes(ContourKernelType);

  
  return module;

}
#endif

