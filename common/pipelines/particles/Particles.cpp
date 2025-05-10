#include "Particles.hpp"

#include <etna/BlockingTransferHelper.hpp>
#include <etna/Etna.hpp>
#include <etna/Profiling.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>

#include <glm/gtc/round.hpp>
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
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer,
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

    etna::create_program("particles_compute", {PARTICLES_PIPELINE_SHADERS_ROOT "particles.comp.spv"});
}

void 
ParticlesPipeline::setup() 
{
    auto& pipelineManager = etna::get_context().getPipelineManager();

    graphicPipeline = pipelineManager.createGraphicsPipeline(
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

    computePipeline = pipelineManager.createComputePipeline("particles_compute", {});
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
  pushConstants.frameTime = (float)ctx.frameTime;
  ETNA_PROFILE_GPU(cmd_buf, pipelines_particles_render);
  {
    auto particlesShader = etna::get_shader_program("particles_shader");

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicPipeline.getVkPipeline());


    auto& particleManager = ctx.sceneMgr->particles();


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
      graphicPipeline.getVkPipelineLayout(),
      0,
      {set.getVkSet(), ctx.sceneMgr->resources().getSetArrayed(), set2.getVkSet()},
      {}
    );

    cmd_buf.pushConstants(
      graphicPipeline.getVkPipelineLayout(), 
      vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
      0,
      uint32_t(sizeof(PushConstants)),
      &pushConstants
    );
    uint32_t cnt = particleManager.size();

    cmd_buf.drawIndirect(commands.get(), 0, cnt, uint32_t(sizeof(vk::DrawIndirectCommand)));

  }
}

void 
ParticlesPipeline::prepare(vk::CommandBuffer cmd_buf, const RenderContext& context) { 
  glm::vec4 zView = glm::vec4(context.worldView[0][2], context.worldView[1][2], context.worldView[2][2], context.worldView[3][2]);

  auto& particleManager = context.sceneMgr->particles();

  particleManager.update(zView, (float)context.frameTime); 

  vk::DrawIndirectCommand* cmds = reinterpret_cast<vk::DrawIndirectCommand*>(commands.data());
  EmitterInfo* ems              = reinterpret_cast<EmitterInfo*>(emitters.data());
  ParticleInfo* pts             = reinterpret_cast<ParticleInfo*>(particles.data());
  uint32_t cnt = 0;
  uint32_t parts = 0;
  for(auto& emitter : particleManager) {
    //TODO: Cull
    cmds[cnt] = vk::DrawIndirectCommand{
      .vertexCount = emitter.verticesPerParticle(),
      .instanceCount = uint32_t(emitter.size()),
      .firstVertex = 0,
      .firstInstance = parts,
    };
    ems[cnt] = emitter.info();
    uint32_t pos = parts;
    for(auto& particle: emitter) {
      pts[pos++].position = particle.position;
    }
    parts += glm::ceilPowerOfTwo(std::max(uint32_t(emitter.size()), 64u));
    if(parts > N_MAX_PARTICLES_PER_DRAW) {
      //FIXME - Implement; 
      spdlog::warn("Drawing more than {} particles is not implemented. Truncating", N_MAX_PARTICLES_PER_DRAW);
      parts = N_MAX_PARTICLES_PER_DRAW;
      break;
    }
    cnt++;
  }

  barrierBefore(cmd_buf, context);
  {
    auto shader = etna::get_shader_program("particles_compute");

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline.getVkPipeline());
    auto set0 = etna::create_descriptor_set(
      shader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        {0, emitters .genBinding()},
        {1, particles.genBinding()},
        {2, commands .genBinding()}
      }
    );
    auto set1 = etna::create_descriptor_set(
      shader.getDescriptorLayoutId(1),
      cmd_buf,
      {
        {0, context.worldViewMatrices.genBinding()}
      }
    );

    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eCompute,
      computePipeline.getVkPipelineLayout(),
      0,
      {set0.getVkSet(), set1.getVkSet()},
      {}
    );

    cmd_buf.pushConstants(
      computePipeline.getVkPipelineLayout(), 
      vk::ShaderStageFlagBits::eCompute,
      0,
      uint32_t(sizeof(PushConstants)),
      &pushConstants
    );

    cmd_buf.dispatch(cnt, 1, 1);
  }
  barrierAfter(cmd_buf, context);

}

void 
ParticlesPipeline::barrierAfter(vk::CommandBuffer cmd_buf, const RenderContext& /* ctx */) {
  std::array<vk::BufferMemoryBarrier2, 3> bmb = {
    vk::BufferMemoryBarrier2{
      .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
      .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eVertexShader,
      .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
      .buffer = particles.get(),
      .size = VK_WHOLE_SIZE,
     },
     vk::BufferMemoryBarrier2{
      .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
      .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
      .dstStageMask = vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eFragmentShader,
      .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
      .buffer = emitters.get(),
      .size = VK_WHOLE_SIZE,
     },
     vk::BufferMemoryBarrier2{
      .srcStageMask = vk::PipelineStageFlagBits2::eComputeShader,
      .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eDrawIndirect,
      .dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead,
      .buffer = commands.get(),
      .size = VK_WHOLE_SIZE,
     },
   };
  vk::DependencyInfo depInfo
  {
    .dependencyFlags = vk::DependencyFlagBits::eByRegion,
    .bufferMemoryBarrierCount=bmb.size(),
    .pBufferMemoryBarriers = bmb.data(),
  };
  cmd_buf.pipelineBarrier2(depInfo);
}

void 
ParticlesPipeline::barrierBefore(vk::CommandBuffer cmd_buf, const RenderContext& /* ctx */) {
  std::array<vk::BufferMemoryBarrier2, 3> bmb = {
    vk::BufferMemoryBarrier2{
      .srcStageMask = vk::PipelineStageFlagBits2::eVertexShader,
      .srcAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
      .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
      .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
      .buffer = particles.get(),
      .size = VK_WHOLE_SIZE,
     },
     vk::BufferMemoryBarrier2{
      .srcStageMask = vk::PipelineStageFlagBits2::eVertexShader | vk::PipelineStageFlagBits2::eFragmentShader,
      .srcAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
      .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
      .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderStorageRead,
      .buffer = emitters.get(),
      .size = VK_WHOLE_SIZE,
     },
     vk::BufferMemoryBarrier2{
      .srcStageMask = vk::PipelineStageFlagBits2::eDrawIndirect,
      .srcAccessMask = vk::AccessFlagBits2::eIndirectCommandRead,
      .dstStageMask = vk::PipelineStageFlagBits2::eComputeShader,
      .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
      .buffer = commands.get(),
      .size = VK_WHOLE_SIZE,
     },
   };
  vk::DependencyInfo depInfo
  {
    .dependencyFlags = vk::DependencyFlagBits::eByRegion,
    .bufferMemoryBarrierCount=bmb.size(),
    .pBufferMemoryBarriers = bmb.data(),
  };
  cmd_buf.pipelineBarrier2(depInfo);
}

} /* namespace pipes */
