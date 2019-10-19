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

// The functions defined here implement the SV Python API image module. 
//
// The module name is 'image'. 

#include "SimVascular.h"

#include <stdio.h>
#include <string.h>
#include "sv_Repository.h"
#include "sv_RepositoryData.h"
#include "sv_StrPts.h"
#include "sv_PolyData.h"
#include "sv_arg.h"
#include "sv_misc_utils.h"
#include "sv2_read_header.h"
#include "sv2_decode.h"
#include "sv2_calc_correction_eqn.h"
#include "sv2_img_threshold.h"
#include "sv2_DistanceMap.h"
#include "sv2_mask_image_in_place.h"
#include "sv2_image_init_py.h"
#include <iostream>
#include <cstdarg>

#ifdef SV_USE_PYTHON
#include "Python.h"
#include "vtkPythonUtil.h"
#include "vtkSmartPointer.h"
#include "sv_PyUtils.h"
#endif

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
static PyObject * PyRunTimeErr;

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

//-----------------------
// GetRepositoryDataList
//-----------------------
// Get a list of repository data objects from a list of names.
//
template <typename T> 
static bool 
GetRepositoryDataList(SvPyUtilApiFunction& api, PyObject* objNames, RepositoryDataT dataType, const char* argName, std::vector<T>& objects)
{
  if (!PyList_Check(objNames)) {
      api.error("The " + std::string(argName) + " argument is not a Python list.");
      return false;
  }

  auto numObjs = PyList_Size(objNames);
  if (numObjs == 0) {
      api.error("The " + std::string(argName) + " argument list is empty.");
      return false;
  }

  objects.clear();

  // Iterate over the object names getting the objects 
  // from the repository.
  //
  for (int i = 0; i < numObjs; i++ ) {
      auto name = PyString_AsString(PyList_GetItem(objNames,i));
      if (name == nullptr) {
          api.error("The " + std::to_string(i) + "th element of the " + argName + " argument is not defined.");
          return false;
      } 
      auto obj = GetRepositoryData(api, name, dataType);
      if (obj == nullptr) { 
          return false;
      }
      objects.push_back((T)obj);
  }

  return true;
}
//------------------
// ValuesToPyString
//------------------
//
PyObject *
ValuesToPyString(const char *fmt, ...)
{
  char buffer[1000];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  return PyString_FromString(buffer);
}

//-----------
// AddValues
//-----------
// Add a variable number of values to a Python list.
//
void 
AddValues(PyObject* pylist, const char *fmt, ...)
{
  auto pyStr = ValuesToPyString(fmt);
  PyList_Append(pylist,  pyStr);
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//----------------------
// Image_read_header_5x 
//----------------------
//
PyDoc_STRVAR(Image_read_header_5x_doc,
  "read_header_5x(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
Image_read_header_5x(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__);
  char *filename;
  int readProtected = 0;

  if (!PyArg_ParseTuple(args, api.format, &filename, &readProtected)) {
    return api.argsError();
  }

  // Do work of command
  float vdims_x, vdims_y;
  int dim_x, dim_y;
  int file_hdr_size;
  float ul[3],ur[3],br[3];
  int venc;
  float vencscale;
  int vas_collapse;
  float user2, user5, user6, user7, user8, user9, user12, user13, user14;

  // These string sizes need to match those contained in the image header.
  char patid[13];
  char patname[25];
  char psdname[33];
  int magWeightFlag;
  int examNumber;
  int acquisitionTime;
  float nrm_RAS[3];
  int heart_rate;
  int im_no;
  int im_seno;

  // [TODO:DaveP] This is hideous! Could we possibly 
  // use that c++ class thingy?
  int status = mrRead_Header (filename, &vdims_x, &vdims_y,
                              &dim_x, &dim_y, &file_hdr_size,
                              ul, ur, br,&venc,&vencscale,
                              &vas_collapse,&user2, &user5, &user6,
			      &user7, &user8 ,&user9,
                              &user12, &user13 ,&user14,
                              patid, patname, psdname,
                              &magWeightFlag, &examNumber, nrm_RAS,
                              &acquisitionTime,&heart_rate,&im_no,&im_seno);
  char tmpStr[1024];

  if (status == SV_ERROR) {
    api.error("Error reading header information from the file '"+std::string(filename)+"'.");
    return nullptr;
  }

  // Return header information.
  //
  // [TODO:DaveP] why not return a dict?
  //
  PyObject *pylist = PyList_New(0);
  AddValues(pylist, "extent {%i %i}", dim_x, dim_y);
  AddValues(pylist, "voxel_dims {%.8f %.8f}", vdims_x, vdims_y); 
  AddValues(pylist, "file_hdr_size %i", file_hdr_size);
  AddValues(pylist, "top_left_corner {%.8f %.8f %.8f}",ul[0],ul[1],ul[2]);
  AddValues(pylist,"top_right_corner {%.8f %.8f %.8f}",ur[0],ur[1],ur[2]);
  AddValues(pylist,"bottom_right_corner {%.8f %.8f %.8f}",br[0],br[1],br[2]);
  AddValues(pylist,"venc %i",venc);
  AddValues(pylist,"vencscale %.8f",vencscale);
  AddValues(pylist,"vas_collapse %i",vas_collapse);
  AddValues(pylist,"user2 %f",user2);
  AddValues(pylist,"user5 %f",user5);
  AddValues(pylist,"user6 %f",user6);
  AddValues(pylist,"user7 %f",user7);
  AddValues(pylist,"user8 %f",user8);
  AddValues(pylist,"user9 %f",user9);
  AddValues(pylist,"user12 %f",user12);
  AddValues(pylist,"user13 %f",user13);
  AddValues(pylist,"user14 %f",user14);

  if (readProtected != 0) {
    AddValues(pylist,"patient_id {%s}",patid);
    AddValues(pylist,"patient_name {%s}",patname);
    AddValues(pylist,"exam_number %i",examNumber);
    AddValues(pylist,"acquisition_time %i",acquisitionTime);
  }

  AddValues(pylist,"psdname {%s}",psdname);
  AddValues(pylist,"mag_weight_flag %i",magWeightFlag);
  AddValues(pylist,"normal_to_plane {%.8f %.8f %.8f}",nrm_RAS[0],nrm_RAS[1],nrm_RAS[2]);
  AddValues(pylist,"heart_rate_bpm %i",heart_rate);
  AddValues(pylist,"im_no %i",im_no);
  AddValues(pylist,"im_seno %i",im_seno);

  return pylist;
}

//--------------
// Image_decode 
//--------------
//
PyDoc_STRVAR(Image_decode_doc,
  "decode(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
Image_decode(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("ssdd|s", PyRunTimeErr, __func__);
  char *phasename;
  char *result;
  double venc,vencscale;
  char *magname = NULL;

  if (!PyArg_ParseTuple(args, api.format, &phasename, &result, &venc, &vencscale, &magname)) {
    return api.argsError();
  }

  // Do work of command
  char tmpStr[1024];
  tmpStr[0]='\0';

  RepositoryDataT type;
  cvRepositoryData *img;
  vtkStructuredPoints *vtkspMag, *vtkspPhase;
  int mag_weight_flag = 0;

  if (magname != NULL) {
    mag_weight_flag = 1;
    img = GetRepositoryData(api, magname, STRUCTURED_PTS_T);
    if (img == NULL) {
      return nullptr;
    }
    vtkspMag = ((cvStrPts*)img)->GetVtkStructuredPoints();
  }

  // Look up given image object:
  img = GetRepositoryData(api, phasename, STRUCTURED_PTS_T);
  if (img == NULL) {
      return nullptr;
  }
  vtkspPhase = ((cvStrPts*)img)->GetVtkStructuredPoints();

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(result)) {
    api.error("The object '"+std::string(result)+"' is already in the repository.");
    return nullptr;
  }

  vtkStructuredPoints *obj;
  int status;
  if (mag_weight_flag == 0) {
    status = mr_decode (vtkspPhase,venc,vencscale,&obj);
  } else {
    status = mr_decode_masked (vtkspMag,vtkspPhase,venc,vencscale,&obj);
  }

  if (status == SV_ERROR) {
    api.error("Error decoding '" +std::string(magname)+ "' and '" + std::string(magname)+"'."); 
    return nullptr;
  }

  cvStrPts *sp = new cvStrPts( obj );
  sp->SetName(result);

  if (!gRepository->Register(sp->GetName(), sp)) {
    api.error("Error adding the decoded image '" + std::string(sp->GetName()) + "' to the repository.");
    delete sp;
    return nullptr;
  }

  return Py_BuildValue("s",sp->GetName());
}

//-------------------------------------
// Image_calculate_correction_equation 
//-------------------------------------
//
PyDoc_STRVAR(Image_calculate_correction_equation_doc,
  "calculate_correction_equation(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
Image_calculate_correction_equation(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("OOi", PyRunTimeErr, __func__);
  PyObject *regionsArg;
  PyObject *imagesArg;
  int order = 0;

  if (!PyArg_ParseTuple(args, api.format, &regionsArg, &imagesArg, &order)) {
    return api.argsError();
  }

  // Check for valid order.
  if (order < 0 || order > 3) {
    api.error("The order argument must be 0, 1 or 3.");
    return nullptr;
  }

  // Get a list of region polydata objects from the repository.
  std::vector<vtkPolyData*> regionObjects;
  if (!GetRepositoryDataList(api, regionsArg, POLY_DATA_T, "regions", regionObjects)) {
    return nullptr;
  } 
  int numRegions = regionObjects.size(); 

  // Get a list of image objects from the repository.
  std::vector<vtkStructuredPoints*> imageObjects;
  if (!GetRepositoryDataList(api, imagesArg, STRUCTURED_PTS_T, "image", imageObjects)) {
    return nullptr;
  } 
  int numImages = imageObjects.size(); 

  // Classify points and calculate correction equation.
  double results[6];
  int status = img_calcCorrectionEqn(numRegions, regionObjects.data(), numImages, imageObjects.data(), order, results);
  if (status == SV_ERROR) {
    api.error("Error finding correction equation.");
    return nullptr;
  }

  // return a string with the correction equation
  char r[2048];
  r[0] = '\0';
  if (order == 0) {
      sprintf(r,"%le",results[0]);

  } else if (order == 1) {
      sprintf(r,"%le %s %le %s %le %s",results[0], " + ", results[1], "*$x + ", results[2], "*$y");

  } else if (order == 2) {
      sprintf(r,"%le %s %le %s %le %s %le %s %le %s %le %s", results[0]," + ",results[1],"*$x + ",results[2],
             "*$y + ",results[3],"*$x*$x + ",results[4], "*$y*$y + ",results[5],"*$x*$y");
  }

  fprintf(stdout,r);
  fprintf(stdout,"\n");

  return Py_BuildValue("s",r);
}

//------------------------------------------
// Image_calculate_correction_equation_auto 
//------------------------------------------
//
// [TODO:DaveP] maybe just call this from the method above
// if the 'automatic' argument is set.
//
PyDoc_STRVAR(Image_calculate_correction_equation_auto_doc,
  "calculate_correction_equation_auto(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
Image_calculate_correction_equation_auto(PyObject *self, PyObject *args )
{
  auto api = SvPyUtilApiFunction("OOids", PyRunTimeErr, __func__);
  PyObject *regionsArg;
  PyObject *imagesArg;
  int order = 0;
  double factor = 0;
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &regionsArg, &imagesArg, &order, &factor, &objName)) {
    return api.argsError();
  }

  // Check for valid order.
  if (order < 0 || order > 3) {
    api.error("The order argument must be 0, 1 or 3.");
    return nullptr;
  }

  // Make sure the specified result object does not exist:
  if (gRepository->Exists( objName ) ) {
    api.error("The '" + std::string(objName) + "' is already in the repository.");
    return nullptr;
  }

  // Get a list of region polydata objects from the repository.
  std::vector<vtkPolyData*> regionObjects;
  if (!GetRepositoryDataList(api, regionsArg, POLY_DATA_T, "regions", regionObjects)) {
    return nullptr;
  } 
  int numRegions = regionObjects.size(); 

  // Get a list of image objects from the repository.
  std::vector<vtkStructuredPoints*> imageObjects;
  if (!GetRepositoryDataList(api, imagesArg, STRUCTURED_PTS_T, "image", imageObjects)) {
    return nullptr;
  }
  int numImages = imageObjects.size();

  // Classify points and calculate correction equation.
  double results[6];
  vtkStructuredPoints* maskImg = NULL;
  int status = img_calcCorrectionEqnAuto(numRegions, regionObjects.data(), numImages, imageObjects.data(), order, factor, results, &maskImg);
  if (status == SV_ERROR) {
    api.error("Error finding correction equation.");
    return nullptr;
  }

  // Register the image
  cvStrPts *sp = new cvStrPts(maskImg);
  if (!gRepository->Register(objName, sp)) {
    delete sp;
    api.error("Error adding the image '" + std::string(objName) + "' to the repository.");
    return nullptr;
  }

  // return a string with the correction equation
  char r[2048];
  r[0] = '\0';
  if (order == 0) {
      sprintf(r,"%le",results[0]);
  } else if (order == 1) {
      sprintf(r,"%le %s %le %s %le %s",results[0], " + ", results[1], "*$x + ", results[2], "*$y");
  } else if (order == 2) {
      sprintf(r,"%le %s %le %s %le %s %le %s %le %s %le %s", results[0]," + ",results[1],"*$x + ",results[2],
             "*$y + ",results[3],"*$x*$x + ",results[4], "*$y*$y + ",results[5],"*$x*$y");
  }

  fprintf(stdout,r);
  fprintf(stdout,"\n");

  return Py_BuildValue("s",r);
}

//---------------------------
// Image_set_image_threshold 
//---------------------------
//
PyDoc_STRVAR(Image_set_image_threshold_doc,
  "set_image_threshold(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
Image_set_image_threshold(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ssddi", PyRunTimeErr, __func__);
  char *imageName = NULL;
  char *result = NULL;
  double thrMin,thrMax;
  int max_num_pts;

  if (!PyArg_ParseTuple(args, api.format, &imageName, &result, &thrMin, &thrMax, &max_num_pts)) {
    return api.argsError();
  }

  auto img = GetRepositoryData(api, imageName, STRUCTURED_PTS_T);
  if (img == NULL) {
    return nullptr;
  }
  auto vtksp = ((cvStrPts*)img)->GetVtkStructuredPoints();

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(result))  {
    api.error("The '" + std::string(result) + "' is already in the repository.");
    return nullptr;
  }

  cvPolyData *obj = NULL;
  int status = img_threshold(vtksp, thrMin, thrMax, max_num_pts, &obj);

  if ((status == SV_ERROR) || (obj == NULL)) {
    api.error("Error in the threshold operation for the image '" + std::string(imageName) + "'.");
    return nullptr;
  }

  obj->SetName(result);
  if (!gRepository->Register(obj->GetName(), obj)) {
    delete obj;
    api.error("Error adding the threshold image '" + std::string(result) + "' to the repository.");
    return nullptr;
  }

  // [TODO:DaveP] this is the first time seeing this kind of return.
  vtkSmartPointer<vtkPolyData> polydataObj = vtkSmartPointer<vtkPolyData>::New();
  polydataObj = obj->GetVtkPolyData();
  PyObject* pyVtkObj = vtkPythonUtil::GetObjectFromPointer(polydataObj);
  return pyVtkObj;
}

//--------------------------------------
// Image_compute_structured_coordinates 
//--------------------------------------
//
PyDoc_STRVAR(Image_compute_structured_coordinates_doc,
  "compute_structured_coordinates(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
Image_compute_structured_coordinates(PyObject *self, PyObject *args )
{
  auto api = SvPyUtilApiFunction("sO", PyRunTimeErr, __func__);
  char *imagename = NULL;
  PyObject* ptList;

  if (!PyArg_ParseTuple(args, api.format, &imagename, &ptList)) {
    return api.argsError();
  }

  double pt[3];
  std::string emsg;
  if (!svPyUtilGetPointData(ptList, emsg, pt)) {
      api.error("The point argument " + emsg);
      return nullptr;
  }

  // Look up given image object:
  auto img = GetRepositoryData(api, imagename, STRUCTURED_PTS_T);
  if (img == NULL) {
      return nullptr;
  }
  auto vtksp = ((cvStrPts*)img)->GetVtkStructuredPoints();

  // Compute the structured coordinates, whatever that is.
  //
  int ijk[3];
  vtkFloatingPointType pcoords[3];
  char rtnstr[2048];
  vtkFloatingPointType x[3] = {pt[0], pt[1], pt[2]};

  if ( (vtksp->ComputeStructuredCoordinates(x, ijk, pcoords)) == 0) {
      return Py_BuildValue("s",rtnstr);
  }

  // Return results as a Python string.
  //
  // [TODO:DaveP] why not return a list of numbers or a dict?
  //
  // ijk.
  PyObject *pylist = PyList_New(3);
  auto pyStr = ValuesToPyString("%i %i %i", ijk[0], ijk[1], ijk[2]);
  PyList_SetItem(pylist, 0, pyStr);

  // pcoords.
  pyStr = ValuesToPyString("%.6e %.6e %.6e", pcoords[0], pcoords[1], pcoords[2]);
  PyList_SetItem(pylist, 1, pyStr);

  // intensity.
  auto intensity = vtksp->GetPointData()->GetScalars()->GetTuple1(vtksp->ComputePointId(ijk));
  pyStr = ValuesToPyString("%f", intensity); 
  PyList_SetItem(pylist, 2, pyStr);

  return Py_BuildValue("s",pylist);
}

//---------------------------
// Image_create_distance_map 
//---------------------------
//
PyDoc_STRVAR(Image_create_distance_map_doc,
  "create_distance_map(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
Image_create_distance_map(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("sOds|i", PyRunTimeErr, __func__);
  char *srcName;
  PyObject* startList;
  double thr;
  char *dstName;
  int useCityBlock = 1;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &startList, &thr, &dstName, &useCityBlock)) {
    return api.argsError();
  }

  // Get the structured points data for the image.
  auto img = GetRepositoryData(api, srcName, STRUCTURED_PTS_T);
  if (img == NULL) {
    return nullptr;
  }
  auto sp = ((cvStrPts*)img)->GetVtkStructuredPoints();

  // Get coordinate.
  //
  int start[3];
  std::string emsg;
  if (!svPyUtilGetPointData(startList, emsg, start)) {
    api.error("The start point argument " + emsg);
    return nullptr;
  }

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(dstName)) {
    api.error("The '" + std::string(dstName) + "' is already in the repository.");
    return nullptr;
  }

  // Calculate the distance map. 
  //
  vtkStructuredPoints *mapsp = NULL;
  vtkFloatingPointType thrval = thr;
  cvDistanceMap* distmap = new cvDistanceMap();

  if (useCityBlock== 0) {
      distmap->setUse26ConnectivityDistance();
  }

  if (distmap->createDistanceMap(sp,thrval,start) == SV_ERROR) {
    api.error("Error in the distance map calculation for the image '" + std::string(srcName) + "'.");
    return nullptr;
  }

  cvStrPts *repossp = new cvStrPts( distmap->getDistanceMap() );
  delete distmap;

  repossp->SetName(dstName);
  auto repoName = repossp->GetName();
  if (!gRepository->Register(repoName, repossp)) {
    api.error("Error adding the distance map data '" + std::string(repoName) + "' to the repository.");
    delete repossp;
    return nullptr;
  }

  return Py_BuildValue("s", repoName); 
}

//-----------------
// Image_find_path 
//-----------------
//
PyDoc_STRVAR(Image_find_path_doc,
  "find_path(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
Image_find_path(PyObject *self, PyObject *args )
{
  auto api = SvPyUtilApiFunction("sOs|iii", PyRunTimeErr, __func__);
  char *srcName;
  PyObject* stopList;
  char *dstName;
  int useCityBlock = 1;
  int maxIter = -1;
  int minqstop = 0;

  if (!PyArg_ParseTuple(args, api.format, &srcName, &stopList,  &dstName, &useCityBlock, &maxIter, &minqstop)) {
    return api.argsError();
  }

  // Get the structured points data for the image.
  auto img = GetRepositoryData(api, srcName, STRUCTURED_PTS_T);
  if (img == NULL) {
    return nullptr;
  }
  auto sp = ((cvStrPts*)img)->GetVtkStructuredPoints();

  // Get stop coordinates.
  //
  int stop[3];
  std::string emsg;
  if (!svPyUtilGetPointData(stopList, emsg, stop)) {
    api.error("The stop point argument " + emsg);
    return nullptr;
  }

  // Make sure the specified result object does not exist:
  if (gRepository->Exists(dstName)) {
    api.error("The '" + std::string(dstName) + "' is already in the repository.");
    return nullptr;
  }

  // Calculate distance map.
  //
  auto distmap = new cvDistanceMap();
  distmap->setDistanceMap(sp);
  if (useCityBlock ==0) {
      distmap->setUse26ConnectivityDistance();
  }

  vtkPolyData *pd;
  if (maxIter < 0) {
    pd = distmap->getPath(stop,minqstop);
  } else {
    pd = distmap->getPathByThinning(stop,minqstop,maxIter);
  }

  if (pd == NULL) {
    api.error("Error in finding a path for the image '" + std::string(srcName) + "'.");
    return nullptr;
  }

  // Add polydata to the repository.
  //
  auto dst = new cvPolyData (pd);
  dst->SetName( dstName );
  auto repoName = dst->GetName();
  if (!gRepository->Register(repoName, dst)) {
    api.error("Error adding the distance map data '" + std::string(repoName) + "' to the repository.");
    delete dst;
    return nullptr;
  }

  // Instead of exporting the object name, output the vtkPolydata object
  //
  // [TODO:DaveP] why is this implemented differently than most api functions?
  //
  PyObject* pyVtkObj = vtkPythonUtil::GetObjectFromPointer(pd);
  return pyVtkObj;
}

//------------
// Image_mask 
//------------
//
PyDoc_STRVAR(Image_mask_doc,
  "mask(name)  \n\ 
  \n\
  ??? Add the unstructured grid mesh to the repository. \n\
  \n\
  Args:                                    \n\
    name (str): Name in the repository to store the unstructured grid. \n\
");

static PyObject *
Image_mask(PyObject *self, PyObject *args)
{
  auto api = SvPyUtilApiFunction("ss|di", PyRunTimeErr, __func__);
  char *objName;
  char *maskName;
  double replaceVal = 0;
  int notval = 0;

  if (!PyArg_ParseTuple(args, api.format, &objName, &maskName, &replaceVal, &notval)) {
    return api.argsError();
  }

  // Get the structured points data for the image and mask.
  auto img = GetRepositoryData(api, objName, STRUCTURED_PTS_T);
  if (img == NULL) {
    return nullptr;
  }
  auto imgsp = ((cvStrPts*)img)->GetVtkStructuredPoints();

  // Get the structured points data for the image.
  auto mask = GetRepositoryData(api, maskName, STRUCTURED_PTS_T);
  if (mask == NULL) {
    return nullptr;
  }
  auto masksp = ((cvStrPts*)mask)->GetVtkStructuredPoints();

  // Calculate the mask.
  //
  bool notvalBool = (notval!=0);
  if (MaskImageInPlace(imgsp,masksp,replaceVal,notvalBool) == SV_ERROR) {
    api.error("Error in the mask calculation for the image '" + std::string(objName) + "'.");
    return nullptr;
  }

  return Py_BuildValue("s",img->GetName());
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

static char* MODULE_NAME = "image";
static char* MODULE_EXCEPTION = "image.ImageException";
static char* MODULE_EXCEPTION_OBJECT = "ImageException";

PyDoc_STRVAR(Image_doc, "image module functions");

#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC
initpyImage(void);
#elif PYTHON_MAJOR_VERSION == 3
PyMODINIT_FUNC
PyInit_pyImage(void);
#endif

// ----------
// Image_Init
// ----------
int Image_pyInit()
{
  #if PYTHON_MAJOR_VERSION == 2
  initpyImage();
  #elif PYTHON_MAJOR_VERSION == 3
  PyInit_pyImage();
  #endif
  return SV_OK;
}

//-----------------
// pyImage_methods
//-----------------
//
PyMethodDef pyImage_methods[] = {

  {"calculate_correction_equation", Image_calculate_correction_equation, METH_VARARGS, Image_calculate_correction_equation_doc},

  {"calculate_correction_equation_auto", Image_calculate_correction_equation_auto, METH_VARARGS, Image_calculate_correction_equation_auto_doc},

  {"compute_structured_coordinates", Image_compute_structured_coordinates, METH_VARARGS, Image_compute_structured_coordinates_doc},

  {"create_distance_map", Image_create_distance_map, METH_VARARGS, Image_create_distance_map_doc},

  {"decode", Image_decode, METH_VARARGS, Image_decode_doc},

  {"find_path", Image_find_path, METH_VARARGS, Image_find_path_doc},

  {"mask", Image_mask, METH_VARARGS, Image_mask_doc},

  {"read_header_5x", Image_read_header_5x, METH_VARARGS, Image_read_header_5x_doc},

  {"set_image_threshold", Image_set_image_threshold, METH_VARARGS, Image_set_image_threshold_doc},

  {NULL, NULL,0,NULL},

};

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
static struct PyModuleDef pyImagemodule = {
   m_base,
   MODULE_NAME, 
   Image_doc, 
   perInterpreterStateSize, 
   pyImage_methods
};

PyMODINIT_FUNC
PyInit_pyImage(void)
{
  auto module = PyModule_Create(&pyImagemodule);

  PyRunTimeErr = PyErr_NewException(MODULE_EXCEPTION,NULL,NULL);
  Py_INCREF(PyRunTimeErr);
  PyModule_AddObject(module ,MODULE_EXCEPTION_OBJECT,PyRunTimeErr);

  return module;
}

#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

// --------------------
// initpyImage
// --------------------
#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC
initpyImage(void)
{
  PyObject *pyIm;

  pyIm = Py_InitModule("pyImage",pyImage_methods);

  PyRunTimeErr = PyErr_NewException("pyImage.error",NULL,NULL);
  Py_INCREF(PyRunTimeErr);
  PyModule_AddObject(pyIm,"error",PyRunTimeErr);
  return;

}
#endif
