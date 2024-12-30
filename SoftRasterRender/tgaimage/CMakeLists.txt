include_directories(
    .
    ./include
)

set(SOURCE_FILES
    tgaimage.cpp
)

set(HEADER_FILES
    ./include/tgaimage.h
)

add_library(tgaimage STATIC ${SOURCE_FILES} ${HEADER_FILES})

