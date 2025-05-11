#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/Backbuffer.hpp"


namespace pipes {

class ParticlesPipeline {
public:
    using RenderTarget = targets::Backbuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 1, "Particles renders into single layer");

    ParticlesPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void prepare(vk::CommandBuffer /*cmd_buf*/, const RenderContext& context);

    
    void render (vk::CommandBuffer cmd_buf, const RenderContext& context);
    void barrierBefore(vk::CommandBuffer cmd_buf, const RenderContext& context);
    void barrierAfter (vk::CommandBuffer cmd_buf, const RenderContext& context);
private: 
private:
    etna::GraphicsPipeline graphicPipeline;
    etna::ComputePipeline  computePipeline;
    etna::Buffer commands, particles, emitters;
    std::vector<etna::Binding> bindings;
    
    struct PushConstants {
        glm::mat4x4 mProjView;
        float frameTime;
        float dt;
    } pushConstants;
};

}
static_assert(Pipeline<pipes::ParticlesPipeline>, "Particles must be valid pipeline");