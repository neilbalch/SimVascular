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

// Define the Python 'modeling.Model' class used to for solid modeling. 
//
// The 'modeling.Model' class provides methods that operate directly on 
// the solid model, for example, getting vtk polydata representing the 
// model surface.
//
//----------------------
// PyModelingModel 
//----------------------
// The Python modeling.Model class internal data.
//
typedef struct {
  PyObject_HEAD
  int id;
  SolidModel_KernelT kernel;
  cvSolidModel* solidModel;
} PyModelingModel;

//////////////////////////////////////////////////////
//          U t i l i t y   F u n c t i o n s       //
//////////////////////////////////////////////////////

//-------------------------
// CheckSimplificationName
//-------------------------
// Check for a valid model simplification name. 
//
// Returns the equivalent ModelingModel_SimplifyT type 
// or SM_Simplify_Invalid if the name is not valid. 
//
static SolidModel_SimplifyT
CheckSimplificationName(PyUtilApiFunction& api, char* name)
{
  if (!name) {
      return SM_Simplify_All;
  }

  auto smpType = SolidModel_SimplifyT_StrToEnum(name);
  if (smpType == SM_Simplify_Invalid ) {
      api.error("Unknown simplification argument '"+std::string(name)+ "'. Valid types are: All or None.");
  }

  return smpType;
}

/////////////////////////////////////////////////////////////////
//              C l a s s   M e t h o d s                      //
/////////////////////////////////////////////////////////////////
//
// Python API functions for the Python solid.Model class. 

//---------------------
// ModelingModel_apply4x4 
//---------------------
//
// [TODO:DaveP] The Apply4x4() method is not implemented.
//
PyDoc_STRVAR(ModelingModel_apply4x4_doc,
" apply4x4(matrix)  \n\ 
  \n\
  Apply a 4x4 transformation matrix to the solid model. \n\
  \n\
  Args: \n\
    matrix (4*[4*[double]]): A list of four lists representing the elements of a 4x4 transformation matrix. \n\
");

static PyObject *  
ModelingModel_apply4x4(PyModelingModel *self ,PyObject* args)
{
  auto api = PyUtilApiFunction("O", PyRunTimeErr, __func__);
  PyObject* matrixArg;

  if (!PyArg_ParseTuple(args, api.format, &matrixArg)) {
      return api.argsError();
  }

  if (PyList_Size(matrixArg) != 4) {
      api.error("The matrix argument is not a 4x4 matrix.");
      return nullptr;
  }

  // Extract the 4x4 matrix.
  //
  double matrix[4][4];
  for (int i = 0; i < PyList_Size(matrixArg); i++) {
      auto rowList = PyList_GetItem(matrixArg, i);
      if (PyList_Size(rowList) != 4) {
          api.error("The matrix argument is not a 4x4 matrix.");
          return nullptr;
      }
      for (int j = 0; j < PyList_Size(rowList); j++) {
          matrix[i][j] = PyFloat_AsDouble(PyList_GetItem(rowList,j));
          std::cout << "[ModelingModel.apply4x4] matrix[i][j] " << matrix[i][j] << std::endl; 
      }
  }

  auto model = self->solidModel; 

  if (model->Apply4x4(matrix) != SV_OK) {
      api.error("Error applying a 4x4 matrix to the solid model.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

PyDoc_STRVAR(ModelingModel_calculate_boundary_faces_doc,
"calculate_boundary_faces(angle)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
ModelingModel_calculate_boundary_faces(PyModelingModel* self, PyObject* args)
{
  auto api = PyUtilApiFunction("d", PyRunTimeErr, __func__);
  double angle = 0.0;

  if (!PyArg_ParseTuple(args,api.format, &angle)) {
      return api.argsError();
  }

  if (angle < 0.0) {
      api.error("The angle argument < 0.0.");
      return nullptr;
  }

  auto model = self->solidModel;

  if (model->GetBoundaryFaces(angle) != SV_OK ) {
      api.error("Error calculating boundary faces for the solid model using angle '" + std::to_string(angle) + ".");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------------
// ModelingModel_check 
//------------------
//
// [TODO:DaveP] The cvModelingModel method is not implemented.
//
PyDoc_STRVAR(ModelingModel_check_doc,
" check()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *
ModelingModel_check(PyModelingModel *self ,PyObject* args)
{
  auto model = self->solidModel;
  int nerr;
  model->Check(&nerr);
  return Py_BuildValue("i",nerr);
}

//------------------------------
// ModelingModel_classify_point 
//------------------------------
//
PyDoc_STRVAR(ModelingModel_classify_point_doc,
" classify_point(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
ModelingModel_classify_point(PyModelingModel* self ,PyObject* args)
{
  auto api = PyUtilApiFunction("dd|di", PyRunTimeErr, __func__);
  double x, y;
  double z = std::numeric_limits<double>::infinity();
  int v = 0;

  if (!PyArg_ParseTuple(args, api.format, &x, &y, &z, &v)) {
      return api.argsError();
  }

  auto model = self->solidModel;

  // Get the spatial and topological dimention.
  int tdim, sdim;
  model->GetTopoDim(&tdim);
  model->GetSpatialDim(&sdim);

  // Classify point.
  int result;
  int status;
  if (!std::isinf(z)) {
      status = model->ClassifyPt(x, y, z, v, &result);
  } else {
      if ((tdim == 2) && (sdim == 2)) {
          status = model->ClassifyPt( x, y, v, &result);
      } else {
          api.error("The solid model must have a topological and spatial dimension of two.");
          return nullptr;
      }
  }

  if (status != SV_OK) {
      api.error("Error classifying a point for the solid model.");
      return nullptr;
  }

  return Py_BuildValue("d", result);
}

//-------------------------
// ModelingModel_delete_faces 
//-------------------------
//
// [TODO:DaveP] This function does not work. 
//
PyDoc_STRVAR(ModelingModel_delete_faces_doc,
" delete_faces(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
ModelingModel_delete_faces(PyModelingModel* self, PyObject* args)
{
  auto api = PyUtilApiFunction("O", PyRunTimeErr, __func__);
  PyObject* faceListArg;

  if (!PyArg_ParseTuple(args, api.format, &faceListArg)) {
      return api.argsError();
  }

  if (PyList_Size(faceListArg) == 0) {
      return SV_PYTHON_OK;
  }

  int numFaces;
  int *faces;
  auto model = self->solidModel;
  
  if (model->GetFaceIds(&numFaces, &faces) != SV_OK) {
      api.error("Error getting the face IDs for the solid model.");
      return nullptr;
  }

  // Create list of faces to delete.
  std::vector<int> faceList;
  for (int i = 0; i < PyList_Size(faceListArg); i++) {
      auto faceID = PyLong_AsLong(PyList_GetItem(faceListArg,i));
      bool faceFound = false;
      for (int i = 0; i < numFaces; i++) {
          if (faceID == faces[i]) {
              faceFound = true;
              break;
          }
      }
      if (!faceFound) {
          delete faces;
          api.error("The face ID " + std::to_string(faceID) + " is not a valid face ID for the model.");
          return nullptr;
      }
      faceList.push_back(faceID);
  }

  delete faces;

  if (model->DeleteFaces(faceList.size(), faceList.data()) != SV_OK) {
      api.error("Error deleting faces for the solid model."); 
  }

  return SV_PYTHON_OK;
}

//--------------------------
// ModelingModel_find_centroid
//--------------------------
//
PyDoc_STRVAR(ModelingModel_find_centroid_doc,
" find_centroid()  \n\
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject *  
ModelingModel_find_centroid(PyModelingModel* self, PyObject* args)
{
  auto api = PyUtilApiFunction("", PyRunTimeErr, __func__);
  auto model = self->solidModel;

  int sdim;
  if (model->GetSpatialDim(&sdim) != SV_OK) {
      api.error("Unable to get the spatial dimension of the solid model.");
      return nullptr;
  }

  if ((sdim != 2) && (sdim != 3)) {
      api.error("The spatial dimension " + std::to_string(sdim) + " is not supported.");
      return nullptr;
  }

  double centroid[3];
  if (model->FindCentroid(centroid) != SV_OK) {
      api.error("Error finding centroid of the solid model.");
      return nullptr;
  }

  // Return the center.
  PyObject* tmpPy = PyList_New(sdim);
  for (int i = 0; i < sdim; i++) {
      PyList_SetItem(tmpPy, i, PyFloat_FromDouble(centroid[i]));
  }

  return tmpPy;
}

//-------------------------
// ModelingModel_get_face_ids 
//-------------------------
//
PyDoc_STRVAR(ModelingModel_get_face_ids_doc,
" get_face_ids_doc()  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
");

static PyObject * 
ModelingModel_get_face_ids(PyModelingModel* self, PyObject* args)
{
  auto api = PyUtilApiFunction("", PyRunTimeErr, __func__);

  auto model = self->solidModel; 
  int numFaces;
  int *faces;

  if (model->GetFaceIds(&numFaces, &faces) != SV_OK) {
      api.error("Error getting the face IDs for the solid model."); 
      return nullptr;
  }

  if (numFaces == 0) {
      Py_INCREF(Py_None);
      return Py_None;
  }

  auto faceList = PyList_New(numFaces);

  for (int i = 0; i < numFaces; i++) {
      auto faceID = faces[i];
      PyList_SetItem(faceList, i, PyLong_FromLong(faceID));
  }

  delete faces;
  return faceList;
}

//----------------------------
// ModelingModel_get_face_normal 
//----------------------------
//[TODO:DaveP] The C++ cvModelingModel GetFaceNormal() method is not implemented.
//
PyDoc_STRVAR(ModelingModel_get_face_normal_doc,
" get_face_normal(face_id, u, v)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
ModelingModel_get_face_normal(PyModelingModel* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("idd", PyRunTimeErr, __func__);
  static char *keywords[] = {"face_id", "u", "v", NULL};
  int faceID;
  double u,v;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &faceID, &u, &v)) { 
      return api.argsError();
  }

  auto model = self->solidModel;
  double normal[3];

  if (model->GetFaceNormal(faceID, u, v, normal) == SV_ERROR ) {
      api.error("Error getting the face normal for the solid model face ID '" + std::to_string(faceID) + "'.");
      return nullptr;
  }

  return Py_BuildValue("ddd",normal[0],normal[1],normal[2]);
}

//------------------------------
// ModelingModel_get_face_polydata
//------------------------------
//
PyDoc_STRVAR(ModelingModel_get_face_polydata_doc,
" get_face_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject * 
ModelingModel_get_face_polydata(PyModelingModel* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("i|d", PyRunTimeErr, __func__);
  static char *keywords[] = {"face_id", "max_dist", NULL};
  int faceID;
  double max_dist = -1.0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &faceID, &max_dist)) { 
      return api.argsError();
  }

  // Check the face ID argument.
  //
  if (faceID <= 0) {
      api.error("The face ID argument <= 0.");
      return nullptr;
  }

  int numFaces;
  int *faces;
  auto model = self->solidModel;
  
  if (model->GetFaceIds(&numFaces, &faces) != SV_OK) {
      api.error("Error getting the face IDs for the solid model.");
      return nullptr;
  }

  bool faceFound = false;
  for (int i = 0; i < numFaces; i++) {
      if (faceID == faces[i]) {
          faceFound = true;
          break; 
      }
  }

  if (!faceFound) {
      api.error("The face ID argument is not a valid face ID for the model.");
      return nullptr;
  }

  int useMaxDist = 0;
  if (max_dist > 0) {
      useMaxDist = 1;
  }

  // Get the cvPolyData:
  //
  auto cvPolydata = model->GetFacePolyData(faceID,useMaxDist,max_dist);
  if (cvPolydata == NULL) {
      api.error("Error getting polydata for the solid model face ID '" + std::to_string(faceID) + "'.");
      return nullptr;
  }
  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata = cvPolydata->GetVtkPolyData();
  if (polydata == NULL) {
      api.error("Error getting polydata for the solid model face ID '" + std::to_string(faceID) + "'.");
      return nullptr;
  }

  return PyUtilGetVtkObject(api, polydata); 
}

//-------------------------
// ModelingModel_get_polydata
//-------------------------
//
PyDoc_STRVAR(ModelingModel_get_polydata_doc,
" get_polydata(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args: \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *  
ModelingModel_get_polydata(PyModelingModel *self, PyObject* args)
{
  std::cout << " " << std::endl;
  std::cout << " ========== ModelingModel_get_polydata ==========" << std::endl;
  auto api = PyUtilApiFunction("|d", PyRunTimeErr, __func__);
  double max_dist = -1.0;

  if (!PyArg_ParseTuple(args, api.format, &max_dist)) {
      return api.argsError();
  }

  auto model = self->solidModel; 

  int useMaxDist = 0;
  if (max_dist > 0) {
      useMaxDist = 1;
  }

  // Get the cvPolyData:
  //
  auto cvPolydata = model->GetPolyData(useMaxDist, max_dist);
  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->DeepCopy(cvPolydata->GetVtkPolyData());
  if (polydata == NULL) {
      api.error("Could not get polydata for the solid model.");
      return nullptr;
  }

  return PyUtilGetVtkObject(api, polydata); 
}

//------------------
// ModelingModel_write
//------------------
//
PyDoc_STRVAR(ModelingModel_write_doc,
" write(file_name)  \n\ 
  \n\
  Write the solid model to a file in its native format. \n\
  \n\
  Args: \n\
    file_name (str): Name in the file to write the model to. \n\
");

static PyObject * 
ModelingModel_write(PyModelingModel* self, PyObject* args, PyObject* kwargs)
{
  auto api = PyUtilApiFunction("ss|i", PyRunTimeErr, __func__);
  static char *keywords[] = {"file_name", "format", "version", NULL};
  char* fileFormat;
  char* fileName;
  int fileVersion = 0;

  if (!PyArg_ParseTupleAndKeywords(args, kwargs, api.format, keywords, &fileName, &fileFormat, &fileVersion)) { 
      return api.argsError();
  }
  auto model = self->solidModel;
  std::cout << "[ModelingModel_write] " << std::endl;
  std::cout << "[ModelingModel_write] kernel: " << self->kernel << std::endl;

  // Add format as file extension. 
  //
  std::string fullFileName = std::string(fileName);
  auto extension = fullFileName.substr(fullFileName.find_last_of(".") + 1);
  if (extension != fullFileName) {
      std::cout << "[ModelingModel_write] extension: " << extension << std::endl;
      api.error("The file name argument has a file extension '" + extension + "'.");
      return nullptr;
  }
  fullFileName += "." + std::string(fileFormat);
  std::vector<char> cstr(fullFileName.c_str(), fullFileName.c_str() + fullFileName.size() + 1);
  std::cout << "[ModelingModel_write] fullFileName: " << fullFileName << std::endl;

  if (model->WriteNative(fileVersion, cstr.data()) != SV_OK) {
      api.error("Error writing the solid model to the file '" + std::string(fileName) + 
        "' using version '" + std::to_string(fileVersion)+"'."); 
      return nullptr;
  }

  Py_RETURN_NONE;
}

////////////////////////////////////////////////////////
//           C l a s s    D e f i n i t i o n         //
////////////////////////////////////////////////////////

static char* MODELING_MODEL_CLASS = "Model";

// Dotted name that includes both the module name and 
// the name of the type within the module.
static char* MODELING_MODEL_MODULE_CLASS = "modeling.Model";

PyDoc_STRVAR(ModelingModelClass_doc, "modeling model class methods.");

//--------------------------
// PyModelingModelMethods
//--------------------------
// Define method names for ModelingModel class 
//
static PyMethodDef PyModelingModelMethods[] = {

  // [TODO:DaveP] The cvModelingModel Apply4x4() method is not implemented.
  // { "apply4x4", (PyCFunction)ModelingModel_apply4x4, METH_VARARGS, ModelingModel_apply4x4_doc },

  { "calculate_boundary_faces", (PyCFunction)ModelingModel_calculate_boundary_faces, METH_VARARGS, ModelingModel_calculate_boundary_faces_doc},

  { "delete_faces", (PyCFunction)ModelingModel_delete_faces, METH_VARARGS, ModelingModel_delete_faces_doc },

  // [TODO:DaveP] The cvModelingModel method is not implemented.
  //{ "check", (PyCFunction)ModelingModel_check, METH_VARARGS, ModelingModel_check_doc },

  //  [TODO:DaveP] The C++ cvModelingModel ClassifyPt() method is not implemented.
  //{ "classify_point", (PyCFunction)ModelingModel_classify_point, METH_VARARGS,   ModelingModel_classify_point_doc },

  // [TODO:DaveP] The cvModelingModel method is not implemented.
  //{ "find_centroid", (PyCFunction)ModelingModel_find_centroid, METH_VARARGS, ModelingModel_find_centroid_doc },

  { "get_face_ids", (PyCFunction)ModelingModel_get_face_ids, METH_NOARGS, ModelingModel_get_face_ids_doc },

  //[TODO:DaveP] The C++ cvModelingModel GetFaceNormal() method is not implemented.
  //{ "get_face_normal", (PyCFunction)ModelingModel_get_face_normal, METH_VARARGS | METH_KEYWORDS, ModelingModel_get_face_normal_doc },

  { "get_face_polydata", (PyCFunction)ModelingModel_get_face_polydata, METH_VARARGS|METH_KEYWORDS, ModelingModel_get_face_polydata_doc},

  { "get_polydata", (PyCFunction)ModelingModel_get_polydata, METH_VARARGS, ModelingModel_get_polydata_doc },

  { "write", (PyCFunction)ModelingModel_write, METH_VARARGS|METH_KEYWORDS, ModelingModel_write_doc },

  {NULL,NULL}

};

//------------------
// PyModelingModelInit 
//------------------
// This is the __init__() method for the ModelingModel class. 
//
// This function is used to initialize an object after it is created.
//
static int
PyModelingModelInit(PyModelingModel* self, PyObject* args, PyObject *kwds)
{
  std::cout << "[PyModelingModelInit] ========== PyModelingModelInit ========== " << std::endl;
  auto api = PyUtilApiFunction("", PyRunTimeErr, "ModelingModel");
  static int numObjs = 1;
  std::cout << "[PyModelingModelInit] New PyModelingModel object: " << numObjs << std::endl;
  char* kernelName = nullptr; 
  if (!PyArg_ParseTuple(args, "s", &kernelName)) {
      return -1;
  }
  std::cout << "[PyModelingModelInit] Kernel name: " << kernelName << std::endl;
  auto kernel = kernelNameEnumMap.at(std::string(kernelName));
  cvSolidModel* solidModel;

  try {
      solidModel = CvSolidModelCtorMap[kernel]();
  } catch (const std::bad_function_call& except) {
      api.error("The '" + std::string(kernelName) + "' kernel is not supported.");
      return -1;
  }

  self->id = numObjs;
  self->kernel = kernel; 
  self->solidModel = solidModel; 
  numObjs += 1;
  return 0;
}

//-----------------------
// PyModelingModelType 
//-----------------------
// This is the definition of the ModelingModel class.
//
// The type object stores a large number of values, mostly C function pointers, 
// each of which implements a small part of the type’s functionality.
//
static PyTypeObject PyModelingModelType = {
  PyVarObject_HEAD_INIT(NULL, 0)
  .tp_name = MODELING_MODEL_MODULE_CLASS, 
  .tp_basicsize = sizeof(PyModelingModel) 
};

//-----------------
// PyModelingModelNew 
//-----------------
//
static PyObject *
PyModelingModelNew(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  std::cout << "[PyModelingModelNew] New ModelingModel" << std::endl;
  auto api = PyUtilApiFunction("s", PyRunTimeErr, "ModelingModel");
  char* kernelName = nullptr; 
  if (!PyArg_ParseTuple(args, api.format, &kernelName)) {
      return api.argsError();
  }

  SolidModel_KernelT kernel;

  try {
      kernel = kernelNameEnumMap.at(std::string(kernelName));
  } catch (const std::out_of_range& except) {
      auto msg = "Unknown kernel name '" + std::string(kernelName) + "'." +
          " Valid names are: " + kernelValidNames + ".";
      api.error(msg);
      return nullptr;
  }

  auto self = (PyModelingModel*)type->tp_alloc(type, 0);
  if (self != NULL) {
      //self->id = 1;
  }

  return (PyObject *) self;
}

//---------------------
// PyModelingModelDealloc 
//---------------------
//
static void
PyModelingModelDealloc(PyModelingModel* self)
{
  std::cout << "[PyModelingModelDealloc] Free PyModelingModel: " << self->id << std::endl;
  //delete self->solidModel;
  Py_TYPE(self)->tp_free(self);
}

//----------------------------
// SetModelingModelTypeFields 
//----------------------------
// Set the Python type object fields that stores ModelingModel data. 
//
// Need to set the fields here because g++ does not suppor non-trivial 
// designated initializers. 
//
static void
SetModelingModelTypeFields(PyTypeObject& solidModelType)
{
  // Doc string for this type.
  solidModelType.tp_doc = ModelingModelClass_doc; 
  // Object creation function, equivalent to the Python __new__() method. 
  // The generic handler creates a new instance using the tp_alloc field.
  solidModelType.tp_new = PyModelingModelNew;
  //solidModelType.tp_new = PyType_GenericNew,
  solidModelType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
  solidModelType.tp_init = (initproc)PyModelingModelInit;
  solidModelType.tp_dealloc = (destructor)PyModelingModelDealloc;
  solidModelType.tp_methods = PyModelingModelMethods;
};

//-------------------------
// CreateModelingModelType 
//-------------------------
static PyModelingModel * 
CreateModelingModelType()
{
  return PyObject_New(PyModelingModel, &PyModelingModelType);
}
