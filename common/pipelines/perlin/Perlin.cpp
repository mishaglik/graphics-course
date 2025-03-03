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
    m_sampler = etna::Sampler({
        .filter = vk::Filter::eLinear,
        .name = "perlinSample",
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
    etna::create_program(
        "perlin_shader_compute",
        { PERLIN_PIPELINE_SHADERS_ROOT "perlin.comp.spv"}
    );
}

void 
PerlinPipeline::setup() 
{
    auto& pipelineManager = etna::get_context().getPipelineManager();

    pipeline = pipelineManager.createGraphicsPipeline(
    "perlin_shader",
    etna::GraphicsPipeline::CreateInfo{
        .blendingConfig = {
            .attachments = {RenderTarget::N_COLOR_ATTACHMENTS, 
                vk::PipelineColorBlendAttachmentState{
                    .blendEnable = vk::False,
                    .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
                }
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

    pipeline2 = pipelineManager.createComputePipeline("perlin_shader_compute", {});
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
#define NO_GRAPHIC_PIPELINE
void 
PerlinPipeline::render(vk::CommandBuffer cmd_buf, targets::TerrainChunk& dst, uint octaves)
{
    ETNA_PROFILE_GPU(cmd_buf, pipelines_perlin_upscale);
    pushConstant.octave = octaves;
    #ifndef NO_GRAPHIC_PIPELINE
    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());
    cmd_buf.pushConstants(pipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eFragment, 0, sizeof(pushConstant), &pushConstant);
    cmd_buf.draw(3, 1, 0, 0);
    (void)dst;
    #else
    auto pipelineInfo = etna::get_shader_program("perlin_shader_compute");
    
    auto set = etna::create_descriptor_set(
        pipelineInfo.getDescriptorLayoutId(0),
        cmd_buf,
        {
            etna::Binding{0, dst.getImage(0).genBinding(m_sampler.get(), vk::ImageLayout::eGeneral)},
            etna::Binding{1, dst.getImage(1).genBinding(m_sampler.get(), vk::ImageLayout::eGeneral)},
            etna::Binding{2, dst.getImage(2).genBinding(m_sampler.get(), vk::ImageLayout::eGeneral)},
        }
    );
        
    cmd_buf.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline2.getVkPipeline());
    
    cmd_buf.bindDescriptorSets(
        vk::PipelineBindPoint::eCompute,
        pipeline2.getVkPipelineLayout(),  
        0,
        {set.getVkSet()},
        {}
    );

    cmd_buf.pushConstants(pipeline2.getVkPipelineLayout(), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushConstant), &pushConstant);


    cmd_buf.dispatch((dst.getResolution().x + 31) / 32, (dst.getResolution().y + 31) / 32, 1);

    #endif
}

void generate_chunk(vk::CommandBuffer cmd_buf, PerlinPipeline& pipeline, targets::TerrainChunk& dst, targets::TerrainChunk& , float frequency, std::size_t octaves)
{
    // a + a/2 + ... + a/2^{octaves-1} = a * (2 - 2 ^ {1-octaves}) => Base multiplier should be 1 / (2 - 2^{1-octaves}) = 2 ^{octaves-1} / (2^{octaves} - 1);
    pipeline.reset(dst.getStartPos(), dst.getExtentPos(), frequency);
    #ifndef NO_GRAPHIC_PIPELINE
    
    {
        auto& target = dst;
        target.setState(cmd_buf,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
            vk::AccessFlagBits2::eColorAttachmentWrite, 
            vk::ImageLayout::eColorAttachmentOptimal, 
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
        pipeline.render(cmd_buf, dst, octaves);
    }
    #else
    auto& target = dst;
    target.setState(cmd_buf,
        vk::PipelineStageFlagBits2::eComputeShader, 
        vk::AccessFlagBits2::eShaderWrite, 
        vk::ImageLayout::eGeneral, 
        vk::ImageAspectFlagBits::eColor
    );
    etna::flush_barriers(cmd_buf);

    pipeline.render(cmd_buf, dst, octaves);
    #endif
}


} /* namespace pipes */
