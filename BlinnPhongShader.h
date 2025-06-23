#ifndef BLINNPHONGSHADER_H
#define BLINNPHONGSHADER_H

#include "Shader.h"
#include "Texture.h"

extern bool SHADERTEXTURE;

class BlinnPhongShader : public Shader
{
public:
    bool m_useLight{true};
    void vertexShader(Vertex& vertex) override;
    void fragmentShader(Fragment& fragment) override;
    void fragmentShaderSIMD(SimdFragment &frag_simd)override;
};

#endif // BLINNPHONGSHADER_H
