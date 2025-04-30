#include "WorldRenderer.hpp"

#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>
#include <etna/Profiling.hpp>
#include <glm/ext.hpp>
#include "backends/imgui_impl_vulkan.h"
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
  resolveGPipeline2           .allocate(resolution);
  // tonemapPipeline2            .allocate();
  aaPipeline2                 .allocate();
  fogPipeline2                .allocate();
  particlesPipeline2          .allocate();

  defaultSampler = etna::Sampler({
      .filter = vk::Filter::eLinear,
      .name = "perlinSample",
  });

  
  gbuffer2.allocate(resolution);
  shadowGBuffer2.allocate({512, 512}, N_MAX_SHADOW_LAYERS);

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
  sceneMgr->finalizeTextures();
}

void WorldRenderer::loadShaders()
{
  scenePipeline2.loadShaders();

  terrainTransparentPipeline2.loadShaders();
  particlesPipeline2         .loadShaders();
  
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
  particlesPipeline2         .setup();
  skyboxPipeline2            .setup();
  resolveGPipeline2          .setup();
  fogPipeline2               .setup();
  // tonemapPipeline2           .setup();
  aaPipeline2                .setup();
  for(size_t i = 0; i < N_MAX_SHADOW_LAYERS; i++) {
    tex[i] = ImGui_ImplVulkan_AddTexture(defaultSampler.get(), shadowGBuffer2.getImage(targets::GBuffer::ImageId::Albedo).getView({.baseLayer=uint32_t(i), .layerCount=1}), VkImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal));
  }

}

void WorldRenderer::debugInput(const Keyboard& kb) 
{

  scenePipeline2             .debugInput(kb);
  terrainTransparentPipeline2.debugInput(kb);
  particlesPipeline2         .debugInput(kb);
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
 
  if (kb[KeyboardKey::kF5] == ButtonState::Falling)
  {
    shadowUpdateEnabled = false;
  }
}

void WorldRenderer::update(const FramePacket& packet) {
  ZoneScoped;
  {
    // const Camera& camera = shadow.camera;
    Camera camera = packet.mainCam;
    camera.zFar = zFar;
    const float aspect = float(resolution.x) / float(resolution.y);
    renderContext.worldViewProj = camera.projTm(aspect) * camera.viewTm();
    renderContext.worldIView = camera.viewItm();
    renderContext.worldView  = camera.viewTm();
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
  matrices->mProjView[0]    = renderContext.worldViewProj;
  matrices->mView[0]        = renderContext.worldView;
  matrices->mIView[0]       = renderContext.worldIView;
  matrices->mProj[0]        = renderContext.worldProj;
  updateShadow();
}

static float zUniform(uint32_t i, uint32_t n, float near, float far) {
  return near + (far-near) * i / n;
}

static float zLogarithmic(uint32_t i, uint32_t n, float near, float far) {
  return near * glm::pow(far/near, float(i) / n);
}

static float zCorner(unsigned z, uint32_t i, uint32_t n, float near, float far, float z_divide)
{
  return glm::mix(zLogarithmic(i+z, n, near, far), zUniform(i+z, n, near, far), z_divide);
  // return ((float)(i+z))/n;
}

void 
WorldRenderer::updateShadow() {
  ZoneScoped;
  if (!shadowUpdateEnabled)
    return;
  assert(shadowCascades <= N_MAX_SHADOW_LAYERS);
  const auto& sun = sceneMgr->getLights()[LightSource::Id::Sun];
  const glm::vec3 lightDir = glm::normalize(glm::vec3(sun.position));

  for (uint32_t i = 0; i < shadowCascades; i++)
  {
    glm::mat4x4 mProj = glm::perspectiveLH_ZO(
      -glm::radians(60.f),
      float(resolution.x) / resolution.y,
      zCorner(0, i, shadowCascades, 0.01f, zFar, zDivide),
      zCorner(1, i, shadowCascades, 0.01f, zFar, zDivide)
    );
    const auto inv = renderContext.worldIView * glm::inverse(mProj);
    
    if(i == renderContext.camWorldId) {
      renderContext.worldViewChunkInv = inv;
    }

    std::array<glm::vec4, 8> frustumCorners = {};
    {
      for (unsigned int x = 0; x < 2; ++x)
      {
        for (unsigned int y = 0; y < 2; ++y)
        {
          for (unsigned int z = 0; z < 2; ++z)
          {
            const glm::vec4 pt =
              inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
            frustumCorners[(x << 2) | (y << 1) | z] = (pt / pt.w);
          }
        }
      }
    }


    glm::vec3 center = glm::vec3(0, 0, 0);
    for (unsigned j = 0; j < 8; ++j)
    {
      center += frustumCorners[j];
    }
    center /= 8.f;
    // spdlog::info("Center: {{{}, {}, {}}}", center.x, center.y, center.z);
    // spdlog::info("LightDir: {{{}, {}, {}}}", lightDir.x, lightDir.y, lightDir.z);
    Camera shadowCamera;
    shadowCamera.lookAt(center, center - lightDir, glm::vec3{0, 1, 0});

    glm::mat4x4 lightView = shadowCamera.viewTm();


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
      
      float lightDist = - minZ + 0.01f;
      shadowCamera.move(lightDir * lightDist);
      lightView = shadowCamera.viewTm();
      lightProj = glm::orthoLH_ZO(minX, maxX, minY, maxY, 0.01f, maxZ+lightDist);

      if(i == renderContext.camWorldId) {
        renderContext.camViewChunkInv = glm::inverse(lightProj * lightView);
        curCamPos = center + lightDir * lightDist;
        curCamTo  = center ;
      }
    } 

    renderContext.getMatrices()->mProjView[1 + i] = lightProj * lightView;
    renderContext.getMatrices()->mProj    [1 + i] = lightProj;
    renderContext.getMatrices()->mView    [1 + i] = lightView;
    renderContext.getMatrices()->mIView   [1 + i] = shadowCamera.viewItm();
  }
}

void WorldRenderer::renderWorld(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{

  updateRenderCtxt();
  scenePipeline2.prepare(cmd_buf, renderContext);
  particlesPipeline2.prepare(cmd_buf, renderContext);

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

  resolveGPipeline2.prepare(cmd_buf, gbuffer2, shadowGBuffer2, shadowCascades, renderContext);

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
    resolveGPipeline2.render(cmd_buf, gbuffer2, shadowGBuffer2, shadowCascades, renderContext);
    terrainTransparentPipeline2.render(cmd_buf, gbuffer2, renderContext);
    if(enableParticles) particlesPipeline2.render(cmd_buf, renderContext);
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
  ImGui::Begin("Geometry settings");
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
  }
  ImGui::End();

  ImGui::Begin("Rendering settings");
  {
    if(ImGui::TreeNode("Lightning settings"))
    {
      ImGui::SeparatorText("Shadow camera");
      {
        ImGui::Checkbox("Enable shadows", &enableShadow);
        ImGui::BeginDisabled(!enableShadow);
        ImGui::Checkbox("Sync shadow cam", &shadowUpdateEnabled);
        ImGui::SliderInt("Cascades", (int*)&shadowCascades, 1, N_MAX_SHADOW_LAYERS);
        ImGui::SliderFloat("ZDiv", &zDivide, 0, 1, "%.5f");
        ImGui::EndDisabled();
      }
      ImGui::SeparatorText("Resolve G buffer");
      resolveGPipeline2.drawGui();
      ImGui::SeparatorText("Fog");
      fogPipeline2.drawGui();
      ImGui::TreePop();
    }

    if(ImGui::TreeNode("Particles pipeline")) 
    {
      ImGui::Checkbox("Enable",&enableParticles);
      particlesPipeline2.drawGui();
    }
  

    if(ImGui::TreeNode("aa")) {
      aaPipeline2.drawGui();
      ImGui::TreePop();
    }
    ImGui::InputFloat("zFar", &zFar);
  }
  ImGui::End();


  ImGui::Begin("Shadow cam view");
  ImGui::SliderInt("Shadow camera", (int*)&renderContext.camWorldId, 0, shadowCascades-1);
  ImGui::SliderFloat("ImageScale", &imageSize, 0, 1);
  
  ImGui::Image(ImTextureID(tex[renderContext.camWorldId]), ImVec2{imageSize * static_cast<float>(gbuffer2.getResolution().x), imageSize * static_cast<float>(gbuffer2.getResolution().y)});
  ImGui::End();
  sceneMgr->particles().drawGui();
}

void
WorldRenderer::renderShadow(vk::CommandBuffer cmd_buf) {
  ETNA_PROFILE_GPU(cmd_buf, renderShadow)
  {
    for(uint32_t i = 0; i < shadowCascades; i++) {
      renderContext.worldId = i+1;
      shadowGBuffer2.setActiveLayer(i);
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

}