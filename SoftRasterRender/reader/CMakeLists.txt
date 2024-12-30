include_directories(
    .
    ./include
)

set(SOURCE_FILES
    imodel.cpp
    objfilemodel.cpp
)

set(HEADER_FILES
    ./include/imodel.h
    objfilemodel.h
)

add_library(reader SHARED ${SOURCE_FILES} ${HEADER_FILES})

