
# SimVascular Python API 

The files in this directory implement most of the SimVascular Python API. The implementation of the **dmg** module is located in **SimVascular/Code/Source/sv4gui/Plugins/org.sv.pythondatanodes** because it needs to interact with the SV Data Manager (MITK) framework.

# Code Organization

The code is organized using seprate files for modules and classes belonging to those modules

  - Modules are implemented in *ModuleName*\_PyModule.(cxx,h) files
  
  - Classes are implemented in *ModuleNameClassName*\_PyClass.cxx files

For example the **path** module code is contained in
```
Path_PyModule.cxx
Path_PyModule.h
Path_PyClass.cxx
PathGroup_PyClass.cxx
PathGroup_PyClass.h
PathSubdivisionMethod_PyClass.cxx
```



