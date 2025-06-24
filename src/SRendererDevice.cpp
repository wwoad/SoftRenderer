#include "SRendererDevice.h"
#include "HelperFunction.h"

EdgeEquation::EdgeEquation(const Triangle& tri)
{
    // https://zhuanlan.zhihu.com/p/140926917
    // 初始化三角形边缘函数的增量 i,j 和常数项 K
    m_i = {
            tri[0].screenPos.y - tri[1].screenPos.y,
            tri[1].screenPos.y - tri[2].screenPos.y,
            tri[2].screenPos.y - tri[0].screenPos.y};
    m_j = {
            tri[1].screenPos.x - tri[0].screenPos.x,
            tri[2].screenPos.x - tri[1].screenPos.x,
            tri[0].screenPos.x - tri[2].screenPos.x};
    m_k = {
            tri[0].screenPos.x * tri[1].screenPos.y - tri[0].screenPos.y * tri[1].screenPos.x,
            tri[1].screenPos.x * tri[2].screenPos.y - tri[1].screenPos.y * tri[2].screenPos.x,
            tri[2].screenPos.x * tri[0].screenPos.y - tri[2].screenPos.y * tri[0].screenPos.x};

    m_topLeftFlag[0] = judgeOnTopLeftEdge(tri[0].screenPos, tri[1].screenPos);
    m_topLeftFlag[1] = judgeOnTopLeftEdge(tri[1].screenPos, tri[2].screenPos);
    m_topLeftFlag[2] = judgeOnTopLeftEdge(tri[2].screenPos, tri[0].screenPos);

    m_twoArea = m_k[0] + m_k[1] + m_k[2]; // 三角形边界函数的三个常数项相加为原三角形面积的两倍
    m_delta = 1.f / m_twoArea;
}

VectorI3D EdgeEquation::getResult(const int& x, const int& y) // 得到对应点(此处为包围盒的起点)的边缘方程的初始值
{
    VectorI3D res = m_i * x + m_j * y + m_k;

    return res;
}

void EdgeEquation::upX(VectorI3D& res) // X方向增加后，边缘方程增量后的结果
{
    res += m_i;
}

void EdgeEquation::upY(VectorI3D& res) // Y方向增加后，边缘方程增量后的结果
{
    res += m_j;
}

Vector3D EdgeEquation::getBarycentric(const VectorI3D& val) // 传入(某点)所在的三角形边界函数的常数项，返回该点的重心坐标
{
    return {val.y * m_delta, val.z * m_delta, val.x * m_delta}; // 返回重心坐标，用于插值
}

EdgeEquationSimd::EdgeEquationSimd(const Triangle& tri)
{
    // // 初始化增量 i, j 和常数项 k
    m_i_simd[0] = _mm256_set1_epi32(tri[0].screenPos.y - tri[1].screenPos.y);
    m_i_simd[1] = _mm256_set1_epi32(tri[1].screenPos.y - tri[2].screenPos.y);
    m_i_simd[2] = _mm256_set1_epi32(tri[2].screenPos.y - tri[0].screenPos.y);

    m_j_simd[0] = _mm256_set1_epi32(tri[1].screenPos.x - tri[0].screenPos.x);
    m_j_simd[1] = _mm256_set1_epi32(tri[2].screenPos.x - tri[1].screenPos.x);
    m_j_simd[2] = _mm256_set1_epi32(tri[0].screenPos.x - tri[2].screenPos.x);

    // 直接计算原始整数常数项
    int k0 = tri[0].screenPos.x * tri[1].screenPos.y - tri[0].screenPos.y * tri[1].screenPos.x;
    int k1 = tri[1].screenPos.x * tri[2].screenPos.y - tri[1].screenPos.y * tri[2].screenPos.x;
    int k2 = tri[2].screenPos.x * tri[0].screenPos.y - tri[2].screenPos.y * tri[0].screenPos.x;

    m_k_simd[0] = _mm256_set1_epi32(k0);
    m_k_simd[1] = _mm256_set1_epi32(k1);
    m_k_simd[2] = _mm256_set1_epi32(k2);

    // 左上角规则标志
    m_topLeftFlag[0] = judgeOnTopLeftEdge(tri[0].screenPos, tri[1].screenPos);
    m_topLeftFlag[1] = judgeOnTopLeftEdge(tri[1].screenPos, tri[2].screenPos);
    m_topLeftFlag[2] = judgeOnTopLeftEdge(tri[2].screenPos, tri[0].screenPos);

    // 边缘函数的常数项相加为原三角形面积的两倍
    m_twoArea = k0 + k1 + k2;
    m_delta_simd = _mm256_set1_ps(1.f / m_twoArea); // 将delta复制到8个浮点数中
}

SimdVectorI3D EdgeEquationSimd::getResultSimd(const __m256i& x_simd, const __m256i& y_simd)
{
    //VectorI3D res = m_i * x + m_j * y + m_k;

    SimdVectorI3D res;
    res.x = _mm256_add_epi32(_mm256_mullo_epi32(m_i_simd[0], x_simd), _mm256_mullo_epi32(m_j_simd[0], y_simd));
    res.x = _mm256_add_epi32(res.x, m_k_simd[0]);
    res.y = _mm256_add_epi32(_mm256_mullo_epi32(m_i_simd[1], x_simd), _mm256_mullo_epi32(m_j_simd[1], y_simd));
    res.y = _mm256_add_epi32(res.y, m_k_simd[1]);
    res.z = _mm256_add_epi32(_mm256_mullo_epi32(m_i_simd[2], x_simd), _mm256_mullo_epi32(m_j_simd[2], y_simd));
    res.z = _mm256_add_epi32(res.z, m_k_simd[2]);

    return res;
}

//一次处理8个像素的重心坐标
SimdVector3D EdgeEquationSimd::getBarycentricSimd(const SimdVectorI3D& val_simd)
{
    SimdVector3D barycentric;
    // 重心坐标 (alpha, beta, gamma) = (val1 * delta, val2 * delta, val0 * delta)
    // 将边缘方程的整型结果转换为浮点数
    __m256 val0_ps = _mm256_cvtepi32_ps(val_simd.x);
    __m256 val1_ps = _mm256_cvtepi32_ps(val_simd.y);
    __m256 val2_ps = _mm256_cvtepi32_ps(val_simd.z);

    // 计算重心坐标
    barycentric.x = _mm256_mul_ps(val1_ps, m_delta_simd); // alpha
    barycentric.y = _mm256_mul_ps(val2_ps, m_delta_simd); // beta
    barycentric.z = _mm256_mul_ps(val0_ps, m_delta_simd); // gamma

    return barycentric;
}

__m256i EdgeEquationSimd::judgeInsideTriangleSimd(const SimdVectorI3D& edge_values_simd)
{
    // 判断 val >= 0 (即 val > -1)
    __m256i edge0_ge_0 = _mm256_cmpgt_epi32(edge_values_simd.x, _mm256_set1_epi32(-1));
    __m256i edge1_ge_0 = _mm256_cmpgt_epi32(edge_values_simd.y, _mm256_set1_epi32(-1));
    __m256i edge2_ge_0 = _mm256_cmpgt_epi32(edge_values_simd.z, _mm256_set1_epi32(-1));
    __m256i all_ge_0_mask = _mm256_and_si256(edge0_ge_0, edge1_ge_0);
    all_ge_0_mask = _mm256_and_si256(all_ge_0_mask, edge2_ge_0);

    // 1. 判断 val < 0
    __m256i edge0_lt_0 = _mm256_cmpgt_epi32(_mm256_setzero_si256(), edge_values_simd.x); // 0 > val => val < 0
    __m256i edge1_lt_0 = _mm256_cmpgt_epi32(_mm256_setzero_si256(), edge_values_simd.y);
    __m256i edge2_lt_0 = _mm256_cmpgt_epi32(_mm256_setzero_si256(), edge_values_simd.z);

    // 2. 判断 val == 0 (这个在后面左上角规则也会用到，可以提前算好)
    __m256i edge0_is_0 = _mm256_cmpeq_epi32(edge_values_simd.x, _mm256_setzero_si256());
    __m256i edge1_is_0 = _mm256_cmpeq_epi32(edge_values_simd.y, _mm256_setzero_si256());
    __m256i edge2_is_0 = _mm256_cmpeq_epi32(edge_values_simd.z, _mm256_setzero_si256());

    // 3. 组合 val <= 0 (val < 0 || val == 0)
    __m256i edge0_le_0 = _mm256_or_si256(edge0_lt_0, edge0_is_0);
    __m256i edge1_le_0 = _mm256_or_si256(edge1_lt_0, edge1_is_0);
    __m256i edge2_le_0 = _mm256_or_si256(edge2_lt_0, edge2_is_0);

    __m256i all_le_0_mask = _mm256_and_si256(edge0_le_0, edge1_le_0);
    all_le_0_mask = _mm256_and_si256(all_le_0_mask, edge2_le_0); // 掩码表示所有边缘值都 <= 0
    // inside_base_mask = (all_ge_0_mask) || (all_le_0_mask)
    __m256i inside_base_mask = _mm256_or_si256(all_ge_0_mask, all_le_0_mask);

    // 构建一个排除掩码：如果 edge_value == 0 AND 对应的 m_topLeftFlag 为 false，则该像素应该被排除
    __m256i exclude_mask = _mm256_setzero_si256();
    // 边缘 0
    if (!m_topLeftFlag[0]) { // 如果边缘0不是左上角规则的边
        exclude_mask = _mm256_or_si256(exclude_mask, edge0_is_0); // 将边缘值为0的像素标记为排除
    }
    // 边缘 1
    if (!m_topLeftFlag[1]) {
        exclude_mask = _mm256_or_si256(exclude_mask, edge1_is_0);
    }
    // 边缘 2
    if (!m_topLeftFlag[2]) {
        exclude_mask = _mm256_or_si256(exclude_mask, edge2_is_0);
    }

    // 最终的内部掩码：inside_base_mask AND (NOT exclude_mask)
    // (NOT exclude_mask) 等价于 all_ones XOR exclude_mask
    __m256i all_ones = _mm256_set1_epi32(0xFFFFFFFF);
    __m256i not_exclude_mask = _mm256_xor_si256(exclude_mask, all_ones);
    __m256i final_inside_mask_simd = _mm256_and_si256(inside_base_mask, not_exclude_mask);

    return final_inside_mask_simd;
}

//--------------------------------------------------------------
// SSRendererDevice public
SRendererDevice::SRendererDevice(int wide, int height)
    :m_shader(nullptr)
    ,m_clearColor(0.5)
    ,m_pointColor(1.0f)
    ,m_lineColor(1.0f)
    ,m_wide(wide)
    ,m_height(height)
    ,m_threadPool(nullptr)
    ,m_frameBuffer(wide, height)
    ,m_rendererMode(RendererMode::Mesh)
    ,m_faceCulling(true)
    ,m_multiThread(true)
    ,m_tbbThread(false)
    ,m_simd(true)
{
    { // 设置视景体为重心在 (0,0,0) 的 1*1*1立方体
        // near
        m_viewPlanes[0] = {0, 0, 1.f, 1.f};
        // far
        m_viewPlanes[1] = {0, 0, -1.f, 1.f};
        // left
        m_viewPlanes[2] = {1.f, 0, 0, 1.f};
        //right
        m_viewPlanes[3] = {-1.f, 0, 0, 1.f};
        // top
        m_viewPlanes[4] = {0, 1.f, 0, 1.f};
        //bottom
        m_viewPlanes[5] = {0, -1.f, 0, 1.f};
    }
    // 设置屏幕范围为width和height
    { //set screen
        // left
        m_screenLines[0] = {1.f, 0, 0};
        // right
        m_screenLines[1] = {-1.f, 0, static_cast<float>(wide)};// (法向量(x,y) +  X偏置)设置可渲染的屏幕宽度
        // bottom
        m_screenLines[2] = {0, 1.f, 0};
        // top
        m_screenLines[3] = {0, -1.f, static_cast<float>(height)}; //（法向量(x,y) + Y偏置）设置可渲染的屏幕高度
    }
    m_threadPool = std::make_unique<ThreadPool>(13, 13);
}

SRendererDevice::~SRendererDevice()
{
}

void SRendererDevice::clearBuffer()
{
    m_frameBuffer.clearBuffer(m_clearColor);
}

QImage& SRendererDevice::getBuffer() // 返回当前帧缓冲内的快照Colorbuffer
{
    return m_frameBuffer.getImage();
}

bool SRendererDevice::saveImage(QString path) // 将当前帧缓冲的快照保存到对应路径
{
    std::cout << "it is  SRendererDevice::saveImage" << std::endl;
    return m_frameBuffer.saveImage(path);
}

void SRendererDevice::render() // 渲染入口
{
    std::vector<Triangle> triangleList;
    for(int i = 0; i < m_indices.size(); i += 3){
        assert(i + 1 < m_indices.size() && i + 2 < m_indices.size());
        triangleList.push_back({
            m_vertexList.at(m_indices.at(i)),
            m_vertexList.at(m_indices.at(i + 1)),
            m_vertexList.at(m_indices.at(i + 2))});
    }

    // 多线程加速入口
    if(m_multiThread){
        //将模型进行分块加载
        const int threadCount = m_threadPool->getThreadNum(); // 得到最大线程数量
        const int chunkSize = triangleList.size() / threadCount; //得到块的大小
        std::vector<std::future<void>> futures;
        futures.reserve(threadCount);

        for(int t = 0; t < threadCount; t++){
            int start = t * chunkSize;
            int end = (t == threadCount - 1) ? (triangleList.size()) : (start + chunkSize);
            futures.push_back(m_threadPool->addTask([this, start, end, &triangleList](){
                for(int i = start; i < end; i++){
                    this->processTriangle(triangleList[i]);
                }
            }));
        }
        for(auto& future : futures){
            future.get();
        }

        // tbb::parallel_for(tbb::blocked_range<size_t>(0, triangleList.size()),
        //                   [&](tbb::blocked_range<size_t> r)
        //                   {
        //                       for(size_t i = r.begin(); i < r.end(); i++)
        //                           processTriangle(triangleList[i]);
        //                   });
    }
    else // 非多线程入口
    {
        for(int i = 0; i < triangleList.size(); i++){
            processTriangle(triangleList[i]);
        }
    }
}

void SRendererDevice::init(int& wide, int& height)
{
    getInstance(wide, height);
}

SRendererDevice& SRendererDevice::getInstance(int wide, int height) // 获取一个RenderDevice静态实例，用于其他类方法的调用
{
    static SRendererDevice Instance(wide, height);
    return Instance;
}
//------------------------------------------
// private
void SRendererDevice::processTriangle(Triangle& tri) // 处理传入的三角形
{
    for(int i = 0; i< 3; i++) // 遍历三角形的顶点
    {
        m_shader->vertexShader(tri[i]);  // 对顶点应用顶点处理(变换)
    }

    if(m_faceCulling){
        std::vector<Triangle> completedTriangleList = clipTriangle(tri); // 剪裁三角形

        for (auto &ctri : completedTriangleList){
            executePerspectiveDivision(ctri); // 透视除法
            convertToScreen(ctri); // 转换为屏幕坐标
            if(m_rendererMode == RendererMode::Rasterization) // 应用光栅化
            {
                rasterizationTriangle(ctri);
            }
            else if(m_rendererMode == RendererMode::Mesh) // 仅画出线框图
            {
                wireFrameTriangle(ctri);
            }
            else if(m_rendererMode == RendererMode::VERTEX) // 仅画出顶点图
            {
                pointTriangle(ctri);
            }
        }
    }else{
        executePerspectiveDivision(tri); // 透视除法
        convertToScreen(tri); // 转换为屏幕坐标
        if(m_rendererMode == RendererMode::Rasterization) // 应用光栅化
        {
            rasterizationTriangle(tri);
        }
        else if(m_rendererMode == RendererMode::Mesh) // 仅画出线框图
        {
            wireFrameTriangle(tri);
        }
        else if(m_rendererMode == RendererMode::VERTEX) // 仅画出顶点图
        {
            pointTriangle(tri);
        }
    }
}

void SRendererDevice::rasterizationTriangle(Triangle& tri) // 光栅化三角形
{
    EdgeEquation triEdge(tri);
    if(m_faceCulling && triEdge.m_twoArea <= 0) // 若三角形非法(不存在)直接返回
    {
        return;
    }
    if(triEdge.m_twoArea == 0) // 若三角形为一条线直接返回
    {
        return;
    }

    // SIMD分支
    if(m_simd){rasterizationTriangleSimd(tri); return;}

    CoordI4D boundingBox = getBoundingBox(tri); // 求三角形的包围盒
    int xMin = std::max(0, boundingBox[0]);
    int yMin = std::max(0, boundingBox[1]);
    int xMax = std::min(m_wide - 1, boundingBox[2]);
    int yMax = std::min(m_height - 1, boundingBox[3]);

    Fragment frag;
    bool flag = false;// 是否进入三角形的标志
    VectorI3D cy = triEdge.getResult(xMin, yMin); // 得到(xMin,yMin)即包围盒左上方顶点的对于三角形的边缘方程初始值
    for(int y = yMin; y <= yMax; y++) // 向屏幕下方开始遍历
    {
        flag = false;
        VectorI3D cx = cy;
        for(int x = xMin; x <= xMax; x++){
            // 判断遍历的点是否在三角形内
            if(judgeInsideTriangle(triEdge, cx)){
                flag = true; // 进入三角形后置 1
                Vector3D bartcenTri = triEdge.getBarycentric(cx); // 得到该点的重心坐标用于插值
                float screenDepth = calculateInterpolation<float>(tri[0].screenDepth, tri[1].screenDepth, tri[2].screenDepth, bartcenTri); // 对深度进行插值

                if(m_frameBuffer.judgeDepth(x, y, screenDepth)) // 对该点进行深度测试，若成功更新深度则绘制该点
                {
                    float bartcen1 = bartcenTri.x / tri[0].ndcSpacePos.w;
                    float bartcen2 = bartcenTri.y / tri[1].ndcSpacePos.w;
                    float bartcen3 = bartcenTri.z / tri[2].ndcSpacePos.w;
                    float bartcen = bartcen1 + bartcen2 + bartcen3;

                    float viewDepth = 1.f / bartcen;// 计算深度插值
                    frag = constructFragment(x, y, screenDepth, viewDepth, tri, bartcenTri); // 构造着色点
                    m_shader->fragmentShader(frag); // 应用片着色
                    m_frameBuffer.setPixel(frag.screenPos.x, frag.screenPos.y, frag.fragmentColor);
                }
            }
            else if(flag){
                break; // 离开三角形，换行
            }
            triEdge.upX(cx); // X自增，边缘方程自增一定值
        }
        triEdge.upY(cy);  // Y自增，边缘方程自增一定值
    }
}

void SRendererDevice::wireFrameTriangle(Triangle& tri) // 画线框三角形
{
    Line triLine[3] =
        {
            {tri[0].screenPos, tri[1].screenPos},
            {tri[1].screenPos, tri[2].screenPos},
            {tri[2].screenPos, tri[0].screenPos}
        };
    for(auto& line: triLine)
    {
        auto res = clipLine(line);
        if(res)
        {
            drawLine(*res);
        }
    }
}

void SRendererDevice::pointTriangle(Triangle& tri) // 画三角形顶点
{
    for(int i = 0; i < 3; i++)
    {
        if(tri[i].screenPos.x >= 0 &&
            tri[i].screenPos.x <= m_wide - 1 &&
            tri[i].screenPos.y >= 0 &&
            tri[i].screenPos.y <= m_height - 1 &&
            tri[i].screenDepth <= 1.f)
        {
            m_frameBuffer.setPixel(tri[i].screenPos.x, tri[i].screenPos.y, m_pointColor);
        }
    }
}

void SRendererDevice::drawLine(Line& line)
{
    int x0 = glm::clamp(static_cast<int>(line[0].x), 0, m_wide - 1);
    int x1 = glm::clamp(static_cast<int>(line[1].x), 0, m_wide - 1);
    int y0 = glm::clamp(static_cast<int>(line[0].y), 0, m_height - 1);
    int y1 = glm::clamp(static_cast<int>(line[1].y), 0, m_height - 1);

    bool steep = false;
    if(abs(x0 - x1) < abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if(x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    int k = dy > 0 ? 1 : -1;

    if(dy < 0)
    {
        dy = - dy;
    }
    float e = -dx;
    int x = x0;
    int y = y0;
    while(x != x1)
    {
        if(steep)
        {
            m_frameBuffer.setPixel(y, x, m_lineColor);
        }
        else
        {
            m_frameBuffer.setPixel(x, y, m_lineColor);
        }

        e += (2 * dy);

        if(e > 0)
        {
            y += k;
            e -= (2 * dx);
        }
        ++x;
    }
}

void SRendererDevice::convertToScreen(Triangle& tri) // 转换为屏幕坐标
{
    for(int i = 0; i < 3; i++)
    {
        tri[i].screenPos.x = static_cast<int>(0.5f * m_wide * (tri[i].ndcSpacePos.x + 1.f) + 0.5f);
        tri[i].screenPos.y = static_cast<int>(0.5f * m_height * (tri[i].ndcSpacePos.y + 1.f) + 0.5f);
        tri[i].screenDepth = tri[i].ndcSpacePos.z;
    }
}

void SRendererDevice::executePerspectiveDivision(Triangle& tri) //透视除法，将透视投影转换为正交投影
{
    for(int i = 0; i < 3; i++)
    {
        tri[i].ndcSpacePos.x /= tri[i].clipSpacePos.w;
        tri[i].ndcSpacePos.y /= tri[i].clipSpacePos.w;
        tri[i].ndcSpacePos.z /= tri[i].clipSpacePos.w;
    }

}

CoordI4D SRendererDevice::getBoundingBox(Triangle& tri) // 求三角形包围盒
{
    int xMin = m_wide - 1;
    int yMin = m_height - 1;
    int xMax = 0;
    int yMax = 0;
    for(int i = 0; i < 3; i++)
    {
        xMin = std::min(xMin, tri[i].screenPos.x);
        yMin = std::min(yMin, tri[i].screenPos.y);
        xMax = std::max(xMax, tri[i].screenPos.x);
        yMax = std::max(yMax, tri[i].screenPos.y);
    }
    return
    {
        xMin > 0 ? xMin : 0,
        yMin > 0 ? yMin : 0,
        xMax > m_wide - 1 ? xMax : m_wide - 1,
        yMax > m_height - 1 ? yMax : m_height -1
    };
}

std::vector<Triangle> SRendererDevice::clipTriangle(Triangle& tri) // 剪裁三角形
{
    std::bitset<6> code[3] =
        {
         getClipCode(tri[0].clipSpacePos, m_viewPlanes),
         getClipCode(tri[1].clipSpacePos, m_viewPlanes),
         getClipCode(tri[2].clipSpacePos, m_viewPlanes)
        };
    // 如果三角形全在视体内，直接返回
    if ((code[0] | code[1] | code[2]).none()){
        return {tri};
    }
    // 如果全不在视体内，则返回一个空三角形(不渲染)
    if ((code[0] & code[1] & code[2]).any()){
        return {};
    }
    if (((code[0] ^ code[1])[0]) || ((code[1] ^ code[2])[0]) || ((code[2] ^ code[0])[0])) // intersects near plane
    {
        std::vector<Vertex> res;
        for (int i = 0; i < 3; i++)
        {
            int k = (i + 1) % 3;
            if (!code[i][0] && !code[k][0])
            {
                res.push_back(tri[k]);
            }
            else if (!code[i][0] && code[k][0])
            {
                float da = calculateDistance(tri[i].clipSpacePos, m_viewPlanes[0]);
                float db = calculateDistance(tri[k].clipSpacePos, m_viewPlanes[0]);
                float alpha = da / (da - db);
                Vertex np = calculateInterpolation(tri[i], tri[k], alpha);
                res.push_back(np);
            }
            else if (code[i][0] && !code[k][0])
            {
                float da = calculateDistance(tri[i].clipSpacePos, m_viewPlanes[0]);
                float db = calculateDistance(tri[k].clipSpacePos, m_viewPlanes[0]);
                float alpha = da / (da - db);
                Vertex np = calculateInterpolation(tri[i], tri[k], alpha);
                res.push_back(np);
                res.push_back(tri[k]);
            }
        }
        return constructTriangle(res);
    }
    return std::vector<Triangle>{tri};
}

std::optional<Line> SRendererDevice::clipLine(Line& line) // 剪裁线框
{
    std::bitset<4> code[2] =
    {
        getClipCode(Coord3D(line[0], 1), m_screenLines),
        getClipCode(Coord3D(line[1], 1), m_screenLines)
    };

    if((code[0] | code[1]).none())
    {
        return line;
    }
    if((code[0] & code[1]).any())
    {
        return std::nullopt; // 有问题
    }
    for(int i = 0; i < 4; i++)
    {
        if((code[0] ^ code[1])[i])
        {
            float da = calculateDistance(Coord3D(line[0], 1), m_screenLines[i]);
            float db = calculateDistance(Coord3D(line[1], 1), m_screenLines[i]);
            float alpha = da / (da - db);
            CoordI2D np = calculateInterpolation(line[0], line[1], alpha);
            if(da > 0)
            {
                line[1] = np;
                code[1] = getClipCode(Coord3D(np, 1), m_screenLines);
            }
            else
            {
                line[0] = np;
                code[0] = getClipCode(Coord3D(np, 1), m_screenLines);
            }
        }
    }
    return line;
}

void SRendererDevice::rasterizationTriangleSimd(Triangle& tri)
{
    EdgeEquationSimd triEdgeSimd(tri);

    CoordI4D boundingBox = getBoundingBox(tri); // 求三角形的包围盒
    int xMin = std::max(0, boundingBox[0]);
    int yMin = std::max(0, boundingBox[1]);
    int xMax = std::min(m_wide - 1, boundingBox[2]);
    int yMax = std::min(m_height - 1, boundingBox[3]);

    // 在x坐标以8个像素为单位遍历包围盒
    for(int y = yMin; y <= yMax; ++y)
    {
        __m256i y_simd = _mm256_set1_epi32(y);// 初始化8个像素的y坐标SIMD向量 (都是当前行的y)
        for(int x_start = xMin; x_start <= xMax; x_start += 8)
        {
            // 生成8个像素的x坐标SIMD向量
            __m256i x_simd = _mm256_setr_epi32(x_start, x_start + 1, x_start + 2, x_start + 3, x_start + 4, x_start + 5, x_start + 6, x_start + 7);

            // 1. 计算8个像素的边缘方程值
            SimdVectorI3D edge_values_simd = triEdgeSimd.getResultSimd(x_simd, y_simd);

            // 2. 判断8个像素是否在三角形内部
            __m256i inside_mask_simd = triEdgeSimd.judgeInsideTriangleSimd(edge_values_simd);

            // 处理包围盒右边界：创建一个掩码，只包含在 [xMin, xMax] 范围内的像素
            __m256i x_coords_simd = _mm256_add_epi32(_mm256_set1_epi32(x_start), _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7));
            __m256i x_in_bounds_mask = _mm256_and_si256(
                _mm256_cmpgt_epi32(x_coords_simd, _mm256_set1_epi32(xMin - 1)), // x >= xMin
                _mm256_cmpgt_epi32(_mm256_set1_epi32(xMax + 1), x_coords_simd)  // x <= xMax
                );
            // 最终的内部掩码还需要与包围盒边界掩码求与
            inside_mask_simd = _mm256_and_si256(inside_mask_simd, x_in_bounds_mask);
            // 将整型掩码转换为浮点型掩码
            __m256 inside_mask_ps = _mm256_castsi256_ps(inside_mask_simd);

            int inside_mask_int = _mm256_movemask_ps(inside_mask_ps);
            if (inside_mask_int == 0) {
                // 如果这个8像素块没有任何像素在三角形内部（且符合规则和边界），直接跳过后续处理
                continue;
            }

            // 3. 计算通过内部测试的像素的重心坐标
            SimdVector3D barycentric_simd = triEdgeSimd.getBarycentricSimd(edge_values_simd);

            // 4. SIMD 属性插值 (仅对通过内部测试的像素进行有效插值)
            // 使用 blend 指令根据 inside_mask_ps 屏蔽插值结果
            // 无效像素的插值结果将设置为一个可以被后续处理忽略的值
            __m256 screenDepth_simd_interp = calculateInterpolationSimdFloat(tri[0].screenDepth, tri[1].screenDepth, tri[2].screenDepth, barycentric_simd);

            //构造片元
            SimdFragment fragment_simd = constructFragmentSimd(x_simd, y_simd, screenDepth_simd_interp, barycentric_simd, tri);

            // 5. SIMD 深度测试
            __m256 depth_test_mask_ps = m_frameBuffer.judgeDepthSimd(inside_mask_ps, fragment_simd.screenPosX, fragment_simd.screenPosY, fragment_simd.screenDepth);

            // 6. 合并掩码：只有同时在三角形内部且通过深度测试的像素才会被绘制
            __m256 final_mask_ps = _mm256_and_ps(inside_mask_ps, depth_test_mask_ps);

            // 检查是否有任何像素通过了所有测试
            int mask_int = _mm256_movemask_ps(final_mask_ps);
            if(mask_int != 0){
                // === Fallback 到非 SIMD 片元着色和像素写入 ===
                // ** 提取所有 SIMD 向量数据到数组 **
                int screenPosX_arr[8], screenPosY_arr[8];
                float screenDepth_arr[8];
                float w_reciprocal_arr[8];
                float texCoord_div_w_x_arr[8], texCoord_div_w_y_arr[8], texCoord_div_w_z_arr[8];
                float normal_div_w_x_arr[8], normal_div_w_y_arr[8], normal_div_w_z_arr[8];
                float worldSpacePos_div_w_x_arr[8], worldSpacePos_div_w_y_arr[8], worldSpacePos_div_w_z_arr[8];
                // ** 替换 _mm256_storeu_epi32 的部分：手动提取整型元素 **
                __m128i screenPosX_low = _mm256_extractf128_si256(fragment_simd.screenPosX, 0); // 提取低 128 位 (前4个元素)
                __m128i screenPosX_high = _mm256_extractf128_si256(fragment_simd.screenPosX, 1); // 提取高 128 位 (后4个元素)
                __m128i screenPosY_low = _mm256_extractf128_si256(fragment_simd.screenPosY, 0); // 提取低 128 位
                __m128i screenPosY_high = _mm256_extractf128_si256(fragment_simd.screenPosY, 1); // 提取高 128 位
                for(int i = 0; i < 4; ++i) {
                    screenPosX_arr[i] = _mm_extract_epi32(screenPosX_low, i); // 从 128 位向量中提取 32 位整数
                    screenPosY_arr[i] = _mm_extract_epi32(screenPosY_low, i);
                }
                for(int i = 0; i < 4; ++i) {
                    screenPosX_arr[i + 4] = _mm_extract_epi32(screenPosX_high, i); // 从 128 位向量中提取 32 位整数
                    screenPosY_arr[i + 4] = _mm_extract_epi32(screenPosY_high, i);
                }
                // ** 替换结束 **
                // ** 使用 _mm256_storeu_ps 提取浮点数据 ** (这部分没有报错，保留)
                _mm256_storeu_ps(screenDepth_arr, fragment_simd.screenDepth);
                _mm256_storeu_ps(w_reciprocal_arr, fragment_simd.viewDepth);
                _mm256_storeu_ps(texCoord_div_w_x_arr, fragment_simd.texCoord.x);
                _mm256_storeu_ps(texCoord_div_w_y_arr, fragment_simd.texCoord.y);
                _mm256_storeu_ps(texCoord_div_w_z_arr, fragment_simd.texCoord.z);
                _mm256_storeu_ps(normal_div_w_x_arr, fragment_simd.normal.x);
                _mm256_storeu_ps(normal_div_w_y_arr, fragment_simd.normal.y);
                _mm256_storeu_ps(normal_div_w_z_arr, fragment_simd.normal.z);
                _mm256_storeu_ps(worldSpacePos_div_w_x_arr, fragment_simd.worldSpacePos.x);
                _mm256_storeu_ps(worldSpacePos_div_w_y_arr, fragment_simd.worldSpacePos.y);
                _mm256_storeu_ps(worldSpacePos_div_w_z_arr, fragment_simd.worldSpacePos.z);

                for (int i = 0; i < 8; ++i) {
                    if ((mask_int >> i) & 1) // 如果第i个像素通过所有测试
                    {
                        int current_x = screenPosX_arr[i];
                        int current_y = screenPosY_arr[i];
                        Fragment single_frag;
                        single_frag.screenPos = { current_x, current_y };
                        single_frag.screenDepth = screenDepth_arr[i];
                        // 应用透视校正： Attribute = ( 插值(Attribute/w) ) / ( 插值(1/w) )
                        float w_recip = w_reciprocal_arr[i]; // 插值后的 1/w
                        // 防止除以零
                        if (w_recip == 0.0f) continue;
                        single_frag.texCoord = { texCoord_div_w_x_arr[i] / w_recip, texCoord_div_w_y_arr[i] / w_recip }; // Assuming Coord2D has 2 components
                        single_frag.normal = { normal_div_w_x_arr[i] / w_recip, normal_div_w_y_arr[i] / w_recip, normal_div_w_z_arr[i] / w_recip };
                        single_frag.worldSpacePos = { worldSpacePos_div_w_x_arr[i] / w_recip, worldSpacePos_div_w_y_arr[i] / w_recip, worldSpacePos_div_w_z_arr[i] / w_recip };
                        // 调用非 SIMD 片元着色器
                        m_shader->fragmentShader(single_frag);
                        // 逐个设置像素
                        m_frameBuffer.setPixel(single_frag.screenPos.x, single_frag.screenPos.y, single_frag.fragmentColor);
                    }
                }
            }
        }
    }
}



