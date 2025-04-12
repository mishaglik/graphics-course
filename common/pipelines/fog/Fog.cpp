#include "Fog.hpp"

#include <etna/BlockingTransferHelper.hpp>
#include <etna/Etna.hpp>
#include <etna/Profiling.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>

#include <imgui.h>
#include "stb_image.h"

#include "targets/Backbuffer.hpp"

namespace pipes {

void 
FogPipeline::allocate()
{
    // auto& ctx = etna::get_context();
}

void 
FogPipeline::loadShaders() 
{
    etna::create_program(
        "fog_shader",
        { FOG_PIPELINE_SHADERS_ROOT "fog.vert.spv",
          FOG_PIPELINE_SHADERS_ROOT "fog.frag.spv"}
    );
    etna::create_program(
      "fog_resolver",
      { FOG_PIPELINE_SHADERS_ROOT "fog_resolver.vert.spv",
        FOG_PIPELINE_SHADERS_ROOT "fog_resolver.frag.spv"}
  );
}

void 
FogPipeline::setup() 
{
    auto& pipelineManager = etna::get_context().getPipelineManager();

    pipeline = pipelineManager.createGraphicsPipeline(
    "fog_shader",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = RenderTarget::COLOR_ATTACHMENT_FORMATS,
          .depthAttachmentFormat  = RenderTarget::DEPTH_ATTACHMENT_FORMAT,
        },
    });

    pipelineResolve = pipelineManager.createGraphicsPipeline(
      "fog_resolver",
      etna::GraphicsPipeline::CreateInfo{
        .blendingConfig = {
          .attachments = {
            vk::PipelineColorBlendAttachmentState{
              .blendEnable = vk::True,
              .srcColorBlendFactor = vk::BlendFactor::eConstantColor,
              .dstColorBlendFactor = vk::BlendFactor::eOneMinusConstantColor,
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
          .depthTestEnable = vk::False,
        },
        .fragmentShaderOutput =
          {
            .colorAttachmentFormats = targets::Backbuffer::COLOR_ATTACHMENT_FORMATS,
            .depthAttachmentFormat  = targets::Backbuffer::DEPTH_ATTACHMENT_FORMAT,
          },
        .dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
            vk::DynamicState::eBlendConstants,
          },
      });

    defaultSampler = etna::Sampler({
      .name = "defaultSampler",
    });

    linearSampler = etna::Sampler({
      .filter = vk::Filter::eLinear,
      .name = "linearSampler",
    });
}

void 
FogPipeline::drawGui()
{
  ImGui::SliderInt("Integral steps", &pushConstants.integralSteps, 0, 100);
  ImGui::InputFloat("Horizon", &pushConstants.horizon);
  ImGui::InputFloat2("Wind", &wind.x);
  ImGui::SliderFloat("Frequency", &pushConstants.frequency, 0, 100, "%.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Precision", &pushConstants.precision, 0, 100, "%.3f", ImGuiSliderFlags_Logarithmic);
  ImGui::SliderFloat("Rarity", &pushConstants.rarity, 0, 1);
  ImGui::SliderFloat("Alpha", &alpha, 0.f, 1.f);
}

void 
FogPipeline::debugInput(const Keyboard& /*kb*/)
{

}

void
FogPipeline::render(vk::CommandBuffer cmd_buf, const targets::GBuffer& gbuffer, const RenderContext& ctx)
{
  pushConstants.position  = ctx.sceneMgr->getLights()[LightSource::Id::Sun].position;
  pushConstants.shift += wind * ctx.dt;
  ETNA_PROFILE_GPU(cmd_buf, pipelines_fog_render);
  {
    auto fogShader = etna::get_shader_program("fog_shader");

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());

    auto set = etna::create_descriptor_set(
      fogShader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, gbuffer.getImage(targets::GBuffer::ImageId::Wc).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{1, gbuffer.getImage(4).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{2, ctx.worldViewMatrices.genBinding()},
      }
    );
    
    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipeline.getVkPipelineLayout(),
      0,
      {set.getVkSet()},
      {}
    );

    cmd_buf.pushConstants(
      pipeline.getVkPipelineLayout(), 
      vk::ShaderStageFlagBits::eFragment,
      0,
      uint32_t(sizeof(PushConstants)),
      &pushConstants
    );

    cmd_buf.draw(3, 1, 0, 0);
  }
}

void 
FogPipeline::renderResolve(vk::CommandBuffer cmd_buf, const FogPipeline::RenderTarget& fog, const RenderContext&)
{
  ETNA_PROFILE_GPU(cmd_buf, pipelines_fog_render);
  {
    auto fogResolveShader = etna::get_shader_program("fog_resolver");

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipelineResolve.getVkPipeline());

    auto set = etna::create_descriptor_set(
      fogResolveShader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, fog.genBinding(linearSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}
      }
    );
    
    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipelineResolve.getVkPipelineLayout(),
      0,
      {set.getVkSet()},
      {}
    );

    std::array<float, 4> blend = {alpha, alpha, alpha, 0};
    cmd_buf.setBlendConstants(blend.data());
    cmd_buf.draw(3, 1, 0, 0);
  }
}


} /* namespace pipes */
