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

// The functions defined here implement the SV Python API itk utils module. 
//
// The module name is 'itk_utils'. 
//
#include "SimVascular.h"
#include "Python.h"
#include "sv_PyUtils.h"
#include "vtkPythonUtil.h"
#include "sv3_ITKLSet_PYTHON_Macros.h"
#include <stdio.h>
#include <string.h>

#include "sv3_ITKLevelSet.h"
#include "sv2_LevelSetVelocity.h"
#include "sv_Repository.h"
#include "sv_SolidModel.h"
#include "sv_misc_utils.h"
#include "sv_arg.h"
#include "itkVersion.h"

#include "sv3_ITKLset_ITKUtils.h"
#include "sv3_ITKUtils_init_py.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
PyObject *PyRunTimeErr;

//////////////////////////////////////////////////////
//        U t i l i t y     F u n c t i o n s       //
//////////////////////////////////////////////////////

//-------------------
// GetRepositoryData 
//-------------------
// Get repository data of the given type. 
//
static cvRepositoryData *
GetRepositoryData(SvPyUtilApiFunction& api, char* name, RepositoryDataT dataType)
{
  auto data = gRepository->GetObject(name);
  if (data == NULL) {
      api.error("'"+std::string(name)+"' is not in the repository.");
      return nullptr;
  }
  if (gRepository->GetType(name) != dataType) {
      auto typeStr = std::string(RepositoryDataT_EnumToStr(dataType));
      api.error("'" + std::string(name) + "' does not have type '" + typeStr + "'.");
      return nullptr;
  }

  return data;
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//--------------------------
// itkutils_generate_circle
//--------------------------
// Generate a polydata circle with a specified radius and location.
//
PyDoc_STRVAR(itkutils_generate_circle_doc,
  "generate_circle(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
itkutils_generate_circle( PyObject *self, PyObject *args )
{
  auto api = SvPyUtilApiFunction("sdddd", PyRunTimeErr, __func__);
  char *result;
  double r=1.0, x, y, z;

  if (!PyArg_ParseTuple(args, api.format, &result, &r, &x, &y, &z)) {
    return api.argsError();
  }

  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(result)) {
      api.error("The object '" + std::string(result) + "' is already in the repository.");
      return nullptr;
  }

  int loc[3];
  loc[0] = x;
  loc[1] = y;
  loc[2] = z;
  double center[3];
  center[0] = x;
  center[1] = y;
  center[2] = z;

  // Create a circle.
  //
  cvPolyData *obj = NULL;
  cvITKLSUtil::vtkGenerateCircle(r, center, 50, &obj);
  if (obj == NULL) {
      api.error("Error creating the circle object named '" + std::string(result) + "'.");
      return nullptr;
  }

  //Save Result
  obj->SetName( result );
  if (!gRepository->Register(obj->GetName(), obj)) {
      delete obj;
      api.error("Error adding the cicle '" + std::string(result) + "' to the repository.");
      return nullptr;
  }

  vtkSmartPointer<vtkPolyData> polydataObj = vtkSmartPointer<vtkPolyData>::New();
  polydataObj = obj->GetVtkPolyData();
  PyObject* pyVtkObj=vtkPythonUtil::GetObjectFromPointer(polydataObj);
  return pyVtkObj;
}

//----------------------------
// itkutils_polydata_to_image 
//----------------------------
// Convert polydata to a 2D image.
//
PyDoc_STRVAR(itkutils_polydata_to_image_doc,
  "polydata_to_image(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
itkutils_polydata_to_image(PyObject *self, PyObject *args )
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *inputPdName;
  char *result;

  if (!PyArg_ParseTuple(args, api.format, &inputPdName, &result)) {
    return api.argsError();
  }

  // [TODO:DaveP] can this happen?
  if (inputPdName != NULL) {
      api.error("The polydata argument is null.");
      return nullptr;
  }

  // Look up given polydata object:
  auto pd = GetRepositoryData(api, inputPdName, POLY_DATA_T);
  if (pd == NULL) {
      return nullptr;
  }
  auto vtkpd = ((cvPolyData*)pd)->GetVtkPolyData();

  // Check that the new Contour object does not already exist.
  if (gRepository->Exists(result)) {
      api.error("The object '" + std::string(result) + "' is already in the repository.");
      return nullptr;
  }

  // Do work of command
  cvStrPts *obj = NULL;
  ImgInfo tempInfo;
  cvITKLSUtil::DefaultImgInfo.Print(std::cout);

  //tempInfo.Print(std::cout);
  cvITKLSUtil::vtkPolyDataTo2DImage(vtkpd,&obj,&tempInfo);

  //Save Result
  obj->SetName( result );
  if (!gRepository->Register(obj->GetName(), obj)) {
      delete obj;
      api.error("Error adding the image '" + std::string(result) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s", obj->GetName());
}

//-----------------------------
// itkutils_polydata_to_volume
//-----------------------------
// Converts a  3D polydata to a 3D image volume
//
// [TODO:DaveP] the argument ording is odd, should 
// be: input, ref, result.
//
PyDoc_STRVAR(itkutils_polydata_to_volume_doc,
  "polydata_to_volume_doc(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
itkutils_polydata_to_volume( PyObject *self, PyObject *args )
{
  auto api = SvPyUtilApiFunction("sss", PyRunTimeErr, __func__);
  char *inputPdName;
  char *result;
  char *refName;

  if (!PyArg_ParseTuple(args, api.format, &inputPdName, &result, &refName)) {
    return api.argsError();
  }

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(result)) {
    api.error("The '" + std::string(result) + "' is already in the repository.");
    return nullptr;
  }

  // [TODO:DaveP] can this happen?
  if (&refName != NULL) {
      api.error("The reference argument is null.");
      return nullptr;
  }

  // Look up given polydata object:
  auto pd = GetRepositoryData(api, inputPdName, POLY_DATA_T);
  if (pd == NULL) {
      return nullptr;
  }
  auto vtkpd = ((cvPolyData*)pd)->GetVtkPolyData();

  // Look up given image object:
  auto ref = GetRepositoryData(api, refName, STRUCTURED_PTS_T);
  if (ref == NULL) {
      return nullptr;
  }
  auto vtkref = (vtkStructuredPoints*)((cvStrPts*)ref)->GetVtkPtr();

  // Convert polydata to volume data.
  //
  cvStrPts *obj = NULL;
  ImgInfo tempInfo = ImgInfo(vtkref);
  vtkref->Delete();
  cvITKLSUtil::vtkPolyDataToVolume(vtkpd,&obj,&tempInfo);

  //Save Result
  obj->SetName(result);
  if (!gRepository->Register(obj->GetName(), obj)) {
      delete obj;
      api.error("Error adding the volume '" + std::string(result) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",obj->GetName());
}

//----------------------
// itkutils_write_image
//----------------------
// Write an image to vtk format
//
PyDoc_STRVAR(itkutils_write_image_doc,
  "write_image(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject * 
itkutils_write_image(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *inputImgName;
  char *fname;

  if (!PyArg_ParseTuple(args, api.format, &inputImgName, &fname)) { 
    return api.argsError();
  }

  // Look up given image object:
  auto image = GetRepositoryData(api, inputImgName, STRUCTURED_PTS_T);
  if (image == NULL) {
      return nullptr;
  }
  auto vtksp = ((cvStrPts*)image)->GetVtkStructuredPoints();

  // Write the image to a file.
  //
  // [TODO:DaveP] can this fail?
  //
  std::string sfname(fname);
  cvITKLSUtil::WritePerciseVtkImage(vtksp,sfname);

  return Py_BuildValue("s",fname);
}

//--------------------------------------
// itkutils_gradient_magnitude_gaussian
//--------------------------------------
// Computer the gradient magnitude of a vtk image, after a gaussian blur.
//
//
// [TODO:DaveP] the last argumnet is wrong, should be 'd'.
//
PyDoc_STRVAR(itkutils_gradient_magnitude_gaussian_doc,
  "gradient_magnitude_gaussian_doc(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkutils_gradient_magnitude_gaussian(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("ssd", PyRunTimeErr, __func__);
  char *inputImgName;
  char *result;
  double sigma = 1;

  if (!PyArg_ParseTuple(args, api.format, &inputImgName, &result, &sigma)) {
    return api.argsError();
  }

  // Look up given image object:
  auto image = GetRepositoryData(api, inputImgName, STRUCTURED_PTS_T);
  if (image == NULL) {
      return nullptr;
  }
  auto vtksp = ((cvStrPts*)image)->GetVtkStructuredPoints();

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(result)) {
    api.error("The '" + std::string(result) + "' is already in the repository.");
    return nullptr;
  }

  // [TODO:DaveP] Do what here?
  //
  vtkStructuredPoints* vtkout = vtkStructuredPoints::New();
  ImgInfo itkinfo;
  itkinfo.SetExtent(vtksp->GetExtent());
  cvITKLSUtil::vtkGenerateFeatureImage <cvITKLSUtil::ITKFloat2DImageType,cvITKLSUtil::ITKShort2DImageType>(vtksp,vtkout,&itkinfo,sigma);

  // Save Result
  cvStrPts* obj = new cvStrPts(vtkout);
  obj->SetName(result);
  if (!gRepository->Register(obj->GetName(), obj)) {
      delete obj;
      api.error("Error adding the image gradient '" + std::string(result) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",obj->GetName());
}

//------------------------
// itkutils_gaussian_blur
//------------------------
//
PyDoc_STRVAR(itkutils_gaussian_blur_doc,
  "gaussian_blur(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkutils_gaussian_blur(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("ssd", PyRunTimeErr, __func__);
  char *inputImgName;
  char *result;
  double sigma = 1;

  if (!PyArg_ParseTuple(args, api.format, &inputImgName, &result, &sigma)) {
    return api.argsError();
  }

  // Look up given image object:
  auto image = GetRepositoryData(api, inputImgName, STRUCTURED_PTS_T);
  if (image == NULL) {
      return nullptr;
  }
  auto vtksp = ((cvStrPts*)image)->GetVtkStructuredPoints();

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(result)) {
    api.error("The '" + std::string(result) + "' is already in the repository.");
    return nullptr;
  }

  // [TODO:DaveP] Do what here?
  //
  vtkStructuredPoints* vtkout = vtkStructuredPoints::New();
  ImgInfo itkinfo;
  itkinfo.SetExtent(vtksp->GetExtent());
  cvITKLSUtil::vtkGenerateFeatureImageNoGrad
  <cvITKLSUtil::ITKFloat2DImageType,cvITKLSUtil::ITKShort2DImageType>(vtksp,vtkout,&itkinfo,sigma);

  // Save Result
  cvStrPts* obj = new cvStrPts(vtkout);
  obj->SetName( result );
  if (!gRepository->Register(obj->GetName(), obj)) {
      delete obj;
      api.error("Error adding the image gradient '" + std::string(result) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",obj->GetName());
}

//-------------------------
// itkutils_distance_image
//-------------------------
//
PyDoc_STRVAR(itkutils_distance_image_doc,
  "distance_image(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkutils_distance_image(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("sss", PyRunTimeErr, __func__);
  char *inputImgName;
  char *result;
  double thres = 0.5;

  if (!PyArg_ParseTuple(args, api.format, &inputImgName, &result, &thres)) {
    return api.argsError();
  }

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(result)) {
      api.error("The '" + std::string(result) + "' is already in the repository.");
      return nullptr;
  }

  // Look up given image object:
  auto image = GetRepositoryData(api, inputImgName, STRUCTURED_PTS_T);
  if (image == NULL) {
      return nullptr;
  }
  auto vtksp = ((cvStrPts*)image)->GetVtkStructuredPoints();

  vtkStructuredPoints* vtkout = vtkStructuredPoints::New();
  ImgInfo itkinfo;
  itkinfo.SetExtent(vtksp->GetExtent());
  cvITKLSUtil::vtkGenerateFeatureImageDistance
  <cvITKLSUtil::ITKFloat2DImageType,cvITKLSUtil::ITKShort2DImageType>(vtksp,vtkout,&itkinfo,thres);

  // Save Result
  cvStrPts* obj = new cvStrPts(vtkout);
  obj->SetName( result );
  if (!gRepository->Register(obj->GetName(), obj)) {
      delete obj;
      api.error("Error adding the image distance '" + std::string(result) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",obj->GetName());
}

//--------------------------
// itkutils_threshold_image
//--------------------------
//
PyDoc_STRVAR(itkutils_threshold_image_doc,
  "threshold_image(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkutils_threshold_image(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("ssd", PyRunTimeErr, __func__);
  char *inputImgName;
  char *result;
  double thres = 0.5;

  if (!PyArg_ParseTuple(args, api.format, &inputImgName, &result, &thres)) {
    return api.argsError();
  }

  // Look up given image object:
  auto image = GetRepositoryData(api, inputImgName, STRUCTURED_PTS_T);
  if (image == NULL) {
      return nullptr;
  }
  auto vtksp = ((cvStrPts*)image)->GetVtkStructuredPoints();

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(result)) {
    api.error("The '" + std::string(result) + "' is already in the repository.");
    return nullptr;
  }

  // Threshold image.
  //
  vtkStructuredPoints* vtkout = vtkStructuredPoints::New();
  ImgInfo itkinfo;
  itkinfo.SetExtent(vtksp->GetExtent());
  cvITKLSUtil::vtkGenerateFeatureImageThreshold
  <cvITKLSUtil::ITKFloat2DImageType,cvITKLSUtil::ITKShort2DImageType>(vtksp,vtkout,&itkinfo,thres);

  // Save Result
  cvStrPts* obj = new cvStrPts(vtkout);
  obj->SetName( result );
  if (!gRepository->Register(obj->GetName(), obj)) {
      api.error("Error adding the image distance '" + std::string(result) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",obj->GetName());
}

//---------------------------------
// itkutils_fract_edge_proximity3D
//---------------------------------
//
PyDoc_STRVAR(itkutils_fract_edge_proximity3D_doc,
  "fract_edge_proximity3D(name, mesh_file_name, solid_file_name) \n\ 
   \n\
   Create a new mesh object. \n\
   \n\
   Args: \n\
     name (str): Name of the new mesh object to store in the repository. \n\
");

static PyObject *
itkutils_fract_edge_proximity3D(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("ssddd", PyRunTimeErr, __func__);
  char *inputImgName;
  char *result;
  double sigma = 1, kappa=5, exponent=5;

  if (!PyArg_ParseTuple(args, api.format, &inputImgName, &result, &sigma, &kappa, &exponent)) {
    return api.argsError();
  }

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(result)) {
    api.error("The '" + std::string(result) + "' is already in the repository.");
    return nullptr;
  }

  // Look up given image object:
  auto image = GetRepositoryData(api, inputImgName, STRUCTURED_PTS_T);
  if (image == NULL) {
      return nullptr;
  }
  auto vtksp = ((cvStrPts*)image)->GetVtkStructuredPoints();


  // Compute fract edge proximity 3D, whatever that is.
  //
  vtkStructuredPoints* vtkout = vtkStructuredPoints::New();
  ImgInfo itkinfo = ImgInfo(vtksp);
  itkinfo.SetExtent(vtksp->GetExtent());
  itkinfo.SetMaxValue(255);
  itkinfo.SetMinValue(0);
  //itkinfo.Print(std::cout);
  typedef cvITKLSUtil::ITKFloat3DImageType IT1;
  typedef cvITKLSUtil::ITKShort3DImageType IT2;
  cvITKLSUtil::vtkGenerateEdgeProxImage<IT1,IT2>(vtksp, vtkout, &itkinfo, sigma, kappa, exponent);

  // Save Result
  cvStrPts* obj = new cvStrPts(vtkout);
  obj->SetName( result );
  if (!gRepository->Register(obj->GetName(), obj)) {
      delete obj;
      api.error("Error adding the image fract edge '" + std::string(result) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",obj->GetName());
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "itk_utils";

// -------------
// Itkutils_Init
// -------------

PyMethodDef Itkutils_methods[] = {

  {"distance_image", itkutils_distance_image, METH_VARARGS, itkutils_distance_image_doc},

  {"fract_edge_proximity3D", itkutils_fract_edge_proximity3D, METH_VARARGS, itkutils_fract_edge_proximity3D_doc},

  {"gaussian_blur", itkutils_gaussian_blur, METH_VARARGS, itkutils_gaussian_blur_doc},

  {"generate_circle", itkutils_generate_circle, METH_VARARGS, itkutils_generate_circle_doc},

  {"gradient_magnitude_gaussian", itkutils_gradient_magnitude_gaussian, METH_VARARGS, itkutils_gradient_magnitude_gaussian_doc},

  {"polydata_to_image", itkutils_polydata_to_image, METH_VARARGS, itkutils_polydata_to_image_doc},

  {"polydata_to_volume", itkutils_polydata_to_volume, METH_VARARGS, itkutils_polydata_to_volume_doc},

  {"threshold_image", itkutils_threshold_image, METH_VARARGS, itkutils_threshold_image_doc},

  {"write_image", itkutils_write_image, METH_VARARGS, itkutils_write_image_doc},

  {NULL, NULL,0,NULL},

};

#if PYTHON_MAJOR_VERSION == 3
static struct PyModuleDef Itkutilsmodule = {
   PyModuleDef_HEAD_INIT,
   MODULE_NAME,  
   "", /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   Itkutils_methods
};
#endif

PyObject*  Itkutils_pyInit( ){

  printf("  %-12s %s\n","Itk:", itk::Version::GetITKVersion());

    PyObject *pyItklsUtils;
#if PYTHON_MAJOR_VERSION == 2
    pyItklsUtils= Py_InitModule("Itkutils",Itkutils_methods);
#elif PYTHON_MAJOR_VERSION == 3
    pyItklsUtils= PyModule_Create(&Itkutilsmodule);
#endif
    PyRunTimeErr = PyErr_NewException("Itkutils.error",NULL,NULL);
    PyModule_AddObject(pyItklsUtils,"error",PyRunTimeErr);

  return pyItklsUtils;

}
