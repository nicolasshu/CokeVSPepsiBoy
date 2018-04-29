# Set CMake minimum version and CMake policy required by UrhoCommon module
cmake_minimum_required (VERSION 3.2.3)

# Set project name
project (Untamed)
# Set CMake modules search path
set (CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/Modules)
set (CMAKE_CXX_STANDARD 14)
set(ENV{URHO3D_HOME} "/home/yzkuris/Urho3D")
# Include UrhoCommon.cmake module after setting project name
include (UrhoCommon)
# Define target name
set (TARGET_NAME ${PROJECT_NAME})
# Define source files
file(GLOB_RECURSE SOURCE_FILES "Source/*.*pp" "Source/*.h")

if (NOT URHO3D_URHO2D)
    return()
endif ()

# Setup target with resource copying
setup_main_executable ()


