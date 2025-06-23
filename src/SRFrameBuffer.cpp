#include "SRFrameBuffer.h"

SRFrameBuffer::SRFrameBuffer(int wide, int height)
    :m_wide(wide)
    ,m_height(height)
    ,m_depthBuffer(wide * height)
    ,m_colorBuffer(m_wide, m_height, QImage::Format_BGR888)
{
    m_colorBuffer.fill(QColor(0.f, 0.f, 0.f)); // 默认颜色缓冲为黑色
    std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 1.f); // 深度缓冲默认填充为1
}

bool SRFrameBuffer::judgeDepth(int x, int y, float z)//深度判定
{
    if(z < m_depthBuffer[y * m_wide + x]) // 若传入坐标(x,y)待更新的深度 z < 此坐标深度缓冲目前保存的值
    {
        m_depthBuffer[y * m_wide + x] = z; // 更新当前深度缓冲值
        return true;
    }
    return false;
}

void SRFrameBuffer::setPixel(int x, int y, const Color& color) //着色像素点
{
    m_colorBuffer.setPixelColor(x,
                                m_height - 1 - y,
                                QColor(color.r * 255.f, color.g * 255.f, color.b * 255.f));
}

bool SRFrameBuffer::saveImage(QString filePath)
{
    return m_colorBuffer.save(filePath);
}

void SRFrameBuffer::clearBuffer(const Color& color)
{
    std::fill(m_depthBuffer.begin(), m_depthBuffer.end(), 1.f); // 深度缓冲填充重置为1
    m_colorBuffer.fill(QColor(color.x * 255.f, color.y * 255.f, color.z * 255.f)); // 颜色缓冲填充重置
}

std::vector<float>& SRFrameBuffer::getDepthBuffer()
{
    return m_depthBuffer;
}

QImage& SRFrameBuffer::getImage()
{
    return m_colorBuffer;
}

int SRFrameBuffer::getWidth()
{
    return m_wide;
}

int SRFrameBuffer::getHeight()
{
    return m_height;
}


__m256 SRFrameBuffer::judgeDepthSimd(const __m256& inside_mask_ps, const __m256i& x_simd, const __m256i& y_simd, const __m256& z_simd)
{

    // __m256 epsilon = _mm256_set1_ps(1e-4f);
    // // 确保 m_wide 和 m_height 可访问
    // __m256i wide_simd = _mm256_set1_epi32(m_wide);
    // __m256i height_simd = _mm256_set1_epi32(m_height);
    // // === 核心修复：钳制 x_simd 和 y_simd，确保索引不越界 ===
    // // 将 x_simd 钳制到 [0, m_wide-1]
    // __m256i clamped_x_simd = _mm256_max_epi32(_mm256_setzero_si256(), x_simd);
    // clamped_x_simd = _mm256_min_epi32(clamped_x_simd, _mm256_sub_epi32(wide_simd, _mm256_set1_epi32(1)));
    // // 将 y_simd 钳制到 [0, m_height-1]
    // __m256i clamped_y_simd = _mm256_max_epi32(_mm256_setzero_si256(), y_simd);
    // clamped_y_simd = _mm256_min_epi32(clamped_y_simd, _mm256_sub_epi32(height_simd, _mm256_set1_epi32(1)));
    // // === 修正：计算深度缓冲区索引并加载深度值 ===
    // // 计算每个像素在深度缓冲区中的索引：index = y * m_wide + x
    // __m256i indices_simd = _mm256_add_epi32(_mm256_mullo_epi32(clamped_y_simd, wide_simd), clamped_x_simd);
    // // 使用 _mm256_i32gather_ps 从深度缓冲区加载深度值
    // // base_addr: 深度缓冲区的起始地址 (m_depthBuffer)
    // // index_simd: 包含要加载元素的索引的 __m256i 向量
    // // scale: 索引的比例因子，对于 float (4字节) 是 4
    // // masked_gather: 如果需要根据掩码有条件地加载，可以使用带有掩码的版本
    // // 这里假设是无条件加载8个位置的深度
    // __m256 current_depths_simd = _mm256_i32gather_ps(m_depthBuffer.data(), indices_simd, 4); // 或 loadu
    // __m256 current_depths_threshold = _mm256_sub_ps(current_depths_simd, epsilon);
    // // === 核心：使用 >= 谓词进行比较，然后取反 ===
    // // _CMP_GE_OS 表示大于等于 (Greater Than or Equal, Ordered, Signalling)
    // __m256 ge_mask_ps = _mm256_cmp_ps(z_simd, current_depths_threshold, _CMP_GE_OS);
    // // 取反掩码：0xFFFFFFFF XOR mask
    // __m256 all_ones_ps = _mm256_castsi256_ps(_mm256_set1_epi32(0xFFFFFFFF));
    // __m256 mask_simd = _mm256_xor_ps(all_ones_ps, ge_mask_ps);
    // return mask_simd;


    __m256 depth_test_mask_ps; // 声明掩码
    // 将 fragment_simd.screenDepth 拆解到数组
    float temp_screenDepth_arr[8];
    _mm256_storeu_ps(temp_screenDepth_arr, z_simd);
    // 遍历这8个像素，逐个进行非SIMD深度测试，并手动构建SIMD掩码
    int temp_depth_mask_int = 0; // 存储8个像素的深度测试结果位掩码
    for (int i = 0; i < 8; ++i) {
        // 只有在三角形内部的像素才进行深度测试
        if (((_mm256_movemask_ps(inside_mask_ps) >> i) & 1)) {
            int current_x = _mm256_extract_epi32(x_simd, i); // 获取单个像素的x
            int current_y = _mm256_extract_epi32(y_simd, i); // 获取单个像素的y
            float current_z = temp_screenDepth_arr[i]; // 获取单个像素的插值深度
            // 使用非SIMD的深度测试逻辑
            if (judgeDepth(current_x, current_y, current_z)) {
                // 如果通过测试，设置对应位
                temp_depth_mask_int |= (1 << i);
                // 注意：m_frameBuffer.judgeDepth 内部已经更新了深度缓冲
            }
        }
    }
    // 将手动构建的整数掩码转换回浮点SIMD掩码
    // 这需要一个临时 __m256i 向量，然后 cast
    __m256i temp_depth_mask_simd_i = _mm256_set_epi32(
        (temp_depth_mask_int >> 7) & 1 ? 0xFFFFFFFF : 0,
        (temp_depth_mask_int >> 6) & 1 ? 0xFFFFFFFF : 0,
        (temp_depth_mask_int >> 5) & 1 ? 0xFFFFFFFF : 0,
        (temp_depth_mask_int >> 4) & 1 ? 0xFFFFFFFF : 0,
        (temp_depth_mask_int >> 3) & 1 ? 0xFFFFFFFF : 0,
        (temp_depth_mask_int >> 2) & 1 ? 0xFFFFFFFF : 0,
        (temp_depth_mask_int >> 1) & 1 ? 0xFFFFFFFF : 0,
        (temp_depth_mask_int >> 0) & 1 ? 0xFFFFFFFF : 0
        );
    return _mm256_castsi256_ps(temp_depth_mask_simd_i);
}

void SRFrameBuffer::setPixelSimd(const __m256& mask_simd, const __m256i& x_simd, const __m256i& y_simd, const SimdColor& colors_simd) // colors_simd现在使用SimdVector3D
{

}

void SRFrameBuffer::updateDepthSimd(const __m256& mask_ps, const __m256i& x_simd, const __m256i& y_simd, const __m256& z_simd)
{
    int current_row_y = _mm256_extract_epi32(y_simd, 0); // 取第一个像素的Y，作为代表行
    __m256i wide_simd_local = _mm256_set1_epi32(m_wide);
    __m256i clamped_x_simd = _mm256_max_epi32(_mm256_setzero_si256(), x_simd);
    clamped_x_simd = _mm256_min_epi32(clamped_x_simd, _mm256_sub_epi32(wide_simd_local, _mm256_set1_epi32(1)));
    __m256i clamped_y_simd = _mm256_max_epi32(_mm256_setzero_si256(), y_simd);
    clamped_y_simd = _mm256_min_epi32(clamped_y_simd, _mm256_sub_epi32(_mm256_set1_epi32(m_height), _mm256_set1_epi32(1)));
    __m256i indices_to_update_simd = _mm256_add_epi32(_mm256_mullo_epi32(clamped_y_simd, wide_simd_local), clamped_x_simd);
    _mm256_maskstore_ps(m_depthBuffer.data(), indices_to_update_simd, z_simd);
}
