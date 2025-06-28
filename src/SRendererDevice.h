#ifndef SRENDERERDEVICE_H
#define SRENDERERDEVICE_H

#include <vector>
#include <memory>
#include <mutex>
#include <future>
#include <atomic>
#include <optional>
#include <immintrin.h>
<<<<<<< HEAD
=======
#include "tbb/parallel_for.h"
#include "tbb/blocked_range3d.h"
#include "tbb/parallel_for_each.h"
>>>>>>> future
#include "Shader.h"
#include "Texture.h"
#include "threadpool.h"
#include "SRFrameBuffer.h"
#include "BasicDataStructure.h"

<<<<<<< HEAD

=======
>>>>>>> future
struct EdgeEquation //三角形(中某点)对应的边缘方程
{
    VectorI3D m_i; // x方向上每移动一个单位距离，边界函数的增量
    VectorI3D m_j; // y方向上每移动一个单位距离，边界函数的增量
    VectorI3D m_k; // 边界函数的常数项
    bool m_topLeftFlag[3]; // 左上角规则标志(用于处理共享边，避免重绘和漏绘)
    int  m_twoArea;   // 三角形双倍面积
    float m_delta;    // twoArea的倒数，方便得到重心坐标

    EdgeEquation(const Triangle& tri);
    VectorI3D getResult(const int& x, const int& y);
    void upX(VectorI3D& res);
    void upY(VectorI3D& res);
    Vector3D getBarycentric(const VectorI3D& val); // 返回重心坐标
};

//SIMD
//=====================================================================

struct EdgeEquationSimd
{
    __m256i m_i_simd[3]; // x方向上的增量，存储3个边缘的8个增量
    __m256i m_j_simd[3]; // y方向上的增量，存储3个边缘的8个增量
    __m256i m_k_simd[3]; // 常数项，存储3个边缘的8个常数项
    __m256  m_delta_simd; // twoArea的倒数，复制到8个浮点数中
    bool    m_topLeftFlag[3]; // 左上角规则标志 (与非SIMD版本相同)
    int     m_twoArea;      // 三角形双倍面积 (与非SIMD版本相同)

    EdgeEquationSimd(const Triangle& tri);
    SimdVectorI3D getResultSimd(const __m256i& x_simd, const __m256i& y_simd);
    SimdVector3D getBarycentricSimd(const SimdVectorI3D& val_simd);
    __m256i judgeInsideTriangleSimd(const SimdVectorI3D& edge_values_simd);
};

class Shader;

<<<<<<< HEAD

class SRendererDevice
{
public:
    RendererMode m_rendererMode;
    bool m_faceCulling; // 面剔除
    bool m_multiThread; // 多线程加速
    bool m_simd;
    std::vector<Vertex> m_vertexList; // 存储模型顶点
    std::vector<unsigned> m_indices;  // 存储模型顶点的绘制顺序
    std::vector<Texture> m_textureList; // 存储每一帧图片
=======
class SRendererDevice
{
public:

    RendererMode m_rendererMode;
    bool m_faceCulling;
    bool m_multiThread;
    bool m_tbbThread;
    bool m_simd;
    bool m_useFXAA;
    std::vector<Vertex> m_vertexList; // 存储模型顶点
    std::vector<unsigned> m_indices;  // 存储模型顶点的绘制顺序
    std::vector<Texture> m_textureList; // 存储每
>>>>>>> future
    std::unique_ptr<Shader> m_shader;  // 着色方式
    Color m_clearColor;
    Color m_pointColor;
    Color m_lineColor;

    SRendererDevice(int wide, int height);
    ~SRendererDevice();
    void clearBuffer();
    QImage& getBuffer();
    bool saveImage(QString path);
    void render();
    static void init(int& wide, int& height);
    static SRendererDevice& getInstance(int wide = 0, int height = 0); // 获取简单的实例，用于外部调用
<<<<<<< HEAD
=======
    SRFrameBuffer& getFrameBuffer();
>>>>>>> future

    //ban
    SRendererDevice(const SRendererDevice&) = delete;
    SRendererDevice(SRendererDevice&&) = delete;
    SRendererDevice& operator=(const SRendererDevice&) = delete;
    SRendererDevice& operator=(SRendererDevice&) = delete;

private:
    int m_wide;
    int m_height;
    std::mutex m_depthBufferMutexes;
    std::array<BorderPlane, 6> m_viewPlanes; //视景体(用于判定渲染范围和面剔除)
    std::array<BorderLine, 4> m_screenLines;
    SRFrameBuffer m_frameBuffer;
    std::unique_ptr<ThreadPool> m_threadPool;

    void processTriangle(Triangle& tri);  //处理三角形
    void rasterizationTriangle(Triangle& tri); //光栅化三角形
    void wireFrameTriangle(Triangle& tri); //绘制线框三角形
    void pointTriangle(Triangle& tri); //绘制点三角形
    void drawLine(Line& line); //绘制线段
    void convertToScreen(Triangle& tri); //转换为屏幕坐标
    void executePerspectiveDivision(Triangle& tri); // 透视除法
    CoordI4D getBoundingBox(Triangle& tri); //算出三角形包围盒
    std::vector<Triangle> clipTriangle(Triangle& tri); // 剪裁三角形
    std::optional<Line> clipLine(Line& line); //剪裁线
<<<<<<< HEAD
    // Color antiAliasing(CoordI2D &coord);
=======
    void extractFragmentData();
>>>>>>> future

    //SIMD
    void rasterizationTriangleSimd(Triangle& tri);
};

#endif // SRENDERERDEVICE_H
