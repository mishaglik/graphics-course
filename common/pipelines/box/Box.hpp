#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/GBuffer.hpp"


namespace pipes {

class BoxPipeline {
public:
    using RenderTarget = targets::GBuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 4, "Box renders into single layer");

    BoxPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void prepare(vk::CommandBuffer /*cmd_buf*/, const RenderContext& /*context*/) {}

    void render (vk::CommandBuffer cmd_buf, const RenderContext& context, bool wireframe);
private: 
    
private:
    etna::GraphicsPipeline pipeline_fill;
    etna::GraphicsPipeline pipeline_wireframe;
    
    struct PushConstants {
        glm::mat4x4 mInvPv;
        glm::uint worldId;
    } pushConstants;
};

}
static_assert(Pipeline<pipes::BoxPipeline>, "Box must be valid pipeline");