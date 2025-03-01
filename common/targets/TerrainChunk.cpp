#include "TerrainChunk.hpp"
#include "etna/Etna.hpp"
#include <etna/GlobalContext.hpp>

namespace targets {

const std::vector<vk::Format> TerrainChunk::COLOR_ATTACHMENT_FORMATS = {
  vk::Format::eR32Sfloat,
};

void
TerrainChunk::allocate(glm::uvec2 extent)
{
  std::array<const char*, N_COLOR_ATTACHMENTS> names = {
    "terrain_height",
  };
  resolution = extent;

  auto& ctx = etna::get_context(); 

  for(std::size_t i = 0; i < N_COLOR_ATTACHMENTS; ++i) {
    color_buffer[i] = ctx.createImage({
      .extent = vk::Extent3D{resolution.x, resolution.y, 1},
      .name = names[i],
      .format = COLOR_ATTACHMENT_FORMATS[i],
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage,
    });
  }

    color_attachments.resize(N_COLOR_ATTACHMENTS);
    for(std::size_t i = 0; i < N_COLOR_ATTACHMENTS; ++i) {
        color_attachments[i] = etna::RenderTargetState::AttachmentParams {
            .image = color_buffer[i].get(),
            .view  = color_buffer[i].getView({}),
            .imageAspect = vk::ImageAspectFlagBits::eColor,
        };
    }

}

void 
TerrainChunk::setState(vk::CommandBuffer com_buffer, 
     vk::PipelineStageFlags2 pipeline_stage_flags, 
     vk::AccessFlags2 access_flags, 
     vk::ImageLayout layout, 
     vk::ImageAspectFlags aspect_flags)
{
    for(std::size_t i = 0; i < N_COLOR_ATTACHMENTS; i++) {
        etna::set_state(com_buffer, 
            color_buffer[i].get(), 
            pipeline_stage_flags, 
            access_flags, 
            layout, 
            aspect_flags,
            ForceSetState::eTrue
        );
    }
}


}
