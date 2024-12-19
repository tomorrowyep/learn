#ifndef __TRIANGLEOBJ_H__
#define __TRIANGLEOBJ_H__

#include "iobject.h"

#ifndef IMPORT_IMODEL
#define RENDER_MODULE __declspec(dllexport)
#else
#define RENDER_MODULE __declspec(dllimport)
#endif

class RENDER_MODULE TriangleObj : public IObject
{
public:
	TriangleObj() = delete; // ��֧��Ĭ�Ϲ���

    TriangleObj(const Vec3f* pVertex);
    TriangleObj(const TriangleObj& other);
    TriangleObj& operator=(const TriangleObj& other);

    IObject* clone() override;

    Vec3f* vertex() override;

    Vec3f center() override;

    // �����Χ��ʹ��
    Vec3f getMinPoint() override;
    Vec3f getMaxPoint() override;

    void setVertexAttr(const VertexAttr type, const Vec3f* pVertexAttr) override;
    void setVertexAttr(const VertexAttr type, const Vec2f* pVertexAttr) override;

    virtual HitResult intersect(const Ray& ray) override;

	Material& getMaterial() { return m_material; }

private:
    Vec3f _getBarycentric(const Vec3f& point);

private:
    std::unique_ptr<Vec3f[]> m_pVertexs; // �����ε���������
    std::unique_ptr<Vec3f[]> m_pNormals; // �����θ�����ķ���
    std::unique_ptr<Vec2f[]> m_pTexCoords; // �����θ�����ķ���
    std::tuple<Vec3f, Vec3f> m_tangent; // �����ε������븱����

    Vec3f m_center; // �����ε����ĵ�
	Material m_material; // �����εĲ���
};

#endif // !__TRIANGLEOBJ_H__