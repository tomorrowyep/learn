#include "stdafx.h"
#include "reader/include/imodel.h"
#include "renderengine/include/irenderengine.h"
#include "renderengine/include/iobject.h"
#include "geometry.h"

constexpr int g_width = 800;
constexpr int g_height = 800;

// 利用Bresenham算法绘制人脸网格
void drawFaceGrid(IModel* pIModel, IRasterRenderEngin* engine)
{
	for (int i = 0; i < pIModel->nfaces(); ++i)
	{
		auto faceVertIndex = pIModel->faceVertIndex(i);

		for (int j = 0; j < 3; ++j)
		{
			Vec3f v0 = pIModel->vert(faceVertIndex[j]);
			Vec3f v1 = pIModel->vert(faceVertIndex[(j + 1) % 3]);

			int x0 = (int)std::round((v0.x + 1.) * g_width / 2);
			int y0 = (int)std::round((v0.y + 1.) * g_height / 2);
			int x1 = (int)std::round((v1.x + 1.) * g_width / 2);
			int y1 = (int)std::round((v1.y + 1.) * g_height / 2);

			// 避免临界条件
			x0 = std::clamp(x0, 0, g_width - 1);
			y0 = std::clamp(y0, 0, g_height - 1);
			x1 = std::clamp(x1, 0, g_width - 1);
			y1 = std::clamp(y1, 0, g_height - 1);

			engine->drawLine(x0, y0, x1, y1, TGAColor(255, 255, 255, 255));
		}
	}
}

void loadTex(IShader* pIShader)
{
	constexpr const char* diffusePath = "E:/yt/data/数据/african_head_diffuse.tga";
	constexpr const char* normTexPath = "E:/yt/data/数据/african_head_nm.tga";
	constexpr const char* specularTexPath = "E:/yt/data/数据/african_head_spec.tga";

	TGAImage diffuse;
	TGAImage norm;
	TGAImage specular;
	diffuse.loadTexture(diffusePath);
	norm.loadTexture(normTexPath);
	specular.loadTexture(specularTexPath);

	pIShader->setTexture(std::move(diffuse), TextureType::Diffuse);
	pIShader->setTexture(std::move(norm), TextureType::Norm);
	pIShader->setTexture(std::move(specular), TextureType::Specular);
}

// 三角形重心算法插值采样纹理
void drawFaceTriangle(IModel* pIModel, IRasterRenderEngin* engine)
{
	Vec3f local_coords[3];
	Vec2f uv_coords[3];
	Vec3f norm_coords[3];
	for (int i = 0; i < pIModel->nfaces(); ++i)
	{
		auto faceVertIndex = pIModel->faceVertIndex(i);

		for (int j = 0; j < 3; j++)
		{
			local_coords[j] = pIModel->vert(faceVertIndex[j]);
			uv_coords[j] = pIModel->uv(i, j);
			norm_coords[j] = pIModel->normal(i, j);
		}

		IShader::VertexInput vertexInput;
		vertexInput.pVert.emplace(std::array<Vec3f, 3>{local_coords[0], local_coords[1], local_coords[2]});
		vertexInput.pUV.emplace(std::array<Vec2f, 3>{uv_coords[0], uv_coords[1], uv_coords[2]});
		vertexInput.pNorm.emplace(std::array<Vec3f, 3>{norm_coords[0], norm_coords[1], norm_coords[2]});
		vertexInput.pTangent.emplace(engine->computeTangentSpace(local_coords, uv_coords)); // 构造切线与副切线

		engine->drawTriangle(vertexInput);
	}
}

class GouraudShader : public IShader
{
public:

	~GouraudShader() = default;

	virtual VertexOutput vertex(VertexInput vertexInput) override
	{
		VertexOutput output;
		output.pPos.emplace();
		output.pTexCoord.emplace();

		if (!vertexInput.pVert || !vertexInput.pUV)
			return output;

		// 转换坐标系
		for (int i = 0; i < 3; ++i)
		{
			output.pPos->at(i) = _transCoords(vertexInput.pVert->at(i));
			output.pTexCoord->at(i) = vertexInput.pUV->at(i);
		}

		return output;
	}

	virtual bool fragment(FragmentInput fragAttrs, TGAColor& outColor) override
	{
		if (fragAttrs.pPixeUV)
		{
			outColor = getTexture(fragAttrs.pPixeUV.value(), TextureType::Diffuse);
		}
		
		return false;
	}

private:
	Vec4f _transCoords(const Vec3f& vec)
	{
		Vec4f homovec{ vec[0], vec[1], vec[2], 1 };
		homovec = m_proMatrix * m_viewMatrix * m_modeMatrix * homovec; // 转换到裁剪坐标

		return homovec;
	}
};

void rasterization(IModel* pIModel, TGAImage& image)
{
	IRasterRenderEngin* pEngine = nullptr;
	engineInstance((void**)&pEngine);
	if (!pEngine)
		return;

	pEngine->setDevice(&image);

	GouraudShader shader;
	Vec3f cameraPos(0, 0, 3);
	Vec3f center(0, 0, 0);
	shader.m_viewMatrix = pEngine->lookat(cameraPos, center, Vec3f(0, 1, 0));
	shader.m_proMatrix = pEngine->perspective(45.f, g_width / g_height, 0.1f, 100);
	shader.m_viewportMatrix = pEngine->viewport(g_width, g_height);

	loadTex(&shader);
	pEngine->setShader(&shader);
	drawFaceTriangle(pIModel, pEngine);

	releaseEngine(pEngine);
}

void rayTracing(IModel* pIModel, TGAImage& image)
{
	IRayTraceRenderEngin* pEngine = nullptr;
	engineInstance((void**)&pEngine, EngineType::Raytracer);
	if (!pEngine)
		return;

	pEngine->setDevice(&image);
	pEngine->setSampleCount(100);
	pEngine->setMaxDepth(3);

	Vec3f cameraPos(0, 0, 3);
	Vec3f center(0, 0, 0);
	pEngine->lookat(cameraPos, center, Vec3f(0, 1, 0));
	pEngine->perspective(45.f, g_width / g_height, 0.1f, 100);
	pEngine->viewport(g_width, g_height);

	// 添加物体
	Vec3f local_coords[3];
	Vec2f uv_coords[3];
	Vec3f norm_coords[3];
	for (int i = 0; i < pIModel->nfaces(); ++i)
	{
		auto faceVertIndex = pIModel->faceVertIndex(i);

		for (int j = 0; j < 3; j++)
		{
			local_coords[j] = pIModel->vert(faceVertIndex[j]);
			uv_coords[j] = pIModel->uv(i, j);
			norm_coords[j] = pIModel->normal(i, j);
		}

		IObject* obj = pEngine->createObj(local_coords);
		obj->setVertexAttr(VertexAttr::TexCoord, uv_coords);
		obj->setVertexAttr(VertexAttr::Normal, norm_coords);
		pEngine->addObj(obj);
	}

	// 添加光源
	Vec3f lightPos[3] = {Vec3f(0.4f, 0.99f, 0.4f), Vec3f(-0.4f, 0.99f, -0.4f), Vec3f(-0.4f, 0.99f, 0.4f)};
	IObject* objLight = pEngine->createObj(lightPos);
	Material& material = objLight->getMaterial();
	material.color = Vec3f(1.f, 1.f, 1.f);
	material.isEmissive = true;
	pEngine->addObj(objLight);

	pEngine->buildBVH(10);

	constexpr const char* diffusePath = "E:/yt/data/数据/african_head_diffuse.tga";

	TGAImage diffuse;
	diffuse.loadTexture(diffusePath);
	pEngine->setTexture("diffuse", std::move(diffuse));

	pEngine->rayGeneration(ExecutexType::Asynchronous); // Asynchronous Synchronous

	releaseEngine(pEngine);
}

int main()
{
	constexpr const char* path = "E:/yt/data/数据/african_head.obj";
	IModel* pIModel = createInstance(path);
	if (!pIModel)
		return 0;
	
	TGAImage image(g_width, g_height, TGAImage::RGB);

	EngineType type = EngineType::Raytracer;// EngineType::Rasterization
	switch (type)
	{
	case EngineType::Rasterization:
		// 光栅化测试
		rasterization(pIModel, image);
		break;
	case EngineType::Raytracer:
		// 光追测试
		rayTracing(pIModel, image);
		break;
	case EngineType::HardwareAcceleration:
		break;
	default:
		break;	
	}

	image.flip_vertically(); // 坐标系原点不同
	image.write_tga_file("output.tga");
	release(pIModel);
	return 0;
}