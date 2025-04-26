#include "Particles.hpp"

#include <etna/BlockingTransferHelper.hpp>
#include <etna/Etna.hpp>
#include <etna/Profiling.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>

#include <imgui.h>

#include "stb_image.h"

namespace pipes {

void 
ParticlesPipeline::allocate()
{
    // auto& ctx = etna::get_context();
}

void 
ParticlesPipeline::loadShaders() 
{
    etna::create_program(
        "particles_shader",
        { PARTICLES_PIPELINE_SHADERS_ROOT "particles.vert.spv",
          PARTICLES_PIPELINE_SHADERS_ROOT "particles.frag.spv"}
    );
}

void 
ParticlesPipeline::setup() 
{
    auto& pipelineManager = etna::get_context().getPipelineManager();

    pipeline = pipelineManager.createGraphicsPipeline(
    "particles_shader",
    etna::GraphicsPipeline::CreateInfo{
      .inputAssemblyConfig = {
        .topology = vk::PrimitiveTopology::eTriangleStrip
      },
      .blendingConfig = {
        .attachments = {
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::True,
            .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eZero,
            .dstAlphaBlendFactor = vk::BlendFactor::eOne,
            .alphaBlendOp = vk::BlendOp::eAdd,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
          },
          
        },
        .logicOpEnable = false,
        .logicOp = {},
        .blendConstants = {},
      },
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = RenderTarget::COLOR_ATTACHMENT_FORMATS,
          .depthAttachmentFormat  = RenderTarget::DEPTH_ATTACHMENT_FORMAT,
        },
    });
}

void 
ParticlesPipeline::drawGui()
{

}

void 
ParticlesPipeline::debugInput(const Keyboard& /*kb*/)
{

}

void
ParticlesPipeline::render(vk::CommandBuffer cmd_buf, const RenderContext& ctx)
{
  pushConstants.mProjView = ctx.worldViewProj;
  ETNA_PROFILE_GPU(cmd_buf, pipelines_particles_render);
  {
    auto particlesShader = etna::get_shader_program("particles_shader");

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());

    auto set = etna::create_descriptor_set(
      particlesShader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, ctx.sceneMgr->particles().genBinding()}
      }
    );
    
    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipeline.getVkPipelineLayout(),
      0,
      {set.getVkSet(), ctx.sceneMgr->resources().getSet()},
      {}
    );
    pushConstants.material = ctx.sceneMgr->particles().begin()->info().material;
    cmd_buf.pushConstants(
      pipeline.getVkPipelineLayout(), 
      vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
      0,
      uint32_t(sizeof(PushConstants)),
      &pushConstants
    );
    std::size_t size = ctx.sceneMgr->particles().begin()->size();
    size = std::min(1024ul, size);
    cmd_buf.draw(14, size, 0, 0);
  }
}

} /* namespace pipes */
