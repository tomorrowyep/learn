#include "stdafx.h"
#include "imodel.h"
#include "objfilemodel.h"
#include "threadpool.h"

namespace
{
	enum class FileType : unsigned int
	{
		None = 0,
		ObjFile
	};

	FileType _getFileType(const std::filesystem::path& path)
	{
		static const std::unordered_map<std::string, FileType> extensionMap
		{
			{".obj", FileType::ObjFile}
			// 新添加的写在后面
		};

		auto item = extensionMap.find(path.extension().string());
		if (item != extensionMap.end())
			return item->second;

		return FileType::None;
	}
}

IModel* createInstance(const char* filePath, ReadType mode)
{
	std::filesystem::path path(filePath);
	FileType type = _getFileType(path);
	IModel* pIModel = nullptr;
	switch (type)
	{
	case FileType::ObjFile:
		pIModel= new ObjFileModel(path);
		break;
	default:
		break;
	}

	return pIModel;
}

void release(IModel* pIMode)
{
	if (pIMode)
		delete pIMode;
}
