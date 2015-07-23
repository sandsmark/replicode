set(r_comp_SRC
    class.cpp
    replistruct.cpp
    #    class_register.cpp
    compiler.cpp
    decompiler.cpp
    preprocessor.cpp
    r_comp.cpp
    segments.cpp
    #    string_utils.cpp
    structure_member.cpp
    )

add_library(r_comp SHARED ${r_comp_SRC})
set_property(TARGET r_comp PROPERTY CXX_STANDARD 11)
set_property(TARGET r_comp PROPERTY CXX_STANDARD_REQUIRED ON)
