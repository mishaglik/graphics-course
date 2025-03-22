#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "pipelines/perlin/Perlin.hpp"
#include "targets/Backbuffer.hpp"
#include "targets/GBuffer.hpp"

namespace pipes {

class TerrainTransparentPipeline {
public:
    using RenderTarget = targets::Backbuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 1, "Terrain transparent renders into 1 layer");

    TerrainTransparentPipeline() {}
    
    void allocate();
    
    void loadShaders();
    
    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void render (vk::CommandBuffer cmd_buf, targets::GBuffer& source, const RenderContext& ctx, const etna::Image& skybox);

private: 
    
    
    void drawChunk(vk::CommandBuffer cmd_buf, targets::TerrainChunk& cur_chunk, uint8_t chunk_mask = 0xF);
    void drawSubChunk(vk::CommandBuffer cmd_buf, targets::TerrainChunk& glob_chunk, glm::uvec2 index, uint8_t chunk_mask = 0xF);


private:

    struct PushConstants {
        glm::vec2 base, extent;
        glm::mat4x4 mat; 
        glm::vec3 camPos;
        int degree;
        float seaLevel = 14.f;
        float maxHeight = 64.f;
        glm::uint nHalfChunks  = 0;
        glm::uint subChunk = 0;
        glm::uint corner = 0;
        float time;
    } pushConstants;

    etna::GraphicsPipeline pipeline;
    etna::GraphicsPipeline pipelineDebug;

    bool wireframe = false;

    etna::DescriptorSet set1;
    etna::Sampler tilingSampler;
};

}
static_assert(Pipeline<pipes::TerrainTransparentPipeline>, "Terrain must be valid pipeline");