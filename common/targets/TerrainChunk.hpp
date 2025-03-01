#pragma once
#include "etna/DescriptorSet.hpp"
#include "etna/Image.hpp"
#include "etna/RenderTargetStates.hpp"
#include <glm/glm.hpp>
namespace targets {

class TerrainChunk {
public:
    static const std::vector<vk::Format> COLOR_ATTACHMENT_FORMATS;
    static const vk::Format              DEPTH_ATTACHMENT_FORMAT  = vk::Format::eUndefined;
    static const constexpr int N_COLOR_ATTACHMENTS = 1;

    const std::vector<etna::RenderTargetState::AttachmentParams>& getColorAttachments() { return color_attachments; }

    etna::Image& getImage(std::size_t i ) { return color_buffer[i]; }
    
    glm::uvec2 getResolution() { return resolution; }

    void allocate(glm::uvec2 extent);

    void setState(vk::CommandBuffer com_buffer, vk::PipelineStageFlags2 pipeline_stage_flags, vk::AccessFlags2 access_flags, vk::ImageLayout layout, vk::ImageAspectFlags aspect_flags);

    void setPosition(glm::vec2 start) { startPos = start; }
    void setExtent(glm::vec2 extent) { extentPos = extent; }

    glm::vec2 getStartPos () const { return startPos;  }
    glm::vec2 getExtentPos() const { return extentPos; }

    glm::ivec2 iPos;
private:
    std::array<etna::Image, N_COLOR_ATTACHMENTS> color_buffer;

    std::vector<etna::RenderTargetState::AttachmentParams> color_attachments;
    
    glm::uvec2 resolution;

    glm::vec2 startPos;
    glm::vec2 extentPos;
};

}