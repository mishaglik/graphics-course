#pragma once

#include "Texture.hpp"
#include <glm/vec4.hpp>

struct Material {
    enum class Id : uint32_t { Invalid = ~uint32_t{0}, Undefined = uint32_t{0} };
    
    Texture::Id baseColorTexture         = Texture::Id::Undefined;
    Texture::Id normalTexture            = Texture::Id::Undefined;
    Texture::Id metallicRoughnessTexture = Texture::Id::Undefined;
    Texture::Id emissiveFactorTexture    = Texture::Id::Undefined;

    glm::vec4 baseColor  = {0.5f, 0.5f, 0.5f, 1.f};
    glm::vec4 EMR_Factor = {0.f, 0.f, 0.f, 0.f};
};
