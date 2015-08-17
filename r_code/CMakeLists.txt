set(r_code_SRC
    atom.cpp
    image.cpp
    image_impl.cpp
    object.cpp
    r_code.cpp
    utils.cpp
    vector.cpp
    )
set(r_code_HDR
    atom.h
    image.h
    image.tpl.h
    image_impl.h
    list.h
    object.h
    r_code.h
    replicode_defs.h
    time_buffer.h
    utils.h
    vector.h
)

add_library(r_code SHARED ${r_code_SRC})
set_property(TARGET r_code PROPERTY CXX_STANDARD 11)
set_property(TARGET r_code PROPERTY CXX_STANDARD_REQUIRED ON)
install(TARGETS r_code DESTINATION lib)
install(FILES ${r_code_HDR} DESTINATION include/r_code)
