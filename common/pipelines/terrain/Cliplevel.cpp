#include "Cliplevel.hpp"

namespace pipelines::terrain {


void 
Cliplevel::allocate(glm::vec2 step, std::size_t resolution)
{ 
    m_step = step;
    for(auto& chunk : m_chunks) {
        chunk.allocate({resolution, resolution});
        chunk.setExtent(m_step);
    }
}

void 
Cliplevel::update(vk::CommandBuffer cmd_buf, pipes::PerlinPipeline& pipeline, glm::vec2 pos, targets::TerrainChunk& tmp, float frequency, std::size_t octaves, bool force)
{
    glm::ivec2 newPos = static_cast<glm::ivec2>(glm::trunc(pos / m_step));
    if(newPos == m_pos && !force) 
        return;
    m_pos = newPos;
    for(int i = -2; i < 2; i++) {
        for(int j = -2; j < 2; j++) {
            //NOTE - Some arithmetics to implement reuse of detailed
            int ix = ((m_pos.x + i) % 4 + 4) % 4;
            int iy = ((m_pos.y + j) % 4 + 4) % 4;
            auto& chunk = m_chunks[4 * ix + iy];
            if(chunk.iPos == (m_pos + glm::ivec2{i, j}) && !force) 
                continue;
            chunk.iPos = (m_pos + glm::ivec2{i, j});
            chunk.setPosition(static_cast<glm::vec2>(m_pos + glm::ivec2{i, j}) * m_step);
            generate_chunk(cmd_buf, pipeline, chunk, tmp, frequency, octaves);
            chunk.setState(cmd_buf,
                vk::PipelineStageFlagBits2::eTessellationEvaluationShader, 
                vk::AccessFlagBits2::eShaderSampledRead, 
                vk::ImageLayout::eShaderReadOnlyOptimal, 
                vk::ImageAspectFlagBits::eColor
            );
        }
    }
}


}