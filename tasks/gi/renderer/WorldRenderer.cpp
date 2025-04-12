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

  // auto& ctx = etna::get_context();

  backbuffer2.allocate(resolution);

  staticMeshPipeline2         .allocate();
  terrainPipeline2            .allocate();
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

  // regenTerrain();
  
  gbuffer2.allocate(resolution);
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
  staticMeshPipeline2.reserve(sceneMgr->getRenderElements().size());
  terrainPipeline2.loadTextures(*sceneMgr);
}

void WorldRenderer::loadShaders()
{
  staticMeshPipeline2.loadShaders();

  terrainPipeline2.loadShaders();
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
  staticMeshPipeline2        .setup();
  terrainPipeline2           .setup();
  terrainTransparentPipeline2.setup();
  skyboxPipeline2            .setup();
  resolveGPipeline2          .setup();
  fogPipeline2               .setup();
  // tonemapPipeline2           .setup();
  aaPipeline2                .setup();
}

void WorldRenderer::debugInput(const Keyboard& kb) 
{

  staticMeshPipeline2        .debugInput(kb);
  terrainPipeline2           .debugInput(kb);
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

void WorldRenderer::update(const FramePacket& packet)
{
  ZoneScoped;

  if (shadowCamSync) {
    auto& sun = sceneMgr->getLights()[LightSource::Id::Sun];
    glm::vec3 target = packet.mainCam.position + packet.mainCam.forward() * (14.f - packet.mainCam.position.y) / std::min(0.1f, glm::dot(packet.mainCam.forward(), glm::vec3{0, 1, 0}));
    shadow.camera.lookAt(sun.position, target, {0, 1 ,0}); //TODO: sync with current view 
    // float length = glm::distance(shadow.camera.position, target) - glm::distance(target, packet.mainCam.position);
    // shadow.camera.move(shadow.camera.forward() * length);
    shadow.lightTargetDist = 2 * glm::distance(shadow.camera.position, target);
  } else {
    auto& sun = sceneMgr->getLights()[LightSource::Id::Sun];
    glm::vec3 target{16, 14, -64};
    shadow.camera.lookAt(sun.position, target, {0, 1 ,0}); //TODO: sync with current view 
    shadow.lightTargetDist = 1.2f * glm::distance(shadow.camera.position, target);
  }
  // calc camera matrix
  {
    // const Camera& camera = shadow.camera;
    const Camera& camera = packet.mainCam;
    const float aspect = float(resolution.x) / float(resolution.y);
    renderContext.worldViewProj = camera.projTm(aspect) * camera.viewTm();
    renderContext.worldView = camera.viewTm();
    renderContext.worldIView = camera.viewItm();
    renderContext.worldProj = camera.projTm(aspect);
    renderContext.camPos = camera.position;
  }


  // calc light matrix
  {
    const auto mProj = shadow.usePerspectiveM
      ? glm::perspectiveLH_ZO(
          -glm::radians(packet.mainCam.fov), 1.0f, 1.0f, shadow.lightTargetDist * 2.0f)
      : glm::orthoLH_ZO(
          +shadow.radius,
          -shadow.radius,
          +shadow.radius,
          -shadow.radius,
          0.0f,
          shadow.lightTargetDist);

    renderContext.lightViewProj = mProj * shadow.camera.viewTm();
  }

  renderContext.dt = static_cast<float>(packet.currentTime - renderContext.frameTime);
  if(!pause) {
    renderContext.frameTime = packet.currentTime;
  }
  
}

void WorldRenderer::renderWorld(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{
  terrainPipeline2.prepare(cmd_buf, renderContext);

  ETNA_PROFILE_GPU(cmd_buf, renderToGBuffer);
  {
    
    for(std::size_t i = 0; i < targets::GBuffer::N_COLOR_ATTACHMENTS; i++) {
      etna::set_state(cmd_buf, 
        gbuffer2.getImage(i).get(), 
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
        vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
        vk::ImageLayout::eColorAttachmentOptimal, 
        vk::ImageAspectFlagBits::eColor
      );
    }
    etna::set_state(cmd_buf, 
        gbuffer2.getDepthImage().get(), 
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
        vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
        vk::ImageLayout::eDepthAttachmentOptimal, 
        vk::ImageAspectFlagBits::eDepth
      );

    
    
    etna::RenderTargetState renderTargets({
      cmd_buf,
      {{0, 0}, {resolution.x, resolution.y}},
      gbuffer2.getColorAttachments(),
      gbuffer2.getDepthAttachment(),
      {}
    });

    terrainPipeline2.render(cmd_buf, gbuffer2, renderContext);
    if (enableStaticMesh)
      staticMeshPipeline2.render(cmd_buf, gbuffer2, renderContext);
  }

  if(enableShadow) {
    etna::set_state(cmd_buf, 
      gbuffer2.shadow().get(), 
      vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
      vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eColorAttachmentRead,
      vk::ImageLayout::eDepthAttachmentOptimal, 
      vk::ImageAspectFlagBits::eDepth
    );
    etna::flush_barriers(cmd_buf);
    etna::RenderTargetState renderTargets({
      cmd_buf,
      {{0, 0}, {gbuffer2.shadow().getResolution().x, gbuffer2.shadow().getResolution().y}},
      gbuffer2.shadow().getColorAttachments(),
      gbuffer2.shadow().getDepthAttachment(),
      {}
    });
    staticMeshPipeline2.renderShadow(cmd_buf, renderContext);
  } else {
    etna::set_state(cmd_buf, 
      gbuffer2.shadow().get(), 
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
        .layerCount = 1,
      }
    };
    cmd_buf.clearDepthStencilImage(gbuffer2.shadow().get(), vk::ImageLayout::eTransferDstOptimal, clear, range);
  }



  terrainTransparentPipeline2.prepare(cmd_buf, renderContext);

  {
    
    for(std::size_t i = 0; i < targets::GBuffer::N_COLOR_ATTACHMENTS; i++) {
      etna::set_state(cmd_buf, 
        gbuffer2.getImage(i).get(), 
        vk::PipelineStageFlagBits2::eFragmentShader, 
        vk::AccessFlagBits2::eShaderSampledRead, 
        vk::ImageLayout::eShaderReadOnlyOptimal, 
        vk::ImageAspectFlagBits::eColor
      );
    }
    etna::set_state(cmd_buf, 
      gbuffer2.getDepthImage().get(), 
      vk::PipelineStageFlagBits2::eFragmentShader, 
      vk::AccessFlagBits2::eShaderSampledRead, 
      vk::ImageLayout::eShaderReadOnlyOptimal, 
      vk::ImageAspectFlagBits::eDepth
    );
    etna::set_state(cmd_buf, 
      gbuffer2.shadow().get(), 
      vk::PipelineStageFlagBits2::eFragmentShader, 
      vk::AccessFlagBits2::eShaderSampledRead, 
      vk::ImageLayout::eShaderReadOnlyOptimal, 
      vk::ImageAspectFlagBits::eDepth
    );
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
      {}, 
      vk::ImageLayout::eShaderReadOnlyOptimal, 
      vk::ImageAspectFlagBits::eColor
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
    
    if(ImGui::TreeNode("Terrain settings"))
    {
      ImGui::SeparatorText("Generator");
      sceneMgr->terrain().drawGui();
      ImGui::SeparatorText("Solid");
      terrainPipeline2.drawGui();
      ImGui::SeparatorText("Transparent");
      terrainTransparentPipeline2.drawGui();
      ImGui::TreePop();
    }
    if(ImGui::TreeNode("StaticMesh renderer settings"))
    {
      ImGui::Checkbox("Enabled", &enableStaticMesh);
      staticMeshPipeline2.drawGui();
      ImGui::TreePop();
    }

    if(ImGui::TreeNode("Lightning settings"))
    {
      ImGui::SeparatorText("Shadow camera");
      {
        ImGui::Checkbox("Enable shadows", &enableShadow);
        ImGui::BeginDisabled(!enableShadow);
        ImGui::Checkbox("Sync shadow cam", &shadowCamSync);
        ImGui::SliderFloat("Radius", &shadow.radius, 0, 100);
        ImGui::SliderFloat("LightTargetDist", &shadow.lightTargetDist, 0, 100);
        ImGui::Checkbox("usePerspectiveM", &shadow.usePerspectiveM);
        ImGui::EndDisabled();
      }
      ImGui::SeparatorText("Resolve G buffer");
      resolveGPipeline2.drawGui();
      ImGui::SeparatorText("Fog");
      fogPipeline2.drawGui();
      ImGui::TreePop();
    }
    
    // if(ImGui::TreeNode("tonemap")) {
    //   tonemapPipeline2.drawGui();
    //   ImGui::TreePop();
    // }

    if(ImGui::TreeNode("aa")) {
      aaPipeline2.drawGui();
      ImGui::TreePop();
    }
  }
  ImGui::End();
}