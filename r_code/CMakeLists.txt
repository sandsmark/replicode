set(r_code_SRC
    atom.cpp
    image.cpp
    #image.tpl.cpp
    image_impl.cpp
    object.cpp
    r_code.cpp
    utils.cpp
    vector.cpp
    )

add_library(r_code SHARED ${r_code_SRC})
set_property(TARGET r_code PROPERTY CXX_STANDARD 11)
set_property(TARGET r_code PROPERTY CXX_STANDARD_REQUIRED ON)
