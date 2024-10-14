#pragma once

#include <etna/Window.hpp>
#include <etna/PerFrameCmdMgr.hpp>
#include <etna/GraphicsPipeline.hpp>
#include <etna/ComputePipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>
#include <scene/Camera.hpp>

#include "wsi/OsWindowingManager.hpp"

#include "Renderer.hpp"

// May gods help us using this lib
#include <chrono>

class App
{
public:
  App();
  ~App();

  void processInput();
  void run();

  void rotateCam(const Mouse& ms);

private:
  void drawFrame();

private:
  Renderer renderer;

  OsWindowingManager windowing;
  std::unique_ptr<OsWindow> osWindow;

  glm::uvec2 resolution;
  bool useVsync;

  std::unique_ptr<etna::Window> vkWindow;
  std::unique_ptr<etna::PerFrameCmdMgr> commandManager;

  std::chrono::high_resolution_clock::time_point timeStart;

  Camera cam;
};
