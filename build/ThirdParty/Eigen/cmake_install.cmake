# Install script for directory: G:/program/FireEngine/ThirdParty/Eigen

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/FireEngine")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Eigen/Cholesky;/Eigen/CholmodSupport;/Eigen/Core;/Eigen/Dense;/Eigen/Eigen;/Eigen/Eigenvalues;/Eigen/Geometry;/Eigen/Householder;/Eigen/IterativeLinearSolvers;/Eigen/Jacobi;/Eigen/KLUSupport;/Eigen/LU;/Eigen/MetisSupport;/Eigen/OrderingMethods;/Eigen/PaStiXSupport;/Eigen/PardisoSupport;/Eigen/QR;/Eigen/QtAlignedMalloc;/Eigen/SPQRSupport;/Eigen/SVD;/Eigen/Sparse;/Eigen/SparseCholesky;/Eigen/SparseCore;/Eigen/SparseLU;/Eigen/SparseQR;/Eigen/StdDeque;/Eigen/StdList;/Eigen/StdVector;/Eigen/SuperLUSupport;/Eigen/UmfPackSupport")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Eigen" TYPE FILE FILES
    "G:/program/FireEngine/ThirdParty/Eigen/Cholesky"
    "G:/program/FireEngine/ThirdParty/Eigen/CholmodSupport"
    "G:/program/FireEngine/ThirdParty/Eigen/Core"
    "G:/program/FireEngine/ThirdParty/Eigen/Dense"
    "G:/program/FireEngine/ThirdParty/Eigen/Eigen"
    "G:/program/FireEngine/ThirdParty/Eigen/Eigenvalues"
    "G:/program/FireEngine/ThirdParty/Eigen/Geometry"
    "G:/program/FireEngine/ThirdParty/Eigen/Householder"
    "G:/program/FireEngine/ThirdParty/Eigen/IterativeLinearSolvers"
    "G:/program/FireEngine/ThirdParty/Eigen/Jacobi"
    "G:/program/FireEngine/ThirdParty/Eigen/KLUSupport"
    "G:/program/FireEngine/ThirdParty/Eigen/LU"
    "G:/program/FireEngine/ThirdParty/Eigen/MetisSupport"
    "G:/program/FireEngine/ThirdParty/Eigen/OrderingMethods"
    "G:/program/FireEngine/ThirdParty/Eigen/PaStiXSupport"
    "G:/program/FireEngine/ThirdParty/Eigen/PardisoSupport"
    "G:/program/FireEngine/ThirdParty/Eigen/QR"
    "G:/program/FireEngine/ThirdParty/Eigen/QtAlignedMalloc"
    "G:/program/FireEngine/ThirdParty/Eigen/SPQRSupport"
    "G:/program/FireEngine/ThirdParty/Eigen/SVD"
    "G:/program/FireEngine/ThirdParty/Eigen/Sparse"
    "G:/program/FireEngine/ThirdParty/Eigen/SparseCholesky"
    "G:/program/FireEngine/ThirdParty/Eigen/SparseCore"
    "G:/program/FireEngine/ThirdParty/Eigen/SparseLU"
    "G:/program/FireEngine/ThirdParty/Eigen/SparseQR"
    "G:/program/FireEngine/ThirdParty/Eigen/StdDeque"
    "G:/program/FireEngine/ThirdParty/Eigen/StdList"
    "G:/program/FireEngine/ThirdParty/Eigen/StdVector"
    "G:/program/FireEngine/ThirdParty/Eigen/SuperLUSupport"
    "G:/program/FireEngine/ThirdParty/Eigen/UmfPackSupport"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Eigen/src")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Eigen" TYPE DIRECTORY FILES "G:/program/FireEngine/ThirdParty/Eigen/src" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

