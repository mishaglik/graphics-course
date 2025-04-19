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
  boxPipeline       .allocate();
}

void 
ScenePipeline::loadShaders() 
{
  staticMeshPipeline.loadShaders();
  terrainPipeline   .loadShaders();
  boxPipeline       .loadShaders();
}

void 
ScenePipeline::prepare(vk::CommandBuffer cmd_buf, const RenderContext& context)
{
  // if(enableStaticMesh) staticMeshPipeline.prepare(cmd_buf, context);
  if(enableTerrain)    terrainPipeline   .prepare(cmd_buf, context);
  // if(enableBox)    boxPipeline   .prepare(cmd_buf, context);
}


void 
ScenePipeline::setup() 
{
  staticMeshPipeline.setup();
  terrainPipeline   .setup();
  boxPipeline      .setup();
}

void 
ScenePipeline::drawGui()
{
  ImGui::SeparatorText("Static mesh");
  {
    ImGui::Checkbox("Enabled##static_mesh", &enableStaticMesh);
    
    ImGui::BeginDisabled(!enableStaticMesh);
    staticMeshPipeline.drawGui();
    ImGui::EndDisabled();
  }
  
  ImGui::SeparatorText("Terrain");
  {
    ImGui::Checkbox("Enabled##terrain", &enableTerrain);
    
    ImGui::BeginDisabled(!enableTerrain);
    terrainPipeline.drawGui();
    ImGui::EndDisabled();
  }

  ImGui::SeparatorText("Box");
  {
    ImGui::Checkbox("Enabled##box", &enableBox);
    ImGui::Checkbox("Sun##box",  &showSunPov);
    ImGui::Checkbox("User##box", &showUserPov);
    ImGui::Checkbox("SunWire##box",  &sunWireframe);
    ImGui::Checkbox("UserWire##box", &userWireframe);
    ImGui::Checkbox("SelfRender", &boxSelfRender);
    
    ImGui::BeginDisabled(!enableBox);
    boxPipeline.drawGui();
    ImGui::EndDisabled();
  }
}

void 
ScenePipeline::debugInput(const Keyboard& kb)
{
  staticMeshPipeline.debugInput(kb);
  terrainPipeline   .debugInput(kb);
  boxPipeline       .debugInput(kb);
}

void
ScenePipeline::render(vk::CommandBuffer cmd_buf, RenderContext& ctx)
{
  if(enableStaticMesh) staticMeshPipeline.render(cmd_buf, ctx);
  if(enableTerrain)    terrainPipeline   .render(cmd_buf, ctx);
  if(enableBox) {
    if(showSunPov && (boxSelfRender || (ctx.worldId != ctx.camWorldId + 1))) {
      ctx.curCamIpv = ctx.camViewChunkInv;
      boxPipeline.render(cmd_buf, ctx, sunWireframe);
    } 
    if(showUserPov && (boxSelfRender || (ctx.worldId != 0))) {
      ctx.curCamIpv = ctx.worldViewChunkInv;
      boxPipeline.render(cmd_buf, ctx, userWireframe);
    }
  }
}


void 
ScenePipeline::loadScene(SceneManager& scene_mgr)
{
  staticMeshPipeline.reserve(scene_mgr.getRenderElements().size());
  terrainPipeline.loadTextures(scene_mgr);
}

} /* namespace pipes */
