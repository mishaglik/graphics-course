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
    auto& ctx = etna::get_context();
    particles = ctx.createBuffer({
      .size =  uint32_t(N_MAX_PARTICLES_PER_DRAW * sizeof(ParticleInfo)),
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
      .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
      .name = "particles",
    });
    particles.map();

    commands = ctx.createBuffer({
      .size =  uint32_t(N_MAX_EMITTERS * sizeof(vk::DrawIndirectCommand)),
      .bufferUsage = vk::BufferUsageFlagBits::eIndirectBuffer,
      .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
      .name = "particles_commands",
    });
    commands.map();

    emitters = ctx.createBuffer({
      .size =  uint32_t(N_MAX_EMITTERS * sizeof(EmitterInfo)),
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
      .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
      .name = "particles_emitters",
    });
    emitters.map();
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
      .depthConfig = {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::False,
        .depthCompareOp = vk::CompareOp::eLessOrEqual,
        .maxDepthBounds = 1.f,
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
  pushConstants.frameTime = ctx.frameTime;
  ETNA_PROFILE_GPU(cmd_buf, pipelines_particles_render);
  {
    auto particlesShader = etna::get_shader_program("particles_shader");

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());


    auto& particleManager = ctx.sceneMgr->particles();
    vk::DrawIndirectCommand* cmds = reinterpret_cast<vk::DrawIndirectCommand*>(commands.data());
    EmitterInfo* ems              = reinterpret_cast<EmitterInfo*>(emitters.data());
    ParticleInfo* pts             = reinterpret_cast<ParticleInfo*>(particles.data());
    uint32_t cnt = 0;
    uint32_t parts = 0;
    for(auto& emitter : particleManager) {
      //TODO: Cull
      cmds[cnt] = vk::DrawIndirectCommand{
        .vertexCount = 14,
        .instanceCount = uint32_t(emitter.size()),
        .firstVertex = 0,
        .firstInstance = parts,
      };
      ems[cnt] = emitter.info();
      for(auto& particle: emitter) {
        pts[parts++].position = particle.position;
      }
      if(parts > N_MAX_PARTICLES_PER_DRAW) {
        //FIXME - Implement; 
        spdlog::warn("Drawing more than {} particles is not implemented. Truncating", N_MAX_PARTICLES_PER_DRAW);
        parts = N_MAX_PARTICLES_PER_DRAW;
        break;
      }
      cnt++;
    }

    auto set = etna::create_descriptor_set(
      particlesShader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, emitters.genBinding() },
        etna::Binding{1, particles.genBinding()}
      }
    );
    auto set2 = etna::create_descriptor_set(
      particlesShader.getDescriptorLayoutId(2),
      cmd_buf,
      {
        etna::Binding{0, ctx.worldViewMatrices.genBinding() },
      }
    );

    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipeline.getVkPipelineLayout(),
      0,
      {set.getVkSet(), ctx.sceneMgr->resources().getSetArrayed(), set2.getVkSet()},
      {}
    );

    cmd_buf.pushConstants(
      pipeline.getVkPipelineLayout(), 
      vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
      0,
      uint32_t(sizeof(PushConstants)),
      &pushConstants
    );

    cmd_buf.drawIndirect(commands.get(), 0, cnt, uint32_t(sizeof(vk::DrawIndirectCommand)));

  }
}



} /* namespace pipes */
