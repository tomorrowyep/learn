#include "triangleobj.h"
#include "triangleobj.h"

constexpr float epsilon = 1e-9f;

TriangleObj::TriangleObj(const Vec3f* pVertex)
{
	m_pVertexs = std::make_unique<Vec3f[]>(3);
	m_pVertexs[0] = pVertex[0];
	m_pVertexs[1] = pVertex[1];
	m_pVertexs[2] = pVertex[2];
}

TriangleObj::TriangleObj(const TriangleObj& other)
{
	m_pVertexs = std::make_unique<Vec3f[]>(3);
	m_pVertexs[0] = other.m_pVertexs[0];
	m_pVertexs[1] = other.m_pVertexs[1];
	m_pVertexs[2] = other.m_pVertexs[2];

	if (other.m_pNormals)
	{
		if (!m_pNormals)
			m_pNormals = std::make_unique<Vec3f[]>(3);

		m_pNormals[0] = other.m_pNormals[0];
		m_pNormals[1] = other.m_pNormals[1];
		m_pNormals[2] = other.m_pNormals[2];
	}

	if (other.m_pTexCoords)
	{
		if (!m_pTexCoords)
			m_pTexCoords = std::make_unique<Vec2f[]>(3);

		m_pTexCoords[0] = other.m_pTexCoords[0];
		m_pTexCoords[1] = other.m_pTexCoords[1];
		m_pTexCoords[2] = other.m_pTexCoords[2];
	}

	m_tangent = other.m_tangent;
	m_material = other.m_material;
}

TriangleObj& TriangleObj::operator=(const TriangleObj& other)
{
	if (this == &other)
		return *this;

	m_pVertexs[0] = other.m_pVertexs[0];
	m_pVertexs[1] = other.m_pVertexs[1];
	m_pVertexs[2] = other.m_pVertexs[2];

	if (other.m_pNormals)
	{
		if (!m_pNormals)
			m_pNormals = std::make_unique<Vec3f[]>(3);

		m_pNormals[0] = other.m_pNormals[0];
		m_pNormals[1] = other.m_pNormals[1];
		m_pNormals[2] = other.m_pNormals[2];
	}

	if (other.m_pTexCoords)
	{
		if (!m_pTexCoords)
			m_pTexCoords = std::make_unique<Vec2f[]>(3);

		m_pTexCoords = std::make_unique<Vec2f[]>(3);
		m_pTexCoords[0] = other.m_pTexCoords[0];
		m_pTexCoords[1] = other.m_pTexCoords[1];
		m_pTexCoords[2] = other.m_pTexCoords[2];
	}

	m_tangent = other.m_tangent;
	m_material = other.m_material;

	return *this;
}

Vec3f* TriangleObj::vertex()
{
	return m_pVertexs.get();
}

Vec3f TriangleObj::center()
{
	return (m_pVertexs[0] + m_pVertexs[1] + m_pVertexs[2]) / 3.f;
}

Vec3f TriangleObj::getMinPoint()
{
	return Vec3f(
		std::min(m_pVertexs[0].x, std::min(m_pVertexs[1].x, m_pVertexs[2].x)),
		std::min(m_pVertexs[0].y, std::min(m_pVertexs[1].y, m_pVertexs[2].y)),
		std::min(m_pVertexs[0].z, std::min(m_pVertexs[1].z, m_pVertexs[2].z))
	);
}

Vec3f TriangleObj::getMaxPoint()
{
	return Vec3f(
		std::max(m_pVertexs[0].x, std::max(m_pVertexs[1].x, m_pVertexs[2].x)),
		std::max(m_pVertexs[0].y, std::max(m_pVertexs[1].y, m_pVertexs[2].y)),
		std::max(m_pVertexs[0].z, std::max(m_pVertexs[1].z, m_pVertexs[2].z))
	);
}

void TriangleObj::setVertexAttr(const VertexAttr type, const Vec3f* pVertexAttr)
{
	switch (type)
	{
	case VertexAttr::Position:
	{
		if (!m_pVertexs)
			m_pVertexs = std::make_unique<Vec3f[]>(3);

		if (m_pVertexs.get() != pVertexAttr)
		{
			m_pVertexs[0] = pVertexAttr[0];
			m_pVertexs[1] = pVertexAttr[1];
			m_pVertexs[2] = pVertexAttr[2];
		}
		break;
	}
	case VertexAttr::Normal:
	{
		if (!m_pNormals)
			m_pNormals = std::make_unique<Vec3f[]>(3);

		m_pNormals[0] = pVertexAttr[0];
		m_pNormals[1] = pVertexAttr[1];
		m_pNormals[2] = pVertexAttr[2];
		break;
	}
	case VertexAttr::Tangent:
	{
		m_tangent = std::make_tuple(pVertexAttr[0], pVertexAttr[1]);
		break;
	}
	default:
		break;
	}
}

void TriangleObj::setVertexAttr(const VertexAttr type, const Vec2f* pVertexAttr)
{
	if (!m_pTexCoords)
		m_pTexCoords = std::make_unique<Vec2f[]>(3);

	m_pTexCoords[0] = pVertexAttr[0];
	m_pTexCoords[1] = pVertexAttr[1];
	m_pTexCoords[2] = pVertexAttr[2];
}

HitResult TriangleObj::intersect(const Ray& ray)
{
	HitResult res;

	Vec3f rayStart = ray.startPoint;
	Vec3f rayDirection = ray.direction;
	Vec3f normal = cross(m_pVertexs[1] - m_pVertexs[0], m_pVertexs[2] - m_pVertexs[0]).normalize();

	// 调整法向量
	if (dot(normal, rayDirection) > 0.0f)
		normal = normal * -1;

	// 如果视线和三角形平行，即与法线垂直则不计算
	if (fabs(dot(normal, rayDirection)) < epsilon)
		return res;

	// 获取距离并判断方向，如果在背面则不计算
	float t = (dot(normal, m_pVertexs[0]) - dot(rayStart, normal)) / dot(rayDirection, normal);
	if (t < epsilon)
		return res;

	Vec3f hitPoint = rayStart + rayDirection * t; // 求交点

	// 判断是否在三角形内
	Vec3f barycentric = _getBarycentric(hitPoint);
	if (barycentric.x < 0 || barycentric.y < 0 || barycentric.z < 0)
		return res;

	// 插值得到法线
	/*normal = (m_pNormals[0] * barycentric.x + m_pNormals[1] * barycentric.y + m_pNormals[2] * barycentric.z).normalize();
	if (dot(normal, rayDirection) > 0.0f)
		normal = normal * -1;*/

	// 插值获取纹理坐标
	Vec2f texCoords = (m_pTexCoords[0] * barycentric.x + m_pTexCoords[1] * barycentric.y + m_pTexCoords[2] * barycentric.z);

	res.isHit = true;
	res.distance = t;
	res.hitPoint = hitPoint;
	res.material.normal = normal;
	res.material.texCoords = texCoords;
	res.material.isEmissive = m_material.isEmissive;
	res.material.color = m_material.color;

	assert(res.material.isEmissive == false);

	return res;
}

Vec3f TriangleObj::_getBarycentric(const Vec3f& point)
{
	Vec3f PA = m_pVertexs[0] - point;
	Vec3f AB = m_pVertexs[1] - m_pVertexs[0];
	Vec3f AC = m_pVertexs[2] - m_pVertexs[0];

	// 通过叉乘获取重心坐标, 对应[area, u, v]，表示面积
	Vec3f barycentric = cross(Vec3f(PA.x, AB.x, AC.x), Vec3f(PA.y, AB.y, AC.y));
	if ((barycentric.x - 0.f) < epsilon)
		return Vec3f(-1, 1, 1); // 等于0表示退化为直线，为无效的三角形

	// 转为1-u-v, u, v的标准格式，即比例模式
	return Vec3f(1.f - (barycentric.y + barycentric.z) / barycentric.x, barycentric.y / barycentric.x, barycentric.z / barycentric.x);
}
