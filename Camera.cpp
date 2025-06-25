#include "Camera.h"

Camera::Camera(float aspect, float far)
    :m_aspect(aspect)
    ,m_zFar(far)
{
}

void Camera::rotateAroundTarget(Vector2D motion)
{
    Vector3D fromTarget = m_position - m_target;
    float radius = glm::length(fromTarget);
    float yaw = static_cast<float>(std::atan2(fromTarget.x, fromTarget.z));
    float pitch = static_cast<float>(std::asin(fromTarget.y / radius));
    float factor = static_cast<float>(M_PI) * 2.f;

    Vector3D offset;
    yaw -= motion.x * factor;
    pitch += motion.y * factor;

    if(pitch + M_PI_2 < FLT_EPSILON){pitch = - glm::radians(89.9f);}
    if(pitch - M_PI_2 > FLT_EPSILON){pitch =   glm::radians(89.9f);}

    offset.x = (radius * static_cast<float>(std::cos(pitch)) * static_cast<float>(std::sin(yaw)));
    offset.y = (radius * static_cast<float>(std::sin(pitch)));
    offset.z = (radius * static_cast<float>(std::cos(pitch)) * static_cast<float>(std::cos(yaw)));

    m_position = m_target + offset;
}

void Camera::moveTarget(Vector2D motion)
{
    Vector3D fromPosition = m_target - m_position;
    Vector3D forward = glm::normalize(fromPosition);
    Vector3D left = glm::normalize(glm::cross({0.f, 1.f, 0.f}, forward));
    Vector3D up = glm::normalize(glm::cross(forward, left));

    float distance = glm::length(fromPosition);
    float factor = distance * static_cast<float>(tan(glm::radians(m_fov) / 2)) * 2.f;
    Vector3D deltaX = factor * m_aspect * motion.x * left;
    Vector3D deltaY = factor * motion.y * up;

    m_target += (deltaX + deltaY);
    m_position += (deltaX + deltaY);
}

void Camera::cloaseToTarget(int ratio)
{
    Vector3D fromTarget = m_position - m_target;
    float radius = glm::length(fromTarget);
    float yaw = static_cast<float>(std::atan2(fromTarget.x, fromTarget.z));
    float pitch = static_cast<float>(std::asin(fromTarget.y / radius));
    Vector3D offset;
    radius *= static_cast<float>(std::pow(0.95, ratio));
    offset.x = (radius * static_cast<float>(std::cos(pitch)) * static_cast<float>(std::sin(yaw)));
    offset.y = (radius * static_cast<float>(std::sin(pitch)));
    offset.z = (radius * static_cast<float>(std::cos(pitch)) * static_cast<float>(std::cos(yaw)));

    m_position = m_target + offset;
}

void Camera::setCamera(Coord3D modelCentre, float yRange)
{
    m_target = modelCentre;
    m_position = modelCentre;
    m_position.z += (yRange / std::tan(glm::radians(m_fov) / 2));
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(m_position, m_target, {0.f, 1.f, 0.f});
}

glm::mat4 Camera::getProjectionMatrix()
{
    return glm::perspective(glm::radians(m_fov), m_aspect, m_zNear, m_zFar);
}

glm::mat4 Camera::getOrthographicProjection()
{
    return glm::orthoRH(-1.f, 1.f,  -1.f, 1.f, m_zNear, m_zFar);
}
