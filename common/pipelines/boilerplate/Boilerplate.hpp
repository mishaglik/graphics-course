#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/Backbuffer.hpp"


namespace pipes {

class AAPipeline {
public:
    using RenderTarget = targets::Backbuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 1, "Boilerplate renders into single layer");

    AAPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void render(vk::CommandBuffer cmd_buf, const RenderContext& context);
private: 
    
private:
    etna::GraphicsPipeline pipeline;
};

}
static_assert(Pipeline<pipes::AAPipeline>, "Boilerplate must be valid pipeline");