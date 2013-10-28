add_executable(Test Test.cpp)
target_link_libraries(Test CoreLibrary r_exec r_comp r_code dl pthread rt)
