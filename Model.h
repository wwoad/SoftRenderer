#ifndef MODEL_H
#define MODEL_H

#include <iostream>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <glm/gtc/matrix_transform.hpp>
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "Mesh.h"
#include "BasicDataStructure.h"


class Model
{
public:
    Coord3D m_centre;
    int m_triangleCount;
    int m_vertexCount;
    bool m_loadSuccess;

    Model(QString path);
    float getYRange();
    void draw();
    glm::mat4 getModelTansformation();
private:
    float m_minX;
    float m_minY;
    float m_minZ;
    float m_maxX;
    float m_maxY;
    float m_maxZ;

    std::vector<Mesh> m_meshes;
    std::vector<Texture> m_textureList;
    QString m_directory;
    glm::mat4 m_modelNormalizationMatrix;

    void loadModel(QString path);
    void processNode(aiNode *node, const aiScene *scene);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    int loadMaterialTextures(Mesh &mesh, aiMaterial *mat, aiTextureType type);
};

#endif // MODEL_H
