#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/TerrainChunk.hpp"


namespace pipes {

class PerlinPipeline {
public:
    using RenderTarget = targets::TerrainChunk;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 1, "Perlin renders into single layer");

    PerlinPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    etna::Sampler& getSampler() { return m_sampler; }

    void render(vk::CommandBuffer cmd_buf, targets::TerrainChunk& prev);

    void reset(glm::vec2 pos, glm::vec2 extent, float frequency = 1, float amplitude = 1) {
        pushConstant.start = pos;
        pushConstant.extent = extent;
        pushConstant.frequency = frequency;
        pushConstant.amplitude = amplitude;
        pushConstant.octave = 0;
    }

    void nextOctave() {
        pushConstant.frequency *= 2;
        pushConstant.amplitude /= 2;
        pushConstant.octave++;
    }

    void setPower(float pow) { pushConstant.pow = pow; }

private: 
    void upscale(vk::CommandBuffer cmd_buf);
    
private:
    etna::GraphicsPipeline pipeline;
    etna::Sampler m_sampler;
    struct PushConstant {
        glm::vec2 start;
        glm::vec2 extent;
        float frequency = 1;
        float amplitude = 1;
        float pow = 1;
        glm::uint octave;

    } pushConstant;
};

void generate_chunk(vk::CommandBuffer cmd_buf, PerlinPipeline& pipeline, targets::TerrainChunk& dst, targets::TerrainChunk& tmp, float frequency, std::size_t octaves);

}

// static_assert(Pipeline<pipes::PerlinPipeline>, "Perlin must be valid pipeline");