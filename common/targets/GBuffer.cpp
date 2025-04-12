#include "GBuffer.hpp"
#include "etna/Etna.hpp"
#include <etna/GlobalContext.hpp>

namespace targets {

const std::vector<vk::Format> GBuffer::COLOR_ATTACHMENT_FORMATS = {
  vk::Format::eB8G8R8A8Unorm,
  vk::Format::eR16G16B16A16Snorm,
  vk::Format::eB8G8R8A8Unorm,
  vk::Format::eR32Sfloat,
};

void
GBuffer::allocate(glm::uvec2 extent, uint32_t layers)
{
  std::array<const char*, N_COLOR_ATTACHMENTS> names = {
    "gbuffer_albedo",
    "gbuffer_normal",
    "gbuffer_material",
    "gbuffer_wc",
  };
  resolution = extent;
  auto& depth = depth_buffer;

  auto& ctx = etna::get_context(); 

  for(std::size_t i = 0; i < N_COLOR_ATTACHMENTS; ++i) {
    color_buffer[i] = ctx.createImage({
      .extent = vk::Extent3D{resolution.x, resolution.y, 1},
      .name = names[i],
      .format = COLOR_ATTACHMENT_FORMATS[i],
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
      .layers = layers,
    });
  }

  depth = ctx.createImage({
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "gBuffer_depth",
    .format = vk::Format::eD32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled,
    .layers = layers,
  });

  color_attachments.resize(N_COLOR_ATTACHMENTS);

  for(std::size_t i = 0; i < N_COLOR_ATTACHMENTS; ++i) {
    color_attachments[i] = etna::RenderTargetState::AttachmentParams {
      .image = color_buffer[i].get(),
      .view  = color_buffer[i].getView({.layerCount=1}),
      .imageAspect = vk::ImageAspectFlagBits::eColor,
    };
  }

  depth_attachment = etna::RenderTargetState::AttachmentParams {
    .image = depth.get(),
    .view  = depth.getView({.layerCount=1}),
    .imageAspect = vk::ImageAspectFlagBits::eDepth,
  };
}

void 
GBuffer::setActiveLayer(uint32_t layer)
{
  for(std::size_t i = 0; i < N_COLOR_ATTACHMENTS; ++i) {
    color_attachments[i].view = color_buffer[i].getView({.baseLayer=layer, .layerCount=1});
  }
  depth_attachment.view = depth_buffer.getView({.baseLayer=layer, .layerCount=1});
}

void
GBuffer::toRenderTarget(vk::CommandBuffer cmd_buf)
{
  for(std::size_t i = 0; i < targets::GBuffer::N_COLOR_ATTACHMENTS; i++) {
    etna::set_state(cmd_buf, 
      color_buffer[i].get(), 
      vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::ImageLayout::eColorAttachmentOptimal, 
      vk::ImageAspectFlagBits::eColor
    );
  }
  etna::set_state(cmd_buf, 
      depth_buffer.get(), 
      vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::ImageLayout::eDepthAttachmentOptimal, 
      vk::ImageAspectFlagBits::eDepth
  );
}

void
GBuffer::toSampler(vk::CommandBuffer cmd_buf)
{
  for(std::size_t i = 0; i < targets::GBuffer::N_COLOR_ATTACHMENTS; i++) {
    etna::set_state(cmd_buf, 
      color_buffer[i].get(), 
      vk::PipelineStageFlagBits2::eFragmentShader, 
      vk::AccessFlagBits2::eShaderSampledRead,
      vk::ImageLayout::eShaderReadOnlyOptimal, 
      vk::ImageAspectFlagBits::eColor
    );
  }
  etna::set_state(cmd_buf, 
      depth_buffer.get(), 
      vk::PipelineStageFlagBits2::eFragmentShader, 
      vk::AccessFlagBits2::eShaderSampledRead,
      vk::ImageLayout::eShaderReadOnlyOptimal, 
      vk::ImageAspectFlagBits::eDepth
  );
}


etna::Image&
GBuffer::getImage(std::size_t i)
{
  if (i == N_COLOR_ATTACHMENTS) {
    return depth_buffer;
  }
  return color_buffer[i];
}


const etna::Image&
GBuffer::getImage(std::size_t i) const
{
  if (i == N_COLOR_ATTACHMENTS) {
    return depth_buffer;
  }
  return color_buffer[i];
}

}
