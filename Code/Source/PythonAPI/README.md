
# SimVascular Python API 

The SimVascular Python API is implemented using Python and C++. The files in this directory implement most of the SimVascular Python API. The following modules have been implemented 

- geometry
- meshing
- modeling
- pathplanning
- segmentation
- vmtk

The module names reflect the names of the SV application tools.

The code for the **dmg** module is located in **SimVascular/Code/Source/sv4gui/Plugins/org.sv.pythondatanodes**. The  **dmg** module must be impemented within a project so it can interact with the SV Data Manager (MITK) framework.

The code for the SimVascular Python API **sv** package is locted in **SimVascular/Python/site-packages/sv**. This directory also contains Python code implementing the following classes

- MeshSimOptions 
- Project
- Visualization


# Code Organization

The code is organized using seprate files for modules and classes defined for those modules

  - Modules are implemented in *ModuleName*\_PyModule.(cxx,h) files
  
  - Classes are implemented in *ModuleNameClassName*\_PyClass.cxx file. 

For example the **pathplanning** module code is contained in the following files
```
PathPlanning_PyModule.cxx
PathPlanning_PyModule.h
PathPlanningPath_PyClass.cxx
PathPlanningGroup_PyClass.cxx
PathPlanningGroup_PyClass.h
PathPlanningSubdivMethod_PyClass.cxx
```

# Implementing Python Extensions in C++

Python extension modules and classes have been implemented using certain coding and naming conventions.

## Modules

- static char* *ModuleName*_MODULE
- static PyMethodDef Py*ModuleName*ModuleMethods[] =


# Modules

## _pathplanning_ Module

The **path** module code is contained in
```
Path_PyModule.cxx
Path_PyModule.h
Path_PyClass.cxx
PathGroup_PyClass.cxx
PathGroup_PyClass.h
PathSubdivisionMethod_PyClass.cxx
```

