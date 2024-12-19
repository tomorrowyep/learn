#include "stdafx.h"
#include "threadpool.h"
#include "raytraceengine.h"
#include "common.h"
#include "iobject.h"
#include "scenemanager.h"

#include <random>

namespace
{
	constexpr float PI = 3.1415926f;
}

RayTraceEngine::RayTraceEngine()
{
	m_pSceneManager = std::make_unique<SceneManager>();
	m_sampleWeight = (2.0f * PI) * (1.0f / m_sampleCount);// 每次采样的权重，平均一下
}

Matrix RayTraceEngine::lookat(Vec3f cameraPos, Vec3f target, Vec3f up)
{
	m_pSceneManager->setCameraPos(cameraPos);

	Matrix view = RenderEngine::lookat(cameraPos, target, up);
	m_pSceneManager->setViewMatrix(view);
	return view;
}

Matrix RayTraceEngine::ortho(float left, float right, float bottom, float top, float near, float far)
{
	Matrix pro = RenderEngine::ortho(left, right, bottom, top, near, far);
	m_pSceneManager->setProMatrix(pro);
	return pro;
}

Matrix RayTraceEngine::ortho(Vec2f width, Vec2f height, float near, float far)
{
	float left = width.x, right = width.y;
	float bottom = height.x, top = height.y;
	Matrix pro = RenderEngine::ortho(left, right, bottom, top, near, far);
	m_pSceneManager->setProMatrix(pro);
	return pro;
}

Matrix RayTraceEngine::perspective(float fov, float aspect, float near, float far)
{
	m_near = near;
	Matrix pro = RenderEngine::perspective(fov, aspect, near, far);
	m_pSceneManager->setProMatrix(pro);
	return pro;
}

Matrix RayTraceEngine::viewport(int width, int height)
{
	Matrix viewport = RenderEngine::viewport(width, height);
	m_pSceneManager->setViewportMatrix(viewport);
	return viewport;
}

std::tuple<Vec3f, Vec3f> RayTraceEngine::computeTangentSpace(const Vec3f* pos, const Vec2f* uv)
{
	return RenderEngine::computeTangentSpace(pos, uv);
}

void RayTraceEngine::setDevice(TGAImage* device)
{
	m_width = device->get_width();
	m_height = device->get_height();
	m_pDevice = device;
}

void RayTraceEngine::addObj(IObject* pScene)
{
	m_pSceneManager->addObj(pScene);
}

void RayTraceEngine::setMaxDepth(int depth)
{
	m_pSceneManager->setMaxDepth(depth);
}

void RayTraceEngine::addLight(IObject* pos, const Vec3f& color)
{
	m_pSceneManager->addLight(pos, color);
}

void RayTraceEngine::setTexture(const std::string& texName, TGAImage* img)
{
	m_pSceneManager->setTexture(texName, img);
}

void RayTraceEngine::setTexture(const std::string& texName, TGAImage&& img)
{
	m_pSceneManager->setTexture(texName, std::move(img));
}

void RayTraceEngine::setModeMatrix(const Matrix& mode)
{
	m_pSceneManager->setModeMatrix(mode);
}

void RayTraceEngine::buildBVH(int count)
{
	m_pSceneManager->buildBVH(count);
}

void RayTraceEngine::setSampleCount(int count)
{
	m_sampleCount = count;
	m_sampleWeight = (2.0f * PI) * (1.0f / m_sampleCount);// 每次采样的权重，平均一下
}

void RayTraceEngine::rayGeneration(const ExecutexType type)
{
	// 并行渲染
	if (type == ExecutexType::Asynchronous)
	{
		_syncRayGeneration();
		return;
	}

	Vec3f cameraPos = m_pSceneManager->getCameraPos();
	for (int count = 0; count < m_sampleCount; ++count)
	{
		for (int row = 0; row < m_height; ++row)
		{
			for (int col = 0; col < m_width; ++col)
			{
				// 转换到NDC坐标
				float clipRow = 2.f * row / m_height - 1.f;
				float clipCol = 2.f * col / m_width - 1.f;

				// 引入随机采样，减少锯齿
				clipRow += (RenderEngine::randf() - 0.5f) / m_height;
				clipCol += (RenderEngine::randf() - 0.5f) / m_width;
				clipRow = std::clamp(clipRow, -1.f, 1.f);
				clipCol = std::clamp(clipCol, -1.f, 1.f);

				// 这里注意，col对应的是y轴，row对应的是x轴
				Vec4f projPos{ clipCol * m_near, clipRow * m_near, -m_near, m_near }; // 都是从近裁剪面发射的

				// 生成光线
				Ray ray;
				ray.startPoint = m_pSceneManager->transform2World(projPos);
				ray.direction = Vec3f(ray.startPoint - cameraPos).normalize();

				// 求交点
				//HitResult hitRes = m_pSceneManager->closestHitByBVH(ray);
				HitResult hitRes = m_pSceneManager->closestHit(ray);
				Vec3f color; // 范围0-1
				if (hitRes.isHit)
				{
					// 命中光源直接返回光源颜色
					if (hitRes.material.isEmissive)
					{
						color = hitRes.material.color;
					}
					else
					{
						Ray randomRay;
						randomRay.startPoint = hitRes.hitPoint;
						randomRay.direction = RenderEngine::randomDirection(hitRes.material.normal);

						// 根据反射率决定光线最终的方向
						float r = RenderEngine::randf();
						if (r < hitRes.material.specularRate)
						{
							// 镜面反射
							Vec3f ref = RenderEngine::reflect(hitRes.material.normal, ray.direction).normalize();
							randomRay.direction = RenderEngine::mix(ref, randomRay.direction, hitRes.material.roughness);
							color = m_pSceneManager->pathTracing(randomRay, 0);
						}
						else if (hitRes.material.specularRate <= r && r <= hitRes.material.refractRate)
						{
							// 折射
							Vec3f ref = RenderEngine::refract(ray.direction, hitRes.material.normal, hitRes.material.refractAngle).normalize();
							randomRay.direction = RenderEngine::mix(ref, randomRay.direction * -1, hitRes.material.refractRoughness);
							color = m_pSceneManager->pathTracing(randomRay, 0);
						}
						else
						{
							// 漫反射
							Vec3f srcColor = m_pSceneManager->getTexture("diffuse", hitRes.material.texCoords);
							Vec3f ptColor = m_pSceneManager->pathTracing(randomRay, 0);
							color = multiply_elements(ptColor, srcColor); // 和原颜色混合
						}
						color = color * m_sampleWeight;
					}
				}

				m_pDevice->set(col, row, m_pDevice->get(col, row) + color);
			}
		}
	}
}

void RayTraceEngine::_syncRayGeneration()
{
	Vec3f cameraPos = m_pSceneManager->getCameraPos();

	// 划分每个线程需要完成的行数
	ThreadPool& threadPool = ThreadPool::instance();
	int threadCount = threadPool.idleThreadCount();
	int taskSize = std::max(1, m_height / threadCount); // 任务大小至少为 1 行
	int totalTasks = (m_height + taskSize - 1) / taskSize; // 向上取整，确保每行都被渲染

	std::vector<std::future<void>> taskFutures;
	for (int taskIndex = 0; taskIndex < totalTasks; ++taskIndex)
	{
		// 计算当前任务负责的行范围
		int startRow = taskIndex * taskSize;
		int endRow = std::min(startRow + taskSize, m_height); // 不要超出 m_height

		// 将任务提交到线程池，传入 startRow 和 endRow
		auto future = threadPool.commit([=]
			{
				for (int count = 0; count < m_sampleCount; ++count)
				{
					for (int row = startRow; row < endRow; ++row)
					{
						for (int col = 0; col < m_width; ++col)
						{
							// 转换到NDC坐标
							float clipRow = 2.f * row / m_height - 1.f;
							float clipCol = 2.f * col / m_width - 1.f;

							// 引入随机采样，减少锯齿
							clipRow += (RenderEngine::randf() - 0.5f) / m_height;
							clipCol += (RenderEngine::randf() - 0.5f) / m_width;
							clipRow = std::clamp(clipRow, -1.f, 1.f);
							clipCol = std::clamp(clipCol, -1.f, 1.f);

							// 这里注意，col对应的是y轴，row对应的是x轴
							Vec4f projPos{ clipCol * m_near, clipRow * m_near, -m_near, m_near }; // 都是从近裁剪面发射的

							// 生成光线
							Ray ray;
							ray.startPoint = m_pSceneManager->transform2World(projPos);
							ray.direction = Vec3f(ray.startPoint - cameraPos).normalize();

							// 求交点
							//HitResult hitRes = m_pSceneManager->closestHitByBVH(ray);
							HitResult hitRes = m_pSceneManager->closestHit(ray);
							Vec3f color; // 范围0-1
							TGAColor tgaColor;
							if (hitRes.isHit)
							{
								if (hitRes.material.isEmissive)
								{
									color = hitRes.material.color;
								}
								else
								{
									Ray randomRay;
									randomRay.startPoint = hitRes.hitPoint;
									randomRay.direction = RenderEngine::randomDirection(hitRes.material.normal);

									float r = RenderEngine::randf();
									if (r < hitRes.material.specularRate)
									{
										Vec3f ref = RenderEngine::reflect(hitRes.material.normal, ray.direction).normalize();
										randomRay.direction = RenderEngine::mix(ref, randomRay.direction, hitRes.material.roughness);
										color = m_pSceneManager->pathTracing(randomRay, 0);
									}
									else if (hitRes.material.specularRate <= r && r <= hitRes.material.refractRate)
									{
										Vec3f ref = RenderEngine::refract(ray.direction, hitRes.material.normal, hitRes.material.refractAngle).normalize();
										randomRay.direction = RenderEngine::mix(ref, randomRay.direction * -1, hitRes.material.refractRoughness);
										color = m_pSceneManager->pathTracing(randomRay, 0);
									}
									else
									{
										// 漫反射
										tgaColor = m_pSceneManager->getTGATexture("diffuse", hitRes.material.texCoords);

										//Vec3f srcColor = m_pSceneManager->getTexture("diffuse", hitRes.material.texCoords);
										//Vec3f ptColor = m_pSceneManager->pathTracing(randomRay, 0);
										//color = multiply_elements(ptColor, srcColor);
									}
									color = color * m_sampleWeight;
								}
							}

							//m_pDevice->set(col, row, m_pDevice->get(col, row) + color);
							m_pDevice->set(col, row, tgaColor);
						}
					}
				}
			});

		taskFutures.push_back(std::move(future));
	}

	// 等待所有任务完成
	for (auto& future : taskFutures)
	{
		future.get();
	}
}
