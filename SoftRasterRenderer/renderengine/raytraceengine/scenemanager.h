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

	// 暴力求解，会遍历每个图元
	HitResult closestHit(const Ray& ray);
	
	// 采用BVH加速结构求交
	HitResult closestHitByBVH(const Ray& ray);

	Vec3f pathTracing(const Ray& ray, int depth);

private:
	struct BVHNode
	{
		BVHNode* left = nullptr;
		BVHNode* right = nullptr;

		int index = -1; // 当前节点包含的对象开始索引
		int nums = 0;  // 当前节点包含的对象数量，nums不为0时表示叶子节点

		Vec3f AA; // 包围盒，左下
		Vec3f BB; // 右上
	};

	HitResult _closestHitByBVH(const Ray& ray, BVHNode* node);

	// 构建BVH树
	BVHNode* _buildBVH(size_t left, size_t right, size_t limitCount = 8);

	/*SVH构建BVH树
	* 查找左盒子的 n1 个三角形需要花费 T * n1 的时间（其中 t 为常数）
	* 查找右盒子的 n2 个三角形需要花费 T * n2 的时间
	* 假设光线有 p1 的概率击中左盒子，有 p2 的概率击中右盒子，这里用表面积代替，成正相关
	* 最终的代价为 cost = p1 * n1 + p2 * n2 （这里省略了常数 T）
	*/
	BVHNode* _buildBVHBySAH(size_t left, size_t right, size_t limitCount = 8);

	// 存在三个返回值，分别表示未相交(-1)、相交(t0)、相交且在包围盒内(t1)
	float _hitAABB(const Ray& ray, const BVHNode* node);

	// 返回最终相交的距离最近三角形
	HitResult _hitTriangleArray(const Ray& ray, const int left, const int right);

private:
	void _transCoords(Vec3f* vec);

private:
	std::vector<IObject*> m_objects; // 场景中的物体，负责对象的生命周期
	std::vector<IObject*> m_lights; // 光源信息, pos、color

	// 相机相关
	Vec3f m_cameraPos; // 相机位置

	// 视图转换相关，默认为单位矩阵
	Matrix m_modeMatrix; // 世界坐标
	Matrix m_viewMatrix; // 相机（视角）坐标
	Matrix m_proMatrix; // 裁剪坐标->正交、透视
	Matrix m_viewportMatrix; // 视口坐标

	int m_maxDepth = 5; // 最大递归深度

	BVHNode* m_pBVHRoot = nullptr; // BVH根节点

	// 排序函数0,1,2分别表示x,y,z轴
	std::array<std::function<bool(IObject*, IObject*)>, 3> m_cmpFuncs;

	TextureContainer m_texs;
};


#endif // !__SCENEMANAGER__H__