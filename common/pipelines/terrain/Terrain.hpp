#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "pipelines/perlin/Perlin.hpp"
#include "targets/GBuffer.hpp"

namespace pipes {

class TerrainPipeline {
public:
    using RenderTarget = targets::GBuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 4, "Terrain renders into 4 layers");

    TerrainPipeline() {}
    
    void allocate();
    
    void loadShaders();
    
    void loadTextures(SceneManager& scene_mgr);

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void prepare(vk::CommandBuffer cmd_buf, const RenderContext& context) { regenerateTerrainIfNeeded(cmd_buf, {context.camPos.x, context.camPos.z}, context.sceneMgr->terrain()); }
    void render (vk::CommandBuffer cmd_buf, const RenderContext& context);

private: 
    
    void regenerateTerrainIfNeeded(vk::CommandBuffer cmd_buf, glm::vec2 pos, scene::TerrainManager& mgr);
    
    void drawChunk(vk::CommandBuffer cmd_buf, targets::TerrainChunk& cur_chunk, const RenderContext& ctx, uint8_t chunk_mask = 0xF);
    void drawSubChunk(vk::CommandBuffer cmd_buf, targets::TerrainChunk& glob_chunk, const RenderContext& ctx, glm::uvec2 index, uint8_t chunk_mask = 0xF);


private:

    struct PushConstants {
        glm::vec2 base, extent;
        glm::vec3 camPos;
        int degree;
        float seaLevel = 14.f;
        float maxHeight = 64.f;
        glm::uint nHalfChunks  = 0;
        glm::uint subChunk = 0;
        glm::uint corner = 0;
        glm::uint worldId; 
        float time;
    } pushConstants;

    etna::GraphicsPipeline pipeline;
    etna::GraphicsPipeline pipelineDebug;
    pipes::PerlinPipeline terrainGenerator;

    bool wireframe = false;

    etna::DescriptorSet set1;
    etna::Sampler tilingSampler;
};

}
static_assert(Pipeline<pipes::TerrainPipeline>, "Terrain must be valid pipeline");