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
    return Color(m_texture.pixelColor(x, y).red() / 255.f,
                 m_texture.pixelColor(x, y).green() / 255.f,
                 m_texture.pixelColor(x, y).blue() / 255.f);
}

SimdColor Texture::simdSample2D(const SimdVector2D& coordSimd)
{
    // 异常处理
    if(m_wide <= 0 || m_height <= 0 || m_texture.isNull()){return {_mm256_set1_ps(1.f), _mm256_set1_ps(1.f), _mm256_set1_ps(1.f)};}

    // 将wide height simd化
    __m256 simdWideF = _mm256_set1_ps(static_cast<float>(m_wide));
    __m256 simdHeightF = _mm256_set1_ps(static_cast<float>(m_height));
    // 半个像素插值simd
    __m256 half = _mm256_set1_ps(0.5f);
    // 对应非simd的 coord.x(.y) * wide(height) - 0.5
    __m256 scaledTextureX = _mm256_sub_ps(_mm256_mul_ps(coordSimd.x, simdWideF), half);
    __m256 scaledTextureY = _mm256_sub_ps(_mm256_mul_ps(coordSimd.y, simdHeightF), half);
    //转换像素坐标
    __m256i pixelX = _mm256_cvttps_epi32(scaledTextureX);
    __m256i pixelY = _mm256_cvttps_epi32(scaledTextureY);
    // 屏幕参数simd化
    __m256i simdWideI = _mm256_set1_epi32(m_wide);
    __m256i simdHeightI = _mm256_set1_epi32(m_height);
    __m256i zeroI = _mm256_setzero_si256();
    //对应非simd的 (coord.x(.y) * wide(height) - 0.5) % wide(height)余数的浮点值
    __m256 qxF = _mm256_div_ps(scaledTextureX, simdWideF);
    __m256 qyF = _mm256_div_ps(scaledTextureY, simdHeightF);
    // 将因数转换为整数
    __m256i qx = _mm256_cvttps_epi32(qxF);
    __m256i qy = _mm256_cvttps_epi32(qyF);
    // 将原值pixelX减去因数乘屏幕数据(wide height)得到余数
    __m256i rx = _mm256_sub_epi32(pixelX, _mm256_mullo_epi32(qx, simdWideI));
    __m256i ry = _mm256_sub_epi32(pixelY, _mm256_mullo_epi32(qy, simdHeightI));

    // 对应非simd的 x = x < 0 ? m_wide + x : x;
    __m256i negRxMask = _mm256_cmpgt_epi32(zeroI, rx);
    __m256i wrappedX  = _mm256_add_epi32(rx, _mm256_and_si256(negRxMask, simdWideI));
    __m256i negRyMask = _mm256_cmpgt_epi32(zeroI, ry);
    __m256i wrappedY  = _mm256_add_epi32(ry, _mm256_and_si256(negRyMask, simdHeightI));

    const uchar* scan0 = m_texture.constBits();
    int bytesPerLine = m_texture.bytesPerLine();
    int bytesPerPixel = m_texture.depth() / 8;
    SimdColor sampleColor = {_mm256_setzero_ps(),
                             _mm256_setzero_ps(),
                             _mm256_setzero_ps()};

    __m256i simdBytPerLine = _mm256_set1_epi32(bytesPerLine);
    __m256i addressOffset = _mm256_add_epi32(_mm256_mullo_epi32(wrappedY, simdBytPerLine), _mm256_mullo_epi32(wrappedX, _mm256_set1_epi32(bytesPerPixel)));
    __m256i pixelIndices = _mm256_srli_epi32(addressOffset, 2);

    const int* baseIntPtr = reinterpret_cast<const int*>(scan0);
    __m256i bgraPixels = _mm256_i32gather_epi32(baseIntPtr, pixelIndices, 4);

    __m256i red   = _mm256_and_si256(_mm256_srli_epi32(bgraPixels, 16), _mm256_set1_epi32(0x000000FF));
    __m256i green = _mm256_and_si256(_mm256_srli_epi32(bgraPixels, 8), _mm256_set1_epi32(0x000000FF));
    __m256i blue  = _mm256_and_si256(bgraPixels, _mm256_set1_epi32(0x000000FF));

    __m256 redFinal   = _mm256_cvtepi32_ps(red);
    __m256 greenFinal = _mm256_cvtepi32_ps(green);
    __m256 blueFinal  = _mm256_cvtepi32_ps(blue);

    __m256 inv255 = _mm256_set1_ps(1.f / 255.f);
    sampleColor.r = _mm256_mul_ps(redFinal, inv255);
    sampleColor.g = _mm256_mul_ps(greenFinal, inv255);
    sampleColor.b = _mm256_mul_ps(blueFinal, inv255);
    return sampleColor;
}
/*else {
        // 不是ARGB32/RGB32 或不支持 gather，使用低效回退
        int pixelXArr[8];
        int pixelYArr[8];
        _mm256_storeu_si256((__m256i*)pixelXArr, wrappedX);
        _mm256_storeu_si256((__m256i*)pixelYArr, wrappedY);
        float r[8];
        float g[8];
        float b[8];
        for(int i = 0; i < 8; ++i) {
            QColor qc = m_texture.pixelColor(pixelXArr[i], pixelYArr[i]); // SLOW SCALAR CALL
            r[i] = qc.redF();
            g[i] = qc.greenF();
            b[i] = qc.blueF();
        }
        sampleColor.r = _mm256_loadu_ps(r);
        sampleColor.g = _mm256_loadu_ps(g);
        sampleColor.b = _mm256_loadu_ps(b);
    }*/

