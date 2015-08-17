set(CoreLibrary_SRC
    base.cpp
    inifile.cpp
    sharedlibrary.cpp
    )

set(CoreLibrary_HDR
    base.h
    inifile.h
    sharedlibrary.h
    )

add_library(CoreLibrary ${CoreLibrary_SRC})
set_property(TARGET CoreLibrary PROPERTY CXX_STANDARD 11)
set_property(TARGET CoreLibrary PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(CoreLibrary dl pthread rt)
