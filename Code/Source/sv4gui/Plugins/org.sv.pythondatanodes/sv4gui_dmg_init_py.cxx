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

// The functions defined here implement the SV Python API data manager module. 
//
// The module name is 'dmg'. 
//
// A Python exception sv.dmg.DmgException is defined for this module. 
// The exception can be used in a Python 'try' statement with an 'except' clause 
// like this
//
//    except sv.dmg.DmgException:
//
#include "SimVascular.h"
#include "SimVascular_python.h"

#include "sv4gui_dmg_init_py.h"
#include "sv_Repository.h"
#include "sv_PolyData.h"
#include "sv_StrPts.h"
#include "sv_UnstructuredGrid.h"
#include "sv_arg.h"
#include "sv_VTK.h"
#include "vtkTclUtil.h"
#include "vtkPythonUtil.h"
#include "sv_PyUtils.h"

// The following is needed for Windows
#ifdef GetObject
#undef GetObject
#endif

#include "sv2_globals.h"

#include "SimVascular.h"
#include "Python.h"
#include "sv4gui_ModelElement.h"
#include "sv4gui_ModelElementAnalytic.h"
#include "sv4gui_ModelElementFactory.h"
#include "sv4gui_Model.h"
#include "sv4gui_Mesh.h"
#include "sv4gui_Path.h"
#include "sv3_PathElement.h"
#include "sv3_Contour.h"
#include "sv4gui_Contour.h"
#include "sv4gui_PathElement.h"
#include "sv4gui_MitkMesh.h"
#include "sv4gui_MeshFactory.h"
#include "sv4gui_PythonDataNodesPluginActivator.h"
#include "sv4gui_ProjectManager.h"
#include "sv4gui_DataNodeOperationInterface.h"
#include "sv4gui_DataNodeOperation.h"
#include "vtkPythonUtil.h"
#include "vtkDataSetSurfaceFilter.h"

#include <mitkNodePredicateDataType.h>
#include <mitkIDataStorageService.h>
#include <mitkDataNode.h>
#include <mitkBaseRenderer.h>
#include <mitkDataStorage.h>
#include <mitkUndoController.h>
#include <mitkIOUtil.h>
#include <array>
#include <vector>
#include <string>

// Exception type used by PyErr_SetString() to set the for the error indicator.
PyObject* PyRunTimeErr;

// Define the SV Data Manager top level folder names. 
//
// [TODO:DaveP] These should be globally defined somewhere.
//
namespace SvDataManagerNodes {
  static char* Image = "svImageFolder";
  static char* Mesh = "sv4guiMeshFolder";
  static char* Model = "sv4guiModelFolder";
  static char* Path = "sv4guiPathFolder";
  static char* Project = "sv4guiProjectFolder";
  static char* Repository = "svRepositoryFolder";
  static char* Segmentation = "sv4guiSegmentationFolder";
}

//////////////////////////////////////////////////////
//        U t i l i t y     F u n c t i o n s       //
//////////////////////////////////////////////////////

//----------------
// BuildModelNode
//----------------
// Create Model node from vtk polydata stored in the global object repository.
//
sv4guiModel::Pointer 
BuildModelNode(cvRepositoryData *poly, sv4guiModel::Pointer model)
{
    vtkSmartPointer<vtkPolyData> polydataObj = (vtkPolyData*)((cvDataObject *)poly)->GetVtkPtr();

    // Hardcodeed type to be PolyData.
    //
    auto modelElement = sv4guiModelElementFactory::CreateModelElement("PolyData");
    modelElement->SetWholeVtkPolyData(polydataObj);
    auto analytic = dynamic_cast<sv4guiModelElementAnalytic*>(modelElement);

    if (analytic) {
        analytic->SetWholeVtkPolyData(analytic->CreateWholeVtkPolyData());
    }
    
    model->SetType(modelElement->GetType());
    model->SetModelElement(modelElement);
    model->SetDataModified();
    return model;
}

//---------------
// BuildMeshNode
//---------------
// Create a TetGen Mesh node from vtk unstructured mesh in global object repository. 
//
sv4guiMitkMesh::Pointer 
BuildMeshNode(cvRepositoryData *obj, sv4guiMesh* mesh,  sv4guiMitkMesh::Pointer mitkMesh)
{
    vtkSmartPointer<vtkUnstructuredGrid> meshObj = (vtkUnstructuredGrid*)((cvDataObject *)obj)->GetVtkPtr();
    //meshObj->Print(std::cout);

    // Get surface polydata from the unstructured grid.
    //
    vtkSmartPointer<vtkDataSetSurfaceFilter> surfaceFilter = vtkSmartPointer<vtkDataSetSurfaceFilter>::New();
    surfaceFilter->SetInputData(meshObj);
    surfaceFilter->Update(); 
    vtkSmartPointer<vtkPolyData> polydata = surfaceFilter->GetOutput();

    // Set the surface and volume mesh to the sv4guiMesh.
    //
    mesh = sv4guiMeshFactory::CreateMesh("TetGen");
    mesh->SetVolumeMesh(meshObj);
    mesh->SetSurfaceMesh(polydata);

    // Set mitk mesh.
    mitkMesh->SetMesh(mesh); 
    mitkMesh->SetType("TetGen");
    mitkMesh->SetDataModified();

    return mitkMesh;
}

//---------------
// BuildPathNode
//---------------
// Create a Path node from a PathElement in the global object repository. 
//
sv4guiPath::Pointer BuildPathNode(cvRepositoryData *obj, sv4guiPath::Pointer path)
{
    sv3::PathElement* pathElem = dynamic_cast<sv3::PathElement*> (obj);
    sv4guiPathElement* guiPath = new sv4guiPathElement();
    
    switch(pathElem->GetMethod()) {
        case sv3::PathElement::CONSTANT_TOTAL_NUMBER:
            guiPath->SetMethod(sv4guiPathElement::CONSTANT_TOTAL_NUMBER);
        break;

        case sv3::PathElement::CONSTANT_SUBDIVISION_NUMBER:
            guiPath->SetMethod(sv4guiPathElement::CONSTANT_SUBDIVISION_NUMBER);
        break;

        case sv3::PathElement::CONSTANT_SPACING:
            guiPath->SetMethod(sv4guiPathElement::CONSTANT_SPACING);
        break;

        default:
        break;
    }

    guiPath->SetCalculationNumber(pathElem->GetCalculationNumber());
    guiPath->SetSpacing(pathElem->GetSpacing());
    
    // Copy control points.
    std::vector<std::array<double,3> > pts = pathElem->GetControlPoints();
    for (int i=0; i<pts.size();i++)
    {
        mitk::Point3D point;
        point[0] = pts[i][0];
        point[1] = pts[i][1];
        point[2] = pts[i][2];
        guiPath->InsertControlPoint(i,point);
    }
    
    //create path points
    guiPath->CreatePathPoints();
    
    path->SetPathElement(guiPath);
    path->SetDataModified();
    
    return path;
}

//------------------
// BuildContourNode
//------------------
// Create a Contour node from a Contour object in the global object repository. 
//
sv4guiContourGroup::Pointer 
BuildContourNode(std::vector<cvRepositoryData *> objs, sv4guiContourGroup::Pointer group, char* pathName)
{
    std::string str(pathName);
    group->SetPathName(str);

    for (int j = 0; j<objs.size(); j++) {
        sv3::Contour* sv3contour = dynamic_cast<sv3::Contour*> (objs[j]);
        sv4guiContour* contour = new sv4guiContour();
        sv3::PathElement::PathPoint pathPoint=sv3contour->GetPathPoint();
        sv4guiPathElement::sv4guiPathPoint pthPt;

        for (int i = 0; i<3; i++) {
            pthPt.pos[i]=pathPoint.pos[i];
            pthPt.tangent[i] = pathPoint.tangent[i];
            pthPt.rotation[i] = pathPoint.rotation[i];
        }

        pthPt.id = pathPoint.id;
        contour->SetPathPoint(pthPt);
        contour->SetMethod(sv3contour->GetMethod());
        contour->SetPlaced(true);
        contour->SetClosed(sv3contour->IsClosed());
        contour->SetContourPoints(sv3contour->GetContourPoints());

        group->InsertContour(j, contour);
        group->SetDataModified();
    }
    
    return group;
}

//-------------
// AddDataNode
//-------------
// Add a Model, Mesh, Path or Contour repository data to the SV Data Manager.
//
int 
AddDataNode(mitk::DataStorage::Pointer dataStorage, cvRepositoryData *rd, mitk::DataNode::Pointer folderNode, char* childName)
{
    RepositoryDataT type = rd->GetType();
    mitk::DataNode::Pointer node = mitk::DataNode::New();

    if ( type == POLY_DATA_T ) {
        sv4guiModel::Pointer model = sv4guiModel::New();
        model = BuildModelNode(rd,model);
        node->SetData(model);
        node->SetName(childName);

    } else if ( type == UNSTRUCTURED_GRID_T) {
        sv4guiMesh* mesh;
        sv4guiMitkMesh::Pointer mitkMesh = sv4guiMitkMesh::New();
        mitkMesh = BuildMeshNode(rd, mesh, mitkMesh);
        //vtkSmartPointer<vtkUnstructuredGrid> tmp = (mitkMesh->GetMesh())->GetVolumeMesh();
        node-> SetData(mitkMesh);
        node->SetName(childName);

    } else if (type ==PATH_T) {
        sv4guiPath::Pointer path = sv4guiPath::New();
        path = BuildPathNode(rd,path);
        int maxPathID=sv4guiPath::GetMaxPathID(dataStorage->GetDerivations(folderNode));
        path->SetPathID(maxPathID+1);
        node -> SetData(path);
        node -> SetName(childName);

    } else {
        printf("Data object is not supported.\n");
        return SV_ERROR;
    }

    // Add new node to its parent node.
    //
    mitk::OperationEvent::IncCurrObjectEventId();
    sv4guiDataNodeOperationInterface* interface = new sv4guiDataNodeOperationInterface;
    bool undoEnabled=true;
    sv4guiDataNodeOperation* doOp = new sv4guiDataNodeOperation(sv4guiDataNodeOperation::OpADDDATANODE,dataStorage,node,folderNode);

    if(undoEnabled) {
        sv4guiDataNodeOperation* undoOp = new sv4guiDataNodeOperation(sv4guiDataNodeOperation::OpREMOVEDATANODE,dataStorage,node,folderNode);
        mitk::OperationEvent *operationEvent = new mitk::OperationEvent(interface, doOp, undoOp, "Add DataNode");
        mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent( operationEvent );
    }
    interface->ExecuteOperation(doOp);
    
    return SV_OK;
}

//----------------
// RemoveDataNode
//----------------
//
int 
RemoveDataNode(mitk::DataStorage::Pointer dataStorage, mitk::DataNode::Pointer folderNode, char* childName)
{
    mitk::DataNode::Pointer childNode =dataStorage->GetNamedDerivedNode(childName,folderNode); 
    
    if (folderNode && childNode) {
        dataStorage->Remove(childNode);
    } else {
        return SV_ERROR;
    }
    
    mitk::OperationEvent::IncCurrObjectEventId();
    sv4guiDataNodeOperationInterface* interface=new sv4guiDataNodeOperationInterface;
    bool undoEnabled=true;
    sv4guiDataNodeOperation* doOp = new sv4guiDataNodeOperation(sv4guiDataNodeOperation::OpREMOVEDATANODE,dataStorage,childNode,folderNode);

    if(undoEnabled) {
        sv4guiDataNodeOperation* undoOp = new sv4guiDataNodeOperation(sv4guiDataNodeOperation::OpADDDATANODE,dataStorage,childNode,folderNode);
        mitk::OperationEvent *operationEvent = new mitk::OperationEvent(interface, doOp, undoOp, "Remove DataNode");
        mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent( operationEvent );
    }
    interface->ExecuteOperation(doOp);
    return SV_OK;
}

//--------------------
// AddContourDataNode
//--------------------
//
int 
AddContourDataNode(mitk::DataStorage::Pointer dataStorage, std::vector<cvRepositoryData *>rd, mitk::DataNode::Pointer folderNode, 
                   char* childName, char* pathName, sv4guiPath::Pointer path)
{
    mitk::DataNode::Pointer node = mitk::DataNode::New();
    sv4guiContourGroup::Pointer contourGroup = sv4guiContourGroup::New();
    contourGroup = BuildContourNode(rd,contourGroup,pathName);

    if(!path.IsNull()) {
        contourGroup->SetPathID(path->GetPathID());
    }
    node->SetData(contourGroup);
    node->SetName(childName);

    //add new node to its parent node
    mitk::OperationEvent::IncCurrObjectEventId();
    sv4guiDataNodeOperationInterface* interface=new sv4guiDataNodeOperationInterface;
    bool undoEnabled=true;
    sv4guiDataNodeOperation* doOp = new sv4guiDataNodeOperation(sv4guiDataNodeOperation::OpADDDATANODE,dataStorage,node,folderNode);
    if(undoEnabled)
    {
        sv4guiDataNodeOperation* undoOp = new sv4guiDataNodeOperation(sv4guiDataNodeOperation::OpREMOVEDATANODE,dataStorage,node,folderNode);
        mitk::OperationEvent *operationEvent = new mitk::OperationEvent(interface, doOp, undoOp, "Add DataNode");
        mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent( operationEvent );
    }
    interface->ExecuteOperation(doOp);
    
    return SV_OK;
}

//-------------
// GetToolNode
//-------------
// Get the tool data node from the SV Data Manager.
//
// The tool nodes are under the root project node and defined for 
// SV tools: Images, Paths, Segmentations, Models and Meshes.
//
mitk::DataNode::Pointer 
GetToolNode(mitk::DataStorage::Pointer dataStorage, mitk::DataNode::Pointer projectNode, char* toolName)
{
  mitk::NodePredicateDataType::Pointer isTool = mitk::NodePredicateDataType::New(toolName);
  mitk::DataStorage::SetOfObjects::ConstPointer rs = dataStorage->GetDerivations(projectNode, isTool);
  mitk::DataNode::Pointer toolNode;

  if (rs->size() > 0) {
      toolNode = rs->GetElement(0);

      if (toolNode.IsNull()) {
          //MITK_ERROR << "Error getting a pointer to the folderNode.";
          return nullptr;
      }
  } else {
      // MITK_ERROR <<"Error getting a pointer to the folderNode.";
      return nullptr;
  }
  return toolNode;
}

//----------------
// GetProjectNode
//----------------
// Get the SV Data Manger root project node 'sv4guiProjectFolder'.
//
mitk::DataNode::Pointer 
GetProjectNode(SvPyUtilApiFunction& api, mitk::DataStorage::Pointer dataStorage)
{
    mitk::NodePredicateDataType::Pointer isProjFolder = mitk::NodePredicateDataType::New(SvDataManagerNodes::Project);
    mitk::DataStorage::SetOfObjects::ConstPointer rs = dataStorage->GetSubset(isProjFolder);
    mitk::DataNode::Pointer projFolderNode;

    if (rs->size()>0) {
        projFolderNode=rs->GetElement(0);
    } else {
        api.error("Could not find a project folder. A project must be active.");
        return nullptr;
    }
    return projFolderNode;
}

//----------------
// SearchDataNode
//----------------
//
mitk::DataNode::Pointer 
SearchDataNode(mitk::DataStorage::Pointer dataStorage, mitk::DataNode::Pointer projFolderNode, char* nodeName, char* folderName)
{
    mitk::DataNode::Pointer pathFolderNode = GetToolNode(dataStorage,projFolderNode,folderName);
    mitk::DataNode::Pointer exitingNode=NULL;

    if(pathFolderNode.IsNull())
        exitingNode=dataStorage->GetNamedNode(nodeName);
    else
        exitingNode=dataStorage->GetNamedDerivedNode(nodeName,pathFolderNode);
    
    return exitingNode;
}

//------------------
// AddImageFromFile
//------------------
//
int 
AddImageFromFile(mitk::DataStorage::Pointer dataStorage, mitk::DataNode::Pointer folderNode, char* fileName, 
                 char*childName, bool copy, double scaleFactor)
{
    try
    {
        
        mitk::DataNode::Pointer imageNode=sv4guiProjectManager::LoadDataNode(std::string(fileName));

        mitk::NodePredicateDataType::Pointer isImage = mitk::NodePredicateDataType::New("Image");
        if(imageNode.IsNull() || !isImage->CheckNode(imageNode))
        {
            fprintf(stderr, "Not Image!", "Please add an image.");
            return SV_ERROR;
        }

        mitk::BaseData::Pointer mimage = imageNode->GetData();
        if(mimage.IsNull() || !mimage->GetTimeGeometry()->IsValid())
        {
            fprintf(stderr,"Not Valid!", "Please add a valid image.");
            return SV_ERROR;
        }


        sv4guiProjectManager::AddImage(dataStorage, fileName, imageNode, folderNode, copy, scaleFactor, childName);

        return SV_OK;
    }
    catch(...)
    {
        fprintf(stderr,"Error adding image.");
        return SV_ERROR;
    }
}

//--------------------
// MitkImage2VtkImage
//--------------------
// copied from svVTKUtils.h - linker command failed to link with this file?
//
vtkImageData * 
MitkImage2VtkImage(mitk::Image* image)
{
    vtkImageData* vtkImg=image->GetVtkImageData();
    mitk::Point3D org = image->GetTimeGeometry()->GetGeometryForTimeStep(0)->GetOrigin();
    mitk::BaseGeometry::BoundsArrayType extent=image->GetTimeGeometry()->GetGeometryForTimeStep(0)->GetBounds();

    vtkImageData* newVtkImg = vtkImageData::New();
    newVtkImg->ShallowCopy(vtkImg);

    int whole[6];
    double *spacing, origin[3];

    whole[0]=extent[0];
    whole[1]=extent[1]-1;
    whole[2]=extent[2];
    whole[3]=extent[3]-1;
    whole[4]=extent[4];
    whole[5]=extent[5]-1;

    spacing = vtkImg->GetSpacing();

    origin[0] = spacing[0] * whole[0] +org[0];
    origin[1] = spacing[1] * whole[2] +org[1];
    whole[1] -= whole[0];
    whole[3] -= whole[2];
    whole[0] = 0;
    whole[2] = 0;
    origin[2] = spacing[2] * whole[4]+org[2];
    whole[5] -= whole[4];
    whole[4] = 0;

    newVtkImg->SetExtent(whole);
    newVtkImg->SetOrigin(origin);
    newVtkImg->SetSpacing(spacing);

    return newVtkImg;
}

//----------------
// GetDataStorage
//----------------
//
static mitk::DataStorage::Pointer 
GetDataStorage(SvPyUtilApiFunction& api)
{
  mitk::DataStorage::Pointer dataStorage; 
  ctkServiceReference dsServiceRef;
  ctkPluginContext* context = sv4guiPythonDataNodesPluginActivator::GetContext();
    
  if (context) {
      dsServiceRef = context->getServiceReference<mitk::IDataStorageService>();
  } else {
      api.error("Could not get the active data storgage. A project must be active.");
      return dataStorage; 
  }

  mitk::IDataStorageService* dss = nullptr;
  if (dsServiceRef) {
      dss = context->getService<mitk::IDataStorageService>(dsServiceRef);
  }
    
  if (!dss) {
      api.error("Could not get the active data storgage. A project must be active.");
      return dataStorage; 
  }
    
  // Get the active data storage (or the default one, if none is active)
  mitk::IDataStorageReference::Pointer dsRef;
  dsRef = dss->GetDataStorage();
  context->ungetService(dsServiceRef);

  return dsRef->GetDataStorage();
}

//-------------
// GetDataNode
//-------------
// Get the SV Data Manager node under the given top level data node.
//
// These data nodes contain data for images, paths, contours, models and meshes.
//
mitk::DataNode::Pointer 
GetDataNode(mitk::DataStorage::Pointer& dataStorage, mitk::DataNode::Pointer& projFolderNode, char *childName, 
            char *nodeName, int useRepositry) 
{
  mitk::DataNode::Pointer dataNode;

  if (useRepositry) {
      dataNode = GetToolNode(dataStorage, projFolderNode, SvDataManagerNodes::Repository);
  } else { 
      dataNode = GetToolNode(dataStorage, projFolderNode, nodeName);
      mitk::DataNode::Pointer existingNode = dataStorage->GetNamedDerivedNode(childName, dataNode);
      if (existingNode) {
          mitk::DataNode::Pointer dataNode;
          return dataNode;
      }
  }

  return dataNode;
}

//---------------------
// GetRepositoryObject
//---------------------
// Get an object from the repository with a 
// given name and type.
//
cvRepositoryData *
GetRepositoryObject(SvPyUtilApiFunction& api, char* name, RepositoryDataT objType, const std::string& desc)
{
  auto obj = gRepository->GetObject(name);

  if (obj == NULL) {
      api.error("The " + desc + " named '"+std::string(name)+"' is not in the repository.");
      return nullptr;
  }

  if (obj->GetType() != objType) {
      auto stype = RepositoryDataT_EnumToStr(objType);
      api.error("The repository object named '"+std::string(name)+"' is not of type '" + stype + "'.");
      return nullptr;
  }

  return obj;
}

//////////////////////////////////////////////////////
//          M o d u l e  F u n c t i o n s          //
//////////////////////////////////////////////////////
//
// Python API functions. 

//----------------------------
// Dmg_import_image_from_file 
//----------------------------
//
PyDoc_STRVAR(Dmg_import_image_from_file_doc,
  "import_image_from_file(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args: \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_import_image_from_file(PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("ss|iid", PyRunTimeErr, __func__);
    char* fileName;
    char* childName;
    int useRepository = 0;
    int copy = 0;
    double factor = 0.;

    if (!PyArg_ParseTuple(args, api.format, &fileName, &childName, &useRepository, &copy, &factor)) {
        return api.argsError();
    }

    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }
    
    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    // Get the SV Data Manager Image node or svRepositoryFolder node.
    auto folderNode = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Image, useRepository);

    if (AddImageFromFile(dataStorage,folderNode,fileName,childName,copy,factor) == SV_ERROR) {
        api.error("Error adding the image from a file data node '" + std::string(childName) + "' to the parent node '" + 
            folderNode->GetName() + "'.");
        return nullptr;
    }

    return SV_PYTHON_OK;
}

//-------------------------------------
// Dmg_import_polydata_from_repository 
//-------------------------------------
//
PyDoc_STRVAR(Dmg_import_polydata_from_repository_doc,
  "import_polydata_from_repository(kernel)  \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args: \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_import_polydata_from_repository(PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__);
    char* childName;
    int useRepository = 0;

    if (!PyArg_ParseTuple(args, api.format, &childName, &useRepository)) {
        return api.argsError();
    }
            
    auto obj = gRepository->GetObject(childName);
    if (obj == NULL) {
        PyErr_SetString(PyRunTimeErr, "couldn't find PolyData" );
        api.error("The polydata named '"+std::string(childName)+"' is not in the repository.");
        return nullptr;
    }

    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }

    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    // Get the SV Data Manager Image node or svRepositoryFolder node.
    auto folderNode = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Model, useRepository);

    if (AddDataNode(dataStorage, obj, folderNode, childName) == SV_ERROR) {
        api.error("Error adding the model data node '" + std::string(childName) + "' to the parent node '" + 
            folderNode->GetName() + "'.");
        return nullptr;
    }
    
    return SV_PYTHON_OK;
}

//----------------------------------------------
// Dmg_import_unstructured_grid_from_repository 
//----------------------------------------------
//
PyDoc_STRVAR(Dmg_import_unstructured_grid_from_repository_doc,
  "import_unstructured_grid_from_repository(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_import_unstructured_grid_from_repository(PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__);
    char* childName;
    int useRepository = 0;

    if (!PyArg_ParseTuple(args, api.format, &childName, &useRepository)) {
        return api.argsError();
    }

    auto obj = gRepository->GetObject(childName);
    if (obj == NULL) {
        api.error("The unstructured grid named '"+std::string(childName)+"' is not in the repository.");
        return nullptr;
    }

    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }
    
    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    // Get the SV Data Manager Image node or svRepositoryFolder node.
    auto folderNode = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Mesh, useRepository);

    if (AddDataNode(dataStorage, obj, folderNode, childName) == SV_ERROR) {
        api.error("Error adding the mesh data node '" + std::string(childName) + "' to the parent node '" + 
            folderNode->GetName() + "'.");
        return nullptr;
    }

    return SV_PYTHON_OK;
}

//--------------------------------
// Dmg_export_model_to_repository 
//--------------------------------
//
PyDoc_STRVAR(Dmg_export_model_to_repository_doc,
  "export_model_to_repository(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_export_model_to_repository(PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("ss|i", PyRunTimeErr, __func__);
    char* childName;
    char* repoName;
    int useRepository = 0;
    
    if (!PyArg_ParseTuple(args, api.format, &childName, &repoName, useRepository)) {
        return api.argsError();
    }
    
    if (gRepository->Exists(repoName)) {
        api.error("The repository object '" + std::string(repoName) + "' already exists.");
        return nullptr;
    }

    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }

    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    // Get the SV Data Manager Model node or svRepositoryFolder node.
    auto node = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Model, useRepository);
    if (node.IsNull()) {
        auto nodeName = (useRepository ? SvDataManagerNodes::Repository : SvDataManagerNodes::Model);
        api.error("The Model node '" + std::string(childName) + "' was not found under '" + nodeName + "'."); 
        return nullptr;
    }

    auto model = dynamic_cast<sv4guiModel*>(node->GetData());
    auto modelElement = model->GetModelElement();
    vtkSmartPointer<vtkPolyData> pd = modelElement->GetWholeVtkPolyData();
    if (pd == NULL) {
        api.error("Unable to get Model polydata for '" + std::string(childName) + "' from the SV Data Manager.");
        return nullptr;
    }

    cvPolyData* cvpd = new cvPolyData(pd);
    if (!gRepository->Register(repoName, cvpd)) {
        delete cvpd;
        api.error("Error adding the Model polydata '" + std::string(repoName) + "' to the repository.");
        return nullptr;
    }
    
    return SV_PYTHON_OK;
}

//-------------------------------
// Dmg_export_mesh_to_repository 
//-------------------------------
//
PyDoc_STRVAR(Dmg_export_mesh_to_repository_doc,
  "export_mesh_to_repository(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_export_mesh_to_repository( PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("ss|i", PyRunTimeErr, __func__);
    char* childName;
    char* repoName;
    int useRepository = 0;
    
    if (!PyArg_ParseTuple(args, api.format, &childName, &repoName, &useRepository)) {
        return api.argsError();
    }

    if (gRepository->Exists(repoName)) {
        api.error("The repository object '" + std::string(repoName) + "' already exists.");
        return nullptr;
    }

    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }

    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    // Get the SV Data Manager Mesh node or svRepositoryFolder node.
    auto node = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Mesh, useRepository);
    if (node.IsNull()) {
        auto nodeName = (useRepository ? SvDataManagerNodes::Repository : SvDataManagerNodes::Mesh);
        api.error("The Mesh node '" + std::string(childName) + "' was not found under '" + nodeName + "'."); 
        return nullptr;
    }
    
    auto  mitkMesh = dynamic_cast<sv4guiMitkMesh*> (node->GetData());
    if (!mitkMesh) { 
        api.error("Unable to get Mesh unstructured grid for '" + std::string(childName) + "' from the SV Data Manager.");
        return nullptr;
     }
    auto mesh = mitkMesh->GetMesh();
    vtkSmartPointer<vtkUnstructuredGrid> ug = mesh->GetVolumeMesh();

    if (ug == NULL) {
        api.error("Unable to get Mesh unstructured grid for '" + std::string(childName) + "' from the SV Data Manager.");
        return nullptr;
    }

    auto cvug = new cvUnstructuredGrid(ug);
    if (!gRepository->Register(repoName, cvug)) {
        delete cvug;
        api.error("Error adding the Mesh unstructured grid '" + std::string(repoName) + "' to the repository.");
        return nullptr;
    }

    return SV_PYTHON_OK;
}

//--------------------------------
// Dmg_export_image_to_repository
//--------------------------------
//
PyDoc_STRVAR(Dmg_export_image_to_repository_doc,
  "export_image_to_repository(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_export_image_to_repository(PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__);
    char* childName;
    char* repoName;
    int useRepository = 0;
    
    if (!PyArg_ParseTuple(args, api.format, &childName, &repoName, &useRepository)) {
        return api.argsError();
    }
    
    if (gRepository->Exists(repoName)) {
        api.error("The repository object '" + std::string(repoName) + "' already exists.");
        return nullptr;
    }

    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }

    // Get the project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    // Get the SV Data Manager Mesh folder or svRepositoryFolder folder.
    auto node = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Image, useRepository);
    if (node.IsNull()) {
        auto nodeName = (useRepository ? SvDataManagerNodes::Repository : SvDataManagerNodes::Mesh);
        api.error("The Image node '" + std::string(childName) + "' was not found under '" + nodeName + "'."); 
        return nullptr;
    }

    auto image = dynamic_cast<mitk::Image*>(node->GetData());
    if (image == nullptr) {
        api.error("Unable to get image for for '" + std::string(childName) + "' from the SV Data Manager.");
        return nullptr;
    }

    auto vtkObj = MitkImage2VtkImage(image);
    if (vtkObj == nullptr) {
        api.error("Unable to get image for '" + std::string(childName) + "' from the SV Data Manager.");
        return nullptr;
    }

    // Convert image data to structured points.
    //
    // Need to shift the origin like what used to be done
    // in vtkImageToStructuredPoints class.
    //
    // [TODO:DaveP] why the 'tmpsp' is needed?
    //
    auto *tmpsp = vtkStructuredPoints::New();
    tmpsp->ShallowCopy(vtkObj);

    int whole[6];
    int extent[6];
    double *spacing, origin[3];
    
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

    tmpsp->SetExtent(whole);
    tmpsp->SetOrigin(origin);
    tmpsp->SetSpacing(spacing);

    auto sp = new cvStrPts(tmpsp);
    sp->SetName(repoName);
    tmpsp->Delete();

    if (!gRepository->Register(repoName, sp)) {
        delete sp;
        api.error("Error adding the Image structured points '" + std::string(repoName) + "' to the repository.");
        return nullptr;
    }
    
    return SV_PYTHON_OK;
}

//-------------------------------
// Dmg_export_path_to_repository 
//-------------------------------
//
PyDoc_STRVAR(Dmg_export_path_to_repository_doc,
  "export_path_to_repository(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_export_path_to_repository(PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("ss|i", PyRunTimeErr, __func__);
    char* childName=NULL;
    char* repoName=NULL;
    int useRepository = 0;
    
    if (!PyArg_ParseTuple(args, api.format, &childName, &repoName, &useRepository)) {
        return api.argsError();
    }
    
    if (gRepository->Exists(repoName)) {
        api.error("The repository object '" + std::string(repoName) + "' already exists.");
        return nullptr;
    }

    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }

    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    // Get the SV Data Manager Path node or svRepositoryFolder node.
    auto node = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Path, useRepository);
    if (node.IsNull()) {
        auto nodeName = (useRepository ? SvDataManagerNodes::Repository : SvDataManagerNodes::Path);
        api.error("The Path node '" + std::string(childName) + "' was not found under '" + nodeName + "'."); 
        return nullptr;
    }

    auto path = dynamic_cast<sv4guiPath*>(node->GetData());
    auto pathElem = path->GetPathElement();
    auto corePath = new sv3::PathElement();

    switch(pathElem->GetMethod()) {
        case sv4guiPathElement::CONSTANT_TOTAL_NUMBER:
            corePath->SetMethod(sv3::PathElement::CONSTANT_TOTAL_NUMBER);
        break;
        case sv4guiPathElement::CONSTANT_SUBDIVISION_NUMBER:
            corePath->SetMethod(sv3::PathElement::CONSTANT_SUBDIVISION_NUMBER);
        break;
        case sv4guiPathElement::CONSTANT_SPACING:
            corePath->SetMethod(sv3::PathElement::CONSTANT_SPACING);
        break;
        default:
        break;
    }

    corePath->SetCalculationNumber(pathElem->GetCalculationNumber());
    corePath->SetSpacing(pathElem->GetSpacing());

    // Copy control points.
    std::vector<mitk::Point3D> pts = pathElem->GetControlPoints();
    for (int i=0; i<pts.size();i++) {
        std::array<double,3> point;
        point[0] = pts[i][0];
        point[1] = pts[i][1];
        point[2] = pts[i][2];
        corePath->InsertControlPoint(i,point);
    }

    // Create path points.
    corePath->CreatePathPoints();
    
    if (!gRepository->Register(repoName, corePath)) {
        delete corePath;
        api.error("Error adding the path element '" + std::string(repoName) + "' to the repository.");
        return nullptr;
    }

    return SV_PYTHON_OK;
}

//---------------------------------
// Dmg_import_path_from_repository 
//---------------------------------
//
PyDoc_STRVAR(Dmg_import_path_from_repository_doc,
  "import_path_from_repository(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_import_path_from_repository(PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("s|i", PyRunTimeErr, __func__);
    char* childName;
    int useRepository = 0;

    if (!PyArg_ParseTuple(args, api.format, &childName, &useRepository)) {
        return api.argsError();
    }

    auto obj = GetRepositoryObject(api, childName, PATH_T, "path");
    if (obj == NULL) {
        api.error("The unstructured grid named '"+std::string(childName)+"' is not in the repository.");
        return nullptr;
    }
    
    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }

    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    // Get the SV Data Manager Image node or svRepositoryFolder node.
    auto folderNode = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Path, useRepository);

    if (AddDataNode(dataStorage, obj,folderNode,childName)==SV_ERROR) {
        api.error("Error adding the image data node '" + std::string(childName) + "' to the parent node '" + 
            folderNode->GetName() + "'.");
        return nullptr;
    }

    return SV_PYTHON_OK;
}

//------------------------------------
// Dmg_import_contour_from_repository 
//------------------------------------
//
PyDoc_STRVAR(Dmg_import_contour_from_repository_doc,
  "import_contour_from_repository(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_import_contour_from_repository(PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("sOs|i", PyRunTimeErr, __func__);
    char* childName;
    PyObject* srcList = NULL;
    char* pathName;
    int useRepository = 0;

    if (!PyArg_ParseTuple(args, api.format, &childName, &srcList, &pathName, &useRepository)) {
        return api.argsError();
    }

    // Get a list of contour objects.
    //
    int numsrc = PyList_Size(srcList);
    std::vector<cvRepositoryData*> objs;

    for (int i = 0; i < numsrc; i++) {
        #if PYTHON_MAJOR_VERSION == 2
        auto srcName = PyString_AsString(PyList_GetItem(srcList,i));
        #endif
        #if PYTHON_MAJOR_VERSION == 3
        auto srcName = PyBytes_AsString(PyUnicode_AsUTF8String(PyList_GetItem(srcList,i)));
        #endif
        auto obj = GetRepositoryObject(api, srcName, CONTOUR_T, "contour");
        if (obj == nullptr) {
            return nullptr;
        }

        objs.push_back(obj);
    }

    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }

    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    // Get the path node.
    //
    auto pathNode = GetDataNode(dataStorage, projFolderNode, pathName, SvDataManagerNodes::Path, useRepository);
    if (pathNode.IsNull()) {
        auto nodeName = (useRepository ? SvDataManagerNodes::Repository : SvDataManagerNodes::Path);
        api.error("The Path node '" + std::string(pathName) + "' was not found under '" + nodeName + "'."); 
        return nullptr;
    }

    // [TODO:DaveP] not sure about this, needing to check path.IsNull() ?
    sv4guiPath::Pointer path = dynamic_cast<sv4guiPath*>(pathNode->GetData());
    if (path.IsNull()) {
        auto nodeName = (useRepository ? SvDataManagerNodes::Repository : SvDataManagerNodes::Path);
        api.error("The Path node '" + std::string(pathName) + "' under '" + nodeName + "' has no data."); 
        return nullptr;
    }

    // Get the segmentation node.
    //
    auto segNode = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Segmentation, useRepository);
    if (segNode.IsNull()) {
        auto nodeName = (useRepository ? SvDataManagerNodes::Repository : SvDataManagerNodes::Path);
        api.error("The Segmentation node '" + std::string(childName) + "' was not found under '" + nodeName + "'."); 
        return nullptr;
    }

    if (AddContourDataNode(dataStorage, objs, segNode, childName, pathName, path)==SV_ERROR) {
        api.error("Error adding the segmentation data node '" + std::string(childName) + "' to the parent node '" + 
            segNode->GetName() + "'.");
        return nullptr;
    }

    return SV_PYTHON_OK;
}

//----------------------------------
// Dmg_export_contour_to_repository 
//----------------------------------
//
PyDoc_STRVAR(Dmg_export_contour_to_repository_doc,
  "export_contour_to_repository(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_export_contour_to_repository( PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("sO|i", PyRunTimeErr, __func__);
    char* childName=NULL;
    PyObject* dstList=NULL;
    int useRepository = 0;
    
    if(!PyArg_ParseTuple(args, api.format, &childName,&dstList, &useRepository)) {
        return api.argsError();
    }
    
    int num = PyList_Size(dstList);
    std::vector<char*> strList;

    for (int i=0; i<num; i++) {
        #if PYTHON_MAJOR_VERSION == 2
        auto name = PyString_AsString(PyList_GetItem(dstList,i));
        #endif
        #if PYTHON_MAJOR_VERSION == 3
        auto name = PyBytes_AsString(PyUnicode_AsUTF8String(PyList_GetItem(dstList,i)));
        #endif
        if (gRepository->Exists(name)) {
            api.error("The repository object '" + std::string(name) + "' already exists.");
            return nullptr;
        }
        strList.push_back(name);
    }

    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }

    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }

    auto node = GetDataNode(dataStorage, projFolderNode, childName, SvDataManagerNodes::Segmentation, useRepository);
    if (node.IsNull()) {
        auto nodeName = (useRepository ? SvDataManagerNodes::Repository : SvDataManagerNodes::Segmentation);
        api.error("The Segmentation node '" + std::string(childName) + "' was not found under '" + nodeName + "'."); 
        return nullptr;
    }

    sv4guiContourGroup* group = dynamic_cast<sv4guiContourGroup*> (node->GetData());
    if (group==NULL) {
        api.error("Unable to get contour groups for '" + std::string(childName) + "' from the SV Data Manager.");
        return nullptr;
    }

    for (int i=0; i<num; i++) {
        cKernelType kernel;
        sv4guiContour* contour = group->GetContour(i);
        vtkSmartPointer<vtkPolyData> contourPd = contour->CreateVtkPolyDataFromContour();
        if (contourPd == NULL) {
            api.error("Unable to get polydata for the contour '" + std::string(strList[i]) + "'.");
            return nullptr;
        } 
        cvPolyData* cvpd = new cvPolyData( contourPd );
        if (!gRepository->Register(strList[i], cvpd)) {
            delete cvpd;
            api.error("Error adding the contour polydata '" + std::string(strList[i]) + "' to the repository.");
            return nullptr;
        }
    }

    return SV_PYTHON_OK;
}

//----------------------
// Dmg_remove_data_node 
//----------------------
//
PyDoc_STRVAR(Dmg_remove_data_node_doc,
  "remove_data_node(kernel)                                    \n\ 
   \n\
   ??? Set the computational kernel used to segment image data.       \n\
   \n\
   Args:                                                          \n\
     kernel (str): Name of the contouring kernel. Valid names are: Circle, Ellipse, LevelSet, Polygon, SplinePolygon or Threshold. \n\
");

static PyObject * 
Dmg_remove_data_node(PyObject* self, PyObject* args)
{
    auto api = SvPyUtilApiFunction("ss", PyRunTimeErr, __func__);
    char* childName=NULL;
    char* parentName=NULL;
    
    if(!PyArg_ParseTuple(args, api.format, &childName,&parentName)) {
        return api.argsError();
    }
    
    // Get the Data Storage node.
    auto dataStorage = GetDataStorage(api);
    if (dataStorage.IsNull()) {
        return nullptr;
    }

    // Get project folder.
    auto projFolderNode = GetProjectNode(api, dataStorage);
    if (projFolderNode.IsNull()) {
        return nullptr;
    }
    
    mitk::DataNode::Pointer parentNode = dataStorage->GetNamedDerivedNode(parentName,projFolderNode);
    if(!parentNode) {
        api.error("The data node '" + std::string(parentName) + "' was not found."); 
        return nullptr;
    }

    if (RemoveDataNode(dataStorage, parentNode,childName)==SV_ERROR) {
        api.error("Error removing the data node '" + std::string(childName) + "' under '" + std::string(parentName) + "'.");
        return nullptr;
    }
    
    return SV_PYTHON_OK;
}

////////////////////////////////////////////////////////
//          M o d u l e  D e f i n i t i o n          //
////////////////////////////////////////////////////////

//--------------------
// dmg module methods
//--------------------
//
PyMethodDef pyDmg_methods[] =
{
    {"export_contour_to_repository", Dmg_export_contour_to_repository, METH_VARARGS, Dmg_export_contour_to_repository_doc},

    {"export_image_to_repository", Dmg_export_image_to_repository, METH_VARARGS, Dmg_export_image_to_repository_doc},

    {"export_mesh_to_repository", Dmg_export_mesh_to_repository, METH_VARARGS, Dmg_export_mesh_to_repository_doc},

    {"export_model_to_repository", Dmg_export_model_to_repository, METH_VARARGS, Dmg_export_model_to_repository_doc},

    {"export_path_to_repository", Dmg_export_path_to_repository, METH_VARARGS, Dmg_export_path_to_repository_doc},

    {"import_contour_from_repository", Dmg_import_contour_from_repository, METH_VARARGS, Dmg_import_contour_from_repository_doc},

    {"import_image", Dmg_import_image_from_file, METH_VARARGS, Dmg_import_image_from_file_doc},

    {"import_path_from_repository", Dmg_import_path_from_repository, METH_VARARGS, Dmg_import_path_from_repository_doc},

    {"import_polydata_from_repository", Dmg_import_polydata_from_repository, METH_VARARGS, Dmg_import_polydata_from_repository_doc},

    {"import_unstructured_grid_from_repository", Dmg_import_unstructured_grid_from_repository, METH_VARARGS, 
        Dmg_import_unstructured_grid_from_repository_doc},

    {"remove_data_node", Dmg_remove_data_node, METH_VARARGS, Dmg_remove_data_node_doc},

    {NULL, NULL,0,NULL},
};

//-----------------------
// Initialize the module
//-----------------------
// Define the initialization function called by the Python 
// interpreter when the module is loaded.

static char* MODULE_NAME = "dmg";

PyDoc_STRVAR(DmgModule_doc, "dmg module functions");

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

static struct PyModuleDef pyDmgmodule = {
   m_base,
   MODULE_NAME, 
   DmgModule_doc, 
   perInterpreterStateSize,  
   pyDmg_methods
};

//--------------
// PyInit_pyDmg
//--------------
//
PyMODINIT_FUNC 
PyInit_pyDmg(void)
{
  PyObject *pyDmg;
  pyDmg = PyModule_Create(&pyDmgmodule);

  if ( gRepository == NULL ) {
    gRepository = new cvRepository();
    fprintf( stdout, "gRepository created from pyDmg\n" );
  }
  
  PyRunTimeErr = PyErr_NewException("pyDmg.error",NULL,NULL);
  Py_INCREF(PyRunTimeErr);
  PyModule_AddObject(pyDmg,"error",PyRunTimeErr);
  
  return pyDmg;

}
#endif

//---------------------------------------------------------------------------
//                           PYTHON_MAJOR_VERSION 2                         
//---------------------------------------------------------------------------

//------------------
//  initpyDmg
//------------------
#if PYTHON_MAJOR_VERSION == 2
PyMODINIT_FUNC initpyDmg(void)

{

  PyObject *pyDmg;
  
  if ( gRepository == NULL ) {
    gRepository = new cvRepository();
    fprintf( stdout, "gRepository created from pyDmg\n" );
    return;
  }
  
  pyDmg = Py_InitModule("pyDmg",pyDmg_methods);

  PyRunTimeErr = PyErr_NewException("dmg.DmgException",NULL,NULL);
  Py_INCREF(PyRunTimeErr);
  PyModule_AddObject(pyDmg,"DmgException",PyRunTimeErr);

}

#endif

