#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "pipelines/perlin/Perlin.hpp"
#include "targets/GBuffer.hpp"
#include "targets/Buffer.hpp"


namespace pipes {

class TerrainPipeline {
public:
    using RenderTarget = targets::GBuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 4, "Terrain renders into 4 layers");

    TerrainPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void prepare(vk::CommandBuffer cmd_buf, const RenderContext& /*context*/) { regenerateTerrainIfNeeded(cmd_buf); }
    RenderTarget& render (vk::CommandBuffer cmd_buf, RenderTarget& target, const RenderContext& context);

private: 
    
    void regenerateTerrainIfNeeded(vk::CommandBuffer cmd_buf);
    
    void drawChunk(vk::CommandBuffer cmd_buf, targets::TerrainChunk& cur_chunk);


private:

    struct PushConstants {
        glm::vec2 base, extent;
        glm::mat4x4 mat; 
        glm::vec3 camPos;
        int degree;
        float seaLevel = 14.f;
        float maxHeight = 64.f;
    } pushConstants;

    etna::GraphicsPipeline pipeline;
    etna::GraphicsPipeline pipelineDebug;
    
    PerlinPipeline terrainGenerator;
    targets::TerrainChunk chunk;
    targets::TerrainChunk tmp;

    int terrainScale = 12;
    bool wireframe = false;

    float startFrequency = 1.f;

    
    bool terrainValid = false;

    uint64_t heightMapResolution = 1024;
    static const uint64_t MAX_TESCELLATION = 64; //FIXME: Use vulkan info
};

}
static_assert(Pipeline<pipes::TerrainPipeline>, "Terrain must be valid pipeline");