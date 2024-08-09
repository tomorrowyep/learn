#include "freetypemanager.h"
#include <QDebug>

FreeTypeManager::FreeTypeManager()
{
    if (FT_Init_FreeType(&m_library))
    {
        qDebug() << "初始化失败！";
    }
}

FreeTypeManager::~FreeTypeManager()
{
    FT_Done_Face(m_face);
    FT_Done_FreeType(m_library);
}

bool FreeTypeManager::loadFontFace(const char* fontPath)
{
    if (FT_New_Face(m_library, fontPath, 0, &m_face))
        return false;

    //从内存加载
    //FT_New_Memory_Face(m_library, (FT_Byte*)buffer, size, 0, &m_face)

     return true;
}

void FreeTypeManager::setFontSize(const size_t size)
{
    FT_Set_Pixel_Sizes(m_face, 0, size);
}

void FreeTypeManager::getCharInfo(CharacterInfo &charInfo, const FT_ULong Character, const FT_Int32 mode)
{
    if (FT_Error error = FT_Load_Char(m_face, Character, mode))
    {
        qDebug() << "加载失败：" << error;
        return;
    }

    charInfo.advance = m_face->glyph->advance.x;
    charInfo.imageSize.setWidth(m_face->glyph->bitmap.width);
    charInfo.imageSize.setHeight(m_face->glyph->bitmap.rows);
    charInfo.bearing.setX(m_face->glyph->bitmap_left);
    charInfo.bearing.setY(m_face->glyph->bitmap_top);
    charInfo.imageData = m_face->glyph->bitmap.buffer;
}

void FreeTypeManager::getCharsInfo(FT_Int32 mode)
{
    Q_UNUSED(mode);
   //实现范围的获取字符数据，待后续开发
   return;
}
