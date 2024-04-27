# Install script for directory: C:/Dev/tda362-labs

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Dev/tda362-labs/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Dev/tda362-labs/out/build/x64-Debug/labhelper/cmake_install.cmake")
  include("C:/Dev/tda362-labs/out/build/x64-Debug/lab1-rasterization/cmake_install.cmake")
  include("C:/Dev/tda362-labs/out/build/x64-Debug/lab2-textures/cmake_install.cmake")
  include("C:/Dev/tda362-labs/out/build/x64-Debug/lab3-camera/cmake_install.cmake")
  include("C:/Dev/tda362-labs/out/build/x64-Debug/lab4-shading/cmake_install.cmake")
  include("C:/Dev/tda362-labs/out/build/x64-Debug/lab5-rendertotexture/cmake_install.cmake")
  include("C:/Dev/tda362-labs/out/build/x64-Debug/lab6-shadowmaps/cmake_install.cmake")
  include("C:/Dev/tda362-labs/out/build/x64-Debug/pathtracer/cmake_install.cmake")
  include("C:/Dev/tda362-labs/out/build/x64-Debug/project/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "C:/Dev/tda362-labs/out/build/x64-Debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
