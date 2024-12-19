#ifndef __SCENEMANAGER__H__
#define __SCENEMANAGER__H__

#include <vector>

#include "iobject.h"
#include "tgaimage.h"

class SceneManager
{
	using TextureContainer = std::unordered_map<std::string, std::unique_ptr<TGAImage>>;
public:
	SceneManager() = default;
	~SceneManager();

	void addObj(IObject* obj);
	void addLight(IObject* pos, const Vec3f& color);
	void setTexture(const std::string texName, TGAImage* img);
	void setTexture(const std::string texName, TGAImage&& img);

	void setCameraPos(const Vec3f& pos);
	Vec3f getCameraPos() const;

	void setModeMatrix(const Matrix& mode);
	void setViewMatrix(const Matrix& view);
	void setProMatrix(const Matrix& pro);
	void setViewportMatrix(const Matrix& viewport);

	Matrix getViewportMatrix();

	Vec3f getTexture(const std::string& texName, const Vec2f& uv);
	TGAColor getTGATexture(const std::string& texName, const Vec2f& uv);

	Vec3f transform2World(const Vec4f& clipPos);

	void buildBVH(int count);

	void setMaxDepth(int depth);

	// ������⣬�����ÿ��ͼԪ
	HitResult closestHit(const Ray& ray);
	
	// ����BVH���ٽṹ��
	HitResult closestHitByBVH(const Ray& ray);

	Vec3f pathTracing(const Ray& ray, int depth);

private:
	struct BVHNode
	{
		BVHNode* left = nullptr;
		BVHNode* right = nullptr;

		int index = -1; // ��ǰ�ڵ�����Ķ���ʼ����
		int nums = 0;  // ��ǰ�ڵ�����Ķ���������nums��Ϊ0ʱ��ʾҶ�ӽڵ�

		Vec3f AA; // ��Χ�У�����
		Vec3f BB; // ����
	};

	HitResult _closestHitByBVH(const Ray& ray, BVHNode* node);

	// ����BVH��
	BVHNode* _buildBVH(size_t left, size_t right, size_t limitCount = 8);

	/*SVH����BVH��
	* ��������ӵ� n1 ����������Ҫ���� T * n1 ��ʱ�䣨���� t Ϊ������
	* �����Һ��ӵ� n2 ����������Ҫ���� T * n2 ��ʱ��
	* ��������� p1 �ĸ��ʻ�������ӣ��� p2 �ĸ��ʻ����Һ��ӣ������ñ�������棬�������
	* ���յĴ���Ϊ cost = p1 * n1 + p2 * n2 ������ʡ���˳��� T��
	*/
	BVHNode* _buildBVHBySAH(size_t left, size_t right, size_t limitCount = 8);

	// ������������ֵ���ֱ��ʾδ�ཻ(-1)���ཻ(t0)���ཻ���ڰ�Χ����(t1)
	float _hitAABB(const Ray& ray, const BVHNode* node);

	// ���������ཻ�ľ������������
	HitResult _hitTriangleArray(const Ray& ray, const int left, const int right);

private:
	void _transCoords(Vec3f* vec);

private:
	std::vector<IObject*> m_objects; // �����е����壬����������������
	std::vector<IObject*> m_lights; // ��Դ��Ϣ, pos��color

	// ������
	Vec3f m_cameraPos; // ���λ��

	// ��ͼת����أ�Ĭ��Ϊ��λ����
	Matrix m_modeMatrix; // ��������
	Matrix m_viewMatrix; // ������ӽǣ�����
	Matrix m_proMatrix; // �ü�����->������͸��
	Matrix m_viewportMatrix; // �ӿ�����

	int m_maxDepth = 5; // ���ݹ����

	BVHNode* m_pBVHRoot = nullptr; // BVH���ڵ�

	// ������0,1,2�ֱ��ʾx,y,z��
	std::array<std::function<bool(IObject*, IObject*)>, 3> m_cmpFuncs;

	TextureContainer m_texs;
};


#endif // !__SCENEMANAGER__H__