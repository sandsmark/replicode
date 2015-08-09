add_executable(compilertest main.cpp)
target_link_libraries(compilertest CoreLibrary r_code r_comp r_exec)
set_property(TARGET compilertest PROPERTY CXX_STANDARD 11)
set_property(TARGET compilertest PROPERTY CXX_STANDARD_REQUIRED ON)

