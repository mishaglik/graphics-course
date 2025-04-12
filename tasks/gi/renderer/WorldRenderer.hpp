#pragma once

#include <etna/Image.hpp>
#include <etna/Sampler.hpp>
#include <etna/Buffer.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <etna/RenderTargetStates.hpp>
#include <glm/glm.hpp>

#include "pipelines/Pipelines.hpp"
#include "pipelines/terrain/TerrainTransparent.hpp"
#include "targets/Backbuffer.hpp"
#include "targets/GBuffer.hpp"
#include "scene/SceneManager.hpp"
#include "wsi/Keyboard.hpp"

#include "FramePacket.hpp"


class WorldRenderer
{
public:
  WorldRenderer();

  void loadScene(std::filesystem::path path);

  void loadShaders();
  void allocateResources(glm::uvec2 swapchain_resolution);
  void allocateGBuffer();
  
  void loadSkybox();

  void setupPipelines(vk::Format swapchain_format);

  void debugInput(const Keyboard& kb);
  void update(const FramePacket& packet);
  
  void drawGui();

  void renderWorld(
    vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view);

  void renderShadow(vk::CommandBuffer cmd_buf);

  void renderPostprocess(
    vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view);
private:
  void renderScene(
    vk::CommandBuffer cmd_buf, const glm::mat4x4& glob_tm, vk::PipelineLayout pipeline_layout);

  void renderSkybox(vk::CommandBuffer cmd_buf);
  void renderLights(vk::CommandBuffer cmd_buf);
  
  void renderSphereDeferred(vk::CommandBuffer cmd_buf);
  void renderSphere(vk::CommandBuffer cmd_buf);

  void prepareFrame(const glm::mat4x4& glob_tm);

  void regenTerrain();
  void updateShadow();
  void updateRenderCtxt();
public:

private:
  std::unique_ptr<SceneManager> sceneMgr;

  etna::Buffer constants;

  
  
  pipes::ScenePipeline    scenePipeline2{};
  pipes::TerrainTransparentPipeline terrainTransparentPipeline2{};
  
  pipes::SkyboxPipeline         skyboxPipeline2{};
  pipes::FogPipeline            fogPipeline2 {};
  pipes::ResolveGBufferPipeline resolveGPipeline2{};

  pipes::TonemapPipeline tonemapPipeline2{};
  pipes::AAPipeline     aaPipeline2{};
  
  pipes::RenderContext renderContext{};

  targets::Backbuffer backbuffer2{};
  targets::FogBuffer fogbuffer2{};
  targets::GBuffer gbuffer2{};

  targets::GBuffer shadowGBuffer2{};


  etna::Sampler defaultSampler;
  glm::uvec2 resolution;

  etna::Image skybox;
  
  
  struct ShadowMapCam
  {
    Camera camera;
    float radius = 10;
    float lightTargetDist = 24;
    bool usePerspectiveM = false;
  } shadow;


  struct RenderGroup {
    RenderElement re;
    std::size_t amount;
  };

  bool pause = false;

  bool enableShadow = true;
  uint32_t shadowCascades = 1;
  float zMult = 10.0f;

  bool shadowUpdateEnabled = true;
  
};
