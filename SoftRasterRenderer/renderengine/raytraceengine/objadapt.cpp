#include "objadapt.h"
#include "triangleobj.h"

IObject* ObjAdapt::createObj(const Vec3f* pos, GraphlyType type)
{
	IObject* pIObject = nullptr;
	switch (type)
	{
	case GraphlyType::Triangle:
		pIObject = new TriangleObj(pos);
		break;
	default:
		break;
	}
	return pIObject;
}