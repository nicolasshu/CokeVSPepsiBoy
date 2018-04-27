# Set CMake minimum version and CMake policy required by UrhoCommon module
cmake_minimum_required (VERSION 3.2.3)

# Set project name
project (SecondProject)

# Set Environment variable                                      #****
set (ENV{URHO3D_HOME} "./Urho3D")                               #****
set (ENV{CMAKE_SOURCE_DIR} "./Urho3D/CMake")                        #****


# Set CMake modules search path
set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules)
set (CMAKE_CXX_STANDARD 14)
# Include UrhoCommon.cmake module after setting project name
include (UrhoCommon)
# Define target name
set (TARGET_NAME ${PROJECT_NAME})

# Define source files
file(GLOB_RECURSE SOURCE_FILES "Source/*.*pp" "Source/*.h" ) #"Utilities2D/*.*pp" "Utilities2D/*.*h"

# NEW THINGS
if (NOT URHO3D_URHO2D)
    return ()
endif ()

# Define source files
#define_source_files (EXTRA_H_FILES ./Utilities2D/Sample2D.h ./Utilities2D/Sample2D.cpp ./Utilities2D/Mover.h ./Utilities2D/Mover.cpp)

# Setup target with resource copying
setup_main_executable ()
