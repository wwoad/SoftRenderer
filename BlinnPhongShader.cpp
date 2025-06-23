#include "BlinnPhongShader.h"
#include "FunctionSIMD.h"

void BlinnPhongShader::vertexShader(Vertex& vertex)
{
    vertex.worldSpacePos = Coord3D(m_modelTransformation  * Coord4D(vertex.worldSpacePos, 1.f)); // 模型变换
    vertex.clipSpacePos = m_projectionTransformation * m_viewTransformation * Coord4D(vertex.worldSpacePos, 1.f);
    vertex.normal = glm::mat3(glm::transpose(glm::inverse(m_modelTransformation))) * vertex.normal;
}

void BlinnPhongShader::fragmentShader(Fragment& fragment)
{
    Color diffuseColor = {0.5f, 0.5f, 0.5f};
    Color specularColor = {0.9f, 0.9f, 0.9f};
    auto& rendererDevice = SRendererDevice::getInstance();
    if(m_material.diffuse != -1){
        diffuseColor = rendererDevice.m_textureList[m_material.diffuse].sample2D(fragment.texCoord);
    }
    if(m_material.specular != -1){
        specularColor = rendererDevice.m_textureList[m_material.specular].sample2D(fragment.texCoord);
    }

    Vector3D normal = glm::normalize(fragment.normal);
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

        Color ambient = light.ambient * diffuseColor;
        Color diffuse = light.diffuse * std::max(glm::dot(normal, lightDir), 0.f) * diffuseColor;
        Color specular = light.specular * std::pow(std::max(glm::dot(normal, glm::normalize(viewDir + lightDir)), 0.f), m_material.shininess) * specularColor;

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


void BlinnPhongShader::fragmentShaderSIMD(SimdFragment& frag_simd)
{
    SimdColor result_colors;
    result_colors.r = _mm256_setzero_ps();
    result_colors.g = _mm256_setzero_ps();
    result_colors.b = _mm256_setzero_ps();
    // 1. 恢复透视校正后的属性
    __m256 zero_ps = _mm256_setzero_ps();
    __m256 one_ps = _mm256_set1_ps(1.0f);
    __m256 w_recip_clamped = simd_max_ps(frag_simd.viewDepth, _mm256_set1_ps(1e-6f)); // 钳位1/w，防止除零
    SimdVector3D normal = simd_normalize_ps(frag_simd.normal); // 归一化法线 (Normal/w 归一化后才是真正的归一化法线)
    SimdVector3D worldSpacePos;
    worldSpacePos.x = _mm256_div_ps(frag_simd.worldSpacePos.x, w_recip_clamped);
    worldSpacePos.y = _mm256_div_ps(frag_simd.worldSpacePos.y, w_recip_clamped);
    worldSpacePos.z = _mm256_div_ps(frag_simd.worldSpacePos.z, w_recip_clamped);
    // SimdVector3D texCoord; // 如果需要纹理坐标
    // texCoord.x = _mm256_div_ps(frag_simd.texCoord.x, w_recip_clamped);
    // texCoord.y = _mm256_div_ps(frag_simd.texCoord.y, w_recip_clamped);
    // 2. 获取材质属性 (m_material 是非SIMD的，需要广播)
    __m256 shininess_simd = _mm256_set1_ps(m_material.shininess);
    // 3. 获取纹理颜色 (如果适用)
    // 警告：纹理采样 sample2D 是逐像素操作，很难 SIMD 化！
    // 这里假设不使用纹理，使用固定颜色。
    SimdColor diffuseColor_simd;
    SimdColor specularColor_simd;
    diffuseColor_simd.r = _mm256_set1_ps(0.5f);
    diffuseColor_simd.g = _mm256_set1_ps(0.5f);
    diffuseColor_simd.b = _mm256_set1_ps(0.5f);
    specularColor_simd.r = _mm256_set1_ps(0.9f);
    specularColor_simd.g = _mm256_set1_ps(0.9f);
    specularColor_simd.b = _mm256_set1_ps(0.9f);
    // 4. 计算 View Direction
    __m256 eyePos_x = _mm256_set1_ps(m_eyePos.x);
    __m256 eyePos_y = _mm256_set1_ps(m_eyePos.y);
    __m256 eyePos_z = _mm256_set1_ps(m_eyePos.z);
    SimdVector3D viewDir;
    viewDir.x = _mm256_sub_ps(eyePos_x, worldSpacePos.x);
    viewDir.y = _mm256_sub_ps(eyePos_y, worldSpacePos.y);
    viewDir.z = _mm256_sub_ps(eyePos_z, worldSpacePos.z);
    viewDir = simd_normalize_ps(viewDir); // 归一化 View Direction
    // 5. 遍历光源并计算光照
    for(const auto& light : m_lightList){
        // 计算 Light Direction
        SimdVector3D lightDir;
        if(light.pos.w != 0.f){ // 点光源
            __m256 lightPos_x = _mm256_set1_ps(light.pos.x);
            __m256 lightPos_y = _mm256_set1_ps(light.pos.y);
            __m256 lightPos_z = _mm256_set1_ps(light.pos.z);
            lightDir.x = _mm256_sub_ps(lightPos_x, worldSpacePos.x);
            lightDir.y = _mm256_sub_ps(lightPos_y, worldSpacePos.y);
            lightDir.z = _mm256_sub_ps(lightPos_z, worldSpacePos.z);
            lightDir = simd_normalize_ps(lightDir); // 归一化 Light Direction
        }
        else{ // 方向光源
            lightDir.x = _mm256_set1_ps(-light.dir.x); // 注意方向光源是 -dir
            lightDir.y = _mm256_set1_ps(-light.dir.y);
            lightDir.z = _mm256_set1_ps(-light.dir.z);
            // 方向光源已经是单位向量，无需归一化
        }
        // 广播光源颜色
        __m256 light_ambient_r = _mm256_set1_ps(light.ambient.r);
        __m256 light_ambient_g = _mm256_set1_ps(light.ambient.g);
        __m256 light_ambient_b = _mm256_set1_ps(light.ambient.b);
        __m256 light_diffuse_r = _mm256_set1_ps(light.diffuse.r);
        __m256 light_diffuse_g = _mm256_set1_ps(light.diffuse.g);
        __m256 light_diffuse_b = _mm256_set1_ps(light.diffuse.b);
        __m256 light_specular_r = _mm256_set1_ps(light.specular.r);
        __m256 light_specular_g = _mm256_set1_ps(light.specular.g);
        __m256 light_specular_b = _mm256_set1_ps(light.specular.b);
        // 环境光 = light.ambient * diffuseColor
        SimdColor ambient_simd;
        ambient_simd.r = _mm256_mul_ps(light_ambient_r, diffuseColor_simd.r);
        ambient_simd.g = _mm256_mul_ps(light_ambient_g, diffuseColor_simd.g);
        ambient_simd.b = _mm256_mul_ps(light_ambient_b, diffuseColor_simd.b);
        // 漫反射 = light.diffuse * max(dot(normal, lightDir), 0.f) * diffuseColor
        __m256 normal_dot_light = simd_dot_ps(normal, lightDir);
        __m256 max_dot_zero = simd_max_ps(normal_dot_light, zero_ps); // max(dot, 0)
        SimdColor diffuse_simd;
        diffuse_simd.r = _mm256_mul_ps(light_diffuse_r, _mm256_mul_ps(max_dot_zero, diffuseColor_simd.r));
        diffuse_simd.g = _mm256_mul_ps(light_diffuse_g, _mm256_mul_ps(max_dot_zero, diffuseColor_simd.g));
        diffuse_simd.b = _mm256_mul_ps(light_diffuse_b, _mm256_mul_ps(max_dot_zero, diffuseColor_simd.b));
        // 镜面反射 = light.specular * pow(max(dot(normal, normalize(viewDir + lightDir)), 0.f), m_material.shininess) * specularColor
        SimdVector3D halfwayDir; // Halfway vector = normalize(viewDir + lightDir)
        halfwayDir.x = _mm256_add_ps(viewDir.x, lightDir.x);
        halfwayDir.y = _mm256_add_ps(viewDir.y, lightDir.y);
        halfwayDir.z = _mm256_add_ps(viewDir.z, lightDir.z);
        halfwayDir = simd_normalize_ps(halfwayDir); // 归一化 Halfway vector
        __m256 normal_dot_halfway = simd_dot_ps(normal, halfwayDir);
        __m256 max_dot_halfway_zero = simd_max_ps(normal_dot_halfway, zero_ps); // max(dot, 0)
        // 镜面反射强度 = pow(max_dot_halfway_zero, shininess_simd)
        // 使用您提供的 simd_pow_ps
        __m256 specular_intensity_simd = simd_pow_ps(max_dot_halfway_zero, shininess_simd);
        SimdColor specular_simd;
        specular_simd.r = _mm256_mul_ps(light_specular_r, _mm256_mul_ps(specular_intensity_simd, specularColor_simd.r));
        specular_simd.g = _mm256_mul_ps(light_specular_g, _mm256_mul_ps(specular_intensity_simd, specularColor_simd.g));
        specular_simd.b = _mm256_mul_ps(light_specular_b, _mm256_mul_ps(specular_intensity_simd, specularColor_simd.b));
        // 累加光照结果
        result_colors.r = _mm256_add_ps(result_colors.r, _mm256_add_ps(ambient_simd.r, _mm256_add_ps(diffuse_simd.r, specular_simd.r)));
        result_colors.g = _mm256_add_ps(result_colors.g, _mm256_add_ps(ambient_simd.g, _mm256_add_ps(diffuse_simd.g, specular_simd.g)));
        result_colors.b = _mm256_add_ps(result_colors.b, _mm256_add_ps(ambient_simd.b, _mm256_add_ps(diffuse_simd.b, specular_simd.b)));
    }
    // 6. 钳位最终颜色到 [0, 1]
    result_colors.r = simd_clamp_ps(result_colors.r, zero_ps, one_ps);
    result_colors.g = simd_clamp_ps(result_colors.g, zero_ps, one_ps);
    result_colors.b = simd_clamp_ps(result_colors.b, zero_ps, one_ps);

    frag_simd.fragmentColor = result_colors;
}
