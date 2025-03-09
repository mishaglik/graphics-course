#include "AA.hpp"

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
AAPipeline::allocate()
{
    // auto& ctx = etna::get_context();
}

void 
AAPipeline::loadShaders() 
{
    etna::create_program(
        "aa_shader",
        { AA_PIPELINE_SHADERS_ROOT "aa.vert.spv",
          AA_PIPELINE_SHADERS_ROOT "aa.frag.spv"}
    );
}

void 
AAPipeline::setup() 
{
    auto& pipelineManager = etna::get_context().getPipelineManager();

    pipeline = pipelineManager.createGraphicsPipeline(
    "aa_shader",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = RenderTarget::COLOR_ATTACHMENT_FORMATS,
          .depthAttachmentFormat  = RenderTarget::DEPTH_ATTACHMENT_FORMAT,
        },
    });
    defaultSampler = etna::Sampler({
      .filter = vk::Filter::eLinear,
      .name = "aa sampler"
    });
}

void 
AAPipeline::drawGui()
{
  ImGui::Checkbox("Enabled", &m_enabled);
  ImGui::LabelText("FXAA", "Algorithm:");
}

void 
AAPipeline::debugInput(const Keyboard& /*kb*/)
{

}

void
AAPipeline::render(vk::CommandBuffer cmd_buf, targets::Backbuffer& source, const RenderContext& /*ctx*/)
{
  ETNA_PROFILE_GPU(cmd_buf, pipelines_aa_render);
  {
    auto aaShader = etna::get_shader_program("aa_shader");

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());

    auto set = etna::create_descriptor_set(
      aaShader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, source.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}
      }
    );
    
    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipeline.getVkPipelineLayout(),
      0,
      {set.getVkSet()},
      {}
    );

    if constexpr (sizeof(pushConstants) > 1) {
      cmd_buf.pushConstants(
        pipeline.getVkPipelineLayout(), 
        vk::ShaderStageFlagBits::eFragment,
        0,
        uint32_t(sizeof(pushConstants)),
        &pushConstants
      );
    }

    cmd_buf.draw(3, 1, 0, 0);
  }
}

} /* namespace pipes */
