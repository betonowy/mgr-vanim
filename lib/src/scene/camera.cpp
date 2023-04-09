#include "camera.hpp"

namespace scene
{
void camera::set_dir(glm::vec3 dir)
{
    _dirty = true;
    _dir = dir;
}

void camera::set_origin(glm::vec3 origin)
{
    _dirty = true;
    _origin = origin;
}

void camera::set_aspect(float aspect)
{
    _dirty = true;
    _aspect = aspect;
}

void camera::recalculate()
{
    float fov_mod = glm::tan(_fov);

    _forward = glm::normalize(_dir);

    if (_aspect > 1)
    {
        _right = glm::normalize(glm::cross(WORLD_UP, _forward)) * fov_mod * _aspect;
        _up = glm::normalize(glm::cross(_forward, _right)) * fov_mod;
    }
    else
    {
        _right = glm::normalize(glm::cross(WORLD_UP, _forward)) * fov_mod;
        _up = glm::normalize(glm::cross(_forward, _right)) * fov_mod / _aspect;
    }

    _dirty = false;
}

void camera::set_fov(float fov)
{
    _dirty = true;
    _fov = glm::radians(fov / 2.f);
}

} // namespace scene
