#ifndef __TRIANGLEOBJ_H__
#define __TRIANGLEOBJ_H__

#include "iobject.h"

class TriangleObj : public IObject
{
public:
	TriangleObj() = delete; // 不支持默认构造

    TriangleObj(const Vec3f* pVertex);
    TriangleObj(const TriangleObj& other);
    TriangleObj& operator=(const TriangleObj& other);

    Vec3f* vertex() override;

    Vec3f center() override;

    // 计算包围盒使用
    Vec3f getMinPoint() override;
    Vec3f getMaxPoint() override;

    void setVertexAttr(const VertexAttr type, const Vec3f* pVertexAttr) override;
    void setVertexAttr(const VertexAttr type, const Vec2f* pVertexAttr) override;

    virtual HitResult intersect(const Ray& ray) override;

    Material& getMaterial() override { return m_material; }

private:
    Vec3f _getBarycentric(const Vec3f& point);

private:
    std::unique_ptr<Vec3f[]> m_pVertexs; // 三角形的三个顶点
    std::unique_ptr<Vec3f[]> m_pNormals; // 三角形各顶点的法线
    std::unique_ptr<Vec2f[]> m_pTexCoords; // 三角形各顶点的法线
    std::tuple<Vec3f, Vec3f> m_tangent; // 三角形的切线与副切线

    Material m_material; // 三角形的材质
};

#endif // !__TRIANGLEOBJ_H__