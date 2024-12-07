#pragma once
#include <glm/vec4.hpp>

struct LightSource {
    enum class Id : uint32_t { Invalid = ~uint32_t{0} };

    glm::vec4 position;
    glm::vec4 colorRange;
    float visibleRadius;
};