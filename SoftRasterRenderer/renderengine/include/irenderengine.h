#ifndef __IRENDERENGINE_H__
#define __IRENDERENGINE_H__

#include "ishader.h"

#ifndef IMPORT_IMODEL
#define RENDER_MODULE __declspec(dllexport)
#else
#define RENDER_MODULE __declspec(dllimport)
#endif

interface IObject;

enum class EngineType : int8_t
{
    Rasterization = 0,
    Raytracer,
    HardwareAcceleration // TODO
};

enum class ExecutexType : int8_t
{
    Synchronous = 0,
    Asynchronous
};

interface IRenderEngin
{
	virtual ~IRenderEngin() = default;

    // 视图转换相关
    virtual Matrix lookat(Vec3f cameraPos, Vec3f target, Vec3f up) PURE;
    virtual Matrix ortho(float left, float right, float bottom, float top, float near, float far) PURE;
    virtual Matrix ortho(Vec2f width, Vec2f height, float near, float far) PURE;
    virtual Matrix perspective(float fov, float aspect, float near, float far) PURE;
    virtual Matrix viewport(int width, int height) PURE;

    // 计算切线与副切线
    virtual std::tuple<Vec3f, Vec3f> computeTangentSpace(const Vec3f* pos, const Vec2f* uv) PURE;
    // 设置目标设备
    virtual void setDevice(TGAImage* device) PURE;
};

// 光栅渲染引擎
interface IRasterRenderEngin : public IRenderEngin
{
	// 设置着色器
    virtual void setShader(IShader* pIShader) PURE;

    // 绘制直线
    virtual void drawLine(Vec2i start, Vec2i end, TGAColor color) PURE;
    virtual void drawLine(int x0, int y0, int x1, int y1, TGAColor color) PURE;

    // 扫线算法填充三角形
    virtual void drawTriangle(Vec3f v0, Vec3f v1, Vec3f v2, TGAColor color) PURE;

    // 重心算法填充三角形
    virtual void drawTriangle(const IShader::VertexInput& input) PURE;
};

// 光线追踪渲染引擎
interface IRayTraceRenderEngin : public IRenderEngin
{
    // 设置场景
    virtual IObject* createObj(const Vec3f* pos) PURE;
    virtual void addObj(IObject* obj) PURE;
    virtual void addLight(IObject* pos, const Vec3f& color) PURE;
    virtual void setTexture(const std::string& texName, TGAImage* img) PURE;
    virtual void setTexture(const std::string& texName, TGAImage&& img) PURE;
    virtual void setModeMatrix(const Matrix& mode) PURE;

	// 构建BVH加速结构（默认采用SAH构建），指定叶子节点存储的三角形个数
	virtual void buildBVH(int count) PURE;

    // 设置采样次数
    virtual void setSampleCount(int count) PURE;

    // 设置光线追踪的最大深度
    virtual void setMaxDepth(int depth) PURE;

    // 开始渲染
    virtual void rayGeneration(const ExecutexType type = ExecutexType::Synchronous) PURE;
};

extern "C"
{
    RENDER_MODULE void engineInstance(void** pIRenderEngin, const EngineType mode = EngineType::Rasterization);
    RENDER_MODULE void releaseEngine(IRenderEngin* pIRenderEngin);
}
#endif // !__IRENDERENGINE_H__
