#ifndef __TGAIMAGE_H__
#define __TGAIMAGE_H__

#include <array>
#include <algorithm>
#include <filesystem>

#include "geometry.h"

#pragma pack(push, 1)
struct TGA_Header
{
	char idLen; // ��ʶ���ֶγ��ȣ�ָ���ļ�ͷ��ĸ�����Ϣ���ȡ�
	char colorMapType; // �Ƿ������ɫ�壨0���ޣ�1���У���
	char dataTypeCode; // ͼ�����ݵ����ͺ�ѹ����ʽ��
	short colorMapOrg; // ��ɫ�����ʼ������
	short colorMapLen; // ��ɫ�����Ŀ����
	char colorMapDepth; // ��ɫ����Ŀ��λ��ȣ����� 24 λ����
	short xOrg; // ͼ�� X �������ʼƫ�ơ�
	short yOrg; // ͼ�� Y �������ʼƫ�ơ�
	short width; // ��
	short height; // ��
	char bitsperpixel; // ÿ���ص�λ��ȣ�8��16��24 �� 32 λ����
	char imagedescriptor; // ͼ�����������洢˳��� Alpha ͨ��λ����
};
#pragma pack(pop)

struct TGAColor
{
	TGAColor() : m_bgra(), m_bytespp(1) { std::fill_n(m_bgra, 4, 0); }

	// ��һ����ɫ�Ĺ���
	TGAColor(Vec3f color) : m_bgra(), m_bytespp(3)
	{
		std::fill_n(m_bgra, 4, 0);
		color.x = std::clamp(color.x, 0.f, 1.f);
		color.y = std::clamp(color.y, 0.f, 1.f);
		color.z = std::clamp(color.z, 0.f, 1.f);

		color = color * 255.f;
		m_bgra[0] = (unsigned char)std::round(color.z);
		m_bgra[1] = (unsigned char)std::round(color.y);
		m_bgra[2] = (unsigned char)std::round(color.x);
	}

	TGAColor(unsigned char R, unsigned char G, unsigned char B, unsigned char A) : m_bytespp(4)
	{
		m_bgra[0] = B;
		m_bgra[1] = G;
		m_bgra[2] = R;
		m_bgra[3] = A;
	}

	// �Ҷ�
	TGAColor(unsigned char v) : m_bgra(), m_bytespp(1)
	{
		std::fill_n(m_bgra, 4, 0);
		m_bgra[0] = v;
	}

	TGAColor(unsigned char* color, unsigned char bpp) : m_bgra(), m_bytespp(bpp)
	{
		for (int i = 0; i < (int)bpp; ++i)
		{
			m_bgra[i] = color[i];
		}

		for (int i = (int)bpp; i < 4; ++i)
		{
			m_bgra[i] = 0;
		}
	}

	unsigned char& operator[] (const int index) { return m_bgra[index]; }

	TGAColor operator+ (const TGAColor& other)
	{
		TGAColor res = *this;
		for (int i = 0; i < 4; i++)
		{
			res.m_bgra[i] += other.m_bgra[i];
			res.m_bgra[i] = std::clamp(res.m_bgra[i], (unsigned char)0, (unsigned char)255);
		}
		return res;
	}

	// ����
	TGAColor operator* (float intensity)
	{
		TGAColor res = *this;
		intensity = (intensity > 1.f ? 1.f : (intensity < 0.f ? 0.f : intensity));
		for (int i = 0; i < 4; i++) res.m_bgra[i] = (unsigned char)std::round(m_bgra[i] * intensity);
		return res;
	}

	unsigned char m_bgra[4];
	unsigned char m_bytespp; // ÿ������ռ�õ��ֽ���
};

class TGAImage 
{
public:
	enum Format 
	{
		GRAYSCALE = 1, 
		RGB = 3,
		RGBA = 4
	};

	TGAImage() = default;
	~TGAImage() = default;

	TGAImage(int w, int h, int bpp);
	TGAImage(const TGAImage& img);
	TGAImage(TGAImage&& img) noexcept;
	TGAImage& operator= (const TGAImage& img);

	void loadTexture(std::filesystem::path filePath);
	bool write_tga_file(const std::filesystem::path& filePath, bool rle = true);

	TGAColor diffuse(Vec2f uvf); // ��ȡ����������
	float specular(Vec2f uvf); // ��ȡ���淴������
	Vec3f normal(Vec2f uvf); // ��ȡ��������

	bool flip_horizontally();
	bool flip_vertically();
	bool scale(int w, int h);

	// ԭ�������Ͻǣ�x��ʾ�У�y��ʾ��
	TGAColor get(int col, int row);
	// ԭ�������Ͻǣ�x��ʾ�У�y��ʾ��
	bool set(int col, int row, const TGAColor& c);

	int get_width();
	int get_height();
	int get_bytespp();

	const unsigned char* buffer();

	void clear();

private:
	bool _read_tga_file(const std::filesystem::path& filePath);
	bool _load_rle_data(std::ifstream& in);
	bool _unload_rle_data(std::ofstream& out);

private:
	std::unique_ptr<unsigned char[]> m_imageData;
	int m_width = 0;
	int m_height = 0;
	int m_bytespp = 0;
};

#endif // !__TGAIMAGE_H__
