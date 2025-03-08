#pragma once

#include "etna/ComputePipeline.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/TerrainChunk.hpp"
#include "wsi/Keyboard.hpp"


namespace pipes {

class PerlinPipeline {
public:
    using RenderTarget = targets::TerrainChunk;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 3, "Perlin renders into single layer");

    PerlinPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    etna::Sampler& getSampler() { return m_sampler; }

    void render(vk::CommandBuffer cmd_buf, targets::TerrainChunk&, uint32_t octaves);

    void reset(glm::vec2 pos, glm::vec2 extent, float frequency = 1, float amplitude = 1) {
        pushConstant.start = pos;
        pushConstant.extent = extent;
        pushConstant.frequency = frequency;
        pushConstant.amplitude = amplitude;
    }

    void setPower(float pow) { pushConstant.pow = pow; }

    void setFull() { pushConstant.texStart = {0.f, 0.f}; pushConstant.texExtent = {1.f, 1.f};}
    void setSubchunk(glm::uvec2 index) { pushConstant.texExtent = {.25f, .25f}; pushConstant.texStart = static_cast<glm::vec2>(index) / 4.f;}
    void setSubregion(glm::vec2 tex_start, glm::vec2 tex_extent) { pushConstant.texExtent = tex_extent; pushConstant.texStart = tex_start; }

private: 
    void upscale(vk::CommandBuffer cmd_buf);
    
private:
    etna::GraphicsPipeline pipeline;
    etna::ComputePipeline pipeline2;
    etna::Sampler m_sampler;
    struct PushConstant {
        glm::vec2 start;
        glm::vec2 extent;
        float frequency = 1;
        float amplitude = 1;
        float pow = 1.25;
        glm::uint octave;
        glm::vec2 texStart ;
        glm::vec2 texExtent;
    } pushConstant;
};

void generate_chunk(vk::CommandBuffer cmd_buf, PerlinPipeline& pipeline, targets::TerrainChunk& dst, targets::TerrainChunk& tmp, float frequency, std::size_t octaves);
void update_chunk(vk::CommandBuffer cmd_buf, PerlinPipeline& pipeline, targets::TerrainChunk& dst, targets::TerrainChunk& chk, glm::uvec2 index, float frequency, std::size_t octaves);

}

// static_assert(Pipeline<pipes::PerlinPipeline>, "Perlin must be valid pipeline");