# Framework

Every time you clone the repository, make sure that it is a sibling directory to the master Urho3D repository that you got from [Urho3D's Github](https://github.com/urho3d/Urho3D/). The CMakeLists.txt will already have everything included in modular form so that you don't have to keep downloading the Urho3D folder. In other words, we will be setting  
`set(ENV{URHO3D_HOME} "../Urho3D/")`  

The `bin/` directory is necessary because that is where all of the data, images, sounds, etc, will be.

The way that the folders are setup are:

```
CokeVSPepsiBoy/
|-- bin/
|-- CMake/
|-- CMakeFiles/
|-- Source/
|-- CMakeLists.txt
|-- CMakeCache.txt
|-- Makefile
`-- ...
Urho3D/
|-- Android/
|-- CMake/
`-- ...
```
