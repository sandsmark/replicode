project(replicode)

cmake_minimum_required(VERSION 2.8)
find_package (Threads)

include_directories(${PROJECT_SOURCE_DIR})

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11 -fPIC -g")
endif()

option(ENABLE_SANITIZERS "Enable runtime sanitizing (for development)")
if (ENABLE_SANITIZERS)
    message("Enabling asan and ubsan")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif()


add_subdirectory(r_code)
add_subdirectory(r_comp)
add_subdirectory(r_exec)
add_subdirectory(usr_operators)
add_subdirectory(tests)

add_executable(replicode executor/main.cpp executor/inifile.cpp replicode_common.h)
set_property(TARGET replicode PROPERTY CXX_STANDARD 11)
set_property(TARGET replicode PROPERTY CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
configure_file(executor/settings.ini ${CMAKE_BINARY_DIR}/settings.ini COPYONLY)
target_link_libraries(replicode r_exec r_comp r_code dl pthread rt)

install(FILES replicode_common.h DESTINATION include)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

enable_testing()

foreach(test_file
        "hello.world.replicode"
        "pong.replicode"
        "test.1.replicode"
        "test.2.replicode"
        "test.3.replicode"
        "test.4.replicode")
    add_test(NAME Compiler-${test_file} WORKING_DIRECTORY /home/sandsmark/src/replicode/tests/compiler/ COMMAND compilertest ${test_file})
endforeach()
