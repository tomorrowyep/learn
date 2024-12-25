#ifndef __IMODEL_H__
#define __IMODEL_H__

#include <vector>
#include "geometry.h"

#define interface struct
#define PURE =0

#ifndef IMPORT_IMODEL
#define READER_Module __declspec(dllexport)
#else
#define READER_Module __declspec(dllimport)
#endif

struct TGAColor;

enum class ReadType : int8_t
{
    Synchronous = 0,
    Asynchronous
};

interface IModel
{
    // 获取顶点数量
	virtual int nverts() PURE;

    // 获取面的数量
	virtual  int nfaces() PURE;

    /*
    * iface：第几个面
    * nthvert：第几个顶点（0->顶点、1->纹理、2->法线
    * ret：返回对应的法线数据*/
    virtual Vec3f normal(int iface, int nthvert) PURE;

    // 根据索引返回对应顶点数据
    virtual Vec3f vert(int i) PURE;

   /*
   * iface：第几个面
   * nthvert：第几个顶点（0->顶点、1->纹理、2->法线
   * ret：返回对应的顶点数据*/
    virtual Vec3f vert(int iface, int nthvert) PURE;

    /*
    * iface：第几个面
    * nthvert：第几个顶点（0->顶点、1->纹理、2->法线
    * ret：返回对应的纹理坐标*/
    virtual Vec2f uv(int iface, int nthvert) PURE;

    // 获取对应面的顶点索引
    virtual std::vector<int> faceVertIndex(int idx) PURE;
};

extern "C"
{
    // 支持中文路径
    READER_Module IModel* createInstance(const char* filePath, ReadType mode = ReadType::Synchronous);
    READER_Module void release(IModel* pIMode);
}
#endif // !__IMODEL_H__
