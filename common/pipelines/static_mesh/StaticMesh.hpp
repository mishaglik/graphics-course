#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/GBuffer.hpp"


namespace pipes {

class StaticMeshPipeline {
public:
    using RenderTarget = targets::GBuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 4, "StaticMesh renders into single layer");

    const std::size_t N_MAX_INSTANCES = 1 << 14;


    StaticMeshPipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void reserve(std::size_t n);

    void render(vk::CommandBuffer cmd_buf, const RenderContext& context);

private: 


    void prepareFrame(const RenderContext& context);

private:
    struct DrawCmd {
        vk::DrawIndexedIndirectCommand cmd;
        glm::uint material;
        glm::uint _pad[2];
    };
    static_assert(sizeof(DrawCmd) == 8 * sizeof(glm::uint), "Size of draw cmd must be sync with shader");

    etna::GraphicsPipeline pipeline;
    etna::GraphicsPipeline shadowPipeline;
    etna::Buffer instanceMatricesBuf;
    etna::Buffer drawCommandsBuf;
    etna::Sampler defaultSampler;
    struct PushConstants
    {
        glm::mat4x4 model;
        glm::vec4 color, emr_factors;
        glm::vec3 pos{16, 14, -64};
        glm::uint  instIdx;
        glm::uint wId, materialId;
    } pushConst2M;

    std::vector<std::size_t> nInstances;
    bool normalMap = true;
    bool enableCulling = false;

};

}
static_assert(Pipeline<pipes::StaticMeshPipeline>, "StaticMesh must be valid pipeline");