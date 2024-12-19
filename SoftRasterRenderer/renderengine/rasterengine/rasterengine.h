#ifndef __RASTERENGINE_H__
#define __RASTERENGINE_H__

#include "irenderengine.h"

class TGAImage;

class RasterEngine : public IRasterRenderEngin
{
public:
    explicit RasterEngine() = default;
    ~RasterEngine() = default;

    // ��ͼת�����
    virtual Matrix lookat(Vec3f cameraPos, Vec3f target, Vec3f up) override;
    virtual Matrix ortho(float left, float right, float bottom, float top, float near, float far) override;
    virtual Matrix ortho(Vec2f width, Vec2f height, float near, float far) override;
    virtual Matrix perspective(float fov, float aspect, float near, float far) override;
    virtual Matrix viewport(int width, int height) override;

    // ���������븱����
    virtual std::tuple<Vec3f, Vec3f> computeTangentSpace(const Vec3f* pos, const Vec2f* uv) override;

    virtual void setDevice(TGAImage* device) override;

    // ������ɫ��
    virtual void setShader(IShader* pIShader) override;

    // Bresenham �߶��㷨
    virtual void drawLine(Vec2i start, Vec2i end, TGAColor color) override;
    virtual void drawLine(int x0, int y0, int x1, int y1, TGAColor color) override;

    // ɨ���㷨���������
    virtual void drawTriangle(Vec3f v0, Vec3f v1, Vec3f v2, TGAColor color) override;
    // �����㷨���������
    virtual void drawTriangle(const IShader::VertexInput& input) override;

private:
    Vec3f _getBarycentric(const Vec3f* pVert, const Vec3f point); // �ж��Ƿ�����������
    void _transViewportCoords(Vec4f& vec);
    void _transAccuracy(Vec4f& vec);

    IShader::VertexOutput _executeVertex(const IShader::VertexInput& input);
    void _interpolationAttrs(IShader::VertexOutput& vertexOutput, IShader::FragmentInput& fragInput, const Vec3f& barycentric);

private:
    int m_width = 0;
    int m_height = 0;
    TGAImage* m_pDevice = nullptr;
    IShader* m_pIShader = nullptr;
    std::unique_ptr<float[]> m_zBuffer; // �洢���ֵ
};
#endif // !__RASTERENGINE_H__
