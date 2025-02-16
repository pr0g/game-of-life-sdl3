cmake_minimum_required(VERSION 3.28)

project(third-party)

include(ExternalProject)

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT isMultiConfig)
  if(NOT CMAKE_BUILD_TYPE)
    # set Debug as default build type if one is not provided (match CMake
    # default behavior)
    set(CMAKE_BUILD_TYPE
        Debug
        CACHE STRING "" FORCE)
  endif()
  # give different build types their own folder with single config generators
  set(build_type_dir ${CMAKE_BUILD_TYPE})
  # pass through build type arg to ExternalProject_Add command
  set(build_type_arg -DCMAKE_BUILD_TYPE=$<CONFIG>)
endif()

if(SUPERBUILD AND NOT PROJECT_IS_TOP_LEVEL)
  set(THIRD_PARTY_BINARY_DIR
      "${CMAKE_SOURCE_DIR}/build-third-party"
      CACHE STRING "Third-party build folder")
  if(NOT IS_ABSOLUTE ${THIRD_PARTY_BINARY_DIR})
    set(THIRD_PARTY_BINARY_DIR "${CMAKE_SOURCE_DIR}/${THIRD_PARTY_BINARY_DIR}")
  endif()
  set(PREFIX_DIR ${THIRD_PARTY_BINARY_DIR})
else()
  set(PREFIX_DIR ${CMAKE_CURRENT_BINARY_DIR})
endif()

ExternalProject_Add(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.4
  GIT_SHALLOW TRUE
  PREFIX ${PREFIX_DIR}/SDL3
  BINARY_DIR ${PREFIX_DIR}/SDL3/build/${build_type_dir}
  INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install
  CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>)

ExternalProject_Add(
  mc-array
  GIT_REPOSITORY https://github.com/PacktPublishing/Minimal-CMake.git
  GIT_TAG e996199b2f7527b8f0ac8ef3ca44b358d245edbd
  GIT_SHALLOW TRUE
  PREFIX ${PREFIX_DIR}/mc-array
  SOURCE_SUBDIR ch11/part-4/lib/array
  BINARY_DIR ${PREFIX_DIR}/mc-array/build/${build_type_dir}
  INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install
  CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  CMAKE_CACHE_ARGS -DCMAKE_DEBUG_POSTFIX:STRING=d)

ExternalProject_Add(
  mc-gol
  GIT_REPOSITORY https://github.com/PacktPublishing/Minimal-CMake.git
  GIT_TAG e996199b2f7527b8f0ac8ef3ca44b358d245edbd
  GIT_SHALLOW TRUE
  DEPENDS mc-array
  PREFIX ${PREFIX_DIR}/mc-gol
  SOURCE_SUBDIR ch11/part-4/lib/gol
  BINARY_DIR ${PREFIX_DIR}/mc-gol/build/${build_type_dir}
  INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/install
  CMAKE_ARGS ${build_type_arg} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
             -DMC_GOL_SHARED=ON
  CMAKE_CACHE_ARGS -DCMAKE_DEBUG_POSTFIX:STRING=d)

if(SUPERBUILD AND NOT PROJECT_IS_TOP_LEVEL)
  ExternalProject_Add(
    ${CMAKE_PROJECT_NAME}_superbuild
    DEPENDS SDL3 mc-array mc-gol
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    BINARY_DIR ${CMAKE_BINARY_DIR}
    CMAKE_ARGS -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} -DSUPERBUILD=OFF
               ${build_type_arg})
endif()
