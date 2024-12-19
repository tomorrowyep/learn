#ifndef __OBJFILEMODEL_H__
#define __OBJFILEMODEL_H__

#include <filesystem>
#include "imodel.h"

class TGAImage;

class ObjFileModel : public IModel
{
public:
	explicit ObjFileModel(std::filesystem::path& filePath);
    ~ObjFileModel() = default;

    virtual int nverts() override;
    virtual  int nfaces() override;
    virtual Vec3f normal(int iface, int nthvert) override;
    virtual Vec3f vert(int i) override;
    virtual Vec3f vert(int iface, int nthvert) override;
    virtual Vec2f uv(int iface, int nthvert) override;
    virtual std::vector<int> faceVertIndex(int idx) override;

private:
    std::vector<Vec3f> m_verts;
    std::vector<Vec3f> m_norms;
    std::vector<Vec2f> m_uvs;
    std::vector<std::vector<Vec3i> > m_faces;
};
#endif // !__OBJFILEMODEL_H__
