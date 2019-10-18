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

// The functions defined here implement the SV Python API 'path' Module. 
//
// The module name is 'path'. The module defines a 'Path' class used
// to store path data. The 'Path' class cannot be imported and must
// be used prefixed by the module name. For example
//
//     ctr = contour.Countour()
//
// A Python exception sv.contour.ContourException is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.contour.ContourException:
//

#include "SimVascular.h"
#include "SimVascular_python.h"
#include "Python.h"

#include "sv3_PathElement.h"
#include "sv3_PathElement_init_py.h"
#include "sv_PyUtils.h"

#include <stdio.h>
#include <string.h>
#include <array>
#include <iostream>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
#include "sv_PolyData.h"
#include "vtkSmartPointer.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif
// Globals:
// --------

#include "sv2_globals.h"
using sv3::PathElement;

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyPath();
#elif PYTHON_MAJOR_VERSION == 3
PyMODINIT_FUNC PyInit_pyPath();
#endif

// Exception type used by PyErr_SetString() to set the for the error indicator.
PyObject* PyRunTimeErr;


//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

// -------------------
// sv4Path_new_object
// -------------------
// This function creates a PathElement for the object.
//
// The PathElement stores the path control points use for curve interpolation
// and points sampled along the interpolating curve.
//
// [TODO:DaveP] The methodName, calcNumm, and splacing args
//   are not used. Why?
//
PyDoc_STRVAR(sv4Path_new_object_doc,
  "Path_new_object(name, path) \n\ 
   \n\
   Create the path element. \n\
   \n\
   Args: \n\
     name (str): The name of the path to create. \n\
");

static PyObject * 
sv4Path_new_object(pyPath* self, PyObject* args)
{
  char* objName;
  //char* methodName;
  //int calcNum=100, splacing=0;

  std::string functionName = svPyUtilGetFunctionName(__func__);
  std::string msgp = svPyUtilGetMsgPrefix(functionName);
  std::string format = "s:" + functionName;
  //std::string format = "s|sii:" + functionName;
  
  if (!PyArg_ParseTuple(args, format.c_str(), &objName)) {
  //if (!PyArg_ParseTuple(args, format.c_str(), &objName, &methodName, &calcNum, &splacing)) {
      return svPyUtilResetException(PyRunTimeErr);
  }
  
  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(objName)) {
      auto msg = msgp + "The Path object '" + objName + "' is already in the repository.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Create new path,
  PathElement *geom = new PathElement();

  // Add the new path geometry to the repository. 
  if (!gRepository->Register(objName, geom)) {
    PyErr_SetString(PyRunTimeErr, "error registering obj in repository");
    delete geom;
    return nullptr;
  }

  Py_INCREF(geom);
  self->geom = geom;
  Py_DECREF(geom);
  return SV_PYTHON_OK; 
}

//--------------------
// sv4Path_get_object
//--------------------
//
// [TODO:DaveP] This is setting the geom of an existing 
// Path object from something stored in the repository!
//
//   Get rid of this function.
//
static PyObject * 
sv4Path_GetObjectCmd( pyPath* self, PyObject* args)
{
  char *pathName = NULL;
  std::string functionName = svPyUtilGetFunctionName(__func__);
  std::string msgp = svPyUtilGetMsgPrefix(functionName);
  std::string format = "s:" + functionName;

  if (!PyArg_ParseTuple(args, format.c_str(), &pathName)) {
      return svPyUtilResetException(PyRunTimeErr);
  }

  // Get path object from the repository. 
  auto rd = gRepository->GetObject(pathName);
  if (rd == nullptr) {
      auto msg = msgp + "The Path object '" + pathName + "' is not in the repository.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

 // Check that the object is a path.
 auto type = rd->GetType();
 if (type != PATH_T) {
     auto msg = msgp + "'" + pathName + "' is not a Path object.";
     PyErr_SetString(PyRunTimeErr, msg.c_str());
     return nullptr;
  }

  auto path = dynamic_cast<PathElement*> (rd);
  Py_INCREF(path);
  self->geom = path;
  Py_DECREF(path);
  return SV_PYTHON_OK; 
}

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
sv4Path_add_control_point(pyPath* self, PyObject* args)
{
  PyObject* pointArg;
  int index = -1;
  std::string functionName = svPyUtilGetFunctionName(__func__);
  std::string msgp = svPyUtilGetMsgPrefix(functionName);
  std::string format = "O|i:" + functionName;

  if (!PyArg_ParseTuple(args, format.c_str(), &pointArg, &index)) {
      return svPyUtilResetException(PyRunTimeErr);
  }

  // Check control point data.
  //
  std::string msg;
  if (!svPyUtilCheckPointData(pointArg, msg)) {
      auto emsg = msgp + "Control point argument " + msg;
      PyErr_SetString(PyRunTimeErr, emsg.c_str());
      return nullptr;
  }

/*
  if ((PyList_Size(pointArg) != 3)) {
      auto msg = msgp + "Control point argument is not a 3D point (three float values).";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  for (int i = 0; i < 3; i++) {
      if (!PyFloat_Check(PyList_GetItem(pointArg,i))) {
          auto msg = msgp + "Control point argument data at " + std::to_string(i) + " in the list is not a float.";
          PyErr_SetString(PyRunTimeErr, msg.c_str());
          return nullptr;
      }
  }
 */

  auto path = self->geom;
  if (path == NULL) {
      auto msg = msgp + "The path element data has not be created.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }
    
  std::array<double,3> point;
  point[0] = PyFloat_AsDouble(PyList_GetItem(pointArg,0));
  point[1] = PyFloat_AsDouble(PyList_GetItem(pointArg,1));
  point[2] = PyFloat_AsDouble(PyList_GetItem(pointArg,2));    
    
  if (path->SearchControlPoint(point,0) !=- 2) {
      auto msg = msgp + "The control point (" + std::to_string(point[0]) + ", " + std::to_string(point[1]) + ", " + 
        std::to_string(point[2]) + ") has already been defined for the path.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // [TODO:DaveP] What does this do?
  //
  if ((index != -1) && (index > path->GetControlPoints().size())) {
      PyErr_SetString(PyRunTimeErr,"Index exceeds path length");
      return nullptr;
  } else {
      index = path->GetInsertintIndexByDistance(point);
      path->InsertControlPoint(index,point);
  }

  Py_INCREF(path);
  self->geom = path;
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
sv4Path_remove_control_point(pyPath* self, PyObject* args)
{
  std::string functionName = svPyUtilGetFunctionName(__func__);
  std::string msgp = svPyUtilGetMsgPrefix(functionName);
  std::string format = "O|i:" + functionName;

  int index;
  if (!PyArg_ParseTuple(args, format.c_str(), &index)) {
      return svPyUtilResetException(PyRunTimeErr);
  }

  auto path = self->geom;
  if (path == NULL) {
      auto msg = msgp + "The path element data has not be created.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  int numControlPoints = path->GetControlPoints().size();
  if (index >= numControlPoints) {
      auto msg = msgp + "The index argument " + std::to_string(index) + " must be < the number of control points ( "
        + std::to_string(numControlPoints) + ").";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  path->RemoveControlPoint(index);
    
  Py_INCREF(path);
  self->geom = path;
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
sv4Path_replace_control_point(pyPath* self, PyObject* args)
{
  PyObject* pointArg;
  int index;
  std::string functionName = svPyUtilGetFunctionName(__func__);
  std::string msgp = svPyUtilGetMsgPrefix(functionName);
  std::string format = "iO:" + functionName;

  if (!PyArg_ParseTuple(args, format.c_str(), &index, &pointArg)) {
    return svPyUtilResetException(PyRunTimeErr);
  }
    
  auto path = self->geom;
  if (path == NULL) {
      auto msg = msgp + "The path element data has not be created.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  std::string msg;
  if (!svPyUtilCheckPointData(pointArg, msg)) {
      auto emsg = msgp + "Control point argument " + msg;
      PyErr_SetString(PyRunTimeErr, emsg.c_str());
      return nullptr;
  }
    
  std::array<double,3> point;
  point[0] = PyFloat_AsDouble(PyList_GetItem(pointArg,0));
  point[1] = PyFloat_AsDouble(PyList_GetItem(pointArg,1));
  point[2] = PyFloat_AsDouble(PyList_GetItem(pointArg,2));    
    
  int numControlPoints = path->GetControlPoints().size();
  if (index >= numControlPoints) { 
      auto msg = msgp + "The index argument " + std::to_string(index) + " must be < the number of control points ( " 
        + std::to_string(numControlPoints) + ")."; 
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  if (index < 0 ) { 
      auto msg = msgp + "The index argument " + std::to_string(index) + " must be >= 0 and < the number of control points (" 
        + std::to_string(numControlPoints) + ")."; 
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  path->SetControlPoint(index, point);
        
  Py_INCREF(path);
  self->geom = path;
  Py_DECREF(path);
  return SV_PYTHON_OK; 
}

//-----------------
// sv4Path_smooth 
//-----------------
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
sv4Path_smooth(pyPath* self, PyObject* args)
{
  std::string functionName = svPyUtilGetFunctionName(__func__);
  std::string msgp = svPyUtilGetMsgPrefix(functionName);
  std::string format = "iii:" + functionName;

  int sampleRate, numModes, controlPointsBased;
  if (!PyArg_ParseTuple(args, format.c_str(), &sampleRate, &numModes, &controlPointsBased)) {
    return svPyUtilResetException(PyRunTimeErr);
  }
    
  auto path = self->geom;
  if (path == NULL) {
      auto msg = msgp + "The path element data has not be created.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
    }  
    
  bool controlPointsBasedBool=controlPointsBased==1?true:false;
    
  path = path->CreateSmoothedPathElement(sampleRate, numModes, controlPointsBasedBool);
        
  Py_INCREF(path);
  self->geom=path;
  Py_DECREF(path);
  return SV_PYTHON_OK; 
}

// --------------------
// sv4Path_PrintCtrlPointCmd
// --------------------

PyObject* sv4Path_PrintCtrlPointCmd( pyPath* self, PyObject* args)
{
    PathElement* path = self->geom;
    std::vector<std::array<double,3> > pts = path->GetControlPoints();
    for (int i=0;i<pts.size();i++)
    {
        std::array<double,3> pt = pts[i];
        PySys_WriteStdout("Point %i, %f, %f, %f \n",i, pt[0],pt[1],pt[2]);
    }
    
    return SV_PYTHON_OK; 
}

// ---------------
// sv4Path_create
// ---------------
//
// [TODO:DaveP] Does this need to be called?
//   Every time a control point is added PathElement::ControlPointsChanged() is called
//   which recalculates the path curve points.
//
PyDoc_STRVAR(sv4Path_create_doc,
  "Path_create() \n\ 
   \n\
   Create the points along the path curve defined by its control points.  \n\
   \n\
   Args: \n\
");

static PyObject * 
sv4Path_create(pyPath* self, PyObject* args)
{
    std::string functionName = svPyUtilGetFunctionName(__func__);
    std::string msgp = svPyUtilGetMsgPrefix(functionName);
    PathElement* path = self->geom; 

    if (path == NULL) {
        auto msg = msgp + "No path element has been created for the path.";
        PyErr_SetString(PyRunTimeErr, msg.c_str());
        return nullptr;
    }  

    // Check that conrol points have be defined for the path.
    int numControlPoints = path->GetControlPoints().size();
    if (numControlPoints == 0) {
        auto msg = msgp + "Path has no control points defined for it.";
        PyErr_SetString(PyRunTimeErr, msg.c_str());
        return nullptr;
    }  

    // Create the sample points along the path curve 
    // defined by its control poitns. 
    path->CreatePathPoints();
    int num = path->GetPathPoints().size();
    if (num == 0) {
        PyErr_SetString(PyRunTimeErr,"Error creating path from control points");
        auto msg = msgp + "Error creating path contol points.";
        return nullptr;
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
sv4Path_get_num_curve_points(pyPath* self, PyObject* args)
{
  std::string functionName = svPyUtilGetFunctionName(__func__);
  std::string msgp = svPyUtilGetMsgPrefix(functionName);
  PathElement* path = self->geom;

  if (path == NULL) {
    auto msg = msgp + "The path element data has not be created.";
    PyErr_SetString(PyRunTimeErr, msg.c_str());
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
sv4Path_get_curve_points(pyPath* self, PyObject* args)
{
  std::string functionName = svPyUtilGetFunctionName(__func__);
  std::string msgp = svPyUtilGetMsgPrefix(functionName);
  PathElement* path = self->geom;

  if (path == NULL) {
    auto msg = msgp + "The path element data has not be created.";
    PyErr_SetString(PyRunTimeErr, msg.c_str());
    return nullptr;
  }  
    
  int num = path->GetPathPointNumber();
  if (num == 0) {
    auto msg = msgp + "The path does not have points created for it.";
    PyErr_SetString(PyRunTimeErr, msg.c_str());
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
    PyErr_SetString(PyRunTimeErr, "error generating pathpospt output");
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
sv4Path_get_control_points(pyPath* self, PyObject* args)
{
  std::string functionName = svPyUtilGetFunctionName(__func__);
  std::string msgp = svPyUtilGetMsgPrefix(functionName);
  PathElement* path = self->geom;

  if (path == NULL) {
    auto msg = msgp + "The path element data has not be created.";
    PyErr_SetString(PyRunTimeErr, msg.c_str());
    return nullptr;
  }

  int num = path->GetControlPointNumber();
  if (num == 0) {
    auto msg = msgp + "The path does not have control points defined for it.";
    PyErr_SetString(PyRunTimeErr, msg.c_str());
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
      PyErr_SetString(PyRunTimeErr, "Error generating path control points output.");
      return nullptr;
  }

  return output;
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
sv4Path_get_polydata(pyPath* self, PyObject* args)
{
    std::string functionName = svPyUtilGetFunctionName(__func__);
    std::string msgp = svPyUtilGetMsgPrefix(functionName);
    std::string format = "s:" + functionName;
    std::cout << "################## sv4Path_get_polydata ################" << std::endl;

    char* dstName = NULL;
    if (!PyArg_ParseTuple(args, format.c_str(), &dstName)) {
        return svPyUtilResetException(PyRunTimeErr);
    }

  auto path = self->geom;
  if (path == NULL) {
      auto msg = msgp + "The path element data has not be created.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }

  // Check that the repository object does not already exist.
  if (gRepository->Exists(dstName)) {
    auto msg = msgp + "The repository object '" + dstName + "' already exists.";
    PyErr_SetString(PyRunTimeErr, msg.c_str());
    return nullptr;
  }

  // Get the VTK polydata.
  vtkSmartPointer<vtkPolyData> vtkpd = path->CreateVtkPolyDataFromPath(true);
  cvPolyData* pd = new cvPolyData(vtkpd);

  if (pd == NULL) {
      auto msg = msgp + "Could not get polydata for the path.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      return nullptr;
  }
   
  if ( !( gRepository->Register( dstName, pd ) ) ) {
      auto msg = msgp + "Could not add the polydata to the repository.";
      PyErr_SetString(PyRunTimeErr, msg.c_str());
      delete pd;
      return nullptr;
    }
   
  return Py_None;
}


////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

int Path_pyInit()
{
#if PYTHON_MAJOR_VERSION == 2
    initpyPath();
#elif PYTHON_MAJOR_VERSION == 3
    PyInit_pyPath();
#endif
  return SV_OK;
}

//---------------------------
// Define API function names
//---------------------------
//
static PyMethodDef pyPath_methods[] = {

  {"add_control_point",
      (PyCFunction)sv4Path_add_control_point,
       METH_VARARGS,
       sv4Path_add_control_point_doc
  },

  {"create",
      (PyCFunction)sv4Path_create, 
       METH_NOARGS,
       sv4Path_create_doc
  },

  {"get_control_points",
      (PyCFunction)sv4Path_get_control_points, 
       METH_NOARGS, 
       sv4Path_get_control_points_doc
  },

  {"get_curve_points",
      (PyCFunction)sv4Path_get_curve_points, 
       METH_NOARGS, 
       sv4Path_get_curve_points_doc
  },

  {"get_num_curve_points",
      (PyCFunction)sv4Path_get_num_curve_points, 
       METH_NOARGS, 
       sv4Path_get_num_curve_points_doc
  },

  {"get_polydata",
      (PyCFunction)sv4Path_get_polydata, 
       METH_VARARGS,
       sv4Path_get_polydata_doc
  },


  /* [TODO:DaveP] Remove this or rename.
  {"GetObject", 
      (PyCFunction)sv4Path_GetObjectCmd,
       METH_VARARGS,
       NULL
  },
  */

  {"new_object", 
      (PyCFunction)sv4Path_new_object,
      METH_VARARGS,
      sv4Path_new_object_doc
  },


  /* [TODO:DaveP] Remove this, just use Python to print points.
  {"PrintPoints",
      (PyCFunction)sv4Path_PrintCtrlPointCmd, 
       METH_NOARGS,
       NULL
  },
  */

  {"remove_control_point",
      (PyCFunction)sv4Path_remove_control_point,
       METH_VARARGS,
       sv4Path_remove_control_point_doc
  },

  {"replace_control_point",
      (PyCFunction)sv4Path_replace_control_point,
       METH_VARARGS,
       sv4Path_replace_control_point_doc
  },


  {"smooth",
      (PyCFunction)sv4Path_smooth, 
       METH_VARARGS,
       sv4Path_smooth_doc
  },

  {NULL,NULL}
};

//-------------
// pyPath_init
//-------------
// This is the __init__() method for the Path class. 
//
// This function is used to initialize an object after it is created.
//
static int 
pyPath_init(pyPath* self, PyObject* args)
{
  //fprintf(stdout,"pyPath initialized.\n");
  return SV_OK;
}

//-----------------------------------
// Define the pyPathType type object
//-----------------------------------
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject pyPathType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "path.Path",               /* tp_name */
  sizeof(pyPath),            /* tp_basicsize */
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
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
  "Path  objects",           /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  pyPath_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)pyPath_init,                            /* tp_init */
  0,                         /* tp_alloc */
  0,                  /* tp_new */
};

static PyMethodDef pyPathModule_methods[] =
{
    {NULL,NULL}
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MODULE_NAME = "path";
static char* MODULE_PATH_CLASS = "Path";
static char* MODULE_EXCEPTION = "path.PathException";
static char* MODULE_EXCEPTION_OBJECT = "PathException";

PyDoc_STRVAR(Path_doc, "path module functions");

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
//
static struct PyModuleDef pyPathModule = {
   m_base,
   MODULE_NAME,
   Path_doc, 
   perInterpreterStateSize,
   pyPathModule_methods
};

//---------------
// PyInit_pyPath
//---------------
// The initialization function called by the Python interpreter when the module is loaded.
//
// [TODO:Davep] The global 'gRepository' is created here, as it is in all other modules init
//     function. Why is this not created in main()?
//
PyMODINIT_FUNC PyInit_pyPath()
{

  if (gRepository == NULL) {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from sv3_PathElement_init\n");
  }

  // Initialize Path class.
  //
  pyPathType.tp_new = PyType_GenericNew;

  if (PyType_Ready(&pyPathType) < 0) {
    fprintf(stdout, "Error initilizing PathType \n");
    return SV_PYTHON_ERROR;
  }

  // Create the path module.
  auto module = PyModule_Create(&pyPathModule);
  if (module == NULL) {
    fprintf(stdout,"Error in initializing 'path' module \n");
    return SV_PYTHON_ERROR;
  }

  // Add path.PathException exception.
  //
  // This defines a Python exception named sv.path.PathException.
  // This can be used in a 'try' statement with an 'except' clause 
  //     'except sv.path.PathException:'
  // 
  PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION, NULL, NULL);
  PyModule_AddObject(module, MODULE_EXCEPTION_OBJECT, PyRunTimeErr);

  Py_INCREF(&pyPathType);
  PyModule_AddObject(module, MODULE_PATH_CLASS, (PyObject*)&pyPathType);
  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyPath()

{
  // Associate the mesh registrar with the python interpreter so it can be
  // retrieved by the DLLs.
  if (gRepository==NULL)
  {
    gRepository = new cvRepository();
    fprintf(stdout,"New gRepository created from cv_mesh_init\n");
  }

  // Initialize
  pyPathType.tp_new=PyType_GenericNew;
  if (PyType_Ready(&pyPathType)<0)
  {
    fprintf(stdout,"Error in pyPathType\n");
    return;
  }
  PyObject* pythonC;
  pythonC = Py_InitModule("pyPath",pyPathModule_methods);
  if(pythonC==NULL)
  {
    fprintf(stdout,"Error in initializing pyPath\n");
    return;
  }
  PyRunTimeErr = PyErr_NewException("pyPath.error",NULL,NULL);
  PyModule_AddObject(pythonC,"error",PyRunTimeErr);
  Py_INCREF(&pyPathType);
  PyModule_AddObject(pythonC,"pyPath",(PyObject*)&pyPathType);
  return ;

}
#endif

