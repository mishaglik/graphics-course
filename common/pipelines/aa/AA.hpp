#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/Frame.hpp"
#include "targets/Backbuffer.hpp"


namespace pipes {

class AAPipeline {
public:
    using RenderTarget = targets::Frame;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 1, "Boilerplate renders into single layer");

    AAPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void render(vk::CommandBuffer cmd_buf, targets::Backbuffer& source, const RenderContext& context);
    bool enabled() { return m_enabled; }
private: 
    struct {

    } pushConstants; 

private:
    etna::GraphicsPipeline pipeline;
    etna::Sampler defaultSampler;

    bool m_enabled = true;
};

}
static_assert(Pipeline<pipes::AAPipeline>, "Boilerplate must be valid pipeline");