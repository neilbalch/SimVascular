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

// The functions defined here implement the SV Python API path class. 
//
// The class name is 'Path'. 
//
//     path = path.Path()
//
using sv3::PathElement;

//////////////////////////////////////////////////////
//          U t i l i t y  F u n c t i o n s        //
//////////////////////////////////////////////////////

//-----------------
// CreatePathCurve
//-----------------
//
static bool 
CreatePathCurve(PathElement* path)
{
  // Check that conrol points have be defined for the path.
  if (path->GetControlPoints().size() == 0) {
    return false;
  }  

  // Create the sample points along the path curve  defined by its control poitns. 
  path->CreatePathPoints();
  int num = path->GetPathPoints().size();
  if (num == 0) {
      //api.error("Error creating path from control points");
      return false;
  } 
  return true; 
}

//////////////////////////////////////////////////////
//          C l a s s   M e t h o d s               //
//////////////////////////////////////////////////////
//
// Python 'Path' class methods.

//--------------------------
//sv4Path_add_control_point 
//--------------------------
//
PyDoc_STRVAR(sv4Path_add_control_point_doc,
  "Path_add_control_point(point) \n\ 
   \n\
   Add a control point to a path. \n\
   \n\
   Args: \n\
     point (list[x,y,z]): A list of three floats represent the 3D coordinates of the control point. \n\
");

static PyObject * 
sv4Path_add_control_point(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("O|i", PyRunTimeErr, __func__);
  // [TODO:DaveP] should be declared as 
  //   const PyObject* pointArg;
  PyObject* pointArg;
  int index = -2;

  if (!PyArg_ParseTuple(args, api.format, &pointArg, &index)) {
      return api.argsError();
  }

  // Check control point data.
  //
  std::string emsg;
  if (!svPyUtilCheckPointData(pointArg, emsg)) {
      api.error("Control point argument " + emsg);
      return nullptr;
  }

  auto path = self->path;
  if (path == NULL) {
      api.error("The path element data has not be created.");
      return nullptr;
  }

  std::array<double,3> point;
  point[0] = PyFloat_AsDouble(PyList_GetItem(pointArg,0));
  point[1] = PyFloat_AsDouble(PyList_GetItem(pointArg,1));
  point[2] = PyFloat_AsDouble(PyList_GetItem(pointArg,2));    

  // Check if the control point is already 
  // defined for the path.
  //
  // [TODO:DaveP] Get rid of this '-2'? What about '-1'?
  //
  if (path->SearchControlPoint(point,0) !=- 2) {
      auto msg = "The control point (" + std::to_string(point[0]) + ", " + std::to_string(point[1]) + ", " + 
        std::to_string(point[2]) + ") has already been defined for the path.";
      api.error(msg);
      return nullptr;
  }

  // Set the path control point by index or by point.
  //
  if (index != -2) { 
      int numCpts = path->GetControlPoints().size();
      if (index > numCpts) {
          auto msg = "Index " + std::to_string(index) + " exceeds path length " + std::to_string(numCpts)+".";
          api.error(msg);
          return nullptr;
      }
  } else {
      index = path->GetInsertintIndexByDistance(point);
  }
  path->InsertControlPoint(index,point);

  // [TODO:DaveP] we don't need to increment non-Python objects.
  Py_INCREF(path);
  self->path = path;
  Py_DECREF(path);
  return SV_PYTHON_OK; 
}

//------------------------------
// sv4Path_remove_control_point
//------------------------------
//
PyDoc_STRVAR(sv4Path_remove_control_point_doc,
  "Path_remove_control_point(index) \n\ 
   \n\
   Remove a control point from a path. \n\
   \n\
   Args: \n\
     index (int): Index into the list of control points. 0 <= index < number of control points. \n\
");

static PyObject * 
sv4Path_remove_control_point(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int index;

  if (!PyArg_ParseTuple(args, api.format, &index)) {
      return svPyUtilResetException(PyRunTimeErr);
  }

  auto path = self->path;
  if (path == NULL) {
      api.error("The path element data has not be created.");
      return nullptr;
  }

  int numControlPoints = path->GetControlPoints().size();
  if (index >= numControlPoints) {
      api.error("The index argument " + std::to_string(index) + " must be < the number of control points ( "
        + std::to_string(numControlPoints) + ").");
      return nullptr;
  }

  path->RemoveControlPoint(index);
    
  Py_INCREF(path);
  self->path = path;
  Py_DECREF(path);
  return SV_PYTHON_OK; 
}

//-------------------------------
// sv4Path_replace_control_point
//-------------------------------
//
PyDoc_STRVAR(sv4Path_replace_control_point_doc,
  "sv4Path_replace_control_point(index, point) \n\ 
   \n\
   Replace a control point. \n\
   \n\
   Args: \n\
     index (int): Index into the list of control points. 0 <= index < number of control points. \n\
     point (list[x,y,z]): A list of three floats represent the coordinates of a 3D point. \n\
");

static PyObject * 
sv4Path_replace_control_point(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("iO", PyRunTimeErr, __func__);
  PyObject* pointArg;
  int index;

  if (!PyArg_ParseTuple(args, api.format, &index, &pointArg)) {
      return api.argsError();
  }
    
  auto path = self->path;
  if (path == NULL) {
      api.error("The path element data has not be created.");
      return nullptr;
  }

  std::string msg;
  if (!svPyUtilCheckPointData(pointArg, msg)) {
      api.error("Control point argument " + msg);
      return nullptr;
  }
    
  std::array<double,3> point;
  point[0] = PyFloat_AsDouble(PyList_GetItem(pointArg,0));
  point[1] = PyFloat_AsDouble(PyList_GetItem(pointArg,1));
  point[2] = PyFloat_AsDouble(PyList_GetItem(pointArg,2));    
    
  int numControlPoints = path->GetControlPoints().size();
  if (index >= numControlPoints) { 
      auto msg = "The index argument " + std::to_string(index) + " must be < the number of control points ( " 
        + std::to_string(numControlPoints) + ")."; 
      api.error(msg);
      return nullptr;
  }

  if (index < 0 ) { 
      auto msg = "The index argument " + std::to_string(index) + " must be >= 0 and < the number of control points (" 
        + std::to_string(numControlPoints) + ")."; 
      api.error(msg);
      return nullptr;
  }

  path->SetControlPoint(index, point);
        
  Py_INCREF(path);
  self->path = path;
  Py_DECREF(path);
  return SV_PYTHON_OK; 
}

//-----------------
// sv4Path_smooth 
//-----------------
//
// [TODO:DaveP] this should return a new smoothed Path or should it
// modify the path?
//
PyDoc_STRVAR(sv4Path_smooth_doc,
  "sv4Path_smooth(index, point) \n\ 
   \n\
   Smooth a path. \n\
   \n\
   Args: \n\
     sample_rate (int):  \n\
     num_modes (int):  \n\
     control_point_based (int):  \n\
");

static PyObject * 
sv4Path_smooth(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("iii", PyRunTimeErr, __func__);
  int sampleRate, numModes, controlPointsBased;

  if (!PyArg_ParseTuple(args, api.format, &sampleRate, &numModes, &controlPointsBased)) {
      return api.argsError();
  }
    
  auto path = self->path;
  if (path == NULL) {
      api.error("The path element data has not be created.");
      return nullptr;
    }  
    
  bool controlPointsBasedBool=controlPointsBased==1?true:false;
    
  path = path->CreateSmoothedPathElement(sampleRate, numModes, controlPointsBasedBool);
        
  Py_INCREF(path);
  self->path=path;
  Py_DECREF(path);
  return SV_PYTHON_OK; 
}

//--------------------
// sv4Path_PrintCtrlPointCmd
//--------------------
//
static PyObject * 
sv4Path_PrintCtrlPointCmd( PyPath* self, PyObject* args)
{
    PathElement* path = self->path;
    std::vector<std::array<double,3> > pts = path->GetControlPoints();
    for (int i=0;i<pts.size();i++)
    {
        std::array<double,3> pt = pts[i];
        PySys_WriteStdout("Point %i, %f, %f, %f \n",i, pt[0],pt[1],pt[2]);
    }
    
    return SV_PYTHON_OK; 
}

//------------------------------
// sv4Path_get_num_curve_points
//------------------------------
//
PyDoc_STRVAR(sv4Path_get_num_curve_points_doc,
  "Path_get_num_curve_points() \n\ 
   \n\
   Get the number of points along the path interpolating curve. \n\
   \n\
   Args: \n\
     None \n\
");

static PyObject * 
sv4Path_get_num_curve_points(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  PathElement* path = self->path;

  if (path == NULL) {
    api.error("The path element data has not be created.");
    return nullptr;
  }  

  int num = path->GetPathPointNumber();
  return Py_BuildValue("i",num);
}

//--------------------------
// sv4Path_get_curve_points
//--------------------------

PyDoc_STRVAR(sv4Path_get_curve_points_doc,
  "Path_get_curve_points() \n\ 
   \n\
   Get the points along the path interpolating curve. \n\
   \n\
   Args: \n\
     None \n\
");

static PyObject * 
sv4Path_get_curve_points(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  PathElement* path = self->path;

  if (path == NULL) {
    api.error("The path element data has not be created.");
    return nullptr;
  }  
    
  int num = path->GetPathPointNumber();
  if (num == 0) {
    api.error("The path does not have points created for it.");
    return nullptr;
  }  

  PyObject* output = PyList_New(num);

  for (int i = 0; i < num; i++) {
    PyObject* tmpList = PyList_New(3);
    std::array<double,3> pos = path->GetPathPosPoint(i);
    for (int j=0; j<3; j++)
        PyList_SetItem(tmpList,j,PyFloat_FromDouble(pos[j]));
        PyList_SetItem(output,i,tmpList);
    }
    
  if(PyErr_Occurred()!=NULL) {
    api.error("error generating pathpospt output");
    return nullptr;
  }
    
  return output;
} 

//----------------------------
// sv4Path_GetControlPts
//----------------------------
//
PyDoc_STRVAR(sv4Path_get_control_points_doc,
  "Path_get_control_points() \n\ 
   \n\
   Get the path control points. \n\
   \n\
   Args: \n\
     None \n\
");

static PyObject * 
sv4Path_get_control_points(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("", PyRunTimeErr, __func__);
  PathElement* path = self->path;

  if (path == NULL) {
    api.error("The path element data has not be created.");
    return nullptr;
  }

  int num = path->GetControlPointNumber();
  if (num == 0) {
    api.error("The path does not have control points defined for it.");
    return nullptr;
  }  

  PyObject* output = PyList_New(num);
  for (int i = 0; i < num; i++) {
      PyObject* tmpList = PyList_New(3);
      std::array<double,3> pos = path->GetControlPoint(i);
      for (int j=0; j<3; j++) {
          PyList_SetItem(tmpList,j,PyFloat_FromDouble(pos[j]));
      }
      PyList_SetItem(output,i,tmpList);
    }

  if (PyErr_Occurred() != NULL) {
      api.error("Error generating path control points output.");
      return nullptr;
  }

  return output;
}

//--------------------------
// sv4Path_get_curve_normal  
//-------------------------
//
PyDoc_STRVAR(sv4Path_get_curve_normal_doc,
  "get_curve_normal(index) \n\ 
   \n\
   Get the path curve normal at a given index. \n\
   \n\
   Args: \n\
     index (int): The index into the path's curve normals. \n\
");

static PyObject * 
sv4Path_get_curve_normal(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int indexArg;

  if (!PyArg_ParseTuple(args, api.format, &indexArg)) {
      return api.argsError();
  }

  auto path = self->path;
  if (path == NULL) {
    api.error("The path element data has not be created.");
    return nullptr;
  }

  int num = path->GetPathPointNumber();
  if ((indexArg < 0) || (indexArg >= num)) {
    api.error("The path index must be between 0 and " + std::to_string(num-1) + ".");
    return nullptr;
  }  

  auto pathPoint = path->GetPathPoint(indexArg);
  return Py_BuildValue("[d, d, d]", pathPoint.rotation[0], pathPoint.rotation[1], pathPoint.rotation[2]);
}

//-------------------------
// sv4Path_get_curve_point 
//-------------------------
//
PyDoc_STRVAR(sv4Path_get_curve_point_doc,
  "get_curve_point(index) \n\ 
   \n\
   Get the path curve point at a given index. \n\
   \n\
   Args: \n\
     index (int): The index into the path's curve points. \n\
");

static PyObject * 
sv4Path_get_curve_point(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int indexArg;

  if (!PyArg_ParseTuple(args, api.format, &indexArg)) {
      return api.argsError();
  }

  auto path = self->path;
  if (path == NULL) {
    api.error("The path element data has not be created.");
    return nullptr;
  }

  int num = path->GetPathPointNumber();
  if ((indexArg < 0) || (indexArg >= num)) {
    api.error("The path index must be between 0 and " + std::to_string(num-1) + ".");
    return nullptr;
  }  

  std::array<double,3> pos = path->GetPathPosPoint(indexArg);
  return Py_BuildValue("[d, d, d]", pos[0], pos[1], pos[2]);
}

//---------------------------
// sv4Path_get_curve_tangent 
//--------------------------
//
PyDoc_STRVAR(sv4Path_get_curve_tangent_doc,
  "get_curve_tangent(index) \n\ 
   \n\
   Get the path curve tangent at a given index. \n\
   \n\
   Args: \n\
     index (int): The index into the path's curve tangents. \n\
");

static PyObject * 
sv4Path_get_curve_tangent(PyPath* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("i", PyRunTimeErr, __func__);
  int indexArg;

  if (!PyArg_ParseTuple(args, api.format, &indexArg)) {
      return api.argsError();
  }

  auto path = self->path;
  if (path == NULL) {
    api.error("The path element data has not be created.");
    return nullptr;
  }

  int num = path->GetPathPointNumber();
  if ((indexArg < 0) || (indexArg >= num)) {
    api.error("The path index must be between 0 and " + std::to_string(num-1) + ".");
    return nullptr;
  }  

  auto pathPoint = path->GetPathPoint(indexArg);
  return Py_BuildValue("[d, d, d]", pathPoint.tangent[0], pathPoint.tangent[1], pathPoint.tangent[2]);
}

//----------------------
// sv4Path_get_polydata
//----------------------
//
PyDoc_STRVAR(sv4Path_get_polydata_doc,
  "Path_get_polydata(name) \n\ 
   \n\
   Store the polydata for the named path into the repository. \n\
   \n\
   Args: \n\
     name (str): \n\
");

static PyObject*
sv4Path_get_polydata(PyPath* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
    char* dstName = NULL;

    if (!PyArg_ParseTuple(args, api.format, &dstName)) {
      return api.argsError();
    }

  auto path = self->path;
  if (path == NULL) {
      api.error("The path element data has not be created.");
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
    api.error("The repository object '" + std::string(dstName) + "' already exists.");
    return nullptr;
  }

  // Get the VTK polydata.
  vtkSmartPointer<vtkPolyData> vtkpd = path->CreateVtkPolyDataFromPath(true);
  cvPolyData* pd = new cvPolyData(vtkpd);

  if (pd == NULL) {
      api.error("Could not get polydata for the path.");
      return nullptr;
  }
   
  if (!gRepository->Register(dstName, pd)) {
      delete pd;
      api.error("Could not add the polydata to the repository.");
      return nullptr;
    }
   
  return Py_None;
}

////////////////////////////////////////////////////////
//           C l a s s   D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* PATH_CLASS = "Path";
// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* PATH_MODULE_CLASS = "path.Path";

PyDoc_STRVAR(PathClass_doc, "Path class functions.");

//--------------------
// PyPathClassMethods 
//--------------------
// Path class methods.
//
static PyMethodDef PyPathClassMethods[] = {

  {"add_control_point", (PyCFunction)sv4Path_add_control_point, METH_VARARGS, sv4Path_add_control_point_doc },

  {"get_control_points", (PyCFunction)sv4Path_get_control_points, METH_NOARGS, sv4Path_get_control_points_doc },

  {"get_curve_normal", (PyCFunction)sv4Path_get_curve_normal, METH_VARARGS, sv4Path_get_curve_normal_doc },
  {"get_curve_point", (PyCFunction)sv4Path_get_curve_point, METH_VARARGS, sv4Path_get_curve_point_doc },
  {"get_curve_points", (PyCFunction)sv4Path_get_curve_points, METH_NOARGS, sv4Path_get_curve_points_doc },
  {"get_curve_tangent", (PyCFunction)sv4Path_get_curve_tangent, METH_VARARGS, sv4Path_get_curve_tangent_doc },

  {"get_num_curve_points", (PyCFunction)sv4Path_get_num_curve_points, METH_NOARGS, sv4Path_get_num_curve_points_doc },

  {"get_polydata", (PyCFunction)sv4Path_get_polydata, METH_VARARGS, sv4Path_get_polydata_doc },

  {"remove_control_point", (PyCFunction)sv4Path_remove_control_point, METH_VARARGS, sv4Path_remove_control_point_doc },

  {"replace_control_point", (PyCFunction)sv4Path_replace_control_point, METH_VARARGS, sv4Path_replace_control_point_doc },

  {"smooth", (PyCFunction)sv4Path_smooth, METH_VARARGS, sv4Path_smooth_doc },

  {NULL,NULL}
};

//------------
// PyPathType 
//------------
// Define the Python type object that stores Path data. 
//
// Can't set all the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
PyTypeObject PyPathType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  // Dotted name that includes both the module name and 
  // the name of the type within the module.
  PATH_MODULE_CLASS, 
  sizeof(PyPath)
};

//------------
// PyPathInit
//------------
// This is the __init__() method for the Path class. 
//
// This function is used to initialize an object after it is created.
//
static int
PyPathInit(PyPath* self, PyObject* args, PyObject *kwds)
{
  static int numObjs = 1;
  std::cout << "[PyPathInit] New Path object: " << numObjs << std::endl;
  self->path = new PathElement();
  self->id = numObjs;
  numObjs += 1;
  return 0;
}

//-----------
// PyPathNew 
//-----------
// Object creation function, equivalent to the Python __new__() method. 
// The generic handler creates a new instance using the tp_alloc field.
//
static PyObject *
PyPathNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyPathNew] PyPathNew " << std::endl;
  auto self = (PyPath*)type->tp_alloc(type, 0);
  if (self != NULL) {
      self->id = 1;
  }

  return (PyObject *) self;
}

//---------------
// PyPathDealloc 
//---------------
//
static void
PyPathDealloc(PyPath* self)
{
  std::cout << "[PyPathDealloc] Free PyPath" << std::endl;
  delete self->path;
  Py_TYPE(self)->tp_free(self);
}

//-------------------
// SetPathTypeFields 
//-------------------
// Set the Python type object fields that stores Path data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetPyPathTypeFields(PyTypeObject& pathType)
{
  // Doc string for this type.
  pathType.tp_doc = "Path  objects";
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  pathType.tp_new = PyPathNew;
  pathType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  pathType.tp_init = (initproc)PyPathInit;
  pathType.tp_dealloc = (destructor)PyPathDealloc;
  pathType.tp_methods = PyPathClassMethods;
}

//--------------
// CreatePyPath
//--------------
// Create a PyPath object. 
//
// If the path argument is not null then use it 
// for the PyPath object.
//
PyObject *
CreatePyPath(PathElement* path)
{
  std::cout << "[CreatePyPath] Create Path object ... " << std::endl;
  auto pathObj = PyObject_CallObject((PyObject*)&PyPathType, NULL);
  auto pyPath = (PyPath*)pathObj;

  if (path != nullptr) {
      delete pyPath->path; 
      pyPath->path = path; 
  }
  std::cout << "[CreatePyPath] pyPath id: " << pyPath->id << std::endl;
  std::cout << "[CreatePyPath] pathObj ref count: " << Py_REFCNT(pathObj) << std::endl;
  return pathObj;
}

