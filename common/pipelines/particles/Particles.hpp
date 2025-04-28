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

    void prepare(vk::CommandBuffer /*cmd_buf*/, const RenderContext& context) { 
        glm::vec4 zView = glm::vec4(context.worldView[0][2], context.worldView[1][2], context.worldView[2][2], context.worldView[3][2]);
        context.sceneMgr->particles().update(zView, (float)context.frameTime); 
    }

    void render (vk::CommandBuffer cmd_buf, const RenderContext& context);
private: 
private:
    etna::GraphicsPipeline pipeline;
    etna::Buffer commands, particles, emitters;
    
    struct PushConstants {
        glm::mat4x4 mProjView;
        float frameTime;
    } pushConstants;
};

}
static_assert(Pipeline<pipes::ParticlesPipeline>, "Particles must be valid pipeline");