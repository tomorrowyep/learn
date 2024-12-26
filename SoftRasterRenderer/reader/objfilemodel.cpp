#include "stdafx.h"
#include "objfilemodel.h"

ObjFileModel::ObjFileModel(std::filesystem::path& filePath)
{
	std::ifstream file(filePath, std::ifstream::binary);
	if (!file.is_open())
		return;

	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;  // 获取数据类型

		if (prefix == "v")
		{
			Vec3f vert;
			iss >> vert.x >> vert.y >> vert.z;
			m_verts.push_back(std::move(vert));
		}
		else if (prefix == "vt")
		{
			Vec2f vert;
			iss >> vert.x >> vert.y;
			m_uvs.push_back(std::move(vert));// 只考虑了二维纹理映射的场景
		}
		else if (prefix == "vn")
		{
			Vec3f vert;
			iss >> vert.x >> vert.y >> vert.z;
			m_norms.push_back(std::move(vert));
		}
		else if (prefix == "f")
		{
			char trash;
			Vec3i vert;
			std::vector<Vec3i> face;
			while (iss >> vert.x >> trash >> vert.y >> trash >> vert.z) 
			{
				for (int i = 0; i < 3; i++) vert[i]--; // 索引需要减1,OBJ文件索引从1开始，不与vector容器直接匹配
				face.push_back(vert);
			}

			m_faces.push_back(std::move(face));// 只考虑了这个情况v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
		}
	}

	file.close();
}

int ObjFileModel::nverts()
{
	return (int)m_verts.size();
}

int ObjFileModel::nfaces()
{
	return (int)m_faces.size();
}

Vec3f ObjFileModel::normal(int iface, int nthvert)
{
	int idx = m_faces[iface][nthvert][2];
	return m_norms[idx].normalize();
}

Vec3f ObjFileModel::vert(int i)
{
	return m_verts[i];
}

Vec3f ObjFileModel::vert(int iface, int nthvert)
{
	return m_verts[m_faces[iface][nthvert][0]];
}

Vec2f ObjFileModel::uv(int iface, int nthvert)
{
	return m_uvs[m_faces[iface][nthvert][1]];
}

std::vector<int> ObjFileModel::faceVertIndex(int idx)
{
	std::vector<int> face;
	for (int i = 0; i < (int)m_faces[idx].size(); i++) face.push_back(m_faces[idx][i][0]);
	return face;
}
