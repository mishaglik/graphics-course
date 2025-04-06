#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/Buffer.hpp"
#include "targets/GBuffer.hpp"


namespace pipes {

class FogPipeline {
public:
    using RenderTarget = targets::FogBuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 1, "Fog renders into single layer");

    FogPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void render(vk::CommandBuffer cmd_buf, const targets::GBuffer& gbuffer, const RenderContext& context);

    void renderResolve(vk::CommandBuffer cmd_buf, const RenderTarget& fog, const RenderContext& context);
private: 
    
private:
    etna::GraphicsPipeline pipeline;
    etna::GraphicsPipeline pipelineResolve;

    struct PushConstants {
        glm::mat4 mProj;
        glm::mat4 mView;
        glm::vec4 position;
        glm::vec2 shift;
        float horizon = 100;
        float precision = 10;
        float frequency = 0.1f;
        float rarity = 0.6f;
        int integralSteps = 20;
    } pushConstants;

    float alpha = 0.3f;
    glm::vec2 wind = {0.3f, 1.f};
    etna::Sampler defaultSampler;
    etna::Sampler linearSampler;
};

}
static_assert(Pipeline<pipes::FogPipeline>, "Fog must be valid pipeline");