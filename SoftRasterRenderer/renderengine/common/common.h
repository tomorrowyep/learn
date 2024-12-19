#ifndef __COMMON_H__
#define __COMMON_H__

#include "geometry.h"
#include <algorithm>
#include <cmath>
#include <tuple>

namespace RenderEngine
{
	/*
	* cameraPos�������λ��
	* target������ķ���
	* up����ʶ��������Ĵ�ֱ����
	* �����ӽǾ���
	*/
	Matrix lookat(Vec3f cameraPos, Vec3f target, Vec3f up);
	/*
	* ����ͶӰ
	* left�������x����ߵ�λ��
	* right�������x���ұߵ�λ��
	* bottom���߶���y���±ߵ�λ��
	* top���߶���y���ϱߵ�λ��
	* near����ƽ��ľ��룬һ����˵>=0
	* far��Զƽ��ľ���,һ����˵>=0
	* ���زü�����
	*/
	Matrix ortho(float left, float right, float bottom, float top, float near, float far);
	/*
	* ����ͶӰ
	* width����left��right��
	* height����bottom�� top��
	* near����ƽ��ľ��룬һ����˵>=0
	* far��Զƽ��ľ���,һ����˵>=0
	* ���زü�����
	*/
	Matrix ortho(Vec2f width, Vec2f height, float near, float far);
	/*
	* ͸��ͶӰ
	* fov����׶��������ӳ��ǣ������˽�ƽ���ϵĸ߶ȣ���λ�Ƕ�
	* aspect����߱����������˽�ƽ���ϵĿ��
	* near����ƽ��ľ��룬һ����˵>=0
	* far��Զƽ��ľ���,һ����˵>=0
	* ���زü�����
	*/
	Matrix perspective(float fov, float aspect, float near, float far);
	/*
	* ������ʾ���ӿڴ�С
	* �����ӿھ���
	*/
	Matrix viewport(int width, int height);

	/*
	* �������ܣ�����������ƽ������߿ռ�
	* pos�������ε���������λ������
	* uv�����������Ӧ����������
	* ret���������ߡ�������
	*/
	std::tuple<Vec3f, Vec3f> computeTangentSpace(const Vec3f* pos, const Vec2f* uv);

	/*
   * N��ʾ���ߵķ���
   * L��ʾ�����
   * ���ط����ķ���
   */
	Vec3f reflect(const Vec3f& N, const Vec3f& L);

	/* ���Բ�ֵһ������
   * a��b��ʾԴ������a��Ӧ�Ĳ�ֵ����δ1-t��b��Ӧ�Ĳ�ֵ����Ϊt
   * t��ʾ��ֵ���ӣ�0-1��
   * ���ز�ֵ�������
   */
	Vec3f mix(const Vec3f& a, const Vec3f& b, float t);

	/* �����ķ���
   * I��ʾ�����
   * N��ʾ���ߵķ���
   * rate��ʾ������
   * ���������ķ���
   */
	Vec3f refract(const Vec3f& I, const Vec3f& N, float rate);

	/* ����һ��0-1.f�������
   * ���������
   */
	float randf();

	/* ���ط��߰����������
	* normal��ʾ���ߵķ���
   * �����������
   */
	Vec3f randomDirection(const Vec3f& normal);

	/* ���ظ�������С��ֵ������ֵ
   * left���Ƚϵ�����֮һ
   * right���Ƚϵ�����֮һ
   */
	Vec3f (min)(const Vec3f& left, const Vec3f& right);
	Vec3f (max)(const Vec3f& left, const Vec3f& right);
}
#endif // !__COMMON_H__


