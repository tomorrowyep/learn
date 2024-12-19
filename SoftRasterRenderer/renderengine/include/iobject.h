#ifndef __IOBJECT_H__
#define __IOBJECT_H__

#include "geometry.h"

#define interface struct
#define PURE =0

// ��������
enum class VertexAttr : int8_t
{
    Position = 0, // ��������
    Normal, // ���㷨��
    TexCoord, // ������������
    Tangent // ��������
};

// ����Ĺ���
struct Ray
{
	Vec3f startPoint; // ���
	Vec3f direction;  // ����
};

// ����
struct Material
{
    bool isEmissive = false;        // �Ƿ񷢹�
    Vec3f normal;                   // ������
    Vec3f color;                    // ��ɫ
    Vec2f texCoords;                    // ��������
    float specularRate = 0.0f;     // �����ռ��
    float roughness = 1.0f;        // �ֲڳ̶�
    float refractRate = 0.0f;      // �����ռ��
    float refractAngle = 1.0f;     // ������
    float refractRoughness = 0.0f; // ����ֲڶ�
};

// �����󽻽��
struct HitResult
{
    bool isHit = false;             // �Ƿ�����
    double distance = 0.0f;         // �뽻��ľ���
    Vec3f hitPoint;                 // �������е�
    Material material;              // ���е�ı������
};

interface IObject
{
    virtual ~IObject() = default;

    // ��¡�����������ɵ����߹���
    virtual IObject* clone() PURE;

    // ��ȡ��������
	virtual Vec3f* vertex() PURE;

	// ����ͼԪ����
    virtual Vec3f center() PURE;

    // �����Χ��ʹ��
    virtual Vec3f getMinPoint() PURE;
    virtual Vec3f getMaxPoint() PURE;

	// ���ö�������
    virtual void setVertexAttr(const VertexAttr type, const Vec3f* pVertexAttr) PURE;
    virtual void setVertexAttr(const VertexAttr type, const Vec2f* pVertexAttr) PURE;

	// ��ȡ����
    virtual HitResult intersect(const Ray& ray) PURE;
};

#endif // !__IOBJECT_H__
