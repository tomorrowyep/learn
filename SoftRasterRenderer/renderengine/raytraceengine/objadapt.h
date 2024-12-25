#ifndef __OBJADAPT_H__
#define __OBJADAPT_H__

#include "iobject.h"

enum class GraphlyType : int8_t
{
	Triangle = 0
};

class ObjAdapt
{
public:
	ObjAdapt() = default;
	~ObjAdapt() = default;

	static IObject* createObj(const Vec3f* pos, GraphlyType type = GraphlyType::Triangle);
};

#endif // !__OBJADAPT_H__