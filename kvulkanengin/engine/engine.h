#pragma once

#include "common.h"
#include "scene/scene.h"
#include "render/forwardpass.h"
#include "render/renderdata.h"
#include "render/scenerenderer.h"
#include "resource/resourcemanager.h"
#include "job/kjobsystem.h"

#include "core/context/kcorecontext.h"
#include "core/context/kswapchain.h"
#include "core/commands/kcommandpool.h"
#include "core/commands/kcommandbuffer.h"
#include "core/sync/kfence.h"
#include "core/sync/ksemaphore.h"

struct GLFWwindow;

namespace kEngine
{

struct FrameData
{
	KCommandPool   commandPool;
	KCommandBuffer commandBuffer;
	KSemaphore     imageAvailable;
	KSemaphore     renderFinished;
	KFence         inFlight;
};

class Engine
{
public:
	Engine() = default;
	~Engine();

	Engine(const Engine&) = delete;
	Engine& operator=(const Engine&) = delete;

	bool init(const EngineConfig& config);
	void shutdown();
	void run(std::function<void(float)> updateCallback = nullptr);

	Scene& scene() { return m_scene; }
	ResourceManager& resources() { return m_resources; }

	uint32_t windowWidth() const { return m_config.width; }
	uint32_t windowHeight() const { return m_config.height; }
	float aspectRatio() const { return static_cast<float>(m_config.width) / static_cast<float>(m_config.height); }
	float deltaTime() const { return m_deltaTime; }
	float totalTime() const { return m_totalTime; }

	void setParallelRendering(bool enable) { m_parallelRendering = enable; }
	bool isParallelRendering() const { return m_parallelRendering; }
	KJobSystem& jobSystem() { return m_jobSystem; }

private:
	bool initWindow();
	bool initVulkan();
	bool initFrameData();
	void destroyFrameData();

	bool beginFrame(uint32_t& imageIndex);
	void render(uint32_t imageIndex);
	void endFrame(uint32_t imageIndex);
	void handleResize();

private:
	EngineConfig             m_config;
	GLFWwindow*              m_window = nullptr;
	bool                     m_framebufferResized = false;

	KCoreContext             m_coreContext;
	KSwapchain               m_swapchain;

	static constexpr uint32_t MAX_FRAMES = 2;
	std::vector<FrameData>   m_frames;
	uint32_t                 m_currentFrame = 0;

	Scene                    m_scene;
	ResourceManager          m_resources;
	ForwardPass              m_forwardPass;
	RenderScene              m_renderScene;
	KJobSystem               m_jobSystem;

	float                    m_deltaTime = 0.0f;
	float                    m_totalTime = 0.0f;
	bool                     m_initialized = false;
	bool                     m_parallelRendering = true;
};

} // namespace kEngine
