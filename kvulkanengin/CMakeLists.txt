# ============================================================================
# KVulkan Engine - CMakeLists.txt
# ============================================================================

cmake_minimum_required(VERSION 3.15)
project(kvulkanengin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 子目录
add_subdirectory(core)
add_subdirectory(engine)

# 链接目录
link_directories(
    ./3rd/glfw/lib-vc2022
    ./3rd/vulkan/Lib
)

# 可执行文件
add_executable(kvulkanengin main.cpp)

target_include_directories(kvulkanengin 
    PUBLIC 
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/3rd/include
        ${CMAKE_SOURCE_DIR}/3rd/include/glfw
        ${CMAKE_SOURCE_DIR}/3rd/include/vulkan
)

target_link_libraries(kvulkanengin 
    vulkan-1 
    glfw3dll 
    kvulkanenginelayer
)

# ========== Shader 编译 ==========
set(GLSLANG_VALIDATOR "${CMAKE_SOURCE_DIR}/3rd/vulkan/Bin/glslangValidator.exe")
set(SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/shaders")
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/assets/shaders")

set(SHADER_SOURCES
    ${SHADER_SOURCE_DIR}/forward.vert
    ${SHADER_SOURCE_DIR}/forward.frag
)

file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

foreach(SHADER_SOURCE ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER_SOURCE} NAME)
    set(SHADER_SPV "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
    
    add_custom_command(
        OUTPUT ${SHADER_SPV}
        COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SOURCE} -o ${SHADER_SPV}
        DEPENDS ${SHADER_SOURCE}
        COMMENT "Compiling shader: ${SHADER_NAME}"
    )
    
    list(APPEND SHADER_SPV_FILES ${SHADER_SPV})
endforeach()

add_custom_target(compile_shaders ALL DEPENDS ${SHADER_SPV_FILES})
add_dependencies(kvulkanengin compile_shaders)

# ========== 资源拷贝 ==========
add_custom_command(TARGET kvulkanengin POST_BUILD
    # DLL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/3rd/vulkan/Bin/vulkan-1.dll
        $<TARGET_FILE_DIR:kvulkanengin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/3rd/glfw/lib-vc2022/glfw3.dll
        $<TARGET_FILE_DIR:kvulkanengin>
    # Shaders
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:kvulkanengin>/assets/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADER_OUTPUT_DIR}
        $<TARGET_FILE_DIR:kvulkanengin>/assets/shaders
    # Models & Textures
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:kvulkanengin>/assets/models
    COMMAND ${CMAKE_COMMAND} -E make_directory
        $<TARGET_FILE_DIR:kvulkanengin>/assets/textures
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/models
        $<TARGET_FILE_DIR:kvulkanengin>/assets/models
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets/textures
        $<TARGET_FILE_DIR:kvulkanengin>/assets/textures
    COMMENT "Copying resources..."
)
