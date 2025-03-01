#include "Perlin.hpp"

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
PerlinPipeline::allocate()
{
    // auto& ctx = etna::get_context();
    auto& ctx = etna::get_context();
    m_sampler = etna::Sampler({
        .filter = vk::Filter::eLinear,
        .name = "perlinSample",
    });

    pipeline = ctx.getPipelineManager().createGraphicsPipeline("perlin_shader", {
        .fragmentShaderOutput = {
            .colorAttachmentFormats = RenderTarget::COLOR_ATTACHMENT_FORMATS,
            .depthAttachmentFormat = RenderTarget::DEPTH_ATTACHMENT_FORMAT
        },
    });
}

void 
PerlinPipeline::loadShaders() 
{
    etna::create_program(
        "perlin_shader",
        { PERLIN_PIPELINE_SHADERS_ROOT "perlin.vert.spv",
          PERLIN_PIPELINE_SHADERS_ROOT "perlin.frag.spv"}
    );
}

void 
PerlinPipeline::setup() 
{
    auto& pipelineManager = etna::get_context().getPipelineManager();

    pipeline = pipelineManager.createGraphicsPipeline(
    "perlin_shader",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = RenderTarget::COLOR_ATTACHMENT_FORMATS,
          .depthAttachmentFormat  = RenderTarget::DEPTH_ATTACHMENT_FORMAT,
        },
    });
}

void 
PerlinPipeline::drawGui()
{
    ImGui::SliderFloat("Pow", &pushConstant.pow, 0, 10);
}

void 
PerlinPipeline::debugInput(const Keyboard& /*kb*/)
{

}

void 
PerlinPipeline::render(vk::CommandBuffer cmd_buf, targets::TerrainChunk& prev)
{
    ETNA_PROFILE_GPU(cmd_buf, pipelines_perlin_upscale);
    auto shader = etna::get_shader_program("perlin_shader");
    auto set = etna::create_descriptor_set(shader.getDescriptorLayoutId(0),
        cmd_buf, {
            etna::Binding(0, prev.getImage(0).genBinding(m_sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal))
        }
    );
    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());
    cmd_buf.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipeline.getVkPipelineLayout(),
        0,
        {set.getVkSet()},
        {}
    );
    cmd_buf.pushConstants(pipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(pushConstant), &pushConstant);
    cmd_buf.draw(3, 1, 0, 0);
    nextOctave();
}

void generate_chunk(vk::CommandBuffer cmd_buf, PerlinPipeline& pipeline, targets::TerrainChunk& dst, targets::TerrainChunk& tmp, float frequency, std::size_t octaves)
{
    // a + a/2 + ... + a/2^{octaves-1} = a * (2 - 2 ^ {1-octaves}) => Base multiplier should be 1 / (2 - 2^{1-octaves}) = 2 ^{octaves-1} / (2^{octaves} - 1);
    pipeline.reset(dst.getStartPos(), dst.getExtentPos(), frequency, 1.f / (2.f - std::ldexp(2.f, -octaves)));
    std::array<targets::TerrainChunk*, 2> targets = {&dst, &tmp};
    if (octaves % 2 == 0) {
        std::swap(targets[0], targets[1]);
    }
    for(std::size_t i = 0; i < octaves; i++) {
        {
            auto& target = *targets[i & 1];
            auto& prev   = *targets[1 - (i & 1)];
            target.setState(cmd_buf,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
                vk::AccessFlagBits2::eColorAttachmentWrite, 
                vk::ImageLayout::eColorAttachmentOptimal, 
                vk::ImageAspectFlagBits::eColor
            );
            prev.setState(cmd_buf, 
                vk::PipelineStageFlagBits2::eFragmentShader, 
                vk::AccessFlagBits2::eShaderSampledRead, 
                vk::ImageLayout::eShaderReadOnlyOptimal, 
                vk::ImageAspectFlagBits::eColor
            );
            etna::flush_barriers(cmd_buf);
            etna::RenderTargetState renderTarget{
                cmd_buf,
                {{0, 0}, {dst.getResolution().x, dst.getResolution().y}},
                target.getColorAttachments(),
                {},
                BarrierBehavoir::eSuppressBarriers
            };
            pipeline.render(cmd_buf, *targets[1 - (i & 1)]);
        }
    }
}


} /* namespace pipes */
