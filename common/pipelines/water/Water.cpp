#include "Water.hpp"

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
WaterPipeline::allocate()
{
    // auto& ctx = etna::get_context();
    m_sampler = etna::Sampler({
        .filter = vk::Filter::eLinear,
        .name = "waterSample",
    });
}

void 
WaterPipeline::loadShaders() 
{
    etna::create_program(
        "water_shader_compute",
        { WATER_PIPELINE_SHADERS_ROOT "water.comp.spv"}
    );
}

void 
WaterPipeline::setup() 
{
    auto& pipelineManager = etna::get_context().getPipelineManager();

    pipeline2 = pipelineManager.createComputePipeline("water_shader_compute", {});
}

void 
WaterPipeline::drawGui()
{
    ImGui::InputFloat2("Wind", &pushConstant.wind.x);
}

void 
WaterPipeline::debugInput(const Keyboard& /*kb*/)
{

}
void 
WaterPipeline::render(vk::CommandBuffer cmd_buf, targets::WaterChunk& dst, float time)
{
    ETNA_PROFILE_GPU(cmd_buf, pipelines_water_render);
    auto resolution = dst.getResolution().x;
    if (dst.getResolution().y != resolution && (resolution & (resolution - 1)) != 0) {
        spdlog::error("Bad image resolution: {}x{}. Both must be same power of 2", dst.getResolution().x, dst.getResolution().y);
    }

    pushConstant.time = time;
    auto pipelineInfo = etna::get_shader_program("water_shader_compute");
    
    auto set = etna::create_descriptor_set(
        pipelineInfo.getDescriptorLayoutId(0),
        cmd_buf,
        {
            etna::Binding{0, dst.genBinding(m_sampler.get(), vk::ImageLayout::eGeneral)},
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

    cmd_buf.dispatch(1, 1, 1);
}

} /* namespace pipes */
