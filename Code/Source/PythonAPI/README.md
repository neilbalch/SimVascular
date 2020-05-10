
# SimVascular Python API 

The SimVascular Python API is implemented using Python and C++. The code for the Python API SimVascular **sv** package is locted in **SimVascular/Python/site-packages/sv**. There is also Python code there implementing the following classes

- MeshSimOptions 

The files in this directory implement most of the SimVascular Python API. The implementation of the **dmg** module is located in **SimVascular/Code/Source/sv4gui/Plugins/org.sv.pythondatanodes** because it needs to interact with the SV Data Manager (MITK) framework.

 

# Code Organization

The code is organized using seprate files for modules and classes defined for those modules

  - Modules are implemented in *ModuleName*\_PyModule.(cxx,h) files
  
  - Classes are implemented in *ModuleNameClassName*\_PyClass.cxx file. 

For example the **path** module code is contained in
```
Path_PyModule.cxx
Path_PyModule.h
Path_PyClass.cxx
PathGroup_PyClass.cxx
PathGroup_PyClass.h
PathSubdivisionMethod_PyClass.cxx
```

The following modules have been implemented 

- geometry
- meshing
- modeling
- path
- segmentation
- vmtk

The modules names reflect the names of the SV application tools.


# Modules

## _path_ Module

The **path** module code is contained in
```
Path_PyModule.cxx
Path_PyModule.h
Path_PyClass.cxx
PathGroup_PyClass.cxx
PathGroup_PyClass.h
PathSubdivisionMethod_PyClass.cxx
```

