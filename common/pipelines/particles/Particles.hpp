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

    void prepare(vk::CommandBuffer /*cmd_buf*/, const RenderContext& context) { context.sceneMgr->particles().update(context.frameTime); }

    void render (vk::CommandBuffer cmd_buf, const RenderContext& context);
private: 
    
private:
    etna::GraphicsPipeline pipeline;
    etna::Buffer commands;

    struct PushConstants {
        glm::mat4x4 mProjView;
        glm::uint material;
    } pushConstants;
};

}
static_assert(Pipeline<pipes::ParticlesPipeline>, "Particles must be valid pipeline");