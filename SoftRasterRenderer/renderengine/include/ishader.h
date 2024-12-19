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
	// ���о��������
};

// �û���Ҫ�Զ�����ɫ����ͨ��setShader�ӿ�������ɫ��
// �ڵ���drawTriangle��ʱ����VertexInput����
interface IShader
{
    using TextureContainer = std::unordered_map<TextureType, std::unique_ptr<TGAImage>>;

    // ������ɫ�����붼��һ�鶥�����ݣ������ε���������
    struct VertexInput
    {
        std::optional<std::array<Vec3f, 3>> pVert; // �����ζ���λ������, ��������
        std::optional<std::array<Vec2f, 3>> pUV; // �����ζ����������ԣ�ѡ��������
        std::optional<std::array<Vec3f, 3>> pNorm; // �����ζ��㷨�����ԣ�ѡ��������
        std::optional<std::tuple<Vec3f, Vec3f>> pTangent; // �����ε������븱���ߣ�һ�������ι���һ��
        // �Ȱ����⼸�����õİ�
    };

    struct VertexOutput
    {
        std::optional<std::array<Vec4f, 3>> pPos; // �ü��ռ�λ��
        std::optional<std::array<Vec2f, 3>> pTexCoord; // �������ԣ�����Ҫת����
        std::optional<std::array<Vec3f, 3>> pNorm; // ת����ķ�������
        std::optional<std::tuple<Vec3f, Vec3f>> pTangent;
    };

    // Ƭ����ɫ�����붼�ǵ���Ƭ�Σ�Ԫ�ص����ݣ�
    struct FragmentInput
    {
        std::optional<Vec3f> pPixelVert; // Ƭ�β�ֵ���λ������
        std::optional<Vec2f> pPixeUV; // Ƭ�β�ֵ����������
        std::optional<Vec3f> pPixeNorm; // Ƭ�β�ֵ��������
        std::optional<std::tuple<Vec3f, Vec3f>> pPixelTangent;
    };

    virtual ~IShader() {};

    // ������ɫ������Ҫ����������ת����һ��ǿ�����λ�����꣨λ�ڲü�����ϵ��
    // ����һ�������εĶ������ԣ������������ת�����һ�鶥������
    virtual VertexOutput vertex(VertexInput vertexInput) PURE;

    // Ƭ����ɫ������Ҫ�����Ǽ�����ɫ
    // ���뾭����ֵ�����Ƭ�����ԣ����������ɫֵ
    // ����ֵ��ʾ�Ƿ�����Ƭ�Σ�true��ʾ������Ƭ��
    virtual bool fragment(FragmentInput fragAttrs, TGAColor& outColor) PURE;

    virtual void setTexture(TGAImage* img, TextureType type)
    {
        TGAImage* data = new TGAImage(*img);
        m_texs.insert(std::make_pair(type, data));
    }

    virtual void setTexture(TGAImage&& img, TextureType type)
    {
        // ��ֵ���ñ����Ǹ���ֵ����Ҫ�ٴ�תΪ��ֵ������������ֵ�������ڽ���ֵ��
        TGAImage* data = new TGAImage(std::move(img));
        m_texs.insert(std::make_pair(type, data));
    }

    virtual TGAColor getTexture(const Vec2f uv, TextureType type)
    {
        // ��ȡ����
        TGAColor color;
        auto itemTex = m_texs.find(type);
        if (itemTex != m_texs.end())
            color = itemTex->second->diffuse(uv);

        return color;
    }

    // ��ͼת����أ�Ĭ��Ϊ��λ����
    Matrix m_modeMatrix; // ��������
    Matrix m_viewMatrix; // ������ӽǣ�����
    Matrix m_proMatrix; // �ü�����->������͸��
    Matrix m_viewportMatrix; // �ӿ�����

private:
    // �洢����
    TextureContainer m_texs;
};

#endif // !__ISHADER_H__