include_directories(
    .
    ./include
    ./common
    ./rasterengine
    ./raytraceengine
)

set(SOURCE_FILES
    ./common/common.cpp

    ./irenderengine.cpp
    ./rasterengine/rasterengine.cpp

    ./raytraceengine/triangleobj.cpp
    ./raytraceengine/scenemanager.cpp
    ./raytraceengine/raytraceengine.cpp
)

set(HEADER_FILES
    ./common/common.h

    ./include/irenderengine.h
    ./rasterengine/rasterengine.h

    ./include/triangleobj.h
    ./raytraceengine/scenemanager.h
    ./raytraceengine/raytraceengine.h
)

add_library(renderengine SHARED ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(renderengine PUBLIC tgaimage)

