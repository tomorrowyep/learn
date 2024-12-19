#include "stdafx.h"
#include "common.h"
#include "scenemanager.h"

namespace
{
	// 按照中心排序 -- 比较函数
	bool cmpx(IObject* left, IObject* right)
	{
		return left->center().x < right->center().x;
	}

	bool cmpy(IObject* left, IObject* right)
	{
		return left->center().y < right->center().y;
	}

	bool cmpz(IObject* left, IObject* right)
	{
		return left->center().z < right->center().z;
	}
}

SceneManager::~SceneManager()
{
	for (auto& obj : m_objects)
	{
		delete obj;
	}
}

void SceneManager::addObj(IObject* obj)
{
	Vec3f* pVertex = obj->vertex();
	_transCoords(pVertex);
	obj->setVertexAttr(VertexAttr::Position, pVertex); // 需要重新计算中心点
	
	m_objects.push_back(obj->clone());
}

void SceneManager::addLight(IObject* obj, const Vec3f& color)
{
	Vec3f* pLightPos = obj->vertex();
	_transCoords(pLightPos);

	m_objects.push_back(obj->clone());
}

void SceneManager::setTexture(const std::string texName, TGAImage* img)
{
	auto item = m_texs.find(texName);
	if (item != m_texs.end())
		item->second.reset(img);
	else
		m_texs.insert(std::make_pair(texName, img));
}

void SceneManager::setTexture(const std::string texName, TGAImage&& img)
{
	auto item = m_texs.find(texName);
	if (item != m_texs.end())
		item->second.reset(new TGAImage(std::move(img)));
	else
		m_texs.insert(std::make_pair(texName, new TGAImage(std::move(img))));
}

void SceneManager::setCameraPos(const Vec3f& pos)
{
	m_cameraPos = pos;
}

Vec3f SceneManager::getCameraPos() const
{
	return m_cameraPos;
}

void SceneManager::setModeMatrix(const Matrix& mode)
{
	m_modeMatrix = mode;
}

void SceneManager::setViewMatrix(const Matrix& view)
{
	m_viewMatrix = view;
}

void SceneManager::setProMatrix(const Matrix& pro)
{
	m_proMatrix = pro;
}

void SceneManager::setViewportMatrix(const Matrix& viewport)
{
	m_viewportMatrix = viewport;
}

Matrix SceneManager::getViewportMatrix()
{
	return m_viewportMatrix;
}

Vec3f SceneManager::getTexture(const std::string& texName, const Vec2f& uv)
{
	// 获取纹理
	TGAColor tgaColor;
	auto itemTex = m_texs.find(texName);
	if (itemTex != m_texs.end())
		tgaColor = itemTex->second->diffuse(uv);

	Vec3f color(tgaColor[2] / 255.f, tgaColor[1] / 255.f, tgaColor[0] / 255.f);
	return color;
}

TGAColor SceneManager::getTGATexture(const std::string& texName, const Vec2f& uv)
{
	// 获取纹理
	TGAColor tgaColor;
	auto itemTex = m_texs.find(texName);
	if (itemTex != m_texs.end())
		tgaColor = itemTex->second->diffuse(uv);

	return tgaColor;
}

Vec3f SceneManager::transform2World(const Vec4f& clipPos)
{
	// 从裁剪坐标到视锥体坐标
	Matrix invProjMatrix = m_proMatrix.invert();
	Vec4f viewPos = invProjMatrix * clipPos;

	// 从视锥体坐标到世界坐标
	Matrix invViewMatrix = m_viewMatrix.invert();
	Vec4f worldPos = invViewMatrix * viewPos;
	return worldPos;
}

void SceneManager::buildBVH(int count)
{
	//m_pBVHRoot = _buildBVH(0, m_objects.size() - 1, count);
	m_cmpFuncs = { cmpx, cmpy, cmpz };
	m_pBVHRoot = _buildBVHBySAH(0, m_objects.size() - 1, count);
}

void SceneManager::setMaxDepth(int depth)
{
	m_maxDepth = depth;
}

HitResult SceneManager::closestHit(const Ray& ray)
{
	HitResult res;
	res.distance = std::numeric_limits<double>::max();

	// 遍历所有图形，求最近交点，这会成为性能瓶颈
	HitResult tmp;
	for (auto& shape : m_objects)
	{
		tmp = shape->intersect(ray);
		if (tmp.isHit && tmp.distance < res.distance)
			res = tmp;
	}

	return res;
}

HitResult SceneManager::closestHitByBVH(const Ray& ray)
{
	return _closestHitByBVH(ray, m_pBVHRoot);
}

Vec3f SceneManager::pathTracing(const Ray& ray, int depth)
{
	if (depth > m_maxDepth)
		return Vec3f();

	HitResult res = closestHitByBVH(ray);
	//HitResult res = closestHit(ray);
	if (!res.isHit)
		return Vec3f();

	// 如果是光源则返回对应颜色
	if (res.material.isEmissive)
		return res.material.color;

	// 有 P 的概率终止，俄罗斯轮盘赌
	double r = RenderEngine::randf();
	float P = 0.8f;
	if (r > P)
		return Vec3f();

	// 生成随机光线
	Ray randomRay;
	randomRay.startPoint = res.hitPoint;
	randomRay.direction = RenderEngine::randomDirection(res.material.normal);

	Vec3f color;
	float cosine = fabs(dot(ray.direction * -1, res.material.normal)); // 光线入射角余弦值，即颜色贡献的权重

	// 根据反射率决定光线最终的方向
	r = RenderEngine::randf();
	if (r < res.material.specularRate)
	{
		// 镜面反射
		Vec3f ref = RenderEngine::reflect(res.material.normal, ray.direction).normalize();
		randomRay.direction = RenderEngine::mix(ref, randomRay.direction, res.material.roughness);
		color = pathTracing(randomRay, depth + 1) * cosine;
	}
	else if (res.material.specularRate <= r && r <= res.material.refractRate)
	{
		// 折射
		Vec3f ref = RenderEngine::refract(ray.direction, res.material.normal, res.material.refractAngle).normalize();
		randomRay.direction = RenderEngine::mix(ref, randomRay.direction * -1, res.material.refractRoughness);
		color = pathTracing(randomRay, depth + 1) * cosine;
	}
	else
	{
		// 漫反射
		Vec3f srcColor = getTexture("diffuse", res.material.texCoords);;
		Vec3f ptColor = pathTracing(randomRay, depth + 1) * cosine;
		color = multiply_elements(ptColor, srcColor);
	}

	return color / P; // 保证总光线强度是一致的
}

HitResult SceneManager::_closestHitByBVH(const Ray& ray, BVHNode* node)
{
	if (!node)
		return HitResult();

	if (node->nums > 0)
		return	_hitTriangleArray(ray, node->index, node->index + node->nums - 1);

	float d1 = -1, d2 = -1;
	if (node->left)
		d1 = _hitAABB(ray, node->left);
	if (node->right)
		d2 = _hitAABB(ray, node->right);

	HitResult res1, res2;
	if (d1 > 0)
		res1 = _closestHitByBVH(ray, node->left);
	if (d2 > 0)
		res2 = _closestHitByBVH(ray, node->right);

	return res1.distance < res2.distance ? res1 : res2;
}

SceneManager::BVHNode* SceneManager::_buildBVH(size_t left, size_t right, size_t limitCount)
{
	// 递归终止条件
	if (left > right)
		return nullptr;

	BVHNode* node = new BVHNode;
	node->AA = Vec3f(std::numeric_limits<float>::max());
	node->BB = Vec3f(std::numeric_limits<float>::lowest());

	// 计算包围盒
	for (size_t i = left; i <= right; ++i)
	{
		node->AA = RenderEngine::min(node->AA, m_objects[i]->getMinPoint());
		node->BB = RenderEngine::max(node->BB, m_objects[i]->getMaxPoint());
	}

	if ((right - left + 1) <= limitCount)
	{
		node->index = left;
		node->nums = right - left + 1;
		return node;
	}

	// 选择最长的轴进行划分
	Vec3f diff = node->BB - node->AA;
	int maxAxis = (diff.x >= diff.y && diff.x >= diff.z) ? 0 : (diff.y >= diff.z ? 1 : 2);
	std::sort(m_objects.begin() + left, m_objects.begin() + right + 1, m_cmpFuncs[maxAxis]);

	size_t mid = (left + right) / 2;
	node->left = _buildBVH(left, mid, limitCount);
	node->right = _buildBVH(mid + 1, right, limitCount);

	return node;
}

SceneManager::BVHNode* SceneManager::_buildBVHBySAH(size_t left, size_t right, size_t limitCount)
{
	// 递归终止条件
	if (left > right)
		return nullptr;

	BVHNode* node = new BVHNode;
	node->AA = Vec3f(std::numeric_limits<float>::max());
	node->BB = Vec3f(std::numeric_limits<float>::lowest());

	// 计算包围盒
	for (size_t i = left; i <= right; ++i)
	{
		node->AA = RenderEngine::min(node->AA, m_objects[i]->getMinPoint());
		node->BB = RenderEngine::max(node->BB, m_objects[i]->getMaxPoint());
	}

	// 叶子节点直接返回
	if ((right - left + 1) <= limitCount)
	{
		node->index = left;
		node->nums = right - left + 1;
		return node;
	}

	// 采取SVH策略选择代价最小的轴进行划分
	float finalCost = std::numeric_limits<float>::max();
	int finalAxis = 0; // 记录最终选择的轴
	int finalSplit = (left + right) / 2; // 记录最终的划分位置

	// 遍历每个轴获取到最小的代价
	for (int axis = 0; axis < 3; ++axis)
	{
		// 按轴排序
		std::sort(m_objects.begin() + left, m_objects.begin() + right + 1, m_cmpFuncs[axis]);

		// 构造左节点前缀，表示left->i的区间的最大值和最小值
		std::vector<Vec3f> preLeftMin(right - left + 1, std::numeric_limits<float>::max());
		std::vector<Vec3f> preLeftMax(right - left + 1, std::numeric_limits<float>::lowest());
		preLeftMin[0] = m_objects[left]->getMinPoint();
		preLeftMax[0] = m_objects[left]->getMaxPoint();
		for (int l = left + 1; l <= (int)right; ++l)
		{
			preLeftMin[l - left] =RenderEngine::min(preLeftMin[l - left - 1], m_objects[l]->getMinPoint());
			preLeftMax[l - left] = RenderEngine::max(preLeftMax[l - left - 1], m_objects[l]->getMaxPoint());
		}

		// 构造右节点前缀，表示right->i区间的最大值和最小值
		std::vector<Vec3f> preRightMin(right - left + 1, std::numeric_limits<float>::max());
		std::vector<Vec3f> preRightMax(right - left + 1, std::numeric_limits<float>::lowest());
		preRightMin[right - left] = m_objects[right]->getMinPoint();
		preRightMax[right - left] = m_objects[right]->getMaxPoint();
		for (int r = right - 1; r >= (int)left; --r)
		{
			preRightMin[r - left] = RenderEngine::min(preRightMin[r - left + 1], m_objects[r]->getMinPoint());
			preRightMax[r - left] = RenderEngine::max(preRightMax[r - left + 1], m_objects[r]->getMaxPoint());
		}

		// 遍历寻找分割
		float cost = std::numeric_limits<float>::max();
		int split = left;
		for (size_t partition = left; partition < right; ++partition)
		{
			// 左边包围盒
			Vec3f leftMin = preLeftMin[partition - left];
			Vec3f leftMax = preLeftMax[partition - left];
			Vec3f lenLeft = leftMax - leftMin;
			float leftSurfaceArea = 2 * (lenLeft.x * lenLeft.y + lenLeft.x * lenLeft.z + lenLeft.y * lenLeft.z);
			float leftCost = leftSurfaceArea * (partition - left + 1);

			// 右边包围盒
			Vec3f rightMin = preRightMin[partition - left + 1];
			Vec3f rightMax = preRightMax[partition - left + 1];
			Vec3f lenRight = rightMax - rightMin;
			float rightSurfaceArea = 2 * (lenRight.x * lenRight.y + lenRight.x * lenRight.z + lenRight.y * lenRight.z);
			float rightCost = rightSurfaceArea * (right - partition);

			// 计算每个轴最小的代价
			float totalCost = leftCost + rightCost;
			if (totalCost < cost)
			{
				cost = totalCost;
				split = partition;
			}
		}

		// 更新最终的最小代价
		if (cost < finalCost)
		{
			finalCost = cost;
			finalAxis = axis;
			finalSplit = split;
		}
	}

	// 根据最终的轴和划分位置进行排序
	assert(finalAxis >= 0 && finalAxis < 3);
	std::sort(m_objects.begin() + left, m_objects.begin() + right + 1, m_cmpFuncs[finalAxis]);
	node->left = _buildBVHBySAH(left, finalSplit, limitCount);
	node->right = _buildBVHBySAH(finalSplit + 1, right, limitCount);

	return node;
}

float SceneManager::_hitAABB(const Ray& ray, const BVHNode* node)
{
	// 计算方向的倒数
	Vec3f invDir(1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z);

	// 计算进入点和出去点在每个轴上的值
	Vec3f in = (node->AA - ray.startPoint) * invDir;
	Vec3f out = (node->BB - ray.startPoint) * invDir;

	// 需要计算一下最大值和最小值，因为可能出现in > out的情况
	Vec3f tMin = RenderEngine::min(in, out);
	Vec3f tMax = RenderEngine::max(in, out);

	float t0 = std::max(tMin.x, std::max(tMin.y, tMin.z));
	float t1 = std::min(tMax.x, std::min(tMax.y, tMax.z));

	return (t1 >= t0) ? ((t0 > 0.0) ? (t0) : (t1)) : (-1);
}

HitResult SceneManager::_hitTriangleArray(const Ray& ray, const int left, const int right)
{
	HitResult res;
	res.distance = std::numeric_limits<double>::max();

	// 遍历节点中的三角形，返回距离最近那一个的交点结果
	HitResult tmp;
	for (int i = left; i <= right; ++i)
	{
		tmp = m_objects[i]->intersect(ray);
		if (tmp.isHit && tmp.distance < res.distance)
			res = tmp;
	}

	return res;
}

void SceneManager::_transCoords(Vec3f* vec)
{
	for (int i = 0; i < 3; ++i)
	{
		Vec4f homovec{ vec[i][0], vec[i][1], vec[i][2], 1 };
		homovec = m_modeMatrix * homovec; // 转换到世界空间

		vec[i] = Vec3f(homovec[0], homovec[1], homovec[2]);
	}
}
