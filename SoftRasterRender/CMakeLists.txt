cmake_minimum_required(VERSION 3.10)

project(SoftRender)
set(CMAKE_CXX_STANDARD  17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_SOURCE_DIR}/utility
   ${CMAKE_CURRENT_SOURCE_DIR}/tgaimage/include
)

add_subdirectory(tgaimage)
add_subdirectory(reader)
add_subdirectory(renderengine)

#添加源文件
file(GLOB SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(render ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(render PRIVATE ./reader/inlude)
target_include_directories(render PRIVATE ./renderengine/inlude)
target_compile_definitions(render PRIVATE IMPORT_IMODEL)
target_link_libraries(render PRIVATE reader renderengine)
