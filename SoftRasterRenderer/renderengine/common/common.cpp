#include "common.h"
#include <random>

namespace RenderEngine
{
	constexpr float PI = 3.1415926f;

	Matrix lookat(Vec3f cameraPos, Vec3f target, Vec3f up)
	{
		Vec3f FZ = (cameraPos - target).normalize();
		Vec3f RX = cross(up, FZ).normalize(); // ���˳���ܸı䣬��ѭ���ַ���
		Vec3f UY = cross(FZ, RX).normalize();

		Matrix rotation;
		Matrix transfer;
		for (int i = 0; i < 3; i++)
		{
			rotation[0][i] = RX[i];
			rotation[1][i] = UY[i];
			rotation[2][i] = FZ[i];
			transfer[i][3] = -cameraPos[i];
		}

		return rotation * transfer;
	}

	Matrix ortho(float left, float right, float bottom, float top, float near, float far)
	{
		// ��Ϊ�������Ƚ�left��righ��Χ�ƶ���ԭ��x - (right + left) / 2
		// �ٽ��ƶ���ԭ��ķ�Χ���Գ��ȱ�ֵ2/right - left
		Matrix proMatrix;
		proMatrix[0][0] = 2.0f / (right - left);
		proMatrix[0][3] = -(right + left) / (right - left);

		proMatrix[1][1] = 2.0f / (top - bottom);
		proMatrix[1][3] = -(top + bottom) / (top - bottom);

		proMatrix[2][2] = -2.0f / (far - near);
		proMatrix[2][3] = -(far + near) / (far - near);

		proMatrix[3][3] = 1.0f;

		return proMatrix;
	}

	Matrix ortho(Vec2f width, Vec2f height, float near, float far)
	{
		float left = width.x, right = width.y;
		float bottom = height.x, top = height.y;
		return ortho(left, right, bottom, top, near, far);
	}

	Matrix perspective(float fov, float aspect, float near, float far)
	{
		// opgl���Ƚ���ͶӰ����ƽ�棬��x = n / -ze * xe, ��Χ��-w/2,w/2
		// ��תΪndc����ϵ-1��1֮�䣬��ͬʱ����2/w
		// Zndc = (Aze  + B) / -ze������-1 = (A*(-n) + B) / n��1 = (A*(-f) + B)
		Matrix proMatrix;
		float fovInRadians = fov * (PI / 180.0f); // �Ƕ�ת����
		float halfHeightDivNear = std::tan(fovInRadians / 2.0f);

		proMatrix[0][0] = 1.0f / (aspect * halfHeightDivNear);
		proMatrix[1][1] = 1.0f / halfHeightDivNear;
		proMatrix[2][2] = -(far + near) / (far - near);
		proMatrix[2][3] = -(2.0f * far * near) / (far - near);
		proMatrix[3][2] = -1.0f;
		proMatrix[3][3] = 0.0f;

		return proMatrix;
	}

	Matrix viewport(int width, int height)
	{
		Matrix viewportMatrix;
		viewportMatrix[0][0] = width / 2.f;
		viewportMatrix[1][1] = height / 2.f;
		viewportMatrix[2][2] = 1 / 2.f;

		viewportMatrix[0][3] = width / 2.f;
		viewportMatrix[1][3] = height / 2.f;
		viewportMatrix[2][3] = 1 / 2.f;

		return viewportMatrix;
	}

	std::tuple<Vec3f, Vec3f> computeTangentSpace(const Vec3f* pos, const Vec2f* uv)
	{
		Vec3f p0 = pos[0], p1 = pos[1], p2 = pos[2];
		Vec2f uv0 = uv[0], uv1 = uv[1], uv2 = uv[2];

		Vec3f edge1 = p1 - p0;
		Vec3f edge2 = p2 - p0;

		Vec2f deltaUV1 = uv1 - uv0;
		Vec2f deltaUV2 = uv2 - uv0;

		// ����ϵ�� f
		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);

		// �������ߣ�Tangent���͸����ߣ�Bitangent��
		Vec3f tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * f;
		Vec3f bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * f;

		return { tangent.normalize(), bitangent.normalize() };
	}

	Vec3f reflect(const Vec3f& N, const Vec3f& L)
	{
		return (N * (N * L)) * 2.0 - L; // ���ݷ��乫ʽ
	}

	Vec3f mix(const Vec3f& a, const Vec3f& b, float t)
	{
		return a * (1 - t) + b * t;
	}

	Vec3f refract(const Vec3f& I, const Vec3f& N, float rate)
	{
		// ������������뷨�ߵĵ��
		float cosI = dot(I, N);

		// ����˹�������ɼ��������ʵı�ֵ (ͨ���� n1/n2)
		float eta = rate;

		// ��������ǵ�ƽ������� k < 0����ʾ������ȫ���䣨ȫ���䣩
		float k = 1.0f - eta * eta * (1.0f - cosI * cosI);
		if (k < 0.0f)
			return Vec3f();

		return I * eta- N * (eta * cosI + sqrt(k));
	}

	float randf()
	{
		static std::mt19937 gen(std::random_device{}());
		static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
		return dis(gen);
	}

	Vec3f randomDirection(const Vec3f& normal)
	{
		Vec3f random;
		do
		{
			random = Vec3f(randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f);
		} while (dot(random, random) >= 1.0f);

		// �ж��Ƿ��ڷ��ߵ�ͬһ��
		if (dot(random, normal) > 0.0f)
			return random;

		return random * -1;
	}

	Vec3f (min)(const Vec3f& left, const Vec3f& right)
	{
		return Vec3f(
			std::min(left.x, right.x),
			std::min(left.y, right.y),
			std::min(left.z, right.z)
		);
	}

	Vec3f (max)(const Vec3f& left, const Vec3f& right)
	{
		return Vec3f(
			std::max(left.x, right.x),
			std::max(left.y, right.y),
			std::max(left.z, right.z)
		);
	}
}
