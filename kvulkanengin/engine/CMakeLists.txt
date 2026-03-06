# ============================================================================
# Engine Layer CMakeLists
# ============================================================================

set(ENGINE_SOURCES
    # Scene
    scene/scene.cpp
    
    # Resource
    resource/resourcemanager.cpp
    
    # Job
    job/kjobsystem.cpp
    
    # Graph
    graph/kgraphpass.cpp
    graph/kresourcepool.cpp
    graph/krendergraph.cpp
    graph/kforwardpass.cpp
    
    # Engine
    engine.cpp
)

set(ENGINE_HEADERS
    common.h
    engine.h
    
    # Scene
    scene/components.h
    scene/entity.h
    scene/scene.h
    
    # Resource
    resource/mesh.h
    resource/texture.h
    resource/material.h
    resource/resourcemanager.h
    
    # Render
    render/renderdata.h
    render/scenerenderer.h
    
    # Job
    job/kthreadpool.h
    job/kjobsystem.h
    
    # Graph
    graph/krenderresource.h
    graph/kgraphpass.h
    graph/kresourcepool.h
    graph/krendergraph.h
    graph/kforwardpass.h
)

add_library(kvulkanenginelayer STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(kvulkanenginelayer 
    PUBLIC 
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/core
        ${CMAKE_SOURCE_DIR}/core/configs
        ${CMAKE_SOURCE_DIR}/3rd/include
        ${CMAKE_SOURCE_DIR}/3rd/include/glfw
        ${CMAKE_SOURCE_DIR}/3rd/include/vulkan
)

target_link_libraries(kvulkanenginelayer kvulkancorelayer)
