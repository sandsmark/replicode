project(replicode)

cmake_minimum_required(VERSION 2.8)

include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(CoreLibrary)
add_subdirectory(r_code)
add_subdirectory(r_comp)
add_subdirectory(r_exec)
add_subdirectory(usr_operators)

add_executable(replicode executor/main.cpp)
set_property(TARGET replicode PROPERTY CXX_STANDARD 11)
set_property(TARGET replicode PROPERTY CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
configure_file(executor/settings.ini ${CMAKE_BINARY_DIR}/settings.ini COPYONLY)
target_link_libraries(replicode CoreLibrary r_exec r_comp r_code dl pthread rt)
