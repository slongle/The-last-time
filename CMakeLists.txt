cmake_minimum_required (VERSION 3.1)
project (The-Last-Time)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

########################################
# Third-party libraries


########################################
# Include
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    # Third party library
    ${CMAKE_SOURCE_DIR}/src/ext
) 

file(GLOB SOURCE_FILE
    ${CMAKE_SOURCE_DIR}/src/accelerator/*.h
    ${CMAKE_SOURCE_DIR}/src/accelerator/*.cpp
    ${CMAKE_SOURCE_DIR}/src/bsdf/*.h
    ${CMAKE_SOURCE_DIR}/src/bsdf/*.cpp
    ${CMAKE_SOURCE_DIR}/src/core/*.h
    ${CMAKE_SOURCE_DIR}/src/core/*.cpp
    ${CMAKE_SOURCE_DIR}/src/ext/*.h
    ${CMAKE_SOURCE_DIR}/src/ext/*.cpp
    ${CMAKE_SOURCE_DIR}/src/integrator/*.h
    ${CMAKE_SOURCE_DIR}/src/integrator/*.cpp
    ${CMAKE_SOURCE_DIR}/src/light/*.h
    ${CMAKE_SOURCE_DIR}/src/light/*.cpp
    ${CMAKE_SOURCE_DIR}/src/shape/*.h
    ${CMAKE_SOURCE_DIR}/src/shape/*.cpp
    ${CMAKE_SOURCE_DIR}/src/texture/*.h
    ${CMAKE_SOURCE_DIR}/src/texture/*.cpp
    ${CMAKE_SOURCE_DIR}/src/utility/*.h
    ${CMAKE_SOURCE_DIR}/src/utility/*.cpp
    ${CMAKE_SOURCE_DIR}/src/main.cpp
)

########################################
# Sort the file into different folders
foreach(_source_file IN ITEMS ${SOURCE_FILE})
    get_filename_component(_source_path "${_source_file}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source_file}")
endforeach()

add_executable(render ${SOURCE_FILE})