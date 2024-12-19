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
	m_sampleWeight = (2.0f * PI) * (1.0f / m_sampleCount);// ÿ�β�����Ȩ�أ�ƽ��һ��
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
	m_sampleWeight = (2.0f * PI) * (1.0f / m_sampleCount);// ÿ�β�����Ȩ�أ�ƽ��һ��
}

void RayTraceEngine::rayGeneration(const ExecutexType type)
{
	// ������Ⱦ
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
				// ת����NDC����
				float clipRow = 2.f * row / m_height - 1.f;
				float clipCol = 2.f * col / m_width - 1.f;

				// ����������������پ��
				clipRow += (RenderEngine::randf() - 0.5f) / m_height;
				clipCol += (RenderEngine::randf() - 0.5f) / m_width;
				clipRow = std::clamp(clipRow, -1.f, 1.f);
				clipCol = std::clamp(clipCol, -1.f, 1.f);

				// ����ע�⣬col��Ӧ����y�ᣬrow��Ӧ����x��
				Vec4f projPos{ clipCol * m_near, clipRow * m_near, -m_near, m_near }; // ���Ǵӽ��ü��淢���

				// ���ɹ���
				Ray ray;
				ray.startPoint = m_pSceneManager->transform2World(projPos);
				ray.direction = Vec3f(ray.startPoint - cameraPos).normalize();

				// �󽻵�
				//HitResult hitRes = m_pSceneManager->closestHitByBVH(ray);
				HitResult hitRes = m_pSceneManager->closestHit(ray);
				Vec3f color; // ��Χ0-1
				if (hitRes.isHit)
				{
					// ���й�Դֱ�ӷ��ع�Դ��ɫ
					if (hitRes.material.isEmissive)
					{
						color = hitRes.material.color;
					}
					else
					{
						Ray randomRay;
						randomRay.startPoint = hitRes.hitPoint;
						randomRay.direction = RenderEngine::randomDirection(hitRes.material.normal);

						// ���ݷ����ʾ����������յķ���
						float r = RenderEngine::randf();
						if (r < hitRes.material.specularRate)
						{
							// ���淴��
							Vec3f ref = RenderEngine::reflect(hitRes.material.normal, ray.direction).normalize();
							randomRay.direction = RenderEngine::mix(ref, randomRay.direction, hitRes.material.roughness);
							color = m_pSceneManager->pathTracing(randomRay, 0);
						}
						else if (hitRes.material.specularRate <= r && r <= hitRes.material.refractRate)
						{
							// ����
							Vec3f ref = RenderEngine::refract(ray.direction, hitRes.material.normal, hitRes.material.refractAngle).normalize();
							randomRay.direction = RenderEngine::mix(ref, randomRay.direction * -1, hitRes.material.refractRoughness);
							color = m_pSceneManager->pathTracing(randomRay, 0);
						}
						else
						{
							// ������
							Vec3f srcColor = m_pSceneManager->getTexture("diffuse", hitRes.material.texCoords);
							Vec3f ptColor = m_pSceneManager->pathTracing(randomRay, 0);
							color = multiply_elements(ptColor, srcColor); // ��ԭ��ɫ���
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

	// ����ÿ���߳���Ҫ��ɵ�����
	ThreadPool& threadPool = ThreadPool::instance();
	int threadCount = threadPool.idleThreadCount();
	int taskSize = std::max(1, m_height / threadCount); // �����С����Ϊ 1 ��
	int totalTasks = (m_height + taskSize - 1) / taskSize; // ����ȡ����ȷ��ÿ�ж�����Ⱦ

	std::vector<std::future<void>> taskFutures;
	for (int taskIndex = 0; taskIndex < totalTasks; ++taskIndex)
	{
		// ���㵱ǰ��������з�Χ
		int startRow = taskIndex * taskSize;
		int endRow = std::min(startRow + taskSize, m_height); // ��Ҫ���� m_height

		// �������ύ���̳߳أ����� startRow �� endRow
		auto future = threadPool.commit([=]
			{
				for (int count = 0; count < m_sampleCount; ++count)
				{
					for (int row = startRow; row < endRow; ++row)
					{
						for (int col = 0; col < m_width; ++col)
						{
							// ת����NDC����
							float clipRow = 2.f * row / m_height - 1.f;
							float clipCol = 2.f * col / m_width - 1.f;

							// ����������������پ��
							clipRow += (RenderEngine::randf() - 0.5f) / m_height;
							clipCol += (RenderEngine::randf() - 0.5f) / m_width;
							clipRow = std::clamp(clipRow, -1.f, 1.f);
							clipCol = std::clamp(clipCol, -1.f, 1.f);

							// ����ע�⣬col��Ӧ����y�ᣬrow��Ӧ����x��
							Vec4f projPos{ clipCol * m_near, clipRow * m_near, -m_near, m_near }; // ���Ǵӽ��ü��淢���

							// ���ɹ���
							Ray ray;
							ray.startPoint = m_pSceneManager->transform2World(projPos);
							ray.direction = Vec3f(ray.startPoint - cameraPos).normalize();

							// �󽻵�
							//HitResult hitRes = m_pSceneManager->closestHitByBVH(ray);
							HitResult hitRes = m_pSceneManager->closestHit(ray);
							Vec3f color; // ��Χ0-1
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
										// ������
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

	// �ȴ������������
	for (auto& future : taskFutures)
	{
		future.get();
	}
}
