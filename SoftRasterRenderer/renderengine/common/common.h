#ifndef __COMMON_H__
#define __COMMON_H__

#include "geometry.h"
#include <algorithm>
#include <cmath>
#include <tuple>

namespace RenderEngine
{
	/*
	* cameraPos：相机的位置
	* target：拍摄的方向
	* up：标识相机所处的垂直方向
	* 返回视角矩阵
	*/
	Matrix lookat(Vec3f cameraPos, Vec3f target, Vec3f up);
	/*
	* 正交投影
	* left：宽度在x轴左边的位置
	* right：宽度在x轴右边的位置
	* bottom：高度在y轴下边的位置
	* top：高度在y轴上边的位置
	* near：近平面的距离，一般来说>=0
	* far：远平面的距离,一般来说>=0
	* 返回裁剪矩阵
	*/
	Matrix ortho(float left, float right, float bottom, float top, float near, float far);
	/*
	* 正交投影
	* width：（left，right）
	* height：（bottom， top）
	* near：近平面的距离，一般来说>=0
	* far：远平面的距离,一般来说>=0
	* 返回裁剪矩阵
	*/
	Matrix ortho(Vec2f width, Vec2f height, float near, float far);
	/*
	* 透视投影
	* fov：视锥体的纵向视场角，决定了近平面上的高度，单位角度
	* aspect：宽高比例，决定了近平面上的宽度
	* near：近平面的距离，一般来说>=0
	* far：远平面的距离,一般来说>=0
	* 返回裁剪矩阵
	*/
	Matrix perspective(float fov, float aspect, float near, float far);
	/*
	* 设置显示的视口大小
	* 返回视口矩阵
	*/
	Matrix viewport(int width, int height);

	/*
	* 函数功能，计算三角形平面的切线空间
	* pos：三角形的三个顶点位置坐标
	* uv：三个顶点对应的纹理坐标
	* ret：返回切线、副切线
	*/
	std::tuple<Vec3f, Vec3f> computeTangentSpace(const Vec3f* pos, const Vec2f* uv);

	/*
   * N表示法线的方向
   * L表示入射光
   * 返回反射光的方向
   */
	Vec3f reflect(const Vec3f& N, const Vec3f& L);

	/* 线性插值一个向量
   * a、b表示源向量，a对应的插值因子未1-t，b对应的插值因子为t
   * t表示插值因子（0-1）
   * 返回插值后的向量
   */
	Vec3f mix(const Vec3f& a, const Vec3f& b, float t);

	/* 折射光的方向
   * I表示入射光
   * N表示法线的方向
   * rate表示折射率
   * 返回折射光的方向
   */
	Vec3f refract(const Vec3f& I, const Vec3f& N, float rate);

	/* 生成一个0-1.f的随机数
   * 返回随机数
   */
	float randf();

	/* 返回法线半球随机方向
	* normal表示法线的方向
   * 返回随机方向
   */
	Vec3f randomDirection(const Vec3f& normal);

	/* 返回各分量最小的值、最大的值
   * left：比较的向量之一
   * right：比较的向量之一
   */
	Vec3f (min)(const Vec3f& left, const Vec3f& right);
	Vec3f (max)(const Vec3f& left, const Vec3f& right);
}
#endif // !__COMMON_H__


