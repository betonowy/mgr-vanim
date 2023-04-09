#pragma once

#include <glm/glm.hpp>

namespace scene
{
class camera
{
public:
    void set_dir(glm::vec3);
    void set_origin(glm::vec3);
    void set_fov(float);
    void set_aspect(float);

    glm::vec3 up()
    {
        return clean(), _up;
    }

    glm::vec3 forward()
    {
        return clean(), _forward;
    }

    glm::vec3 right()
    {
        return clean(), _right;
    }

    float fov()
    {
        return _fov;
    }

    glm::vec3 origin()
    {
        return clean(), _origin;
    }

    glm::vec3 dir()
    {
        return _dir;
    }

private:
    inline void clean()
    {
        if (_dirty)
        {
            recalculate();
        }
    }

    void recalculate();

    glm::vec3 _up, _forward, _right;
    float _fov;
    float _aspect;
    bool _dirty = true;
    ;

    glm::vec3 _origin;
    glm::vec3 _dir;

    static constexpr glm::vec3 WORLD_UP{0, 1, 0};
};
} // namespace scene
