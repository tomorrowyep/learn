#include "stdafx.h"
#include "tgaimage.h"

TGAImage::TGAImage(int w, int h, int bpp)
{
    m_width = w;
    m_height = h;
    m_bytespp = bpp;
	unsigned long nbytes = w * h * bpp;
	m_imageData = std::make_unique<unsigned char[]>(nbytes);
	std::fill_n(m_imageData.get(), nbytes, 0);
}

TGAImage::TGAImage(const TGAImage& img)
    : m_width(img.m_width)
    , m_height(img.m_height)
    , m_bytespp(img.m_bytespp)
{
	unsigned long nbytes = m_width * m_height * m_bytespp;
	m_imageData = std::make_unique<unsigned char[]>(nbytes);
	std::memcpy(m_imageData.get(), img.m_imageData.get(), nbytes);
}

TGAImage::TGAImage(TGAImage&& img) noexcept 
    : m_width(img.m_width)
    , m_height(img.m_height)
    , m_bytespp(img.m_bytespp)
    , m_imageData(std::move(img.m_imageData))
{
}

TGAImage& TGAImage::operator= (const TGAImage& img)
{
	if (this == &img)
		return *this;

	unsigned long nbytes = m_width * m_height * m_bytespp;
	m_imageData = std::make_unique<unsigned char[]>(nbytes);
	std::memcpy(m_imageData.get(), img.m_imageData.get(), nbytes);

	m_width = img.m_width;
	m_height = img.m_height;
	m_bytespp = img.m_bytespp;

	return *this;
}

void TGAImage::loadTexture(std::filesystem::path filePath)
{
   _read_tga_file(filePath);
   flip_vertically();
}

bool TGAImage::write_tga_file(const std::filesystem::path& filePath, bool rle)
{
    constexpr unsigned char developer_area_ref[4] = { 0, 0, 0, 0 };
    constexpr unsigned char extension_area_ref[4] = { 0, 0, 0, 0 };
    constexpr unsigned char footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };

    std::ofstream out;
    out.open(filePath, std::ios::binary);
    if (!out.is_open())
        return false;

    TGA_Header header;
    memset((void*)&header, 0, sizeof(header));
    header.bitsperpixel = m_bytespp << 3;
    header.width = m_width;
    header.height = m_height;
    header.dataTypeCode = (m_bytespp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));

    header.imagedescriptor = 0x20; // top-left origin
    out.write((char*)&header, sizeof(header));
    if (!out.good())
    {
        out.close();
        return false;
    }

    if (!rle) 
    {
        out.write((char*)m_imageData.get(), m_width * m_height * m_bytespp);
        if (!out.good()) 
        {
            out.close();
            return false;
        }
    }
    else 
    {
        if (!_unload_rle_data(out)) 
        {
            out.close();
            return false;
        }
    }

    out.write((char*)developer_area_ref, sizeof(developer_area_ref));
    if (!out.good()) 
    {
        out.close();
        return false;
    }

    out.write((char*)extension_area_ref, sizeof(extension_area_ref));
    if (!out.good())
    {
        out.close();
        return false;
    }

    out.write((char*)footer, sizeof(footer));
    if (!out.good())
    {
        out.close();
        return false;
    }

    out.close();
    return true;
}

TGAColor TGAImage::diffuse(Vec2f uvf)
{
    Vec2i uv((int)std::round(uvf[0] * m_width), (int)std::round(uvf[1] * m_height));
    return get(uv[0], uv[1]);
}

float TGAImage::specular(Vec2f uvf)
{
    Vec2i uv((int)std::round(uvf[0] * m_width), (int)std::round(uvf[1] * m_height));
    return get(uv[0], uv[1])[0] / 1.f;
}

Vec3f TGAImage::normal(Vec2f uvf)
{
    Vec2i uv((int)std::round(uvf[0] * m_width), (int)std::round(uvf[1] * m_height));
    TGAColor color = get(uv[0], uv[1]);
    Vec3f res;
    for (int i = 0; i < 3; i++)
        res[2 - i] = (float)color[i] / 255.f * 2.f - 1.f;// 因为TGA的颜色顺序是bgr

    return res;
}

bool TGAImage::flip_horizontally()
{
    if (!m_imageData) 
        return false;

    int half = m_width >> 1;
    std::vector<int> rowsIndex(m_height);
    std::iota(rowsIndex.begin(), rowsIndex.end(), 0);
    std::for_each(std::execution::par, rowsIndex.begin(), rowsIndex.end(), [&](int& row)
        {
            for (int col = 0; col < half; ++col)
            {
                TGAColor c1 = get(col, row);
                TGAColor c2 = get(m_width - 1 - col, row);
                set(col, row, c2);
                set(m_width - 1 - col, row, c1);
            }
        });

    return true;
}

bool TGAImage::flip_vertically()
{
    if (!m_imageData)
        return false;

    int half = m_height >> 1;
    std::vector<int> colsIndex(m_width);
    std::iota(colsIndex.begin(), colsIndex.end(), 0);
    std::for_each(std::execution::par, colsIndex.begin(), colsIndex.end(), [&](int& col)
        {
            for (int row = 0; row < half; ++row)
            {
                TGAColor c1 = get(col, row);
                TGAColor c2 = get(col, m_width - 1 - row);
                set(col, row, c2);
                set(col, m_width - 1 - row, c1);
            }
        });

    return true;
}

bool TGAImage::scale(int w, int h)
{
    if (w <= 0 || h <= 0 || !m_imageData)
        return false;

    auto newdata = std::make_unique<unsigned char[]>(w * h * m_bytespp);
    int nscanline = 0;
    int oscanline = 0;
    int erry = 0;
    unsigned long nlinebytes = w * m_bytespp;
    unsigned long olinebytes = m_width * m_bytespp;
    for (int row = 0; row < m_height; ++row)
    {
        int errx = m_width - w;
        int nx = -m_bytespp;
        int ox = -m_bytespp;
        for (int col = 0; col < m_width; ++col)
        {
            ox += m_bytespp;
            errx += w;
            while (errx >= (int)m_width)
            {
                errx -= m_width;
                nx += m_bytespp;
                memcpy(newdata.get() + nscanline + nx, m_imageData.get() + oscanline + ox, m_bytespp);
            }
        }

        erry += h;
        oscanline += olinebytes;
        while (erry >= (int)m_height)
        {
            if (erry >= (int)m_height << 1) // it means we jump over a scanline
                memcpy(newdata.get() + nscanline + nlinebytes, newdata.get() + nscanline, nlinebytes);

            erry -= m_height;
            nscanline += nlinebytes;
        }
    }

    m_width = w;
    m_height = h;
    m_imageData.reset();
    m_imageData = std::move(newdata);
    return true;
}

TGAColor TGAImage::get(int col, int row)
{
    if (!m_imageData || col < 0 || col >= m_width || row < 0 || row >= m_height)
        return TGAColor();

	return TGAColor(m_imageData.get() + (col + row * m_width) * m_bytespp, m_bytespp);
}

bool TGAImage::set(int col, int row, const TGAColor& c)
{
    if (!m_imageData || col < 0 || col >= m_width || row < 0 || row >= m_height)
        return false;

    std::memcpy(m_imageData.get() + (col + row * m_width) * m_bytespp, c.m_bgra, m_bytespp);
    return true;
}

int TGAImage::get_width()
{
	return m_width;
}

int TGAImage::get_height()
{
	return m_height;
}

int TGAImage::get_bytespp()
{
	return m_bytespp;
}

const unsigned char* TGAImage::buffer()
{
	return m_imageData.get();
}

void TGAImage::clear()
{
	std::fill_n(m_imageData.get(), m_width * m_height * m_bytespp, 0);
}

bool TGAImage::_read_tga_file(const std::filesystem::path& filePath)
{
    if (m_imageData)
        m_imageData.reset();

    std::ifstream in;
    in.open(filePath, std::ios::binary);
    if (!in.is_open())
        return false;

    TGA_Header header;
    in.read((char*)&header, sizeof(header));
    if (!in.good())
    {
        in.close();
        return false;
    }

    m_width = header.width;
    m_height = header.height;
    m_bytespp = header.bitsperpixel >> 3; // 除以8获取到每个像素所占字节数
    if (m_width <= 0 || m_height <= 0 || (m_bytespp != GRAYSCALE && m_bytespp != RGB && m_bytespp != RGBA))
    {
        in.close();
        return false;
    }

    unsigned long nbytes = m_bytespp * m_width * m_height;
    m_imageData = std::make_unique<unsigned char[]>(nbytes);
    if (3 == header.dataTypeCode || 2 == header.dataTypeCode)
    {
        in.read((char*)m_imageData.get(), nbytes);
        if (!in.good())
        {
            in.close();
            return false;
        }
    }
    else if (10 == header.dataTypeCode || 11 == header.dataTypeCode)
    {
        if (!_load_rle_data(in))
        {
            in.close();
            return false;
        }
    }
    else
    {
        in.close();
        return false;
    }

    if (!(header.imagedescriptor & 0x20))
        flip_vertically();

    if (header.imagedescriptor & 0x10)
        flip_horizontally();

    in.close();
    return true;
}

// 解码游程编码，重复次数+数据
bool TGAImage::_load_rle_data(std::ifstream& in)
{
    unsigned long pixelcount = m_width * m_height;
    unsigned long currentpixel = 0;
    unsigned long currentbyte = 0;
    TGAColor colorbuffer;

    do
    {
        unsigned char chunkheader = 0; // RLE 压缩数据块的头部（标识该块的长度或重复次数）
        chunkheader = in.get();
        if (!in.good())
            return false;

        // 如果 chunkheader 小于 128，表示该块是未压缩数据块
        if (chunkheader < 128)
        {
            chunkheader++; // chunkheader 表示的是当前块的长度，需加 1

            // 逐个读取不重复的像素数据
            for (int i = 0; i < chunkheader; i++)
            {
                in.read((char*)colorbuffer.m_bgra, m_bytespp); // 读取一个像素的颜色数据
                if (!in.good())
                    return false;

                // 将颜色数据复制到图像数据中
                for (int t = 0; t < m_bytespp; t++)
                {
                    m_imageData[currentbyte++] = colorbuffer.m_bgra[t];
                }

                currentpixel++; // 增加已处理的像素数
                if (currentpixel > pixelcount) // 如果处理的像素数超过了总像素数，返回 false
                    return false;
            }
        }
        else
        {
            // 如果 chunkheader >= 128，表示该块是重复数据块
            chunkheader -= 127; // chunkheader 表示重复次数，减去 127 后得到真正的重复次数

            in.read((char*)colorbuffer.m_bgra, m_bytespp); // 读取一个像素的颜色数据
            if (!in.good()) // 如果读取失败，返回 false
                return false;

            // 将读取到的颜色数据重复写入图像数据
            for (int i = 0; i < chunkheader; i++)
            {
                for (int t = 0; t < m_bytespp; t++)
                {
                    m_imageData[currentbyte++] = colorbuffer.m_bgra[t];
                }

                currentpixel++; // 增加已处理的像素数
                if (currentpixel > pixelcount)
                    return false;
            }
        }
    } while (currentpixel < pixelcount); // 继续处理直到所有像素都解码完成

    return true;
}

bool TGAImage::_unload_rle_data(std::ofstream& out)
{
    const unsigned char max_chunk_length = 128;
    unsigned long npixels = m_width * m_height;
    unsigned long curpix = 0;
    while (curpix < npixels)
    {
        unsigned long chunkstart = curpix * m_bytespp;
        unsigned long curbyte = curpix * m_bytespp;
        unsigned char run_length = 1;
        bool raw = true;
        while (curpix + run_length < npixels && run_length < max_chunk_length)
        {
            bool succ_eq = true;
            for (int t = 0; succ_eq && t < m_bytespp; t++)
            {
                succ_eq = (m_imageData[curbyte + t] == m_imageData[curbyte + t + m_bytespp]);
            }
            curbyte += m_bytespp;
            if (1 == run_length)
                raw = !succ_eq;
  
            if (raw && succ_eq)
            {
                run_length--;
                break;
            }

            if (!raw && !succ_eq)
            {
                break;
            }

            run_length++;
        }

        curpix += run_length;
        out.put(raw ? run_length - 1 : run_length + 127);
        if (!out.good())
            return false;

        out.write((char*)(m_imageData.get() + chunkstart), (raw ? run_length * m_bytespp : m_bytespp));
        if (!out.good())
            return false;
    }
    return true;
}