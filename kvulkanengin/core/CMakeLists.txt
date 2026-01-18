cmake_minimum_required(VERSION 3.15)
project(kvulkancorelayer)

set(SOURCE_FILES
	#context
	./context/kinstance.h
	./context/kinstance.cpp
	./context/kphysicaldevice.h
	./context/kphysicaldevice.cpp
	./context/kdevice.h
	./context/kdevice.cpp
	./context/ksurface.h
	./context/ksurface.cpp
	./context/kswapchain.h
	./context/kswapchain.cpp
	./context/kcorecontext.h
	./context/kcorecontext.cpp

	#objects
	./objects/kbuffer.h
	./objects/kbuffer.cpp
	./objects/kimage.h
	./objects/kimage.cpp
	./objects/kimageview.h
	./objects/kimageview.cpp
	./objects/ksampler.h
	./objects/ksampler.cpp
	./objects/kframebuffer.h
	./objects/kframebuffer.cpp

	#krenderpass
	./renderPass/krenderpass.h
	./renderPass/krenderpass.cpp

	#kpipeline
	./pipeline/kshadermodule.h
	./pipeline/kshadermodule.cpp
	./pipeline/kpipelinelayout.h
	./pipeline/kpipelinelayout.cpp
	./pipeline/kpipeline.h
	./pipeline/kpipeline.cpp

	#commands
	./commands/kcommandbuffer.h
	./commands/kcommandbuffer.cpp
	./commands/kcommandpool.h
	./commands/kcommandpool.cpp

	#descriptor
	./descriptor/kdescriptorsetlayout.h
	./descriptor/kdescriptorsetlayout.cpp
	./descriptor/kdescriptorpool.h
	./descriptor/kdescriptorpool.cpp
	./descriptor/kdescriptorset.h
	./descriptor/kdescriptorset.cpp

	#sync
	./sync/kfence.h
	./sync/kfence.cpp
	./sync/ksemaphore.h
	./sync/ksemaphore.cpp
)

add_library(kvulkancore STATIC ${SOURCE_FILES})

# 设置包含路径：PUBLIC 确保依赖此库的目标也能访问这些路径
# 注意：代码中使用 "core/context/..." 这样的路径，所以需要将项目根目录添加到包含路径
target_include_directories(kvulkancore 
	PUBLIC 
		${CMAKE_SOURCE_DIR}                    # 项目根目录（用于 "core/context/..." 等）
		${CMAKE_CURRENT_SOURCE_DIR}            # core 目录（用于 "context/", "objects/" 等）
		${CMAKE_CURRENT_SOURCE_DIR}/configs    # core/configs 目录
		${CMAKE_SOURCE_DIR}/3rd/include        # 3rd/include 目录
		${CMAKE_SOURCE_DIR}/3rd/include/vulkan # 3rd/include/vulkan 目录
)
