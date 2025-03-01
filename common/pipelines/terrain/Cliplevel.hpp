#ifndef PIPELINES_TERRAIN_CLIPLEVEL_HPP
#define PIPELINES_TERRAIN_CLIPLEVEL_HPP

#include "pipelines/perlin/Perlin.hpp"
#include "targets/TerrainChunk.hpp"

namespace pipelines::terrain {

class Cliplevel {
public:
    std::span<targets::TerrainChunk, 16> getChunks() { return m_chunks; }

    void allocate(glm::vec2 step, std::size_t resolution);

    void update(vk::CommandBuffer, pipes::PerlinPipeline& pipeline, glm::vec2 pos, targets::TerrainChunk& tmp, float frequency, std::size_t octaves, bool force = false);

    glm::ivec2 getIPos() { return m_pos; }
private:
    glm::ivec2 m_pos = {};
    glm::vec2 m_step;
    std::array<targets::TerrainChunk, 16> m_chunks;
};

}

#endif /* PIPELINES_TERRAIN_CLIPLEVEL_HPP */
