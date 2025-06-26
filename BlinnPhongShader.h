#ifndef BLINNPHONGSHADER_H
#define BLINNPHONGSHADER_H

#include <iostream>
#include "Shader.h"
#include "Texture.h"

extern bool SHADERTEXTURE;

class BlinnPhongShader : public Shader
{
public:
    void FXAAShader(QImage& image, float edgeThresshold, float subpixBlendStrength, float lumaThresholdMin)override;
    void simdFXAAShader(QImage& image, int wide, int height, float edgeThresshold)override;
    void vertexShader(Vertex& vertex) override;
    void fragmentShader(Fragment& fragment) override;
    void fragmentShaderSIMD(SimdFragment& frag_simd, __m256& final_mask)override;
};

#endif // BLINNPHONGSHADER_H
