#include "Model.h"

Model::Model(QString path)
    :m_triangleCount(0)
    ,m_vertexCount(0)
    ,m_loadSuccess(true)
    ,m_minX(FLT_MAX)
    ,m_minY(FLT_MAX)
    ,m_minZ(FLT_MAX)
    ,m_maxX(FLT_MIN)
    ,m_maxY(FLT_MIN)
    ,m_maxZ(FLT_MIN)
    ,m_modelNormalizationMatrix(1.f)
{
    std::cout << "Model constructor: " << path.toStdString() << std::endl;
    loadModel(path);
    if (m_loadSuccess) {
        m_centre = {(m_maxX + m_minX) / 2.f, (m_maxY + m_minY) / 2.f, (m_maxZ + m_minZ) / 2.f};
    }

    // 新增的模型标准化矩阵计算逻辑
    float initialRangeX = m_maxX - m_minX;
    float initialRangeY = m_maxY - m_minY;
    float initialRangeZ = m_maxZ - m_minZ;
    float maxInitialDimension = std::max({initialRangeX, initialRangeY, initialRangeZ});

    if (m_loadSuccess && maxInitialDimension > 100.f) {
        float targetMaxDimension = 2.0f; // 目标最大维度，例如希望模型最大维度缩放到2.0f (即范围从-1.0到1.0)
        float scaleFactor = (maxInitialDimension > FLT_EPSILON) ? (targetMaxDimension / maxInitialDimension) : 1.0f; // 避免除以0
        // 1. 创建平移矩阵，将模型中心移动到原点
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-m_centre.x, -m_centre.y, -m_centre.z));
        // 2. 创建缩放矩阵
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor, scaleFactor, scaleFactor));

        m_modelNormalizationMatrix = scaleMatrix * translationMatrix; // 先平移再缩放
        m_centre = m_modelNormalizationMatrix * Vector4D(m_centre, 1.f);
    }
}

float Model::getYRange()
{
    if(m_modelNormalizationMatrix == glm::mat4(1.f)){return m_maxY - m_minY;}

    Vector4D minPoint(m_minX, m_minY, m_minZ, 1.0f);
    Vector4D maxPoint(m_maxX, m_maxY, m_maxZ, 1.0f);

    // 应用模型变换矩阵
    Vector4D transformedMin = m_modelNormalizationMatrix * minPoint;
    Vector4D transformedMax = m_modelNormalizationMatrix * maxPoint;

    // 返回变换后的Y范围
    return transformedMax.y - transformedMin.y;

}

void Model::draw()
{
    SRendererDevice::getInstance().m_textureList = m_textureList;
    for(int i = 0; i < m_meshes.size(); i++){
        m_meshes[i].draw();
    }
    // if(FXAA)
    // SRendererDevice::getInstance().m_shader->FXAAShader(SRendererDevice::getInstance().getFrameBuffer().getImage(), 0.0833f, 0.75f, 0.0312f);
}

//====================================================================

void Model::loadModel(QString path)
{
    Assimp::Importer import;
    // 对加载的模型进行标准化，包括将QString转换成标准字符串
    // aiProcess_Triangulate，将多边形转换成三角形
    // aiProcess_GenNormals， 自动生成法线
    const aiScene *scene = import.ReadFile(path.toStdString(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace); //

    // 检查模型场景指针是否生成成功，加载是被则直接返回
    // AI_SCENE_FLAGS_INCOMPLETE场景是否不完整
    // !scene->mRootNode 根节点一定非空
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        m_loadSuccess = false;
        std::cout << "Failed to load model: " << path.toStdString() << std::endl;
        return;
    }

    std::cout << "Model loaded. Number of materials:" << scene->mNumMaterials << std::endl;
    m_directory = path.mid(0, path.lastIndexOf('/'));
    // 传入场景根节点和场景指针
    // 调用节点处理函数，从根节点处理模型
    processNode(scene->mRootNode, scene);
    std::cout << "model load success" << std::endl;
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    // 从根节点开始，遍历模型所有网格块
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        // 传入网格指针和场景指针，处理网格
        m_meshes.push_back(processMesh(mesh, scene));
    }
    // 递归调用子网格
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
{
    m_vertexCount += mesh->mNumVertices; // 记录模型顶点数量
    m_triangleCount += mesh->mNumFaces;  // 记录模型三角形数量

    Mesh res;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // 遍历处理当前网格的所有顶点
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        m_minX = std::min(m_minX, mesh->mVertices[i].x);
        m_minY = std::min(m_minY, mesh->mVertices[i].y);
        m_minZ = std::min(m_minZ, mesh->mVertices[i].z);
        m_maxX = std::max(m_maxX, mesh->mVertices[i].x);
        m_maxY = std::max(m_maxY, mesh->mVertices[i].y);
        m_maxZ = std::max(m_maxZ, mesh->mVertices[i].z);

        // 获取模型顶点的世界坐标
        vertex.worldSpacePos = Coord3D(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        if(mesh->HasNormals()){
            // 获取模型顶点的法向量
            vertex.normal = Coord3D(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }/*else{
            // 如果没有法线，可以设置一个默认值或警告
            vertex.normal = {0.0f, 1.0f, 0.0f}; // 默认向上
            std::cerr << "Warning: Mesh " << mesh->mName.C_Str() << " has no normals. Using default." << std::endl;
        }*/

        // 获取模型顶点的纹理坐标
        if(mesh->mTextureCoords[0]){
            vertex.texCoord = Coord2D(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        else{
            vertex.texCoord = Coord2D(0.0f, 0.0f);
        }
        vertices.push_back(vertex);
    }

    res.m_vertices = std::move(vertices); // 移动语义，避免拷贝

    // 获取每一个模型每一个三角形面的索引
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for(unsigned int j = 0; j < face.mNumIndices; j++){
            indices.push_back(face.mIndices[j]);
        }
    }

    res.m_indices = std::move(indices);


    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    res.m_normalTextureIndex = loadMaterialTextures(res, material, aiTextureType_HEIGHT);
    res.m_diffuseTextureIndex = loadMaterialTextures(res, material, aiTextureType_DIFFUSE);
    res.m_specularTextureIndex = loadMaterialTextures(res, material, aiTextureType_SPECULAR);

    return res;
}

int Model::loadMaterialTextures(Mesh &mesh, aiMaterial *mat, aiTextureType type)
{
    if(mat->GetTextureCount(type) > 0)
    {
        aiString str;
        mat->GetTexture(type, 0, &str);
        QString path = m_directory + '/' + str.C_Str();
        for(int i = 0; i < m_textureList.size(); i++)
        {
            if(m_textureList[i].m_path == path){return i;}
        }
        Texture texture;
        if(texture.loadFromImage(path))
        {
            qDebug() << path;
            m_textureList.push_back(texture);
            return (int)m_textureList.size() - 1;
        }
    }
    return -1;
}

glm::mat4 Model::getModelTansformation()
{
    return m_modelNormalizationMatrix;
}
