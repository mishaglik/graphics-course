#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "pipelines/perlin/Perlin.hpp"
#include "targets/GBuffer.hpp"
#include "Cliplevel.hpp"


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

    void prepare(vk::CommandBuffer cmd_buf, const RenderContext& context) { regenerateTerrainIfNeeded(cmd_buf, {context.camPos.x, context.camPos.z}); }
    RenderTarget& render (vk::CommandBuffer cmd_buf, RenderTarget& target, const RenderContext& context);

private: 
    
    void regenerateTerrainIfNeeded(vk::CommandBuffer cmd_buf, glm::vec2 pos);
    
    void drawChunk(vk::CommandBuffer cmd_buf, targets::TerrainChunk& cur_chunk, uint8_t chunk_mask = 0xF);


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

    } pushConstants;

    etna::GraphicsPipeline pipeline;
    etna::GraphicsPipeline pipelineDebug;
    
    PerlinPipeline terrainGenerator;
    static const std::size_t N_CLIP_LEVELS = 5;
    std::array<pipelines::terrain::Cliplevel, N_CLIP_LEVELS> levels;

    targets::TerrainChunk tmp;

    int terrainScale = 7;
    int activeLayers = 3;
    bool wireframe = false;

    float startFrequency = 0.003f;

    
    bool terrainValid = false;

    uint64_t heightMapResolution = 256;
    static const uint64_t MAX_TESCELLATION = 64; //FIXME: Use vulkan info

    std::array<Texture::Id, 5> m_textures;
    etna::DescriptorSet set1;
    etna::Sampler tilingSampler;
};

}
static_assert(Pipeline<pipes::TerrainPipeline>, "Terrain must be valid pipeline");