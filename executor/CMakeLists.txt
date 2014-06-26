add_executable(replicode main.cpp)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
configure_file(settings.ini ${CMAKE_BINARY_DIR}/settings.ini COPYONLY)
target_link_libraries(replicode CoreLibrary r_exec r_comp r_code dl pthread rt)
