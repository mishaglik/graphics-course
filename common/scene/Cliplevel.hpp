#ifndef PIPELINES_TERRAIN_CLIPLEVEL_HPP
#define PIPELINES_TERRAIN_CLIPLEVEL_HPP

#include "pipelines/perlin/Perlin.hpp"
#include "targets/TerrainChunk.hpp"

namespace scene::terrain {

class Cliplevel {
public:
    void allocate(glm::vec2 step, std::size_t resolution);

    glm::ivec2 pos = {};
    glm::vec2 step;
    std::array<glm::ivec2, 16> ipos;
    targets::TerrainChunk chunk;
};

}

#endif /* PIPELINES_TERRAIN_CLIPLEVEL_HPP */
