#ifndef SRFRAMEBUFFER_H
#define SRFRAMEBUFFER_H

#include <iostream>
#include <iomanip>
#include <QImage>
#include <QString>
#include <vector>
#include <immintrin.h>
#include "BasicDataStructure.h"


class SRFrameBuffer  //帧缓冲
{
public:
    SRFrameBuffer(int wide, int height); // 初始化帧缓冲范围
    bool judgeDepth(int x, int y, float z); // 深度判定
    void setPixel(int x, int y, const Color& color); // 像素着色
    bool saveImage(QString filePath); // 保存缓冲图片
    void clearBuffer(const Color& color); // 清除缓存
    std::vector<float>& getDepthBuffer();
    QImage& getImage();
    int getWidth();
    int getHeight();

    //SIMD
    __m256 judgeDepthSimd(const __m256& inside_mask_ps,  const __m256i& x_simd, const __m256i& y_simd, const __m256& z_simd);
    void setPixelSimd(const __m256& mask_simd, const __m256i& x_simd, const __m256i& y_simd, const SimdColor &colors_simd); // colors_simd现在使用SimdVector3D
    void updateDepthSimd(const __m256& mask_ps, const __m256i& x_simd, const __m256i& y_simd, const __m256& z_simd);
private:
    int m_wide;
    int m_height;
    std::vector<float> m_depthBuffer;
    QImage m_colorBuffer;
};



#endif // SRFRAMEBUFFER_H
