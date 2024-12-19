#ifndef __IOBJECT_H__
#define __IOBJECT_H__

#include "geometry.h"

#define interface struct
#define PURE =0

// 顶点属性
enum class VertexAttr : int8_t
{
    Position = 0, // 顶点坐标
    Normal, // 顶点法线
    TexCoord, // 顶点纹理坐标
    Tangent // 顶点切线
};

// 发射的光线
struct Ray
{
	Vec3f startPoint; // 起点
	Vec3f direction;  // 方向
};

// 材质
struct Material
{
    bool isEmissive = false;        // 是否发光
    Vec3f normal;                   // 法向量
    Vec3f color;                    // 颜色
    Vec2f texCoords;                    // 纹理坐标
    float specularRate = 0.0f;     // 反射光占比
    float roughness = 1.0f;        // 粗糙程度
    float refractRate = 0.0f;      // 折射光占比
    float refractAngle = 1.0f;     // 折射率
    float refractRoughness = 0.0f; // 折射粗糙度
};

// 光线求交结果
struct HitResult
{
    bool isHit = false;             // 是否命中
    double distance = 0.0f;         // 与交点的距离
    Vec3f hitPoint;                 // 光线命中点
    Material material;              // 命中点的表面材质
};

interface IObject
{
    virtual ~IObject() = default;

    // 克隆，生命周期由调用者管理
    virtual IObject* clone() PURE;

    // 获取顶点坐标
	virtual Vec3f* vertex() PURE;

	// 计算图元中心
    virtual Vec3f center() PURE;

    // 计算包围盒使用
    virtual Vec3f getMinPoint() PURE;
    virtual Vec3f getMaxPoint() PURE;

	// 设置顶点属性
    virtual void setVertexAttr(const VertexAttr type, const Vec3f* pVertexAttr) PURE;
    virtual void setVertexAttr(const VertexAttr type, const Vec2f* pVertexAttr) PURE;

	// 获取交点
    virtual HitResult intersect(const Ray& ray) PURE;
};

#endif // !__IOBJECT_H__
