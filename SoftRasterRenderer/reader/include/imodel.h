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
    // ��ȡ��������
	virtual int nverts() PURE;

    // ��ȡ�������
	virtual  int nfaces() PURE;

    /*
    * iface���ڼ�����
    * nthvert���ڼ������㣨0->���㡢1->����2->����
    * ret�����ض�Ӧ�ķ�������*/
    virtual Vec3f normal(int iface, int nthvert) PURE;

    // �����������ض�Ӧ��������
    virtual Vec3f vert(int i) PURE;

   /*
   * iface���ڼ�����
   * nthvert���ڼ������㣨0->���㡢1->����2->����
   * ret�����ض�Ӧ�Ķ�������*/
    virtual Vec3f vert(int iface, int nthvert) PURE;

    /*
    * iface���ڼ�����
    * nthvert���ڼ������㣨0->���㡢1->����2->����
    * ret�����ض�Ӧ����������*/
    virtual Vec2f uv(int iface, int nthvert) PURE;

    // ��ȡ��Ӧ��Ķ�������
    virtual std::vector<int> faceVertIndex(int idx) PURE;
};

extern "C"
{
    // ֧������·��
    READER_Module IModel* createInstance(const char* filePath, ReadType mode = ReadType::Synchronous);
    READER_Module void release(IModel* pIMode);
}
#endif // !__IMODEL_H__
