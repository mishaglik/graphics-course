#pragma once

#include "etna/ComputePipeline.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/Buffer.hpp"
#include "wsi/Keyboard.hpp"


namespace pipes {

class WaterPipeline {
public:
    using RenderTarget = targets::WaterChunk;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 1, "Water renders into single layer");

    WaterPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    etna::Sampler& getSampler() { return m_sampler; }

    void render(vk::CommandBuffer cmd_buf, targets::WaterChunk& target, float time);

private:
    etna::Sampler m_sampler;
    etna::ComputePipeline pipeline2;
    struct PushConstant {
        float time;
        glm::vec2 wind = {1.f, 0.f};
    } pushConstant;
};

}

// static_assert(Pipeline<pipes::WaterPipeline>, "Water must be valid pipeline");