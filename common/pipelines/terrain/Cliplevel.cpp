#include "Cliplevel.hpp"

namespace pipes::terrain {


void 
Cliplevel::allocate(glm::vec2 step, std::size_t resolution)
{ 
    m_step = step;
    m_chunk.allocate({4 * resolution, 4 * resolution});
    m_chunk.setExtent(4.f * m_step);
}

void 
Cliplevel::update(vk::CommandBuffer cmd_buf, pipes::PerlinPipeline& pipeline, glm::vec2 pos, targets::TerrainChunk& , float frequency, std::size_t octaves, bool force)
{
    glm::ivec2 newPos = static_cast<glm::ivec2>(glm::trunc(pos / m_step));
    if(newPos == m_pos && !force) 
        return;
    m_pos = newPos;
    m_chunk.iPos = m_pos + glm::ivec2{-2, -2};
    m_chunk.setPosition(static_cast<glm::vec2>(m_pos + glm::ivec2{-2, -2}) * m_step);
    m_chunk.setState(cmd_buf,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
        vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite, 
        vk::ImageLayout::eColorAttachmentOptimal, 
        vk::ImageAspectFlagBits::eColor
    );
    etna::flush_barriers(cmd_buf);
    {
        etna::RenderTargetState renderTarget{
            cmd_buf,
            {{0, 0}, {m_chunk.getResolution().x, m_chunk.getResolution().y}},
            m_chunk.getColorAttachments(),
            {},
            BarrierBehavoir::eSuppressBarriers
        };
        if(force) {
            std::array<vk::ClearAttachment, targets::TerrainChunk::N_COLOR_ATTACHMENTS> clearAtts{
                vk::ClearAttachment{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .colorAttachment = 0,
                },
                vk::ClearAttachment{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .colorAttachment = 1,
                },
                vk::ClearAttachment{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .colorAttachment = 2,
                }
            };
            std::array<vk::ClearRect, targets::TerrainChunk::N_COLOR_ATTACHMENTS> clearRects{
                vk::ClearRect{
                    .rect = {{0, 0}, {m_chunk.getResolution().x, m_chunk.getResolution().y}},
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                vk::ClearRect{
                    .rect = {{0, 0}, {m_chunk.getResolution().x, m_chunk.getResolution().y}},
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                vk::ClearRect{
                    .rect = {{0, 0}, {m_chunk.getResolution().x, m_chunk.getResolution().y}},
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            cmd_buf.clearAttachments(clearAtts, clearRects);
        }
        for(int i = -2; i < 2; i++) {
            for(int j = -2; j < 2; j++) {
                //NOTE - Some arithmetics to implement reuse of detailed
                int ix = ((m_pos.x + i) % 4 + 4) % 4;
                int iy = ((m_pos.y + j) % 4 + 4) % 4;
                auto& chunk = m_ipos[4 * ix + iy];
                if(chunk == (m_pos + glm::ivec2{i, j}) && !force) 
                    continue;
                chunk = (m_pos + glm::ivec2{i, j});
                glm::vec2 pos = static_cast<glm::vec2>(m_pos + glm::ivec2{i, j}) * m_step;
                pipeline.reset(pos, m_chunk.getExtentPos() / 4.f, frequency);
                glm::vec2 texStart  = glm::vec2(ix / 4.f, iy / 4.f);
                glm::vec2 texExtent{.25f, .25f};
               
                pipeline.setSubregion(texStart, texExtent);
                pipeline.render(cmd_buf, m_chunk, static_cast<uint32_t>(octaves));

            }
        }
    }
    m_chunk.setState(cmd_buf,
        vk::PipelineStageFlagBits2::eTessellationEvaluationShader | vk::PipelineStageFlagBits2::eFragmentShader, 
        vk::AccessFlagBits2::eShaderSampledRead, 
        vk::ImageLayout::eShaderReadOnlyOptimal, 
        vk::ImageAspectFlagBits::eColor
    );
}


}