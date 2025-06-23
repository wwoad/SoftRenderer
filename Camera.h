#ifndef CAMERA_H
#define CAMERA_H


#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "BasicDataStructure.h"

enum class CameraPara
{
    FOV,
    NEAR
};

class Camera
{
public:
    float m_aspect;
    Vector3D m_position;
    Vector3D m_target;
    float m_zNear;
    float m_zFar;
    float m_fov;

    Camera(float aspect, float far);
    ~Camera() = default;
    void rotateAroundTarget(Vector2D motion);
    void moveTarget(Vector2D m_motion);
    void cloaseToTarget(int ratio);
    void setCamera(Coord3D modelCentre, float yRange);
    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix();
    glm::mat4 getOrthographicProjection();
};

#endif // CAMERA_H
