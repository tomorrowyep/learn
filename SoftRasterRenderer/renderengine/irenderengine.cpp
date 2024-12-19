#include "stdafx.h"
#include "irenderengine.h"
#include "rasterengine.h"
#include "raytraceengine.h"
#include "tgaimage.h"

void engineInstance(void** pIRenderEngin, const EngineType type)
{
	switch (type)
	{
	case EngineType::Rasterization:
		*pIRenderEngin = new RasterEngine;
		break;
	case EngineType::Raytracer:
		*pIRenderEngin = new RayTraceEngine;
		break;
	case EngineType::HardwareAcceleration:
		// TODO
		break;
	default:
		break;
	}
}

void releaseEngine(IRenderEngin* pIRenderEngin)
{
	if (pIRenderEngin)
		delete pIRenderEngin;
}
