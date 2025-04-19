#include "Box.hpp"

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
BoxPipeline::allocate()
{
    // auto& ctx = etna::get_context();
}

void 
BoxPipeline::loadShaders() 
{
    etna::create_program(
        "box_shader",
        { BOX_PIPELINE_SHADERS_ROOT "box.vert.spv",
          BOX_PIPELINE_SHADERS_ROOT "box.frag.spv"}
    );
}

void 
BoxPipeline::setup() 
{
    auto& pipelineManager = etna::get_context().getPipelineManager();
    pipeline_fill = pipelineManager.createGraphicsPipeline(
    "box_shader",
    etna::GraphicsPipeline::CreateInfo{
      .inputAssemblyConfig = {
        .topology = vk::PrimitiveTopology::eTriangleStrip,
      },
      .rasterizationConfig = {
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eClockwise,
        .lineWidth = 1.f,
      },
      .blendingConfig = {
        .attachments = {
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
          },
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
          },
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
          },
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::False,
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
    pipeline_wireframe = pipelineManager.createGraphicsPipeline(
      "box_shader",
      etna::GraphicsPipeline::CreateInfo{
        .inputAssemblyConfig = {
          .topology = vk::PrimitiveTopology::eTriangleStrip,
        },
        .rasterizationConfig = {
          .polygonMode = vk::PolygonMode::eLine,
          .cullMode = vk::CullModeFlagBits::eNone,
          .frontFace = vk::FrontFace::eClockwise,
          .lineWidth = 1.f,
        },
        .blendingConfig = {
          .attachments = {
            vk::PipelineColorBlendAttachmentState{
              .blendEnable = vk::False,
              .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
            },
            vk::PipelineColorBlendAttachmentState{
              .blendEnable = vk::False,
              .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
            },
            vk::PipelineColorBlendAttachmentState{
              .blendEnable = vk::False,
              .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
            },
            vk::PipelineColorBlendAttachmentState{
              .blendEnable = vk::False,
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
BoxPipeline::drawGui() {}

void 
BoxPipeline::debugInput(const Keyboard& /*kb*/)
{

}

void
BoxPipeline::render(vk::CommandBuffer cmd_buf, const RenderContext& ctx, bool wireframe)
{
  pushConstants.mInvPv = ctx.curCamIpv;
  pushConstants.worldId = ctx.worldId;
  ETNA_PROFILE_GPU(cmd_buf, pipelines_box_render);
  {
    auto& pipeline = wireframe ? pipeline_wireframe : pipeline_fill;
    auto boxShader = etna::get_shader_program("box_shader");

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());

    auto set = etna::create_descriptor_set(
      boxShader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, ctx.worldViewMatrices.genBinding()},
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
      vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex,
      0,
      uint32_t(sizeof(PushConstants)),
      &pushConstants
    );

    cmd_buf.draw(14, 1, 0, 0);
  }
}

} /* namespace pipes */
