project(replicode)

cmake_minimum_required(VERSION 2.8)

include_directories(${PROJECT_SOURCE_DIR})

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=gnu++0x -fPIC -g")
endif()

add_subdirectory(CoreLibrary)
add_subdirectory(r_code)
add_subdirectory(r_comp)
add_subdirectory(r_exec)
add_subdirectory(executor)
add_subdirectory(usr_operators)
