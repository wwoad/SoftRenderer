#include "Texture.h"

bool Texture::loadFromImage(QString path)
{
    m_path = path;
    if(m_texture.load(path))
    {
        //m_texture.flip(Qt::Vertical); // 垂直翻转适应渲染
        m_wide = m_texture.width();
        m_height = m_texture.height();
        return true;
    }
    return false;
}

Color Texture::sample2D(const Coord2D& coord)
{
    // int x = static_cast<int>(coord.x * m_wide); // 或者 floor(coord.x * m_wide)
    // int y = static_cast<int>((1.0 - coord.y) * m_height); // Y轴反转
    // // 钳制到有效范围
    // x = std::clamp(x, 0, m_wide - 1);
    // y = std::clamp(y, 0, m_height - 1);

    // QRgb rgb = m_texture.pixel(x, y);
    // return Color(qRed(rgb) / 255.f, qGreen(rgb) / 255.f, qBlue(rgb) / 255.f);

    int x = static_cast<int>(coord.x * m_wide - 0.5f) % m_wide;
    int y = static_cast<int>(coord.y * m_height - 0.5f) % m_height;
    x = x < 0 ? m_wide + x : x;
    y = y < 0 ? m_height + y : y;
    return Color(m_texture.pixelColor(x, y).red() / 255.f, m_texture.pixelColor(x, y).green() / 255.f, m_texture.pixelColor(x, y).blue() / 255.f);
}

