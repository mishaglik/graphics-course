#include "Scene.hpp"

#include <etna/BlockingTransferHelper.hpp>
#include <etna/Etna.hpp>
#include <etna/Profiling.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>

#include <imgui.h>

#include "stb_image.h"

namespace pipes {

void 
ScenePipeline::allocate()
{
  staticMeshPipeline.allocate();
  terrainPipeline   .allocate();
}

void 
ScenePipeline::loadShaders() 
{
  staticMeshPipeline.loadShaders();
  terrainPipeline   .loadShaders();
}

void 
ScenePipeline::prepare(vk::CommandBuffer cmd_buf, const RenderContext& context)
{
  // if(enableStaticMesh) staticMeshPipeline.prepare(cmd_buf, context);
  if(enableTerrain)    terrainPipeline   .prepare(cmd_buf, context);
}


void 
ScenePipeline::setup() 
{
  staticMeshPipeline.setup();
  terrainPipeline   .setup();
}

void 
ScenePipeline::drawGui()
{
  ImGui::SeparatorText("Static mesh");
  {
    ImGui::Checkbox("Enabled##static_mesh", &enableStaticMesh);
    
    ImGui::BeginDisabled(enableStaticMesh);
    staticMeshPipeline.drawGui();
    ImGui::EndDisabled();
  }
  
  ImGui::SeparatorText("Terrain");
  {
    ImGui::Checkbox("Enabled##terrain", &enableTerrain);
    
    ImGui::BeginDisabled(enableTerrain);
    terrainPipeline.drawGui();
    ImGui::EndDisabled();
  }
}

void 
ScenePipeline::debugInput(const Keyboard& kb)
{
  staticMeshPipeline.debugInput(kb);
  terrainPipeline   .debugInput(kb);
}

void
ScenePipeline::render(vk::CommandBuffer cmd_buf, const RenderContext& ctx)
{
  if(enableStaticMesh) staticMeshPipeline.render(cmd_buf, ctx);
  if(enableTerrain)    terrainPipeline   .render(cmd_buf, ctx);
}


void 
ScenePipeline::loadScene(SceneManager& scene_mgr)
{
  staticMeshPipeline.reserve(scene_mgr.getRenderElements().size());
  terrainPipeline.loadTextures(scene_mgr);
}

} /* namespace pipes */
