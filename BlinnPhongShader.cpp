#include "BlinnPhongShader.h"
#include "FunctionSIMD.h"

inline Color sampleQImage_nearest(const QImage& image, int x, int y)
{
    // int tmpX = static_cast<int>(x * image.width() - 0.5f) % image.width();
    // int tmpY = static_cast<int>(y * image.height() - 0.5f) % image.height();
    // tmpX = tmpX < 0 ? image.width() + tmpX : tmpX;
    // tmpY = tmpY < 0 ? image.height() + tmpY : tmpY;

    // QColor qc = image.pixelColor(tmpX, tmpY);
    // return Color(qc.redF(), qc.greenF(), qc.blueF());

    // 钳制坐标到图像有效范围 [0, width-1], [0, height-1]
    int clampedX = std::clamp(x, 0, image.width() - 1);
    int clampedY = std::clamp(y, 0, image.height() - 1); // QImage 的 Y 轴是自顶向下
    // 使用 QImage::pixelColor 获取像素颜色
    QColor qc = image.pixelColor(clampedX, clampedY);
    // 将 QColor 转换为 Color (glm::vec3)，并转换为浮点数 [0.0, 1.0]
    return Color(qc.redF(), qc.greenF(), qc.blueF()); // 使用 QColor 的浮点分量方法
}

inline float calculateLuma(const Color& color)
{
    return glm::dot(color, Vector3D(0.2126f, 0.7152f, 0.0722f));
}

void BlinnPhongShader::FXAAShader(QImage& image, float edgeThresshold, float subpixBlendStrength, float lumaThresholdMin)
{
    int wide = image.width();
    int height = image.height();
    for(int y = 0; y < height; ++y){
        for(int x = 0; x < wide; ++x){
            Color colorCenter = sampleQImage_nearest(image, x, y);
            Color colorUp = sampleQImage_nearest(image, x, y - 1);
            Color colordown = sampleQImage_nearest(image, x, y + 1);
            Color colorLeft = sampleQImage_nearest(image, x - 1, y);
            Color colorRight = sampleQImage_nearest(image, x + 1, y);

            float lumaCenter = calculateLuma(colorCenter);
            float lumaUp = calculateLuma(colorUp);
            float lumadown = calculateLuma(colordown);
            float lumaLeft = calculateLuma(colorLeft);
            float lumaRight = calculateLuma(colorRight);

            float maxLuma = std::max({lumaUp, lumadown, lumaLeft, lumaRight});
            float minLuma = std::min({lumaUp, lumadown, lumaLeft, lumaRight});
            float rangeLuma = maxLuma - minLuma;
            //float absLunmDiff = std::max({std::abs(lumaUp - lumaCenter), std::abs(lumadown - lumaCenter), std::abs(lumaRight - lumaCenter), std::abs(lumaLeft - lumaCenter)});

            if(rangeLuma < edgeThresshold || lumaCenter < lumaThresholdMin){
                image.setPixelColor(x, y, QColor(colorCenter.x * 255.f, colorCenter.y * 255.f, colorCenter.z * 255.f));
                continue;
            }

            float lumaHorz = std::abs(lumaUp + lumadown - 2.f * lumaCenter);
            float lumaVert = std::abs(lumaLeft + lumaRight - 2.f * lumaCenter);
            bool isHorizontal = lumaHorz >= lumaVert;
            // float edgeLuma = isHorizontal ? lumaHorz : lumaVert;
            // if(edgeLuma < edgeThresshold){
            //     image.setPixelColor(x, y, QColor(colorCenter.x * 255.f, colorCenter.y * 255.f, colorCenter.z * 255.f));
            //     std::cout << " 2 " <<std::endl;
            //     continue;
            // }

            float blendStepX = 0.f;
            float blendStepY = 0.f;
            // 根据主方向，确定应该向哪个方向采样以进行平滑
            if (isHorizontal) { // 水平边缘，沿着 Y 方向采样对侧
                blendStepY = (lumaUp > lumadown) ? -1.0f : 1.0f; // 根据 N 和 S 亮度判断方向
            } else { // 垂直边缘，沿着 X 方向采样对侧
                blendStepX = (lumaRight > lumaLeft) ? 1.0f : -1.0f; // 根据 E 和 W 亮度判断方向
            }
            // float edgeStepX = isHorizontal ? 0.f : (lumaRight > lumaLeft ? 1.f : -1.f);
            // float edgeStepY = isHorizontal ? (lumaUp > lumadown ? -1.f : 1.f) : 0.f;
            // float blendStepX = isHorizontal ? (lumaUp + lumadown > 2.f * lumaCenter ? 1.f : -1.f) : 0.f;
            // float blendStepY = isHorizontal ? 0.f : (lumaRight + lumaLeft > 2.f * lumaCenter ? 1.f : -1.f);
            if(lumaCenter < (isHorizontal ? (lumaUp + lumadown) * 0.5f : (lumaRight + lumaLeft) * 0.5f)){
                blendStepX = -blendStepX;
                blendStepY = -blendStepY;
            }

            float edgeLuma = isHorizontal ? lumaHorz : lumaVert;
            float blendFactor = (edgeLuma == 0.f) ? 0.f : rangeLuma / edgeLuma;
            blendFactor = std::pow(blendFactor, subpixBlendStrength);

            int sampleBlendX = x + static_cast<int>(blendStepX);
            int sampleBlendY = y + static_cast<int>(blendStepY);
            Color colorBlend = sampleQImage_nearest(image, sampleBlendX, sampleBlendY);
            Color finalColor = colorCenter * (1.f - blendFactor) + colorBlend * blendFactor;
            image.setPixelColor(x, y, QColor(finalColor.x * 255.f, finalColor.y * 255.f, finalColor.z * 255.f));
        }
    }
}

void BlinnPhongShader::simdFXAAShader(QImage& image, int wide, int height, float edgeThresshold)
{
    // for(int y = 0; y < height; ++y){
    //     __m256i simdY = _mm256_set1_epi32(y);
    //     for(int x = 0; x < wide; x += 8){
    //         __m256i simdXBase = _mm256_set1_epi32(x);
    //         __m256i simdXOffset = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
    //         __m256i simdX = _mm256_add_epi32(simdXBase, simdXOffset);

    //         __m256i simdImgWide = _mm256_set1_epi32(wide);
    //         __m256i xInBoundMaskI = _mm256_cmpgt_epi32(simdX, simdImgWide);
    //         __m256 xInBoundMask = _mm256_castps_si256(xInBoundMaskI);
    //     }
    // }
}


void BlinnPhongShader::vertexShader(Vertex& vertex)
{
    vertex.worldSpacePos = Coord3D(m_modelTransformation  * Coord4D(vertex.worldSpacePos, 1.f)); // 模型变换
    vertex.clipSpacePos = m_projectionTransformation * m_viewTransformation * Coord4D(vertex.worldSpacePos, 1.f);
    vertex.normal = glm::mat3(glm::transpose(glm::inverse(m_modelTransformation))) * vertex.normal;
}

void BlinnPhongShader::fragmentShader(Fragment& fragment)
{
    Color diffuseColor  = {0.5f, 0.5f, 0.5f};
    Color specularColor = {0.2f, 0.2f, 0.2f};
    auto& rendererDevice = SRendererDevice::getInstance();
    if(SHADERTEXTURE){
        if(m_material.diffuse != -1){
            diffuseColor = rendererDevice.m_textureList[m_material.diffuse].sample2D(fragment.texCoord);
        }
        if(m_material.specular != -1){
            specularColor = rendererDevice.m_textureList[m_material.specular].sample2D(fragment.texCoord);
        }
    }

    Vector3D normal  = glm::normalize(fragment.normal);
    Vector3D viewDir = glm::normalize(m_eyePos - fragment.worldSpacePos);

    auto calculateLight = [&](const Light& light) -> Color
    {
        Vector3D lightDir;
        if(light.pos.w != 0.f){
            lightDir = glm::normalize(Coord3D(light.pos) - fragment.worldSpacePos);
        }
        else{
            lightDir = - Vector3D(light.dir);
        }

        Color ambient  = light.ambient * diffuseColor;
        Color diffuse  = light.diffuse * std::max(glm::dot(normal, lightDir), 0.f) * diffuseColor;
        Color specular = light.specular * std::pow(std::max(glm::dot(normal, glm::normalize(viewDir + lightDir)), 0.f), m_material.shininess) * specularColor;

        if(AMBIENT){return (ambient);}
        if(DIFFUSE){return (diffuse);}
        if(SPECULAR){return (specular);}
        return (ambient + diffuse + specular);
    };

    Color result(0.f, 0.f, 0.f);

    for(const auto& light : m_lightList){
        result += calculateLight(light);
    }
    result.x = std::clamp(result.x, 0.f, 1.f);
    result.y = std::clamp(result.y, 0.f, 1.f);
    result.z = std::clamp(result.z, 0.f, 1.f);

    fragment.fragmentColor = result;
}


void BlinnPhongShader::fragmentShaderSIMD(SimdFragment& frag_simd, __m256& final_mask)
{
    SimdColor diffuseColor = {_mm256_set1_ps(0.5),
                              _mm256_set1_ps(0.5),
                              _mm256_set1_ps(0.5)};

    SimdColor specularColor = {_mm256_set1_ps(0.2),
                               _mm256_set1_ps(0.2),
                               _mm256_set1_ps(0.2)};

    SimdMaterial simdMaterial = {_mm256_set1_ps(m_material.shininess),
                                 _mm256_set1_epi32(m_material.diffuse),
                                 _mm256_set1_epi32(m_material.specular)};

    __m256 w = _mm256_rcp_ps(frag_simd.viewDepth);
    auto& renderDevice = SRendererDevice::getInstance();
    if(SHADERTEXTURE){
        __m256i negOne = _mm256_set1_epi32(-1);
        __m256 diffTextureMask = _mm256_castsi256_ps(_mm256_cmpgt_epi32(simdMaterial.diffTextureIdx, negOne));
        __m256 specTextureMask = _mm256_castsi256_ps(_mm256_cmpgt_epi32(simdMaterial.specTextureIdx, negOne));

        __m256 diffTextureMaskFinal = _mm256_and_ps(final_mask, diffTextureMask);
        __m256 specTextureMaskFinal = _mm256_and_ps(final_mask, specTextureMask);

        SimdVector2D corrected_texCoord = {
            _mm256_mul_ps(frag_simd.texCoord.x, w),
            _mm256_mul_ps(frag_simd.texCoord.y, w)
        };

        if(_mm256_movemask_ps(diffTextureMaskFinal) != 0){
            Texture& diffTexture = renderDevice.m_textureList[m_material.diffuse];
            SimdColor sampleDiffColor = diffTexture.simdSample2D(corrected_texCoord);

            diffuseColor.r = _mm256_blendv_ps(diffuseColor.r, sampleDiffColor.r, diffTextureMaskFinal);
            diffuseColor.g = _mm256_blendv_ps(diffuseColor.g, sampleDiffColor.g, diffTextureMaskFinal);
            diffuseColor.b = _mm256_blendv_ps(diffuseColor.b, sampleDiffColor.b, diffTextureMaskFinal);
        }
        if(_mm256_movemask_ps(specTextureMaskFinal) != 0){
            Texture& specTexture = renderDevice.m_textureList[m_material.specular];
            SimdColor sampleSpecColor = specTexture.simdSample2D(corrected_texCoord);

            specularColor.r = _mm256_blendv_ps(specularColor.r, sampleSpecColor.r, specTextureMaskFinal);
            specularColor.g = _mm256_blendv_ps(specularColor.g, sampleSpecColor.g, specTextureMaskFinal);
            specularColor.b = _mm256_blendv_ps(specularColor.b, sampleSpecColor.b, specTextureMaskFinal);
        }
    }

    SimdVector3D normal = simd_normalize_ps(frag_simd.normal);
    SimdVector3D simdEyes = {_mm256_set1_ps(m_eyePos.x), _mm256_set1_ps(m_eyePos.y), _mm256_set1_ps(m_eyePos.z)};
    SimdVector3D viewDir = simd_normalize_ps(simd_sub_ps(simdEyes, frag_simd.worldSpacePos));
    SimdColor simdResult = {_mm256_set1_ps(0.f), _mm256_set1_ps(0.f), _mm256_set1_ps(0.f)};
    for(const auto& light : m_lightList){
        SimdColor simdLightAmbient = {_mm256_set1_ps(light.ambient.x),
                                 _mm256_set1_ps(light.ambient.y),
                                 _mm256_set1_ps(light.ambient.z)};
        SimdColor simdLightDiffuse = {_mm256_set1_ps(light.diffuse.x),
                                 _mm256_set1_ps(light.diffuse.y),
                                 _mm256_set1_ps(light.diffuse.z)};
        SimdColor simdLightSpecular = {_mm256_set1_ps(light.specular.x),
                                  _mm256_set1_ps(light.specular.y),
                                  _mm256_set1_ps(light.specular.z)};
        SimdVector3D lightPos = {_mm256_set1_ps(light.pos.x),
                                 _mm256_set1_ps(light.pos.y),
                                 _mm256_set1_ps(light.pos.z)};

        SimdVector3D lightDir = simd_normalize_ps(simd_sub_ps(lightPos, frag_simd.worldSpacePos));

        //ambient
        SimdColor ambient = simd_mul_ps(simdLightAmbient, diffuseColor);

        //diffuse
        __m256 dotNormalLight = simd_dot_ps(normal, lightDir);
        __m256 maxDotZero = simd_max_ps(dotNormalLight, _mm256_setzero_ps());
        SimdColor diffuse = simd_scalar_mul_ps(simd_mul_ps(simdLightDiffuse, diffuseColor), maxDotZero);

        //specular
        SimdVector3D halfVec = simd_normalize_ps(simd_add_ps(viewDir, lightDir));
        __m256 dotNormalHalf = simd_dot_ps(normal, halfVec);
        __m256 maxDotHalfZero = simd_max_ps(dotNormalHalf, _mm256_setzero_ps());
        // __m256 simdShininess = simdMaterial.shininess;

        __m256 specularIntensity = _mm256_sub_ps(maxDotHalfZero, _mm256_set1_ps(0.03f));
        SimdColor specular = simd_scalar_mul_ps(simd_mul_ps(simdLightSpecular, specularColor), specularIntensity);

        simdResult = simd_add_ps(simdResult, ambient);
        simdResult = simd_add_ps(simdResult, diffuse);
        // simdResult = simd_add_ps(simdResult, specular);
    }
    frag_simd.fragmentColor = simdResult;
}
