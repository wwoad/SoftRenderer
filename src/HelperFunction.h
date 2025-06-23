#ifndef HELPERFUNCTION_H
#define HELPERFUNCTION_H

#include <bitset>
#include "SRendererDevice.h"
#include "BasicDataStructure.h"


static inline bool judgeOnTopLeftEdge(const CoordI2D& v0, const CoordI2D& v1)  // 左上角规则判断，用于避免三角形共享边重绘和漏绘出现
{
    return (v0.y > v1.y) || (v0.x > v1.x && v1.y == v0.y); // 若边AB为向上趋势或者水平边则返回true
}

// 判断点是否在三角形内
static inline bool judgeInsideTriangle(const EdgeEquation& triEdge, const VectorI3D& res)
{
    bool flag = true;
    if(res.x == 0)
    {
        flag &= triEdge.m_topLeftFlag[0];
    }
    if(res.y == 0)
    {
        flag &= triEdge.m_topLeftFlag[1];
    }
    if(res.z == 0)
    {
        flag &= triEdge.m_topLeftFlag[2];
    }

    return flag && ((res.x >= 0 && res.y >= 0 && res.z >= 0) || (res.x <= 0 && res.y <= 0 && res.z <= 0));
}

// 正确的视角
template <class T>
static inline T correctPerspective(float viewDepth, const std::vector<T>& attribute, const Triangle &tri, const Vector3D &barycentric)
{
    return viewDepth * (barycentric.x * attribute[0] / tri[0].clipSpacePos.w + barycentric.y * attribute[1] / tri[1].clipSpacePos.w + barycentric.z * attribute[2] / tri[2].clipSpacePos.w);
}

// 计算插值
template<class T>
static inline T calculateInterpolation(T a, T b, T c, const Vector3D& barycentric)
{
    return a * barycentric.x + b * barycentric.y + c * barycentric.z;
}

template<class T>
static inline T calculateInterpolation(T a, T b, float alpha)
{
    return a * (1 - alpha) + b * alpha;
}


static inline CoordI2D calculateInterpolation(const CoordI2D& a, const CoordI2D& b, float alpha)
{
    CoordI2D res;
    res.x = static_cast<int> (a.x * (1 - alpha) + b.x * alpha + 0.5f);
    res.y = static_cast<int> (a.y * (1 - alpha) + b.y * alpha + 0.5f);

    return res;
}

static inline Vertex calculateInterpolation(const Vertex& a, const Vertex& b, float alpha)
{
    Vertex res;
    res.clipSpacePos = calculateInterpolation<Coord4D>(a.clipSpacePos, b.clipSpacePos, alpha);
    res.worldSpacePos = calculateInterpolation<Coord3D>(a.worldSpacePos, b.worldSpacePos, alpha);
    res.normal = calculateInterpolation<Vector3D>(a.normal, b.normal, alpha);
    res.texCoord = calculateInterpolation<Coord2D>(a.texCoord, b.texCoord, alpha);

    return res;
}

// 计算距离
template<class T>
static inline float calculateDistance(const T& point, const T& border)
{
    return glm::dot(point, border);
}

// 获取剪辑代码
template<class T, size_t N>
static inline std::bitset<N> getClipCode(T point, const std::array<T,N>& clip)
{
    std::bitset<N> res;
    for(int i = 0; i < N; i++)
    {
        if(calculateDistance(point, clip[i]) < 0)
        {
            res.set(i, 1);
        }
    }

    return res;
}

// 剪裁重新构建三角形
std::vector<Triangle> constructTriangle(const std::vector<Vertex>& vertexList)
{
    std::vector<Triangle> res;
    for(int i = 0; i< vertexList.size() -2; i++)
    {
        int k = (i + 1) % vertexList.size();
        int m = (i + 2) % vertexList.size();
        Triangle tri{vertexList[0], vertexList[k], vertexList[m]};
        res.push_back(tri);
    }

    return res;
}

// 构造片段
Fragment constructFragment(int x, int y, float z, float viewDepth, const Triangle& tri, const Vector3D& barycentric)
{
    Fragment frag;
    frag.screenPos.x = x;
    frag.screenPos.y = y;
    frag.screenDepth = z;
    frag.worldSpacePos = correctPerspective<Color>(viewDepth,
                                            std::vector<Color>{tri[0].worldSpacePos, tri[1].worldSpacePos, tri[2].worldSpacePos},
                                            tri,
                                            barycentric);

    frag.normal = correctPerspective<Vector3D>(viewDepth,
                                     std::vector<Vector3D>{tri[0].normal, tri[1].normal, tri[2].normal},
                                     tri,
                                     barycentric);

    frag.texCoord = correctPerspective<Coord2D>(viewDepth,
                                       std::vector<Coord2D>{tri[0].texCoord, tri[1].texCoord, tri[2].texCoord},
                                       tri,
                                       barycentric);

    return frag;
}

//SIMD
SimdVector3D broadcastVector3D(const Vector3D& vec)
{
    SimdVector3D res;
    res.x = _mm256_set1_ps(vec.x);
    res.y = _mm256_set1_ps(vec.y);
    res.z = _mm256_set1_ps(vec.z);
    return res;
}

// SIMD 版本的浮点属性插值
// bartcenTri_simd: 包含8个像素重心坐标 (alpha, beta, gamma) 的 SimdVector3D
// 返回: 包含8个插值后浮点属性值的 __m256 向量
__m256 calculateInterpolationSimdFloat(float v0, float v1, float v2, const SimdVector3D& bartcenTri_simd)
{
    // 插值公式: result = alpha * v0 + beta * v1 + gamma * v2
    // 将顶点属性值复制到8个浮点数的SIMD向量中
    __m256 v0_simd = _mm256_set1_ps(v0);
    __m256 v1_simd = _mm256_set1_ps(v1);
    __m256 v2_simd = _mm256_set1_ps(v2);
    // 执行 SIMD 插值
    __m256 term0 = _mm256_mul_ps(bartcenTri_simd.x, v0_simd); // alpha * v0
    __m256 term1 = _mm256_mul_ps(bartcenTri_simd.y, v1_simd); // beta * v1
    __m256 term2 = _mm256_mul_ps(bartcenTri_simd.z, v2_simd); // gamma * v2
    __m256 result_simd = _mm256_add_ps(_mm256_add_ps(term0, term1), term2); // term0 + term1 + term2
    return result_simd;
}


// SIMD 版本的 Vector3D 属性插值
// v0, v1, v2: 三角形三个顶点的 Vector3D 属性值
// bartcenTri_simd: 包含8个像素重心坐标 (alpha, beta, gamma) 的 SimdVector3D
// 返回: 包含8个插值后 Vector3D 属性值的 SimdVector3D 结构体
SimdVector3D calculateInterpolationSimdVector3D(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const SimdVector3D& bartcenTri_simd)
{
    // 插值公式: result = alpha * v0 + beta * v1 + gamma * v2
    // 这里的 v0, v1, v2 是 Vector3D，意味着需要对每个分量 (x, y, z) 分别进行插值
    SimdVector3D result_simd;
    // 插值 x 分量
    __m256 v0x_simd = _mm256_set1_ps(v0.x);
    __m256 v1x_simd = _mm256_set1_ps(v1.x);
    __m256 v2x_simd = _mm256_set1_ps(v2.x);
    __m256 term0x = _mm256_mul_ps(bartcenTri_simd.x, v0x_simd);
    __m256 term1x = _mm256_mul_ps(bartcenTri_simd.y, v1x_simd);
    __m256 term2x = _mm256_mul_ps(bartcenTri_simd.z, v2x_simd);
    result_simd.x = _mm256_add_ps(_mm256_add_ps(term0x, term1x), term2x);
    // 插值 y 分量
    __m256 v0y_simd = _mm256_set1_ps(v0.y);
    __m256 v1y_simd = _mm256_set1_ps(v1.y);
    __m256 v2y_simd = _mm256_set1_ps(v2.y);
    __m256 term0y = _mm256_mul_ps(bartcenTri_simd.x, v0y_simd);
    __m256 term1y = _mm256_mul_ps(bartcenTri_simd.y, v1y_simd);
    __m256 term2y = _mm256_mul_ps(bartcenTri_simd.z, v2y_simd);
    result_simd.y = _mm256_add_ps(_mm256_add_ps(term0y, term1y), term2y);
    // 插值 z 分量
    __m256 v0z_simd = _mm256_set1_ps(v0.z);
    __m256 v1z_simd = _mm256_set1_ps(v1.z);
    __m256 v2z_simd = _mm256_set1_ps(v2.z);
    __m256 term0z = _mm256_mul_ps(bartcenTri_simd.x, v0z_simd);
    __m256 term1z = _mm256_mul_ps(bartcenTri_simd.y, v1z_simd);
    __m256 term2z = _mm256_mul_ps(bartcenTri_simd.z, v2z_simd);
    result_simd.z = _mm256_add_ps(_mm256_add_ps(term0z, term1z), term2z);
    return result_simd;
}

SimdFragment constructFragmentSimd(const __m256i& x_simd, const __m256i& y_simd,
                                   const __m256& screenDepth_simd,
                                   const SimdVector3D& barycentric_simd,
                                   const Triangle& tri)
{
    SimdFragment frag_simd;
    // 填充屏幕坐标和屏幕深度
    frag_simd.screenPosX = x_simd;
    frag_simd.screenPosY = y_simd;
    frag_simd.screenDepth = screenDepth_simd;
    // 计算并存储插值后的 1/w
    float w0_recip = 1.f / tri[0].ndcSpacePos.w; // 使用 ndcSpacePos.w，与您原始代码一致
    float w1_recip = 1.f / tri[1].ndcSpacePos.w;
    float w2_recip = 1.f / tri[2].ndcSpacePos.w;
    frag_simd.viewDepth = calculateInterpolationSimdFloat(w0_recip, w1_recip, w2_recip, barycentric_simd); // !!! IMPORTANT: Storing interpolated 1/w here
    // 插值属性除以 w (用于透视校正的分子)
    // 纹理坐标 / w 插值
    glm::vec3 texCoord0_div_w = glm::vec3(tri[0].texCoord.x / tri[0].ndcSpacePos.w, tri[0].texCoord.y / tri[0].ndcSpacePos.w, 0.0f); // Assuming Coord2D is vec2 like
    glm::vec3 texCoord1_div_w = glm::vec3(tri[1].texCoord.x / tri[1].ndcSpacePos.w, tri[1].texCoord.y / tri[1].ndcSpacePos.w, 0.0f);
    glm::vec3 texCoord2_div_w = glm::vec3(tri[2].texCoord.x / tri[2].ndcSpacePos.w, tri[2].texCoord.y / tri[2].ndcSpacePos.w, 0.0f);
    frag_simd.texCoord = calculateInterpolationSimdVector3D(texCoord0_div_w, texCoord1_div_w, texCoord2_div_w, barycentric_simd);
    // 法线 / w 插值
    glm::vec3 normal0_div_w = tri[0].normal / tri[0].ndcSpacePos.w;
    glm::vec3 normal1_div_w = tri[1].normal / tri[1].ndcSpacePos.w;
    glm::vec3 normal2_div_w = tri[2].normal / tri[2].ndcSpacePos.w;
    frag_simd.normal = calculateInterpolationSimdVector3D(normal0_div_w, normal1_div_w, normal2_div_w, barycentric_simd);
    // 世界空间位置 / w 插值
    glm::vec3 worldSpacePos0_div_w = tri[0].worldSpacePos / tri[0].ndcSpacePos.w;
    glm::vec3 worldSpacePos1_div_w = tri[1].worldSpacePos / tri[1].ndcSpacePos.w;
    glm::vec3 worldSpacePos2_div_w = tri[2].worldSpacePos / tri[2].ndcSpacePos.w;
    frag_simd.worldSpacePos = calculateInterpolationSimdVector3D(worldSpacePos0_div_w, worldSpacePos1_div_w, worldSpacePos2_div_w, barycentric_simd);

    return frag_simd;
}

#endif // HELPERFUNCTION_H
