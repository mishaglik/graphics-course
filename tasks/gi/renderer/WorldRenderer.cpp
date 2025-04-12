#include "WorldRenderer.hpp"

#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>
#include <etna/Profiling.hpp>
#include <glm/ext.hpp>
#include "imgui.h"
#include "stb_image.h"


WorldRenderer::WorldRenderer()
  : sceneMgr{std::make_unique<SceneManager>()}
{
  renderContext.sceneMgr = &*sceneMgr;
}

void WorldRenderer::allocateResources(glm::uvec2 swapchain_resolution)
{
  resolution = swapchain_resolution;

  auto& ctx = etna::get_context();

  renderContext.worldViewMatrices = ctx.createBuffer({
    .size = sizeof(WorldViewProjMatrices),
    .bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .name = "worldProjMatrices",
  });
  renderContext.worldViewMatrices.map();


  backbuffer2.allocate(resolution);

  scenePipeline2              .allocate();
  terrainTransparentPipeline2 .allocate();
  skyboxPipeline2             .allocate();
  resolveGPipeline2           .allocate();
  // tonemapPipeline2            .allocate();
  aaPipeline2                 .allocate();
  fogPipeline2                .allocate();

  defaultSampler = etna::Sampler({
      .filter = vk::Filter::eLinear,
      .name = "perlinSample",
  });

  
  gbuffer2.allocate(resolution);
  shadowGBuffer2.allocate({128, 128}, N_MAX_SHADOW_LAYERS);
  fogbuffer2.allocate(resolution / 2u);
}


void WorldRenderer::loadScene(std::filesystem::path path)
{
  if (path.stem().string().ends_with("_baked"))
  {
    sceneMgr->selectSceneBaked(path);
  }
  else
  {
    sceneMgr->selectScene(path);
  }
  scenePipeline2.loadScene(*sceneMgr);
  
}

void WorldRenderer::loadShaders()
{
  scenePipeline2.loadShaders();

  terrainTransparentPipeline2.loadShaders();
  
  // tonemapPipeline2.loadShaders();
  aaPipeline2.loadShaders();
  
  skyboxPipeline2.loadShaders();
  
  fogPipeline2.loadShaders();
  resolveGPipeline2.loadShaders();
  // etna::create_program("static_mesh", {IMGUI_RENDERER_SHADERS_ROOT "static_mesh.vert.spv"});
  spdlog::info("Shaders loaded");
}

void WorldRenderer::setupPipelines(vk::Format /*swapchain_format*/)
{
  scenePipeline2             .setup();
  terrainTransparentPipeline2.setup();
  skyboxPipeline2            .setup();
  resolveGPipeline2          .setup();
  fogPipeline2               .setup();
  // tonemapPipeline2           .setup();
  aaPipeline2                .setup();
}

void WorldRenderer::debugInput(const Keyboard& kb) 
{

  scenePipeline2             .debugInput(kb);
  terrainTransparentPipeline2.debugInput(kb);
  skyboxPipeline2            .debugInput(kb);
  fogPipeline2               .debugInput(kb);
  resolveGPipeline2          .debugInput(kb);
  // tonemapPipeline2           .debugInput(kb);
  aaPipeline2                .debugInput(kb);

  if (kb[KeyboardKey::kPause] == ButtonState::Falling)
  {
    pause = !pause;
    spdlog::info("Pause is {}", pause ? "on" : "off");
  }
 
}

void WorldRenderer::update(const FramePacket& packet) {
  ZoneScoped;
  {
    // const Camera& camera = shadow.camera;
    const Camera& camera = packet.mainCam;
    const float aspect = float(resolution.x) / float(resolution.y);
    renderContext.worldViewProj = camera.projTm(aspect) * camera.viewTm();
    renderContext.worldView  = camera.viewTm();
    renderContext.worldIView = camera.viewItm();
    renderContext.worldProj  = camera.projTm(aspect);

    renderContext.camPos = camera.position;
  }

  renderContext.dt = static_cast<float>(packet.currentTime - renderContext.frameTime);
  if(!pause) {
    renderContext.frameTime = packet.currentTime;
  }
 

}

void WorldRenderer::updateRenderCtxt()
{
  auto* matrices = renderContext.getMatrices();
  matrices->mProjView[0] = renderContext.worldViewProj;
  matrices->mView        = renderContext.worldView;
  matrices->mIView       = renderContext.worldIView;
  matrices->mProj        = renderContext.worldProj;
  updateShadow();
}

void 
WorldRenderer::updateShadow() {
  ZoneScoped;
  if(!shadowUpdateEnabled)
    return;
  
  std::array<glm::vec4, 8> frustumCorners;
  {
    const auto inv = glm::inverse(renderContext.worldViewProj);
    for (unsigned int x = 0; x < 2; ++x) {
      for (unsigned int y = 0; y < 2; ++y) {
        for (unsigned int z = 0; z < 2; ++z) {
          const glm::vec4 pt =
            inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
          frustumCorners[(x << 2) | (y << 1) | z] = (pt / pt.w);
        }
      }
    }
  }

  glm::vec3 center = glm::vec3(0, 0, 0);
  for(unsigned i = 0; i < 8; ++i) {
    center += glm::vec3(frustumCorners[i]);
  }
  center /= 8;

  const auto& sun = sceneMgr->getLights()[LightSource::Id::Sun];
  const glm::vec3 lightDir = glm::normalize(glm::vec3(sun.position));

  const glm::mat4x4 lightView = glm::lookAt(center + lightDir, center, glm::vec3{0, 1, 0});
  glm::mat4x4 lightProj;
  {
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : frustumCorners)
    {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }
    if (minZ < 0)
    {
      minZ *= zMult;
    }
    else
    {
      minZ /= zMult;
    }
    if (maxZ < 0)
    {
      maxZ /= zMult;
    }
    else
    {
      maxZ *= zMult;
    }
      
    lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

  }

  assert(shadowCascades <= N_MAX_SHADOW_LAYERS);
  for(uint32_t i = 0; i < shadowCascades; i++) {
    renderContext.getMatrices()->mProjView[1 + i] = lightProj * lightView;
  }
}

void WorldRenderer::renderWorld(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{

  updateRenderCtxt();

  scenePipeline2.prepare(cmd_buf, renderContext);

  ETNA_PROFILE_GPU(cmd_buf, renderToGBuffer);
  {
    renderContext.worldId = 0;
    gbuffer2.toRenderTarget(cmd_buf);
    etna::flush_barriers(cmd_buf);
    etna::RenderTargetState renderTargets({
      cmd_buf,
      {{0, 0}, {resolution.x, resolution.y}},
      gbuffer2.getColorAttachments(),
      gbuffer2.getDepthAttachment(),
      {}
    });

    scenePipeline2.render(cmd_buf, renderContext);
  }

  if(enableShadow) {

    shadowGBuffer2.toRenderTarget(cmd_buf);
    etna::flush_barriers(cmd_buf);
    renderShadow(cmd_buf);

  } else {
    
    etna::set_state(cmd_buf, 
      shadowGBuffer2.getDepthImage().get(), 
      vk::PipelineStageFlagBits2::eTransfer, 
      vk::AccessFlagBits2::eTransferWrite,
      vk::ImageLayout::eTransferDstOptimal, 
      vk::ImageAspectFlagBits::eDepth
    );
    etna::flush_barriers(cmd_buf);
    vk::ClearDepthStencilValue clear{1.f, 0};
    std::array<vk::ImageSubresourceRange, 1> range{
      vk::ImageSubresourceRange{
        .aspectMask = vk::ImageAspectFlagBits::eDepth,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers,
      }
    };
    cmd_buf.clearDepthStencilImage(shadowGBuffer2.getDepthImage().get(), vk::ImageLayout::eTransferDstOptimal, clear, range);
  }



  terrainTransparentPipeline2.prepare(cmd_buf, renderContext);

  {
    
    gbuffer2.toSampler(cmd_buf);
    shadowGBuffer2.toSampler(cmd_buf);
    etna::flush_barriers(cmd_buf);
  }
  {
    ETNA_PROFILE_GPU(cmd_buf, renderFog);
    etna::set_state(cmd_buf, 
      fogbuffer2.get(), 
      vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
      {}, 
      vk::ImageLayout::eColorAttachmentOptimal, 
      vk::ImageAspectFlagBits::eColor
    );
    etna::flush_barriers(cmd_buf);
    etna::RenderTargetState renderTargets({
      cmd_buf,
      {{0, 0}, {resolution.x / 2, resolution.y / 2}},
      fogbuffer2.getColorAttachments(),
      fogbuffer2.getDepthAttachment(),
      {}
    });
    fogPipeline2.render(cmd_buf, gbuffer2, renderContext);
  }
  { 
    ETNA_PROFILE_GPU(cmd_buf, renderToBackbuffer);

    etna::set_state(cmd_buf, 
      backbuffer2.get(), 
      vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
      {}, 
      vk::ImageLayout::eColorAttachmentOptimal, 
      vk::ImageAspectFlagBits::eColor
    );
  
    etna::set_state(cmd_buf, 
      fogbuffer2.get(), 
      vk::PipelineStageFlagBits2::eFragmentShader, 
      vk::AccessFlagBits2::eShaderSampledRead, 
      vk::ImageLayout::eShaderReadOnlyOptimal, 
      vk::ImageAspectFlagBits::eColor,
      ForceSetState::eTrue
    );
    etna::flush_barriers(cmd_buf);
    etna::RenderTargetState renderTargets({
      cmd_buf,
      {{0, 0}, {resolution.x, resolution.y}},
      backbuffer2.getColorAttachments(),
      backbuffer2.getDepthAttachment(),
      {}
    });
    
    skyboxPipeline2.render(cmd_buf, backbuffer2, renderContext);
    resolveGPipeline2.render(cmd_buf, gbuffer2, renderContext);
    terrainTransparentPipeline2.render(cmd_buf, gbuffer2, renderContext);
    fogPipeline2.renderResolve(cmd_buf, fogbuffer2, renderContext);
  }


  renderPostprocess(cmd_buf, target_image, target_image_view);
}

void WorldRenderer::renderPostprocess(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{


  ETNA_PROFILE_GPU(cmd_buf, renderWorld);
  if(!aaPipeline2.enabled())
  {
    etna::set_state(cmd_buf, 
      target_image, 
      vk::PipelineStageFlagBits2::eTransfer, 
      vk::AccessFlagBits2::eTransferWrite, 
      vk::ImageLayout::eTransferDstOptimal, 
      vk::ImageAspectFlagBits::eColor
    );
    etna::set_state(cmd_buf, 
      backbuffer2.get(), 
      vk::PipelineStageFlagBits2::eTransfer, 
      vk::AccessFlagBits2::eTransferRead, 
      vk::ImageLayout::eTransferSrcOptimal, 
      vk::ImageAspectFlagBits::eColor
    );
    etna::flush_barriers(cmd_buf);

    std::array<vk::Offset3D, 2> offs = {
        vk::Offset3D{},
        vk::Offset3D{.x =static_cast<int32_t>(resolution.x), .y=static_cast<int32_t>(resolution.y), .z = 1}
    };
    std::array<vk::ImageBlit, 1> blit = {
      vk::ImageBlit{
          .srcSubresource = {.aspectMask=vk::ImageAspectFlagBits::eColor, .layerCount=1,},
          .srcOffsets = offs,
          .dstSubresource = {.aspectMask=vk::ImageAspectFlagBits::eColor, .layerCount=1},
          .dstOffsets = offs,
        }
    };
    cmd_buf.blitImage(backbuffer2.get(), vk::ImageLayout::eTransferSrcOptimal,
      target_image, vk::ImageLayout::eTransferDstOptimal,
      blit,
      vk::Filter::eLinear
    );
    etna::set_state(cmd_buf, 
      target_image, 
      vk::PipelineStageFlagBits2::eAllCommands, 
      vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite, 
      vk::ImageLayout::eColorAttachmentOptimal, 
      vk::ImageAspectFlagBits::eColor,
      ForceSetState::eTrue
    );
    etna::flush_barriers(cmd_buf);
    return;
  }

#if 0 //NOTE - Tonemap is disabled
  etna::set_state(cmd_buf, 
    backbuffer2.get(), 
    vk::PipelineStageFlagBits2::eComputeShader, 
    vk::AccessFlagBits2::eShaderRead, 
    vk::ImageLayout::eGeneral, 
    vk::ImageAspectFlagBits::eColor
  );
  etna::flush_barriers(cmd_buf);

  tonemapPipeline2.tonemapEvaluate(cmd_buf, backbuffer2, renderContext);
#endif 

  etna::set_state(cmd_buf, 
    backbuffer2.get(), 
    vk::PipelineStageFlagBits2::eFragmentShader, 
    vk::AccessFlagBits2::eShaderSampledRead, 
    vk::ImageLayout::eShaderReadOnlyOptimal, 
    vk::ImageAspectFlagBits::eColor
  );
  etna::flush_barriers(cmd_buf);

  etna::RenderTargetState renderTargets(
    cmd_buf,
    {{0, 0}, {resolution.x, resolution.y}},
    {{.image = target_image, .view = target_image_view}},
    {}
  );

  // tonemapPipeline2.render(cmd_buf, backbuffer2, renderContext);
  aaPipeline2.render(cmd_buf, backbuffer2, renderContext);
}




void 
WorldRenderer::drawGui()
{
  ImGui::Begin("Render settings");
  {
    
    if(ImGui::TreeNode("Scene settings"))
    {
      scenePipeline2.drawGui();
      ImGui::SeparatorText("Terrain transparent");
      terrainTransparentPipeline2.drawGui();
      ImGui::SeparatorText("Terrain generator");
      sceneMgr->terrain().drawGui();
      ImGui::TreePop();
    }

    if(ImGui::TreeNode("Lightning settings"))
    {
      ImGui::SeparatorText("Shadow camera");
      {
        ImGui::Checkbox("Enable shadows", &enableShadow);
        ImGui::BeginDisabled(!enableShadow);
        ImGui::Checkbox("Sync shadow cam", &shadowUpdateEnabled);
        ImGui::InputFloat("zMult", &zMult);
        ImGui::EndDisabled();
      }
      ImGui::SeparatorText("Resolve G buffer");
      resolveGPipeline2.drawGui();
      ImGui::SeparatorText("Fog");
      fogPipeline2.drawGui();
      ImGui::TreePop();
    }
  

    if(ImGui::TreeNode("aa")) {
      aaPipeline2.drawGui();
      ImGui::TreePop();
    }
  }
  ImGui::End();
}

void
WorldRenderer::renderShadow(vk::CommandBuffer cmd_buf) {
  ETNA_PROFILE_GPU(cmd_buf, renderShadow)
  {
    renderContext.worldId = 1;
    shadowGBuffer2.setActiveLayer(0);
    etna::RenderTargetState renderTargetState({
      cmd_buf,
      {{0, 0}, {shadowGBuffer2.getResolution().x, shadowGBuffer2.getResolution().y}},
      shadowGBuffer2.getColorAttachments(),
      shadowGBuffer2.getDepthAttachment(),
      {}     
    });
    scenePipeline2.render(cmd_buf, renderContext);
  }

}