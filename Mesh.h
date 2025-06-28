#ifndef MESH_H
#define MESH_H

#include <QString>
#include "Texture.h"
#include "SRendererDevice.h"
#include "BasicDataStructure.h"


<<<<<<< HEAD
=======
extern bool FXAA;

>>>>>>> future
class Mesh
{
public:
    std::vector<Vertex> m_vertices;
    std::vector<unsigned> m_indices;
    int m_normalTextureIndex{-1};
    int m_diffuseTextureIndex{-1};
    int m_specularTextureIndex{-1};

    Mesh();
    ~Mesh() = default;
    void draw();
};

#endif // MESH_H
