set(usr_operators_SRC
    auto_focus.cpp
    usr_operators.cpp
    Callbacks/callbacks.cpp
    Operators/operators.cpp
    TestProgram/test_program.cpp
    )

add_library(usr_operators SHARED ${usr_operators_SRC})
target_link_libraries(usr_operators r_exec)
set_property(TARGET usr_operators PROPERTY CXX_STANDARD 11)
set_property(TARGET usr_operators PROPERTY CXX_STANDARD_REQUIRED ON)
