#ifndef __ISHADER_H__
#define __ISHADER_H__

#include <unordered_map>
#include <memory>
#include <optional>
#include <tuple>
#include "geometry.h"
#include "tgaimage.h"

#define interface struct
#define PURE =0

enum class TextureType : int8_t
{
	Diffuse = 0,
	Norm,
	Specular,
	// 还有就往后面加
};

// 用户需要自定义着色器，通过setShader接口设置着色器
// 在调用drawTriangle的时候传入VertexInput即可
interface IShader
{
    using TextureContainer = std::unordered_map<TextureType, std::unique_ptr<TGAImage>>;

    // 顶点着色器输入都是一组顶点数据，三角形的三个顶点
    struct VertexInput
    {
        std::optional<std::array<Vec3f, 3>> pVert; // 三角形顶点位置属性, 必须设置
        std::optional<std::array<Vec2f, 3>> pUV; // 三角形顶点纹理属性，选择性设置
        std::optional<std::array<Vec3f, 3>> pNorm; // 三角形顶点法线属性，选择性设置
        std::optional<std::tuple<Vec3f, Vec3f>> pTangent; // 三角形的切线与副切线，一个三角形公用一组
        // 先包括这几个常用的吧
    };

    struct VertexOutput
    {
        std::optional<std::array<Vec4f, 3>> pPos; // 裁剪空间位置
        std::optional<std::array<Vec2f, 3>> pTexCoord; // 纹理属性（不需要转换）
        std::optional<std::array<Vec3f, 3>> pNorm; // 转换后的法线坐标
        std::optional<std::tuple<Vec3f, Vec3f>> pTangent;
    };

    // 片段着色器输入都是单个片段（元素的数据）
    struct FragmentInput
    {
        std::optional<Vec3f> pPixelVert; // 片段插值后的位置属性
        std::optional<Vec2f> pPixeUV; // 片段插值后纹理属性
        std::optional<Vec3f> pPixeNorm; // 片段插值后法线属性
        std::optional<std::tuple<Vec3f, Vec3f>> pPixelTangent;
    };

    virtual ~IShader() {};

    // 顶点着色器的主要功能是坐标转换，一般强制输出位置坐标（位于裁剪坐标系）
    // 输入一组三角形的顶点属性，输出经过坐标转换后的一组顶点属性
    virtual VertexOutput vertex(VertexInput vertexInput) PURE;

    // 片段着色器的主要功能是计算颜色
    // 输入经过插值过后的片段属性，输出最后的颜色值
    // 返回值表示是否丢弃该片段，true表示丢弃该片段
    virtual bool fragment(FragmentInput fragAttrs, TGAColor& outColor) PURE;

    virtual void setTexture(TGAImage* img, TextureType type)
    {
        TGAImage* data = new TGAImage(*img);
        m_texs.insert(std::make_pair(type, data));
    }

    virtual void setTexture(TGAImage&& img, TextureType type)
    {
        // 右值引用本身是个左值，需要再次转为右值（函数返回右值引用属于将亡值）
        TGAImage* data = new TGAImage(std::move(img));
        m_texs.insert(std::make_pair(type, data));
    }

    virtual TGAColor getTexture(const Vec2f uv, TextureType type)
    {
        // 获取纹理
        TGAColor color;
        auto itemTex = m_texs.find(type);
        if (itemTex != m_texs.end())
            color = itemTex->second->diffuse(uv);

        return color;
    }

    // 视图转换相关，默认为单位矩阵
    Matrix m_modeMatrix; // 世界坐标
    Matrix m_viewMatrix; // 相机（视角）坐标
    Matrix m_proMatrix; // 裁剪坐标->正交、透视
    Matrix m_viewportMatrix; // 视口坐标

private:
    // 存储纹理
    TextureContainer m_texs;
};

#endif // !__ISHADER_H__