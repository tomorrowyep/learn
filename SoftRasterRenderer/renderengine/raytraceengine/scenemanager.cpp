#include "stdafx.h"
#include "common.h"
#include "scenemanager.h"

namespace
{
	// ������������ -- �ȽϺ���
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
	obj->setVertexAttr(VertexAttr::Position, pVertex); // ��Ҫ���¼������ĵ�
	
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
	// ��ȡ����
	TGAColor tgaColor;
	auto itemTex = m_texs.find(texName);
	if (itemTex != m_texs.end())
		tgaColor = itemTex->second->diffuse(uv);

	Vec3f color(tgaColor[2] / 255.f, tgaColor[1] / 255.f, tgaColor[0] / 255.f);
	return color;
}

TGAColor SceneManager::getTGATexture(const std::string& texName, const Vec2f& uv)
{
	// ��ȡ����
	TGAColor tgaColor;
	auto itemTex = m_texs.find(texName);
	if (itemTex != m_texs.end())
		tgaColor = itemTex->second->diffuse(uv);

	return tgaColor;
}

Vec3f SceneManager::transform2World(const Vec4f& clipPos)
{
	// �Ӳü����굽��׶������
	Matrix invProjMatrix = m_proMatrix.invert();
	Vec4f viewPos = invProjMatrix * clipPos;

	// ����׶�����굽��������
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

	// ��������ͼ�Σ���������㣬����Ϊ����ƿ��
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

	// ����ǹ�Դ�򷵻ض�Ӧ��ɫ
	if (res.material.isEmissive)
		return res.material.color;

	// �� P �ĸ�����ֹ������˹���̶�
	double r = RenderEngine::randf();
	float P = 0.8f;
	if (r > P)
		return Vec3f();

	// �����������
	Ray randomRay;
	randomRay.startPoint = res.hitPoint;
	randomRay.direction = RenderEngine::randomDirection(res.material.normal);

	Vec3f color;
	float cosine = fabs(dot(ray.direction * -1, res.material.normal)); // �������������ֵ������ɫ���׵�Ȩ��

	// ���ݷ����ʾ����������յķ���
	r = RenderEngine::randf();
	if (r < res.material.specularRate)
	{
		// ���淴��
		Vec3f ref = RenderEngine::reflect(res.material.normal, ray.direction).normalize();
		randomRay.direction = RenderEngine::mix(ref, randomRay.direction, res.material.roughness);
		color = pathTracing(randomRay, depth + 1) * cosine;
	}
	else if (res.material.specularRate <= r && r <= res.material.refractRate)
	{
		// ����
		Vec3f ref = RenderEngine::refract(ray.direction, res.material.normal, res.material.refractAngle).normalize();
		randomRay.direction = RenderEngine::mix(ref, randomRay.direction * -1, res.material.refractRoughness);
		color = pathTracing(randomRay, depth + 1) * cosine;
	}
	else
	{
		// ������
		Vec3f srcColor = getTexture("diffuse", res.material.texCoords);;
		Vec3f ptColor = pathTracing(randomRay, depth + 1) * cosine;
		color = multiply_elements(ptColor, srcColor);
	}

	return color / P; // ��֤�ܹ���ǿ����һ�µ�
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
	// �ݹ���ֹ����
	if (left > right)
		return nullptr;

	BVHNode* node = new BVHNode;
	node->AA = Vec3f(std::numeric_limits<float>::max());
	node->BB = Vec3f(std::numeric_limits<float>::lowest());

	// �����Χ��
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

	// ѡ���������л���
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
	// �ݹ���ֹ����
	if (left > right)
		return nullptr;

	BVHNode* node = new BVHNode;
	node->AA = Vec3f(std::numeric_limits<float>::max());
	node->BB = Vec3f(std::numeric_limits<float>::lowest());

	// �����Χ��
	for (size_t i = left; i <= right; ++i)
	{
		node->AA = RenderEngine::min(node->AA, m_objects[i]->getMinPoint());
		node->BB = RenderEngine::max(node->BB, m_objects[i]->getMaxPoint());
	}

	// Ҷ�ӽڵ�ֱ�ӷ���
	if ((right - left + 1) <= limitCount)
	{
		node->index = left;
		node->nums = right - left + 1;
		return node;
	}

	// ��ȡSVH����ѡ�������С������л���
	float finalCost = std::numeric_limits<float>::max();
	int finalAxis = 0; // ��¼����ѡ�����
	int finalSplit = (left + right) / 2; // ��¼���յĻ���λ��

	// ����ÿ�����ȡ����С�Ĵ���
	for (int axis = 0; axis < 3; ++axis)
	{
		// ��������
		std::sort(m_objects.begin() + left, m_objects.begin() + right + 1, m_cmpFuncs[axis]);

		// ������ڵ�ǰ׺����ʾleft->i����������ֵ����Сֵ
		std::vector<Vec3f> preLeftMin(right - left + 1, std::numeric_limits<float>::max());
		std::vector<Vec3f> preLeftMax(right - left + 1, std::numeric_limits<float>::lowest());
		preLeftMin[0] = m_objects[left]->getMinPoint();
		preLeftMax[0] = m_objects[left]->getMaxPoint();
		for (int l = left + 1; l <= (int)right; ++l)
		{
			preLeftMin[l - left] =RenderEngine::min(preLeftMin[l - left - 1], m_objects[l]->getMinPoint());
			preLeftMax[l - left] = RenderEngine::max(preLeftMax[l - left - 1], m_objects[l]->getMaxPoint());
		}

		// �����ҽڵ�ǰ׺����ʾright->i��������ֵ����Сֵ
		std::vector<Vec3f> preRightMin(right - left + 1, std::numeric_limits<float>::max());
		std::vector<Vec3f> preRightMax(right - left + 1, std::numeric_limits<float>::lowest());
		preRightMin[right - left] = m_objects[right]->getMinPoint();
		preRightMax[right - left] = m_objects[right]->getMaxPoint();
		for (int r = right - 1; r >= (int)left; --r)
		{
			preRightMin[r - left] = RenderEngine::min(preRightMin[r - left + 1], m_objects[r]->getMinPoint());
			preRightMax[r - left] = RenderEngine::max(preRightMax[r - left + 1], m_objects[r]->getMaxPoint());
		}

		// ����Ѱ�ҷָ�
		float cost = std::numeric_limits<float>::max();
		int split = left;
		for (size_t partition = left; partition < right; ++partition)
		{
			// ��߰�Χ��
			Vec3f leftMin = preLeftMin[partition - left];
			Vec3f leftMax = preLeftMax[partition - left];
			Vec3f lenLeft = leftMax - leftMin;
			float leftSurfaceArea = 2 * (lenLeft.x * lenLeft.y + lenLeft.x * lenLeft.z + lenLeft.y * lenLeft.z);
			float leftCost = leftSurfaceArea * (partition - left + 1);

			// �ұ߰�Χ��
			Vec3f rightMin = preRightMin[partition - left + 1];
			Vec3f rightMax = preRightMax[partition - left + 1];
			Vec3f lenRight = rightMax - rightMin;
			float rightSurfaceArea = 2 * (lenRight.x * lenRight.y + lenRight.x * lenRight.z + lenRight.y * lenRight.z);
			float rightCost = rightSurfaceArea * (right - partition);

			// ����ÿ������С�Ĵ���
			float totalCost = leftCost + rightCost;
			if (totalCost < cost)
			{
				cost = totalCost;
				split = partition;
			}
		}

		// �������յ���С����
		if (cost < finalCost)
		{
			finalCost = cost;
			finalAxis = axis;
			finalSplit = split;
		}
	}

	// �������յ���ͻ���λ�ý�������
	assert(finalAxis >= 0 && finalAxis < 3);
	std::sort(m_objects.begin() + left, m_objects.begin() + right + 1, m_cmpFuncs[finalAxis]);
	node->left = _buildBVHBySAH(left, finalSplit, limitCount);
	node->right = _buildBVHBySAH(finalSplit + 1, right, limitCount);

	return node;
}

float SceneManager::_hitAABB(const Ray& ray, const BVHNode* node)
{
	// ���㷽��ĵ���
	Vec3f invDir(1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z);

	// ��������ͳ�ȥ����ÿ�����ϵ�ֵ
	Vec3f in = (node->AA - ray.startPoint) * invDir;
	Vec3f out = (node->BB - ray.startPoint) * invDir;

	// ��Ҫ����һ�����ֵ����Сֵ����Ϊ���ܳ���in > out�����
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

	// �����ڵ��е������Σ����ؾ��������һ���Ľ�����
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
		homovec = m_modeMatrix * homovec; // ת��������ռ�

		vec[i] = Vec3f(homovec[0], homovec[1], homovec[2]);
	}
}
