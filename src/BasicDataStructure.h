#ifndef BASICDATASTRUCTURE_H
#define BASICDATASTRUCTURE_H

#include <array>
#include <immintrin.h>
#include "glm/glm.hpp"

using BorderPlane = glm::vec4;
using BorderLine  = glm::vec3;

using Color    = glm::vec3;
using Coord2D  = glm::vec2;
using CoordI2D = glm::ivec2;
using Coord3D  = glm::vec3;
using CoordI3D = glm::ivec3;
using Coord4D  = glm::vec4;
using CoordI4D = glm::ivec4;

using Vector2D = glm::vec2;
using Vector3D = glm::vec3;
using VectorI3D = glm::ivec3;
using Vector4D = glm::vec4;
using VectorI4D = glm::ivec4;

enum class RendererMode
{
    Rasterization,
    Mesh,
    VERTEX
};

enum class RenderColorType //着色方式 面 线 点
{
    BACKGROUND,
    LINE,
    POINT
};

enum class LightColorType //光着色属性(漫反射，高光，环境光)
{
    DIFFUSE,
    SPECULAR,
    AMBIENT
};

struct Vertex //顶点
{
    Coord3D worldSpacePos;
    union //内联，共享内存，提高性能
    {
        Coord4D clipSpacePos; // 剪裁空间坐标
        Coord4D ndcSpacePos; // 标准设备空间坐标
    };
    CoordI2D screenPos;
    float screenDepth;
    Vector3D normal;
    Coord2D texCoord;
};

using Triangle = std::array<Vertex, 3>;
using Line     = std::array<CoordI2D, 2>;

struct Fragment //片
{
    Coord3D worldSpacePos;
    CoordI2D screenPos;
    float screenDepth;
    Color fragmentColor;
    Vector3D normal;
    Coord2D texCoord;
};

struct Light //光着色
{
    union
    {
        Coord4D pos;
        Vector4D dir;
    };
    Color ambient;
    Color diffuse;
    Color specular;
};

struct Material //材质属性
{
    int diffuse;
    int specular;
    float shininess;
};

//SIMD
// 用于存储8个整数向量 (例如，8个VectorI3D) 的结构体
struct SimdVectorI3D
{
    __m256i x, y, z;
};

<<<<<<< HEAD
=======
struct SimdVector2D
{
    __m256 x,y;
};

>>>>>>> future
// 用于存储8个浮点向量 (例如，8个Vector3D) 的结构体
struct SimdVector3D
{
    __m256 x, y, z;
};

struct SimdColor // 用于存储8个浮点颜色向量 (R, G, B)
{
    __m256 r, g, b;
};

struct SimdFragment
{
    __m256i screenPosX, screenPosY;
    __m256  screenDepth;
    __m256  viewDepth; // 存储插值后的 1/w (用于透视校正)
<<<<<<< HEAD
    SimdVector3D texCoord; // 存储插值后的 TexCoord/w
=======
    SimdVector2D texCoord; // 存储插值后的 TexCoord/w
>>>>>>> future
    SimdVector3D normal;   // 存储插值后的 Normal/w
    SimdVector3D worldSpacePos; // 存储插值后的 WorldSpacePos/w
    SimdColor fragmentColor; // 由 SIMD 片元着色器计算
    // 构造函数或辅助函数用于填充
};

<<<<<<< HEAD


=======
struct SimdMaterial {
    __m256 shininess;
    __m256i diffTextureIdx;
    __m256i specTextureIdx;
};
>>>>>>> future

#endif // BASICDATASTRUCTURE_H
