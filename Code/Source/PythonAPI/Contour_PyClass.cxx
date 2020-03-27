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

// The functions defined here implement the SV Python API 'Contour' class. 
//
// The 'Contour' class is used to store contour data. It is base class for the
// contour cirlce, ellipse, level set, polygon and threshold types.
//
// The 'Contour' class cannot be imported and must be used prefixed by the 
// module name. For example
//
//     contour = contour.Countour()
//
using sv3::Contour;
using sv3::PathElement;

//----------------
// ContourCtorMap
//----------------
// Define an object factory for creating objects for Contour derived classes.
//
using ContourCtorMapType = std::map<cKernelType, std::function<Contour*()>>;
ContourCtorMapType ContourCtorMap = {
    {cKernelType::cKERNEL_CIRCLE, []() -> Contour* { return new sv3::circleContour(); } },
    {cKernelType::cKERNEL_ELLIPSE, []() -> Contour* { return new sv3::circleContour(); } },
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
// If 'contourType' is not valid then create a Contour object.
// Contour objects can be used with diffetent methods. 
//
static Contour * 
CreateContourObject(cKernelType contourType, PathElement::PathPoint pathPoint)
{
  Contour* contour = nullptr;

  try {
      contour = ContourCtorMap[contourType]();
  } catch (const std::bad_function_call& except) {
      contour = new sv3::Contour();
  }

  contour->SetPathPoint(pathPoint);
  return contour;
}

//////////////////////////////////////////////////////
//          C l a s s   M e t h o d s               //
//////////////////////////////////////////////////////
//
// Python 'Contour' class methods.

//--------------------
// Contour_get_center
//--------------------
//
PyDoc_STRVAR(Contour_get_center_doc,
  "get_center()  \n\ 
   \n\
   Get the center of the contour. \n\
   \n\
   Args:                                    \n\
     None \n\
   \n\
   Returns list([x,y,z]): The center of the contour. \n\
");

static PyObject * 
Contour_get_center(PyContour* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto contour = self->contour;
  auto center = contour->GetCenterPoint();
  return Py_BuildValue("[d,d,d]", center[0], center[1], center[2]);
}

//----------------------------
// Contour_get_contour_points
//----------------------------
//
PyDoc_STRVAR(Contour_get_contour_points_doc,
  "get_contour_points()  \n\ 
   \n\
   Get the center of the contour. \n\
   \n\
   Args:                                    \n\
     None \n\
   \n\
   Returns list([x,y,z]): The center of the contour. \n\
");

static PyObject *
Contour_get_contour_points(PyContour* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto contour = self->contour;
  auto contour_points = contour->GetContourPoints();
  auto pointList = PyList_New(contour_points.size());
  int n = 0;

  for (auto const& point : contour_points) {
      auto pointValues = PyList_New(3);
      for (int i = 0; i < 3; i++) {
          auto val = PyFloat_FromDouble((double)point[i]); 
          PyList_SetItem(pointValues, i, val); 
      }
      PyList_SetItem(pointList, n, pointValues); 
      n += 1;
  }

  return Py_BuildValue("N", pointList); 
}

//----------------------------
// Contour_get_control_points
//----------------------------
//
PyDoc_STRVAR(Contour_get_control_points_doc,
  "get_control_points()  \n\ 
   \n\
   Get the center of the contour. \n\
   \n\
   Args:                                    \n\
     None \n\
   \n\
   Returns list([x,y,z]): The center of the contour. \n\
");

static PyObject *
Contour_get_control_points(PyContour* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto contour = self->contour;
  auto control_points = contour->GetControlPoints();
  auto pointList = PyList_New(control_points.size());
  int n = 0;

  for (auto const& point : control_points) {
      auto pointValues = PyList_New(3);
      for (int i = 0; i < 3; i++) {
          auto val = PyFloat_FromDouble((double)point[i]); 
          PyList_SetItem(pointValues, i, val); 
      }
      PyList_SetItem(pointList, n, pointValues); 
      n += 1;
  }

  return Py_BuildValue("N", pointList); 
}

//------------------------
// Contour_get_path_point
//------------------------
//
PyDoc_STRVAR(Contour_get_path_point_doc,
  "get_path_point()  \n\ 
   \n\
   Get the contour path point. \n\
   \n\
   Args:                                    \n\
     None \n\
   \n\
   Returns dict(pos:[x,y,z], tangent:[x,y,z], rotation:[x,y,z]): The contour path point. \n\
");

static PyObject * 
Contour_get_path_point(PyContour* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto contour = self->contour;
  auto pathPoint = contour->GetPathPoint();
  return Py_BuildValue("{s:[d,d,d], s:[d,d,d], s:[d,d,d]}",
    "pos", pathPoint.pos[0], pathPoint.pos[1], pathPoint.pos[1], 
    "tangent", pathPoint.tangent[0], pathPoint.tangent[1], pathPoint.tangent[1], 
    "rotation", pathPoint.rotation[0], pathPoint.rotation[1], pathPoint.rotation[1] );
}

//------------------
// Contour_get_type
//------------------
//
PyDoc_STRVAR(Contour_get_type_doc,
  "get_type()  \n\ 
   \n\
   Get the contour type. \n\
   \n\
   Args: \n\
     None \n\
   Returns (str): contour type. \n\
");

static PyObject * 
Contour_get_type(PyContour* self, PyObject* args)
{   
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto contour = self->contour;
  auto contourType = contour->GetType();
  return Py_BuildValue("s", contourType.c_str());
}

//----------------------
// Contour_get_polydata
//----------------------
//
PyDoc_STRVAR(Contour_get_polydata_doc,
  "get_vtk_polydata()  \n\ 
   \n\
   Get the contour type. \n\
   \n\
   Args: \n\
     None \n\
   Returns (str): contour type. \n\
");

static PyObject *
Contour_get_polydata(PyContour* self, PyObject* args)
{ 
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  auto contour = self->contour;
  vtkSmartPointer<vtkPolyData> polydata = contour->CreateVtkPolyDataFromContour();
  return vtkPythonUtil::GetObjectFromPointer(polydata);
}

//=======================================================================================================
//                                   O L D   M E T H O D S
//=======================================================================================================

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

static PyObject * 
Contour_set_threshold_value(PyContour* self, PyObject* args)
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

PyDoc_STRVAR(ContourClass_doc, "Contour class functions.");

//------------------
// PyContourMethods 
//------------------
// Define the methods for the Python 'Contour' class.
//
static PyMethodDef PyContourClassMethods[] = {

  {"get_center", (PyCFunction)Contour_get_center, METH_NOARGS, Contour_get_center_doc }, 

  {"get_contour_points", (PyCFunction)Contour_get_contour_points, METH_NOARGS, Contour_get_contour_points_doc}, 

  {"get_control_points", (PyCFunction)Contour_get_control_points, METH_NOARGS, Contour_get_control_points_doc}, 

  {"get_path_point", (PyCFunction)Contour_get_path_point, METH_NOARGS, Contour_get_path_point_doc}, 

  {"get_polydata", (PyCFunction)Contour_get_polydata, METH_NOARGS, Contour_get_polydata_doc}, 

  { "get_type", (PyCFunction)Contour_get_type, METH_NOARGS, Contour_get_type_doc},


  // ======================= old methods ================================================ //
  /*

  { "area", (PyCFunction)Contour_get_area, METH_NOARGS, Contour_get_area_doc },

  {"create_smooth_contour", (PyCFunction)Contour_create_smooth_contour, METH_VARARGS, Contour_create_smooth_contour_doc },

  {"get_polydata", (PyCFunction)Contour_get_polydata, METH_VARARGS, Contour_get_polydata_doc },

  {"perimeter", (PyCFunction)Contour_get_perimeter, METH_NOARGS, Contour_get_perimeter_doc },

  {"set_control_points", (PyCFunction)Contour_set_control_points, METH_VARARGS, Contour_set_control_points_doc },

  {"set_control_points_by_radius", (PyCFunction)Contour_set_control_points_by_radius, METH_VARARGS, Contour_set_control_points_by_radius_doc },

  {"set_image", (PyCFunction)Contour_set_image, METH_VARARGS, Contour_set_image_doc },

  {"set_threshold_value", (PyCFunction)Contour_set_threshold_value, METH_VARARGS, Contour_set_threshold_value_doc },
  */

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

//-------------------
// ContourObjectInit
//-------------------
// This function is used to initialize an object after it is created.
//
// This implements the Python __init__ method for the Contour class. 
// It is called after calling the __new__ method.
//
static int
PyContourInit(PyContour* self, PyObject* args, PyObject *kwds)
{
  static int numObjs = 1;
  std::cout << "[PyContourInit] New Contour object: " << numObjs << std::endl;
  char* kernelName = nullptr;
  if (!PyArg_ParseTuple(args, "|s", &kernelName)) {
      return -1;
  }

  if (kernelName != nullptr) { 
      std::cout << "[ContourObjectInit] Kernel name: " << kernelName << std::endl;
  }
  self->contour = new Contour();
  self->id = numObjs; 
  numObjs += 1;
  return 0;
}

//---------------
// PyContourtNew
//---------------
// Create a new instance of a PyContour object.
//
// This implements the Python __new__ method. It is called before the
// __init__ method.
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

//------------------
// PyContourDealloc 
//------------------
//
static void
PyContourDealloc(PyContour* self)
{
  std::cout << "[PyContourDealloc] Free PyContour: " << self->id << std::endl;
  //delete self->contour;
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
  contourType.tp_methods = PyContourClassMethods;
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

