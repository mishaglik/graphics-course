#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/Backbuffer.hpp"


namespace pipes {

class BoilerplatePipeline {
public:
    using RenderTarget = targets::Backbuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 1, "Boilerplate renders into single layer");

    BoilerplatePipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void prepare(vk::CommandBuffer /*cmd_buf*/, const RenderContext& /*context*/) {}

    void render (vk::CommandBuffer cmd_buf, const RenderContext& context);
private: 
    
private:
    etna::GraphicsPipeline pipeline;
    
    struct PushConstants {
        glm::mat4x4 mProjView;
    } pushConstants;
};

}
static_assert(Pipeline<pipes::BoilerplatePipeline>, "Boilerplate must be valid pipeline");