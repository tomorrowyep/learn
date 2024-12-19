#include "stdafx.h"
#include "common.h"
#include "rasterengine.h"

Matrix RasterEngine::lookat(Vec3f cameraPos, Vec3f target, Vec3f up)
{
	return RenderEngine::lookat(cameraPos, target, up);
}

Matrix RasterEngine::ortho(float left, float right, float bottom, float top, float near, float far)
{
	return RenderEngine::ortho(left, right, bottom, top, near, far);
}

Matrix RasterEngine::ortho(Vec2f width, Vec2f height, float near, float far)
{
	float left = width.x, right = width.y;
	float bottom = height.x, top = height.y;
	return RenderEngine::ortho(left, right, bottom, top, near, far);
}

Matrix RasterEngine::perspective(float fov, float aspect, float near, float far)
{
	return RenderEngine::perspective(fov, aspect, near, far);
}

Matrix RasterEngine::viewport(int width, int height)
{
	return RenderEngine::viewport(width, height);
}

std::tuple<Vec3f, Vec3f> RasterEngine::computeTangentSpace(const Vec3f* pos, const Vec2f* uv)
{
	return RenderEngine::computeTangentSpace(pos, uv);
}

void RasterEngine::setDevice(TGAImage* device)
{
	m_pDevice = device;
	m_width = device->get_width();
	m_height = device->get_height();
	m_zBuffer = std::make_unique<float[]>(m_width * m_height);
	std::fill_n(m_zBuffer.get(), m_width * m_height, 1.f);
}

void RasterEngine::setShader(IShader* pIShader)
{
	m_pIShader = pIShader;
}

void RasterEngine::drawLine(Vec2i start, Vec2i end, TGAColor color)
{
	drawLine(start.x, start.y, end.x, end.y, color);
}

void RasterEngine::drawLine(int x0, int y0, int x1, int y1, TGAColor color)
{
	bool bSteep = std::abs(x0 - x1) < std::abs(y0 - y1);
	// ת�������ص�������һ������
	if (bSteep)
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
	}

	// �����ұ���
	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dx = x1 - x0;
	int dy = y1 - y0;
	int derror2 = std::abs(dy) * 2; //  ���⸡������
	int error2 = 0; // ����ۻ��������Ƿ�ı�y��ֵ
	int y = y0;
	for (int x = x0; x <= x1; ++x)
	{
		if (bSteep)
			m_pDevice->set(y, x, color);
		else
			m_pDevice->set(x, y, color);

		error2 += derror2;
		if (error2 > dx)
		{
			y += y0 > y1 ? -1 : 1;
			error2 -= dx * 2;
		}
	}
}

void RasterEngine::drawTriangle(Vec3f v0, Vec3f v1, Vec3f v2, TGAColor color)
{
	// ��y������
	if (v0.y > v1.y)
		std::swap(v0, v1);
	if (v0.y > v2.y)
		std::swap(v0, v2);
	if (v1.y > v2.y)
		std::swap(v1, v2);

	// �˻��������ж�
	if (v2.y == v0.y) 
		return;

	float height = v2.y - v0.y;

	// ͨ������ɨ��y��ͨ����ֵȷ���������ߵı߽�
	// ��Ϊ����������
	auto drawScanLine = [&](Vec3f start, Vec3f end)
		{
			float segmentHeight = end.y - start.y;
			for (int y = (int)(start.y); y <= (int)end.y; ++y)
			{
				float alpha = (float)(y - v0.y) / height;
				float beta = (float)(y - start.y) / segmentHeight;

				Vec3f boundPoint0 = v0 + (v2 - v0) * alpha;
				Vec3f boundPoint1 = start + (end - start) * beta;

				if ((boundPoint0.x - boundPoint1.x) > 1e-2)
					std::swap(boundPoint0, boundPoint1);

				for (int x = (int)(boundPoint0.x); x <= (int)(boundPoint1.x); ++x)
					m_pDevice->set(x, y, color);
			}
		};

	drawScanLine(v0, v1);
	drawScanLine(v1, v2);
}

void RasterEngine::drawTriangle(const IShader::VertexInput& input)
{
	// ��������shader
	if (!m_pIShader)
		return;

	// ִ�ж�����ɫ��
	IShader::VertexOutput vertexOutput = _executeVertex(input);
	if (!vertexOutput.pPos)
		return;

	Vec3f pVert[3] = { vertexOutput.pPos.value()[0], vertexOutput.pPos.value()[1], vertexOutput.pPos.value()[2] };

	// �����Χ�У������Ϻ�����
	Vec2f  leftTop(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f  rightBottom(0.f, 0.f);
	Vec2f  clamp(m_width - 1.f, m_height - 1.f);

	for (int i = 0; i < 3; ++i)
	{
		leftTop.x = std::max(0.f, std::min(pVert[i].x, leftTop.x));
		leftTop.y = std::max(0.f, std::min(pVert[i].y, leftTop.y));

		rightBottom.x = std::min(clamp.x, std::max(pVert[i].x, rightBottom.x));
		rightBottom.y = std::min(clamp.y, std::max(pVert[i].y, rightBottom.y));
	}

	// ִ�й�դ������Ҫ������������ȷ����Щ�������������ڡ���ֵ�������������
	// �����ڰ�Χ���е����أ��ж��Ƿ�����������
	Vec3f point;
	TGAColor color;
	for (point.x = leftTop.x; point.x <= rightBottom.x; ++point.x)
	{
		for (point.y = leftTop.y; point.y <= rightBottom.y; ++point.y)
		{
			Vec3f barycentric = _getBarycentric(pVert, point);
			if (barycentric.x < 0 || barycentric.y < 0 || barycentric.z < 0)
				continue;

			// ͨ���������ķ���ֵ��ȡ��p�����ֵ
			point.z = pVert[0].z * barycentric[0] + pVert[1].z * barycentric[1] + pVert[2].z * barycentric[2];
			if (point.z < m_zBuffer[(int)(point.x + point.y * m_width)]) // ��ǰ��Ȳ�����
			{
				m_zBuffer[(int)(point.x + point.y * m_width)] = point.z; // ������Ȼ���

				IShader::FragmentInput fragInput;
				fragInput.pPixelVert.emplace(point); // ��ʼ��pPixelVert
				_interpolationAttrs(vertexOutput, fragInput, barycentric); // ��ֵ��������

				if (!m_pIShader->fragment(fragInput, color)) // ִ��Ƭ����ɫ������Ҫ����ȷ�������ص���ɫ
					m_pDevice->set((int)(point.x), (int)(point.y), color); // ֻ�в������������ز������ɫ
			}
		}
	}
}

Vec3f RasterEngine::_getBarycentric(const Vec3f* pVert, const Vec3f point)
{
	Vec3f PA = pVert[0] - point;
	Vec3f AB = pVert[1] - pVert[0];
	Vec3f AC = pVert[2] - pVert[0];

	// ͨ����˻�ȡ��������, ��Ӧ[area, u, v]��area��ʾ���
	Vec3f barycentric = cross(Vec3f(PA.x, AB.x, AC.x), Vec3f(PA.y, AB.y, AC.y));
	if ((barycentric.x - 0.f) < 1e-2)
		return Vec3f(-1, 1, 1); // ����0��ʾ�˻�Ϊֱ�ߣ�Ϊ��Ч��������

	// תΪ1-u-v, u, v�ı�׼��ʽ��������ģʽ
	return Vec3f(1.f - (barycentric.y + barycentric.z) / barycentric.x, barycentric.y / barycentric.x, barycentric.z / barycentric.x);
}

void RasterEngine::_transViewportCoords(Vec4f& vec)
{
	vec = vec / vec[3]; // ͸�ӳ�����ת����NDC
	vec = m_pIShader->m_viewportMatrix * vec; // ת���ӿ�����

	_transAccuracy(vec); // �����¾�������
}

void RasterEngine::_transAccuracy(Vec4f& vec)
{
	vec[0] = std::round(vec[0]);
	vec[1] = std::round(vec[1]);

	vec[0] = std::clamp(vec[0], 0.f, (float)m_width - 1.f);
	vec[1] = std::clamp(vec[1], 0.f, (float)m_height - 1.f);
	vec[2] = std::clamp(vec[2], 0.f, 1.f);
}

IShader::VertexOutput RasterEngine::_executeVertex(const IShader::VertexInput& input)
{
	IShader::VertexOutput vertexOutput = m_pIShader->vertex(input);
	if (vertexOutput.pPos.has_value())
	{
		std::for_each(vertexOutput.pPos.value().begin(), vertexOutput.pPos.value().end(), [this](Vec4f& vec)
			{
				_transViewportCoords(vec); // ִ��͸�ӳ�����ת�����ӿ�����
			});
	}

	return vertexOutput;
}

void RasterEngine::_interpolationAttrs(IShader::VertexOutput& vertexOutput, IShader::FragmentInput& fragInput, const Vec3f& barycentric)
{
	if (vertexOutput.pTexCoord)
	{
		// ��ֵ��ȡ����������
		Vec2f* uv = vertexOutput.pTexCoord.value().data();
		Vec2f pixelUV = uv[0] * barycentric[0] + uv[1] * barycentric[1] + uv[2] * barycentric[2];

		if (!fragInput.pPixeUV)
			fragInput.pPixeUV.emplace(pixelUV);
	}

	if (vertexOutput.pNorm)
	{
		// ��ֵ��ȡ����������
		Vec3f* norm = vertexOutput.pNorm.value().data();
		Vec3f pixelNorm = norm[0] * barycentric[0] + norm[1] * barycentric[1] + norm[2] * barycentric[2];

		if (!fragInput.pPixeNorm)
			fragInput.pPixeNorm.emplace(pixelNorm);
	}

	if (vertexOutput.pTangent)
	{
		// ��ֵ�����븱����
		auto& [tangent, bitangent] = vertexOutput.pTangent.value();
		tangent = tangent * barycentric[0] + tangent * barycentric[1] + tangent * barycentric[2];
		bitangent = bitangent * barycentric[0] + bitangent * barycentric[1] + bitangent * barycentric[2];

		if (!fragInput.pPixelTangent)
			fragInput.pPixelTangent.emplace(tangent, bitangent);
	}
}

