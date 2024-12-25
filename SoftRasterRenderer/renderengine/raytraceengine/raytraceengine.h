#ifndef __RAYTRACEENGINE_H__
#define __RAYTRACEENGINE_H__

#include "irenderengine.h"
#include <memory>

interface IObject;
class SceneManager;

class RayTraceEngine : public IRayTraceRenderEngin
{
public:
	RayTraceEngine();
	~RayTraceEngine() = default;

	// 视图转换相关
	virtual Matrix lookat(Vec3f cameraPos, Vec3f target, Vec3f up) override;
	virtual Matrix ortho(float left, float right, float bottom, float top, float near, float far) override;
	virtual Matrix ortho(Vec2f width, Vec2f height, float near, float far) override;
	virtual Matrix perspective(float fov, float aspect, float near, float far) override;
	virtual Matrix viewport(int width, int height) override;

	// 计算切线与副切线
	virtual std::tuple<Vec3f, Vec3f> computeTangentSpace(const Vec3f* pos, const Vec2f* uv) override;

	virtual void setDevice(TGAImage* device) override;

	// 设置场景
	virtual IObject* createObj(const Vec3f* pos) override;
	virtual void addObj(IObject* obj) override;
	virtual void addLight(IObject* pos, const Vec3f& color) override;
	virtual void setTexture(const std::string& texName, TGAImage* img) override;
	virtual void setTexture(const std::string& texName, TGAImage&& img) override;
	virtual void setModeMatrix(const Matrix& mode) override;

	virtual void buildBVH(int count) override;

	virtual void setSampleCount(int count) override;

	// 设置光线追踪的最大深度
	virtual void setMaxDepth(int depth) override;
	
	// 开始渲染
	virtual void rayGeneration(const ExecutexType type = ExecutexType::Asynchronous) override;

private:
	void _syncRayGeneration();

private:
	std::unique_ptr<SceneManager> m_pSceneManager;
	TGAImage* m_pDevice = nullptr;
	float m_near = 0.1f;
	float m_sampleWeight = 0;
	int m_width = 0;
	int m_height = 0;
	int m_sampleCount = 4096;
};
#endif // !__RAYTRACEENGINE_H__
