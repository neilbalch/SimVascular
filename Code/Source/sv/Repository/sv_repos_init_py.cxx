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

// The functions defined here implement the SV Python API 'repository' module. 
//
// A Python exception sv.repository.RepositoryException is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.repository.RepositoryException:
//
#include "SimVascular.h"
#include "Python.h"
#include "SimVascular_python.h"

#include "sv_repos_init_py.h"
#include "sv_Repository.h"
#include "sv_PolyData.h"
#include "sv_StrPts.h"
#include "sv_UnstructuredGrid.h"
#include "sv_arg.h"
#include "sv_VTK.h"
#include "sv_PyUtils.h"

#include "vtkTclUtil.h"
#include "vtkPythonUtil.h"
#include <vtkXMLPolyDataReader.h>

#include <set>

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"

// Exception type used by PyErr_SetString() to set the for the error indicator.
PyObject* PyRunTimeErr;

// The valid repository types that can be exported to vtk.
static std::set<RepositoryDataT> validVtkExportTypes{POLY_DATA_T, STRUCTURED_PTS_T, UNSTRUCTURED_GRID_T, TEMPORALDATASET_T};

//---------------
// CheckFileType
//---------------
// Check for valid file type. 
//
static bool
CheckFileType(SvPyUtilApiFunction& api, vtkDataWriter* writer, const std::string& fileType)
{
  if (fileType == "binary") {
      writer->SetFileTypeToBinary();
  } else if (fileType == "ascii") {
      writer->SetFileTypeToASCII();
  } else {
      api.error("Unknown file type argument '" + fileType + "'. Valid types are: ascii or binary.");
      return false;
  }

  return true;
}

//--------------
// GetVtkObject
//--------------
// Get a vtk object for a given type.
//
static cvRepositoryData * 
GetVtkObject(SvPyUtilApiFunction& api, const char *name, const RepositoryDataT type, const std::string& desc)
{
  auto obj = gRepository->GetObject(name);
  if (obj == nullptr) {
      api.error("The object '"+std::string(name)+"' is not in the repository.");
      return nullptr;
  }

  if (gRepository->GetType(name) != type) {
      api.error("The object '" + std::string(name) + "' is not a vtk " + desc + " object.");
      return nullptr;
  }

  return obj;
}


//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//-------------
// Repository_PassCmd
//-------------
//
PyDoc_STRVAR(Repository_set_string_doc,
  "set_string(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_set_string( PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *stringPd;
  char *stringArg;

  if (!PyArg_ParseTuple(args,api.format, &stringPd, &stringArg)) {
     return api.argsError();
  }

  // Retrieve source object:
  auto pd = gRepository->GetObject( stringPd );
  if (pd == NULL) {
      api.error("The object '"+std::string(stringPd)+"' is not in the repository.");
      return nullptr;
  }

  pd->SetName(stringArg);

  std::string result(stringArg);
  return Py_BuildValue("s",result.c_str());
}


//------------------
// Repository_get_string
//------------------
//
PyDoc_STRVAR(Repository_get_string_doc,
  "get_string(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_get_string(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *stringPd;

  if (!PyArg_ParseTuple(args, api.format, &stringPd)) {
     return api.argsError();
  }

  // Retrieve source object:
  auto pd = gRepository->GetObject( stringPd );
  if ( pd == NULL ) {
      api.error("The object '"+std::string(stringPd)+"' is not in the repository.");
      return nullptr;
  }
  return Py_BuildValue("s",pd->GetName());
}

//------------
// Repository_list 
//------------
//
PyDoc_STRVAR(Repository_list_doc,
  "list(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_list(PyObject* self, PyObject* args)
{
  PyObject *pylist=PyList_New(0);
  gRepository->InitIterator();

  while (auto name = gRepository->GetNextName()) {
    PyObject* pyName = PyString_FromString(name);
    PyList_Append( pylist, pyName );
  }
  return pylist;
}

//--------------
// Repository_exists 
//--------------
//
PyDoc_STRVAR(Repository_exists_doc,
  "exists(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject *  
Repository_exists(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *name;

  if (!PyArg_ParseTuple(args, api.format, &name)) {
     return api.argsError();
  }

  auto exists = gRepository->Exists( name );

  return Py_BuildValue("N",PyBool_FromLong(exists));
}

//--------------
// Repository_delete 
//--------------
//
PyDoc_STRVAR(Repository_delete_doc,
  "delete(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject *
Repository_delete( PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
     return api.argsError();
  }

  auto exists = gRepository->Exists(objName);
  if (!exists) {
      api.error("The object '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  auto obj_t = gRepository->GetType( objName );
  auto unreg_status = gRepository->UnRegister( objName );

  if (!unreg_status) {
      api.error("Error deleting the object '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//------------
// Repository_type 
//------------
//
PyDoc_STRVAR(Repository_type_doc,
  "type(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_type(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *name;

  if (!PyArg_ParseTuple(args, api.format, &name)) {
     return api.argsError();
  }

  if (!gRepository->Exists(name)) {
      api.error("The object '"+std::string(name)+"' is not in the repository.");
      return nullptr;
  }

  auto type = gRepository->GetType(name);
  auto typeStr = RepositoryDataT_EnumToStr(type);
  return Py_BuildValue("s",typeStr);
}

//---------------------------
// Repository_import_vtk_polydata 
//---------------------------
//
PyDoc_STRVAR(Repository_import_vtk_polydata_doc,
  "import_vtk_polydata(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_import_vtk_polydata(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Os", PyRunTimeErr, __func__);
  PyObject *vtkName;
  char *objName;

  if (!PyArg_ParseTuple(args,api.format, &vtkName, &objName)) {
      return api.argsError();
  }

  auto vtkObj = (vtkPolyData *)vtkPythonUtil::GetPointerFromObject(vtkName, "vtkPolyData");
  if (vtkObj == NULL) {
      api.error("The vtk argument object is not vtkPolyData.");
      return nullptr;
  }

  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  auto pd = new cvPolyData(vtkObj);
  pd->SetName(objName);

  if (!gRepository->Register(pd->GetName(), pd)) {
      delete pd;
      api.error("Error adding the vtk polydata '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",pd->GetName());
}

//---------------------
// Repository_export_to_vtk 
//---------------------
//
PyDoc_STRVAR(Repository_export_to_vtk_doc,
  "export_to_vtk(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_export_to_vtk(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  // Retrieve source object:
  auto pd = gRepository->GetObject(objName);
  if (pd == NULL) {
      api.error("The object '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  // Check that the object has a type that can be exported to vtk.
  auto type = pd->GetType();
  if (validVtkExportTypes.count(type) == 0) {
      auto typeStr = std::string(RepositoryDataT_EnumToStr(type));
      api.error("Cannot export object '"+std::string(objName)+"' of type '" + typeStr + "'." +  
          " Valid types: polydata, structured points, temporal data set or unstructured grid.");
      return nullptr;
  }

  auto vtkObj = ((cvDataObject *)pd)->GetVtkPtr();
  PyObject* pyVtkObj=vtkPythonUtil::GetObjectFromPointer(vtkObj);

  return pyVtkObj;
}

//------------------------------------
// Repository_import_vtk_structured_points 
//------------------------------------
//
PyDoc_STRVAR(Repository_import_vtk_structured_points_doc,
  "import_vtk_structured_points(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_import_vtk_structured_points(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Os", PyRunTimeErr, __func__);
  PyObject *vtkName;
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &vtkName,&objName)) {
      return api.argsError();
  }

  auto vtkObj = (vtkStructuredPoints*)vtkPythonUtil::GetPointerFromObject(vtkName, "vtkStructuredPoints");
  if (vtkObj == NULL) {
      api.error("The vtk argument object is not vtkStructuredPoints.");
      return nullptr;
  }

  // Is the specified repository object name already in use?
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  auto sp = new cvStrPts( vtkObj );
  sp->SetName( objName );

  if (!gRepository->Register(sp->GetName(), sp)) {
      delete sp;
      api.error("Error adding the vtk structure points '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",sp->GetName());
}

//------------------------------------
// Repository_import_vtk_unstructured_grid 
//------------------------------------
//
PyDoc_STRVAR(Repository_import_vtk_unstructured_grid_doc,
  "import_vtk_unstructured_grid(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_import_vtk_unstructured_grid(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("Os", PyRunTimeErr, __func__);
  PyObject *vtkName;
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &vtkName,&objName)) {
      return api.argsError();
  }

  // Look up the named vtk object:
  auto vtkObj = (vtkUnstructuredGrid *)vtkPythonUtil::GetPointerFromObject(vtkName, "vtkUnstructuredGrid");
  if (vtkObj == NULL) {
      api.error("The vtk argument object is not an vtkUnstructuredGrid.");
      return nullptr;
  }

  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  auto sp = new cvUnstructuredGrid(vtkObj);
  sp->SetName( objName );
  if (!gRepository->Register(sp->GetName(), sp)) { 
      delete sp;
      api.error("Error adding the vtk unstructure points '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",sp->GetName());
}

//------------------------
// Repository_import_vtk_image 
//------------------------
//
PyDoc_STRVAR(Repository_import_vtk_image_doc,
  "import_vtk_image(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_import_vtk_image( PyObject* self, PyObject* args )
{
  auto api = SvPyUtilApiFunction("Os", PyRunTimeErr, __func__);
  PyObject *vtkName = NULL;
  char *objName = NULL;

  if (!PyArg_ParseTuple(args, api.format, &vtkName,&objName)) {
      return api.argsError();
  }

  // Look up the named vtk object:
  auto vtkObj = (vtkImageData *)vtkPythonUtil::GetPointerFromObject( vtkName, "vtkImageData");
  if (vtkObj == NULL) {
      api.error("The vtk argument object is not vtkImageData.");
      return nullptr;
  }

  // Is the specified repository object name already in use?
  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  auto mysp = vtkStructuredPoints::New();
  mysp->ShallowCopy(vtkObj);

  // Need to shift the origin like what used to be done
  // in vtkImageToStructuredPoints class.
  int whole[6];
  int extent[6];
  double *spacing, origin[3];

  // so we just assume that we have the whole extent loaded.
  vtkObj->GetExtent(whole);
  spacing = vtkObj->GetSpacing();
  vtkObj->GetOrigin(origin);

  origin[0] += spacing[0] * whole[0];
  origin[1] += spacing[1] * whole[2];
  whole[1] -= whole[0];
  whole[3] -= whole[2];
  whole[0] = 0;
  whole[2] = 0;
  // shift Z origin for 3-D images
  if (whole[4] > 0 && whole[5] > 0) {
    origin[2] += spacing[2] * whole[4];
    whole[5] -= whole[4];
    whole[4] = 0;
  }
  // no longer available in vtk-6.0.0  mysp->SetWholeExtent(whole);
  mysp->SetExtent(whole);
  // Now should Origin and Spacing really be part of information?
  // How about xyx arrays in RectilinearGrid of Points in StructuredGrid?
  mysp->SetOrigin(origin);
  mysp->SetSpacing(spacing);
  auto sp = new cvStrPts (mysp);
  mysp->Delete();
  sp->SetName( objName );

  if (!gRepository->Register(sp->GetName(), sp)) {
      delete sp;
      api.error("Error adding the vtk image '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",sp->GetName());
}

//------------
// Repository_save 
//------------
//
PyDoc_STRVAR(Repository_save_doc,
  "save(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_save(PyObject* self, PyObject* args )
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* fileName;

  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      return api.argsError();
  }

  if (!gRepository->Save(fileName)) {
      api.error("Error saving the repository to the file '" + std::string(fileName) + "'.");
      return nullptr;
  }

  return Py_BuildValue("s","repository successfully saved");
}

//------------
// Repository_load 
//------------
//
PyDoc_STRVAR(Repository_load_doc,
  "load(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_load(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char* fileName;

  if (!PyArg_ParseTuple(args, api.format, &fileName)) {
      return api.argsError();
  }

  if (!gRepository->Load(fileName)) {
      api.error("Error loading the repository from the file '" + std::string(fileName) + "'.");
      return nullptr;
  }

  return Py_BuildValue("s","repository successfully load");
}

// -------------------------
// Repository_write_vtk_polydata
// -------------------------
//
PyDoc_STRVAR(Repository_write_vtk_polydata_doc,
  "write_vtk_polydata(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static 
PyObject * 
Repository_write_vtk_polydata(PyObject* self, PyObject* args )
{
  auto api = SvPyUtilApiFunction("sss", PyRunTimeErr, __func__);
  char *objName, *ft, *fn;

  if (!PyArg_ParseTuple(args, api.format, &objName, &ft, &fn)) {
      return api.argsError();
  }

  auto obj = GetVtkObject(api, objName, POLY_DATA_T, "polydata");
  if (obj == nullptr) {
      return nullptr;
  }

  auto pd = ((cvPolyData*)obj)->GetVtkPolyData();
  if (pd == nullptr) {
      api.error("Error getting the polydata for the object '" + std::string(objName) + "'.");
      return nullptr;
  }

  vtkPolyDataWriter *pdWriter = vtkPolyDataWriter::New();
  pdWriter->SetInputDataObject( pd );
  pdWriter->SetFileName( fn );

  if (!CheckFileType(api, pdWriter, std::string(ft))) {
     return nullptr;
  }

  // [TODO:DaveP] check that the write completed?
  pdWriter->Write();
  pdWriter->Delete();
  return SV_PYTHON_OK;
}

//-------------------------
// Repository_read_vtk_polydata 
//-------------------------
//
PyDoc_STRVAR(Repository_read_vtk_polydata_doc,
  "read_vtk_polydata(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_read_vtk_polydata( PyObject* self, PyObject* args )
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *objName, *fn;

  if (!PyArg_ParseTuple(args, api.format, &objName,&fn)) {
      return api.argsError();
  }

  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Read the polydata froma a file.
  //
  // Note that it is critical to call Update even for vtk readers.
  //
  auto pdReader = vtkPolyDataReader::New();
  pdReader->SetFileName( fn );
  pdReader->Update();

  auto vtkPd = pdReader->GetOutput();
  if ((vtkPd == NULL) || (vtkPd->GetNumberOfPolys()==0)) {
      pdReader->Delete();
      api.error("Error reading polydata from the file '" + std::string(fn) + "'.");
      return nullptr;
  }
  pdReader->Delete();

  auto pd = new cvPolyData(vtkPd);
  if (!gRepository->Register(objName, pd)) {
      delete pd;
      api.error("Error adding the vtk polydata '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",pd->GetName());
}

//-----------------------------
// Repository_read_vtk_xml_polydata 
//-----------------------------
//
PyDoc_STRVAR(Repository_read_vtk_xml_polydata_doc,
  "read_vtk_xml_polydata(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_read_vtk_xml_polydata( PyObject* self, PyObject* args )
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *objName, *fn;

  if (!PyArg_ParseTuple(args, api.format, &objName,&fn)) {
      return api.argsError();
  }

  if (gRepository->Exists(objName)) {
      api.error("The repository object '" + std::string(objName) + "' already exists.");
      return nullptr;
  }

  // Read the polydata from a file.
  //
  // Note that it is critical to call Update even for vtk readers.
  //
  auto pdReader = vtkXMLPolyDataReader::New();
  pdReader->SetFileName(fn);
  pdReader->Update();

  auto vtkPd = pdReader->GetOutput();
  if ((vtkPd == NULL) || (vtkPd->GetNumberOfPolys() == 0)) {
      pdReader->Delete();
      api.error("Error reading polydata from the file '" + std::string(fn) + "'.");
      return nullptr;
  }
  pdReader->Delete();

  auto pd = new cvPolyData(vtkPd);
  if (!gRepository->Register(objName, pd)) {
      delete pd;
      api.error("Error adding the vtk polydata '" + std::string(objName) + "' to the repository.");
      return nullptr;
  }

  return Py_BuildValue("s",pd->GetName());
}

//---------------------------------
// Repository_WriteVtkStructuredPointsCmd
//---------------------------------
//
PyDoc_STRVAR(Repository_write_vtk_structured_points_doc,
  "write_vtk_structured_points(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_write_vtk_structured_points(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss", PyRunTimeErr, __func__);
  char *objName, *ft, *fn;

  if (!PyArg_ParseTuple(args, api.format, &objName, &ft, &fn)) {
      return api.argsError();
  }

  auto obj = GetVtkObject(api, objName, STRUCTURED_PTS_T, "structured points");
  if (obj == nullptr) {
      return nullptr;
  }

  auto sp = ((cvStrPts*)obj)->GetVtkStructuredPoints();
  auto spWriter = vtkStructuredPointsWriter::New();
  spWriter->SetInputDataObject( sp );
  spWriter->SetFileName(fn);

  if (!CheckFileType(api, spWriter, std::string(ft))) {
     return nullptr;
  }

  spWriter->Write();
  spWriter->Delete();

  return SV_PYTHON_OK;
}

//-----------------------------------
// Repository_write_vtk_unstructured_grid 
//-----------------------------------
//
PyDoc_STRVAR(Repository_write_vtk_unstructured_grid_doc,
  "write_vtk_unstructured_grid(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_write_vtk_unstructured_grid(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss", PyRunTimeErr, __func__);
  char *objName, *ft, *fn;

  if (!PyArg_ParseTuple(args, api.format, &objName,&ft,&fn)) {
      return api.argsError();
  }

  auto obj = GetVtkObject(api, objName, UNSTRUCTURED_GRID_T, "unstructured grid");
  if (obj == nullptr) {
      return nullptr;
  }

  // Write the unstructured grid.
  //
  auto sp = ((cvUnstructuredGrid*)obj)->GetVtkUnstructuredGrid();
  auto spWriter = vtkUnstructuredGridWriter::New();
  spWriter->SetInputDataObject(sp);
  spWriter->SetFileName(fn);

  if (!CheckFileType(api, spWriter, std::string(ft))) {
     return nullptr;
  }

  spWriter->Write();
  spWriter->Delete();
  return SV_PYTHON_OK;
}

//----------------------
// Repository_get_label_keys 
//---------------------
//
PyDoc_STRVAR(Repository_get_label_keys_doc,
  "get_label_keys(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_get_label_keys(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("s", PyRunTimeErr, __func__);
  char *objName;

  if (!PyArg_ParseTuple(args, api.format, &objName)) {
      return api.argsError();
  }

  auto obj = gRepository->GetObject( objName );
  if (obj == NULL) {
      api.error("The object '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  // Create a list of kets to return.
  int numKeys;
  char **keys;
  obj->GetLabelKeys( &numKeys, &keys );
  PyObject *pylist=PyList_New(0);

  for (int i = 0; i < numKeys; i++) {
    PyObject* pyKeys = PyString_FromString(keys[i]);
    PyList_Append( pylist,  pyKeys );
  }

  return pylist;
}

//-----------------
// Repository_get_label 
//-----------------
//
PyDoc_STRVAR(Repository_get_label_doc,
  "get_label(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_get_label(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *objName;
  char *key; 

  char *value;

  if (!PyArg_ParseTuple(args, api.format, &objName,&key)) {
      return api.argsError();
  }

  auto obj = gRepository->GetObject( objName );
  if (obj == NULL) {
      api.error("The object '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  if (!obj->GetLabel(key, &value)) {
      api.error("The key argument '"+std::string(key)+"' was not found for the object '"+std::string(objName)+"'."); 
      return nullptr;
  }

  return Py_BuildValue("s",value);
}

//-----------------
// Repository_set_label 
//-----------------
//
PyDoc_STRVAR(Repository_set_label_doc,
  "set_label(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_set_label(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("sss", PyRunTimeErr, __func__);
  char *objName;
  char *key, *value;

  if (!PyArg_ParseTuple(args, api.format, &objName,&key,&value)) {
      return api.argsError();
  }

  auto obj = gRepository->GetObject(objName);
  if (obj == NULL) {
      api.error("The object '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  if (obj->IsLabelPresent(key)) {
      api.error("The key argument '"+std::string(key)+"' is already in use for the object '"+
          std::string(objName)+"'."); 
      return nullptr;
  }

  if (!obj->SetLabel(key, value)) {
      api.error("Error setting the key '"+std::string(key)+"' for the object '"+ std::string(objName)+"'."); 
      return nullptr;
  }

  return SV_PYTHON_OK;
}

//-------------------
// Repository_clear_label 
//-------------------
//
PyDoc_STRVAR(Repository_clear_label_doc,
  "clear_label(file)  \n\ 
   \n\
   Set the solid modeling kernel. \n\
   \n\
   Args: \n\
     file (str): The name of the file ??? \n\
");

static PyObject * 
Repository_clear_label(PyObject* self, PyObject* args)
{
  auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
  char *objName;
  char *key;

  if (!PyArg_ParseTuple(args, api.format, &objName,&key)) {
      return api.argsError();
  }

  auto obj = gRepository->GetObject(objName);
  if (obj == NULL) {
      api.error("The object '"+std::string(objName)+"' is not in the repository.");
      return nullptr;
  }

  if (!obj->IsLabelPresent(key)) {
      api.error("The key argument '"+std::string(key)+"' was not found for the object '"+std::string(objName)+"'."); 
      return nullptr;
  }

  obj->ClearLabel(key);

  return SV_PYTHON_OK;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

//----------------------------
// Define API function names
//----------------------------

PyMethodDef pyRepository_methods[] = {

    {"clear_label", Repository_clear_label, METH_VARARGS, Repository_clear_label_doc},

    {"delete", Repository_delete, METH_VARARGS, Repository_delete_doc},

    {"export_to_vtk", Repository_export_to_vtk, METH_VARARGS, Repository_export_to_vtk_doc},

    {"exists", Repository_exists, METH_VARARGS, Repository_exists_doc},

    {"get_label", Repository_get_label, METH_VARARGS, Repository_get_label_doc},

    {"get_label_keys", Repository_get_label_keys, METH_VARARGS, Repository_get_label_keys_doc},

    {"get_string", Repository_get_string, METH_VARARGS, Repository_get_string_doc},

    {"import_vtk_image", Repository_import_vtk_image, METH_VARARGS, Repository_import_vtk_image_doc},

    // Rename: ImportVtkPd
    {"import_vtk_polydata", Repository_import_vtk_polydata, METH_VARARGS, Repository_import_vtk_polydata_doc},

    {"import_vtk_structured_points", Repository_import_vtk_structured_points, METH_VARARGS, Repository_import_vtk_structured_points_doc},

    {"import_vtk_unstructured_grid", Repository_import_vtk_unstructured_grid, METH_VARARGS, Repository_import_vtk_unstructured_grid_doc},

    {"list", Repository_list, METH_NOARGS, Repository_list_doc},
    {"load", Repository_load, METH_VARARGS, Repository_load_doc},

    {"read_vtk_polydata", Repository_read_vtk_polydata, METH_VARARGS, Repository_read_vtk_polydata_doc},

    {"read_vtk_xml_polydata", Repository_read_vtk_xml_polydata, METH_VARARGS, Repository_read_vtk_xml_polydata_doc},

    {"save", Repository_save, METH_VARARGS, Repository_save_doc},

    {"set_label", Repository_set_label, METH_VARARGS, Repository_set_label_doc},
    {"set_string", Repository_set_string, METH_VARARGS,Repository_set_string_doc},

    {"type", Repository_type, METH_VARARGS, Repository_type_doc},

    {"write_vtk_polydata", Repository_write_vtk_polydata, METH_VARARGS, Repository_write_vtk_polydata_doc},
    {"write_vtk_structured_points", Repository_write_vtk_structured_points, METH_VARARGS, Repository_write_vtk_structured_points_doc},
    {"write_vtk_unstructured_grid", Repository_write_vtk_unstructured_grid, METH_VARARGS, Repository_write_vtk_unstructured_grid_doc},

    {NULL, NULL,0,NULL},

};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MODULE_NAME = "repository";

PyDoc_STRVAR(Repository_module_doc, "repository module functions");


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

static struct PyModuleDef pyRepositorymodule = {
   m_base,
   MODULE_NAME,  
   Repository_module_doc,
   perInterpreterStateSize, 
   pyRepository_methods
};

//---------------------
// PyInit_pyRepository 
//---------------------
// The initialization function called by the Python interpreter when the module is loaded.
//
PyMODINIT_FUNC 
PyInit_pyRepository(void)
{
  gRepository = new cvRepository();

  if ( gRepository == NULL ) {
    fprintf( stderr, "error allocating gRepository\n" );
    return SV_PYTHON_ERROR;
  }

  auto module = PyModule_Create(&pyRepositorymodule);

  PyRunTimeErr = PyErr_NewException("repository.RepositoryException",NULL,NULL);
  Py_INCREF(PyRunTimeErr);
  PyModule_AddObject(module,"RepositoryException",PyRunTimeErr);
  return module;
}


#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

#if PYTHON_MAJOR_VERSION == 2

PyMODINIT_FUNC initpyRepository(void)
{

  gRepository = new cvRepository();
  if ( gRepository == NULL ) {
    fprintf( stderr, "error allocating gRepository\n" );
    return;
  }
  auto module = Py_InitModule("pyRepository",pyRepository_methods);
  PyRunTimeErr = PyErr_NewException("repository.RepositoryException",NULL,NULL);
  Py_INCREF(PyRunTimeErr);
  PyModule_AddObject(module, "RepositoryException",PyRunTimeErr);

}

#endif

