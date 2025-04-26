#include "Renderer.hpp"

#include <etna/GlobalContext.hpp>
#include <etna/Etna.hpp>
#include <etna/RenderTargetStates.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/Profiling.hpp>
#include <imgui.h>

#include <gui/ImGuiRenderer.hpp>

Renderer::Renderer(glm::uvec2 res)
  : resolution{res}
{
}

void Renderer::initVulkan(std::span<const char*> instance_extensions)
{
  std::vector<const char*> instanceExtensions;

  for (auto ext : instance_extensions)
    instanceExtensions.push_back(ext);

  std::vector<const char*> deviceExtensions;

  deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  vk::PhysicalDeviceFeatures2 features{.features = {.tessellationShader=1, .multiDrawIndirect=1, .drawIndirectFirstInstance=1, .fillModeNonSolid=1,}};

  vk::PhysicalDeviceVulkan11Features features11{.shaderDrawParameters=1,};
  vk::PhysicalDeviceVulkan12Features features12{.descriptorIndexing=1, .shaderSampledImageArrayNonUniformIndexing=1, .shaderStorageBufferArrayNonUniformIndexing=1, .descriptorBindingVariableDescriptorCount=1, .runtimeDescriptorArray=1, };
  features11.setPNext(&features12);
  features.setPNext(&features11);



  etna::initialize(etna::InitParams{
    .applicationName = "gi_renderer",
    .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
    .instanceExtensions = instanceExtensions,
    .deviceExtensions = deviceExtensions,
    .features = features,
    .physicalDeviceIndexOverride = {},
    .numFramesInFlight = 1,
  });
}

void Renderer::initFrameDelivery(vk::UniqueSurfaceKHR a_surface, ResolutionProvider res_provider)
{
  resolutionProvider = std::move(res_provider);

  auto& ctx = etna::get_context();

  commandManager = ctx.createPerFrameCmdMgr();

  window = ctx.createWindow(etna::Window::CreateInfo{
    .surface = std::move(a_surface),
  });

  auto [w, h] = window->recreateSwapchain(etna::Window::DesiredProperties{
    .resolution = {resolution.x, resolution.y},
    .vsync = useVsync,
  });

  resolution = {w, h};

  worldRenderer = std::make_unique<WorldRenderer>();

  guiRenderer = std::make_unique<ImGuiRenderer>(window->getCurrentFormat());
  
  worldRenderer->loadShaders();
  worldRenderer->allocateResources(resolution);
  worldRenderer->setupPipelines(window->getCurrentFormat());

}

void Renderer::loadScene(std::filesystem::path path)
{
  worldRenderer->loadScene(path);
}

void Renderer::debugInput(const Keyboard& kb)
{
  worldRenderer->debugInput(kb);

  if (kb[KeyboardKey::kB] == ButtonState::Falling)
  {
    const int retval = std::system("cd " GRAPHICS_COURSE_ROOT "/build"
                                   " && cmake --build . --target pipeline_shaders");
    if (retval != 0)
      spdlog::warn("Shader recompilation returned a non-zero return code!");
    else
    {
      ETNA_CHECK_VK_RESULT(etna::get_context().getDevice().waitIdle());
      etna::reload_shaders();
      spdlog::info("Successfully reloaded shaders!");
    }
  }

  // if (kb[KeyboardKey::kU] == ButtonState::Falling)
  // {
  //   worldRenderer->heightmap.upscale(*etna::get_context().createOneShotCmdMgr());
  // }
}

void Renderer::update(const FramePacket& packet)
{
  pos = packet.mainCam.position;
  mat = packet.mainCam.viewItm();
  const float aspect = float(resolution.x) / float(resolution.y);
  mat2 = packet.mainCam.projTm(aspect);
  worldRenderer->update(packet);
}

void Renderer::drawFrame()
{
  ZoneScoped;

  {
    ZoneScopedN("drawGui");
    guiRenderer->nextFrame();
    ImGui::NewFrame();
    ImGui::InputFloat3("Position", &pos.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
    if(ImGui::TreeNode("View matrix")) {
      ImGui::InputFloat4("r0x", &mat[0][0], "%.3f", ImGuiInputTextFlags_ReadOnly);
      ImGui::InputFloat4("r1x", &mat[1][0], "%.3f", ImGuiInputTextFlags_ReadOnly);
      ImGui::InputFloat4("r2x", &mat[2][0], "%.3f", ImGuiInputTextFlags_ReadOnly);
      ImGui::InputFloat4("r3x", &mat[3][0], "%.3f", ImGuiInputTextFlags_ReadOnly);
      ImGui::TreePop();
    }
    if(ImGui::TreeNode("Proj matrix")) {
      ImGui::InputFloat4("r0x", &mat2[0][0], "%.3f", ImGuiInputTextFlags_ReadOnly);
      ImGui::InputFloat4("r1x", &mat2[1][0], "%.3f", ImGuiInputTextFlags_ReadOnly);
      ImGui::InputFloat4("r2x", &mat2[2][0], "%.3f", ImGuiInputTextFlags_ReadOnly);
      ImGui::InputFloat4("r3x", &mat2[3][0], "%.3f", ImGuiInputTextFlags_ReadOnly);
      ImGui::TreePop();
    }
    worldRenderer->drawGui();
    ImGui::Render();
  }

  auto currentCmdBuf = commandManager->acquireNext();

  etna::begin_frame();

  auto nextSwapchainImage = window->acquireNext();

  if (nextSwapchainImage)
  {
    auto [image, view, availableSem] = *nextSwapchainImage;

    ETNA_CHECK_VK_RESULT(currentCmdBuf.begin(vk::CommandBufferBeginInfo{}));
    {
      ETNA_PROFILE_GPU(currentCmdBuf, renderFrame);

      worldRenderer->renderWorld(currentCmdBuf, image, view);

      {
        ImDrawData* pDrawData = ImGui::GetDrawData();
        guiRenderer->render(
          currentCmdBuf, {{0, 0}, {resolution.x, resolution.y}}, image, view, pDrawData);
      }

      etna::set_state(
        currentCmdBuf,
        image,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        {},
        vk::ImageLayout::ePresentSrcKHR,
        vk::ImageAspectFlagBits::eColor);

      etna::flush_barriers(currentCmdBuf);

      ETNA_READ_BACK_GPU_PROFILING(currentCmdBuf);
    }
    ETNA_CHECK_VK_RESULT(currentCmdBuf.end());

    auto renderingDone = commandManager->submit(std::move(currentCmdBuf), std::move(availableSem));

    const bool presented = window->present(std::move(renderingDone), view);

    if (!presented)
      nextSwapchainImage = std::nullopt;
  }

  if (!nextSwapchainImage && resolutionProvider() != glm::uvec2{0, 0})
  {
    auto [w, h] = window->recreateSwapchain(etna::Window::DesiredProperties{
      .resolution = {resolution.x, resolution.y},
      .vsync = useVsync,
    });
    ETNA_VERIFY((resolution == glm::uvec2{w, h}));
  }

  etna::end_frame();
}

Renderer::~Renderer()
{
  ETNA_CHECK_VK_RESULT(etna::get_context().getDevice().waitIdle());
}
