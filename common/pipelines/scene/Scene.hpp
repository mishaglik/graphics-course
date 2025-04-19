#pragma once

#include "pipelines/Pipeline.hpp"
#include "pipelines/RenderContext.hpp"

#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>

#include "targets/GBuffer.hpp"

#include "pipelines/static_mesh/StaticMesh.hpp"
#include "pipelines/terrain/Terrain.hpp"
#include "pipelines/box/Box.hpp"


namespace pipes {

class ScenePipeline {
public:
    using RenderTarget = targets::GBuffer;
    static_assert(RenderTarget::N_COLOR_ATTACHMENTS == 4, "Scene renders into 4 layers");

    ScenePipeline() {}
    
    void allocate();
    
    void loadShaders();

    void setup();
    
    void drawGui();
    
    void debugInput(const Keyboard& /*kb*/);

    void prepare(vk::CommandBuffer cmd_buf, const RenderContext& context);

    void render(vk::CommandBuffer cmd_buf, RenderContext& context);

    void loadScene(SceneManager& scene_mgr);
private: 
    
private:
    
    StaticMeshPipeline staticMeshPipeline;
    TerrainPipeline    terrainPipeline;
    BoxPipeline        boxPipeline;

    bool enableStaticMesh = true;
    bool enableTerrain    = true;
    bool enableBox        = false;

    bool showSunPov = false;
    bool sunWireframe = true;
    bool showUserPov = true;
    bool userWireframe = true;
    bool boxSelfRender = false;


};

}
static_assert(Pipeline<pipes::ScenePipeline>, "Scene must be valid pipeline");