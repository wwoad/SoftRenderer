#ifndef SHADER_H
#define SHADER_H

#include "SRendererDevice.h"
#include "BasicDataStructure.h"

class SRendererDevice;

class Shader //虚基类，待重写
{
public:
    glm::mat4 m_modelTransformation; // 模型变换矩阵
    glm::mat4 m_viewTransformation;   // 视口变换矩阵
    glm::mat4 m_projectionTransformation; //投影变换矩阵
    std::vector<Light> m_lightList;
    Material m_material;
    Coord3D m_eyePos;

    virtual void vertexShader(Vertex& vertex) = 0;
    virtual void fragmentShader(Fragment& fragment) = 0;
    virtual void fragmentShaderSIMD(SimdFragment& frag_simd) = 0;
};

#endif // SHADER_H
