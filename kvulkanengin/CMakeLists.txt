cmake_minimum_required(VERSION 3.15)
project(kvulkanengin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置运行时库（避免 LNK4098 警告）
# 使用多线程调试 DLL (/MDd) 用于 Debug，多线程 DLL (/MD) 用于 Release
if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(core)
add_subdirectory(logic)

set(SOURCE_FILES
	#App Layer
	main.cpp
)

link_directories(
    ./3rd/glfw/lib-vc2022
    ./3rd/vulkan/Lib
    )

add_executable(kvulkanengin ${SOURCE_FILES})

# 设置包含路径：确保能找到所有头文件
# 注意：代码中使用 "core/context/..." 和 "logic/context/..." 这样的路径，所以需要将项目根目录添加到包含路径
target_include_directories(kvulkanengin 
	PUBLIC 
		${CMAKE_SOURCE_DIR}                    # 项目根目录（用于 "core/context/...", "logic/context/..." 等）
		${CMAKE_SOURCE_DIR}/logic              # logic 目录（备用）
		${CMAKE_SOURCE_DIR}/core               # core 目录（备用）
		${CMAKE_SOURCE_DIR}/core/configs       # core/configs 目录
		${CMAKE_SOURCE_DIR}/3rd/include        # 3rd/include 目录
		${CMAKE_SOURCE_DIR}/3rd/include/glfw   # 3rd/include/glfw 目录
		${CMAKE_SOURCE_DIR}/3rd/include/vulkan # 3rd/include/vulkan 目录
)

target_link_libraries(kvulkanengin vulkan-1 glfw3 kvulkanlogiclayer)

add_custom_command(TARGET kvulkanengin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/3rd/vulkan/Bin/vulkan-1.dll
        $<TARGET_FILE_DIR:kvulkanengin>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/3rd/glfw/lib-vc2022/glfw3.dll
        $<TARGET_FILE_DIR:kvulkanengin>
    COMMENT "Copying dll"
)
