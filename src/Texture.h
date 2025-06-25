#ifndef TEXTURE_H
#define TEXTURE_H

#include <QImage>
#include <QString>
#include "BasicDataStructure.h"


class Texture
{
public:
    QString m_path;

    Texture() = default;
    bool loadFromImage(QString path);
    Color sample2D(const Coord2D& coord);
private:
    enum class TextureColorType
    {
        DIFFUSE,
        SPECLUAR
    };
    int m_wide;
    int m_height;
    QImage m_texture;
};

#endif // TEXTURE_H
