set(CoreLibrary_SRC
    base.cpp
    utils.cpp
    xml_parser.cpp
    )

set(CoreLibrary_HDR
    base.h
    pipe.h
    types.h
    utils.h
    xml_parser.h
    )

add_library(CoreLibrary ${CoreLibrary_SRC})
target_link_libraries(CoreLibrary dl)
install(TARGETS CoreLibrary DESTINATION lib)
install(FILES ${CoreLibrary_HDR} DESTINATION include)
