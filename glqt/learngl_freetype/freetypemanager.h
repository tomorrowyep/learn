#ifndef FREETYPEMANAGER_H
#define FREETYPEMANAGER_H

#include <ft2build.h>
#include <QVector2D>
#include <QSize>
#include FT_FREETYPE_H
#include FT_STROKER_H

class FreeTypeManager
{
public:
    struct CharacterInfo
    {
        const unsigned char* imageData = nullptr;
        size_t textureId = 0;
        size_t advance = 0;
        QVector2D bearing;
        QSize imageSize;
    };

    FreeTypeManager();
    ~FreeTypeManager();

    bool loadFontFace(const char* fontPath);
    void setFontSize(const size_t size);
    void getCharInfo(CharacterInfo &charInfo, const FT_ULong Character, const FT_Int32 mode = FT_LOAD_RENDER);
    void getCharsInfo(FT_Int32 mode = FT_LOAD_RENDER);

private:
    FT_Library m_library = nullptr;
    FT_Face m_face = nullptr;
};

#endif // FREETYPEMANAGER_H
