cmake_minimum_required(VERSION 3.15)
project(kvulkanlogiclayer)

set(SOURCE_FILES
	#logic-context
	./context/klogiccontext.h
	./context/klogiccontext.cpp

	#logic-frame
	./frame/kframeresources.h
	./frame/kframeresources.cpp

	#logic-render
	./renderer/include/irenderer.h
	./renderer/forwardrenderer/kforwardrenderer.h
	./renderer/forwardrenderer/kforwardrenderer.cpp

	#logic-platform
	./platform/include/ikwindow.h
	./platform/glfw/glfwwindow.h
	./platform/glfw/glfwwindow.cpp
)

add_library(kvulkanlogiclayer STATIC ${SOURCE_FILES})

# 链接 core 库（PUBLIC 确保依赖此库的目标也能访问 core 的包含路径）
target_link_libraries(kvulkanlogiclayer PUBLIC kvulkancore)

# 设置包含路径：PUBLIC 确保依赖此库的目标也能访问这些路径
# 注意：代码中使用 "core/context/..." 这样的路径，所以需要将项目根目录添加到包含路径
target_include_directories(kvulkanlogiclayer 
	PUBLIC 
		${CMAKE_CURRENT_SOURCE_DIR}            # logic 目录（用于 "frame/", "renderer/" 等）
		${CMAKE_SOURCE_DIR}                    # 项目根目录（用于 "core/context/..." 等）
		${CMAKE_SOURCE_DIR}/core               # core 目录（备用）
		${CMAKE_SOURCE_DIR}/core/configs       # core/configs 目录
		${CMAKE_SOURCE_DIR}/3rd/include        # 3rd/include 目录
		${CMAKE_SOURCE_DIR}/3rd/include/glm     # 3rd/include/glm 目录
		${CMAKE_SOURCE_DIR}/3rd/include/glfw   # 3rd/include/glfw 目录
		${CMAKE_SOURCE_DIR}/3rd/include/vulkan  # 3rd/include/vulkan 目录
)
