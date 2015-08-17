set(r_comp_SRC
    class.cpp
    replistruct.cpp
    compiler.cpp
    decompiler.cpp
    preprocessor.cpp
    segments.cpp
    structure_member.cpp
    )
set(r_comp_HDR
    class.h
    compiler.h
    decompiler.h
    out_stream.h
    preprocessor.h
    replistruct.h
    segments.h
    structure_member.h
    )


add_library(r_comp SHARED ${r_comp_SRC})
set_property(TARGET r_comp PROPERTY CXX_STANDARD 11)
set_property(TARGET r_comp PROPERTY CXX_STANDARD_REQUIRED ON)
install(TARGETS r_comp DESTINATION lib)
install(FILES ${r_comp_HDR} DESTINATION include/r_comp)
