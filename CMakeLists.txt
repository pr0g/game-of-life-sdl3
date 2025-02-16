cmake_minimum_required(VERSION 3.28)
project(game-of-life-sdl3)

include(GNUInstallDirs)

option(SUPERBUILD "Perform a superbuild (or not)" OFF)

if(SUPERBUILD)
  add_subdirectory(third-party)
  return()
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(mc-gol CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(
  as
  GIT_REPOSITORY https://github.com/pr0g/as.git
  GIT_TAG a47a6deb8b5dbe408137aa4ce5cd556d26dc77c6)
FetchContent_MakeAvailable(as)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE main.cpp fps.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 as
                                              minimal-cmake::game-of-life)
