#include "Mesh.h"

Mesh::Mesh()
    :m_normalTextureIndex(-1)
    ,m_diffuseTextureIndex(-1)
    ,m_specularTextureIndex(-1)
{
}

void Mesh::draw()
{
    SRendererDevice::getInstance().m_vertexList = m_vertices;
    SRendererDevice::getInstance().m_indices = m_indices;
    SRendererDevice::getInstance().m_shader->m_material.diffuse = m_diffuseTextureIndex;
    SRendererDevice::getInstance().m_shader->m_material.specular = m_specularTextureIndex;
    SRendererDevice::getInstance().render();
}
