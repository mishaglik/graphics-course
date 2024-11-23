#include "WorldRenderer.hpp"

#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>
#include <etna/Profiling.hpp>
#include <glm/ext.hpp>

const std::size_t N_MAX_INSTANCES = 1 << 14;

WorldRenderer::WorldRenderer()
  : sceneMgr{std::make_unique<SceneManager>()}
{
}

void WorldRenderer::allocateResources(glm::uvec2 swapchain_resolution)
{
  resolution = swapchain_resolution;

  auto& ctx = etna::get_context();

  mainViewDepth = ctx.createImage(etna::Image::CreateInfo{
    .extent = vk::Extent3D{resolution.x, resolution.y, 1},
    .name = "main_view_dept h",
    .format = vk::Format::eD32Sfloat,
    .imageUsage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
  });

  instanceMatricesBuf = ctx.createBuffer({
    .size = N_MAX_INSTANCES * sizeof(glm::mat4x4),
    .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .name = "instanceMatrices",
  });
  instanceMatricesBuf.map();

  heightmap.initImage({4096, 4096});
  for (std::size_t i = 0; i < 12; ++i)
  heightmap.upscale(*ctx.createOneShotCmdMgr());
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
  nInstances.assign(sceneMgr->getRenderElements().size(), 0);
}

void WorldRenderer::loadShaders()
{
  etna::create_program(
    "static_mesh_material",
    {TONEMAPPING_RENDERER_SHADERS_ROOT "static_mesh.frag.spv",
     TONEMAPPING_RENDERER_SHADERS_ROOT "static_mesh.vert.spv"}
  );
  
  etna::create_program(
    "perlin_shader",
    {TONEMAPPING_RENDERER_SHADERS_ROOT "quad.vert.spv",
     TONEMAPPING_RENDERER_SHADERS_ROOT "perlin.frag.spv"}
  );

  etna::create_program(
    "terrain_shader",
    {TONEMAPPING_RENDERER_SHADERS_ROOT "terrain.vert.spv",
     TONEMAPPING_RENDERER_SHADERS_ROOT "terrain.tesc.spv",
     TONEMAPPING_RENDERER_SHADERS_ROOT "terrain.tese.spv",
     TONEMAPPING_RENDERER_SHADERS_ROOT "terrain.frag.spv"
     }
  )
  ;
  etna::create_program("static_mesh", {TONEMAPPING_RENDERER_SHADERS_ROOT "static_mesh.vert.spv"});
  spdlog::info("Shaders loaded");
}

void WorldRenderer::setupPipelines(vk::Format swapchain_format)
{
  etna::VertexShaderInputDescription sceneVertexInputDesc{
    .bindings = {etna::VertexShaderInputDescription::Binding{
      .byteStreamDescription = sceneMgr->getVertexFormatDescription(),
    }},
  };

  auto& pipelineManager = etna::get_context().getPipelineManager();

  staticMeshPipeline = {};
  staticMeshPipeline = pipelineManager.createGraphicsPipeline(
    "static_mesh_material",
    etna::GraphicsPipeline::CreateInfo{
      .vertexShaderInput = sceneVertexInputDesc,
      .rasterizationConfig =
        vk::PipelineRasterizationStateCreateInfo{
          .polygonMode = vk::PolygonMode::eFill,
          .cullMode = vk::CullModeFlagBits::eBack,
          .frontFace = vk::FrontFace::eCounterClockwise,
          .lineWidth = 1.f,
        },
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {swapchain_format},
          .depthAttachmentFormat = vk::Format::eD32Sfloat,
        },
    });

   terrainPipeline = pipelineManager.createGraphicsPipeline(
    "terrain_shader",
    etna::GraphicsPipeline::CreateInfo{
      .inputAssemblyConfig = {.topology = vk::PrimitiveTopology::ePatchList},
      .tessellationConfig = {
        .patchControlPoints = 4,
      },
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = {swapchain_format},
        },
    });
}

void WorldRenderer::debugInput(const Keyboard&) {}

void WorldRenderer::update(const FramePacket& packet)
{
  ZoneScoped;

  // calc camera matrix
  {
    const float aspect = float(resolution.x) / float(resolution.y);
    worldViewProj = packet.mainCam.projTm(aspect) * packet.mainCam.viewTm();
  }
}

void WorldRenderer::renderScene(
  vk::CommandBuffer cmd_buf, const glm::mat4x4& glob_tm, vk::PipelineLayout pipeline_layout)
{
  if (!sceneMgr->getVertexBuffer())
    return;

  prepareFrame(glob_tm);

  auto staticMesh = etna::get_shader_program("static_mesh");

  cmd_buf.bindVertexBuffers(0, {sceneMgr->getVertexBuffer()}, {0});
  cmd_buf.bindIndexBuffer(sceneMgr->getIndexBuffer(), 0, vk::IndexType::eUint32);

  auto set = etna::create_descriptor_set(
    staticMesh.getDescriptorLayoutId(0),
    cmd_buf,
    {etna::Binding{0, instanceMatricesBuf.genBinding()}});

  cmd_buf.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    staticMeshPipeline.getVkPipelineLayout(),
    0,
    {set.getVkSet()},
    {});


  pushConst2M.projView = glob_tm;

  auto relems = sceneMgr->getRenderElements();
  std::size_t firstInstance = 0;
  for (std::size_t j = 0; j < relems.size(); ++j)
  {
    if (nInstances[j] != 0)
    {
      const auto& relem = relems[j];
      cmd_buf.pushConstants<PushConstants>(
        pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, {pushConst2M});
      cmd_buf.drawIndexed(
        relem.indexCount, nInstances[j], relem.indexOffset, relem.vertexOffset, firstInstance);
      firstInstance += nInstances[j];
    }
  }
}

void WorldRenderer::renderTerrain(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{
  ETNA_PROFILE_GPU(cmd_buf, renderTerrain);
  auto& hmap = heightmap.getImage(); 

  etna::RenderTargetState renderTargets(
    cmd_buf,
    {{0, 0}, {resolution.x, resolution.y}},
    {{.image = target_image, .view = target_image_view}},
    {}
  );

  auto terrainShader = etna::get_shader_program("terrain_shader");

  cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, terrainPipeline.getVkPipeline());


  auto set = etna::create_descriptor_set(
    terrainShader.getDescriptorLayoutId(0),
    cmd_buf,
    {etna::Binding{0, hmap.genBinding(heightmap.getSampler().get(), vk::ImageLayout::eShaderReadOnlyOptimal)}}
  );

  cmd_buf.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    terrainPipeline.getVkPipelineLayout(),
    0,
    {set.getVkSet()},
    {}
  );

  const size_t nChunks = 64;
  const float step = 2.f / nChunks;
  for (size_t x = 0; x < nChunks; ++x) {
    for (size_t y = 0; y < nChunks; ++y) {
        struct {glm::vec2 base, extent; glm::mat4x4 mat; int degree;} pc {{-1.f + x * step, -1.f + y * step}, {step, step}, pushConst2M.projView, 1024};

        cmd_buf.pushConstants(
              terrainPipeline.getVkPipelineLayout(), 
              vk::ShaderStageFlagBits::eVertex |
              vk::ShaderStageFlagBits::eTessellationEvaluation |
              vk::ShaderStageFlagBits::eTessellationControl,
              0, 
              sizeof(pc), &pc
        );

        cmd_buf.draw(4, 1, 0, 0);
    }
  }

}

void WorldRenderer::renderWorld(
  vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view)
{
  ETNA_PROFILE_GPU(cmd_buf, renderWorld);
  renderTerrain(cmd_buf, target_image, target_image_view);
  // draw final scene to screen
  {
    ETNA_PROFILE_GPU(cmd_buf, renderForward);

    etna::RenderTargetState renderTargets(
      cmd_buf,
      {{0, 0}, {resolution.x, resolution.y}},
      {{.image = target_image, .view = target_image_view, .loadOp = vk::AttachmentLoadOp::eLoad}},
      {.image = mainViewDepth.get(), .view = mainViewDepth.getView({})}
    );
    

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, staticMeshPipeline.getVkPipeline());
    renderScene(cmd_buf, worldViewProj, staticMeshPipeline.getVkPipelineLayout());
  }
}

static bool IsVisble(const glm::mat2x3 bounds, const glm::mat4x4& transform)
{
  glm::vec4 origin = glm::vec4(bounds[0], 1.f);
  glm::mat4x4 corners[2] = {
    {
      { bounds[1].x,  bounds[1].y,  bounds[1].z, 0.},
      { bounds[1].x,  bounds[1].y, -bounds[1].z, 0.},
      { bounds[1].x, -bounds[1].y,  bounds[1].z, 0.},
      { bounds[1].x, -bounds[1].y, -bounds[1].z, 0.},
    },
    {
      {-bounds[1].x,  bounds[1].y,  bounds[1].z, 0.},
      {-bounds[1].x,  bounds[1].y, -bounds[1].z, 0.},
      {-bounds[1].x, -bounds[1].y,  bounds[1].z, 0.},
      {-bounds[1].x, -bounds[1].y, -bounds[1].z, 0.},
    }};
  corners[0] += origin;
  corners[0] = transform * corners[0];

  for (size_t i = 0; i < 4; ++i)
    corners[0][i] /= corners[0][i][3];

  corners[1] += origin;
  corners[1] = transform * corners[1];

  for (size_t i = 0; i < 4; ++i)
    corners[1][i] /= corners[1][i][3];

  glm::vec3 min = corners[0][0];
  glm::vec3 max = corners[0][0];
  for (size_t i = 0; i < 2; ++i)
    for (size_t j = 0; j < 4; ++j)
    {
      min = glm::min(min, glm::vec3(corners[i][j]));
      max = glm::max(max, glm::vec3(corners[i][j]));
    }

  return min.x <  1. && 
         max.x > -1. && 
         min.y <  1. && 
         max.y > -1. && 
         min.y <  1. && 
         max.y > -1. ;
}

void WorldRenderer::prepareFrame(const glm::mat4x4& glob_tm)
{
  auto instanceMeshes = sceneMgr->getInstanceMeshes();
  auto instanceMatrices = sceneMgr->getInstanceMatrices();
  auto meshes = sceneMgr->getMeshes();
  auto bbs = sceneMgr->getBounds();

  auto* matrices = reinterpret_cast<glm::mat4x4*>(instanceMatricesBuf.data());

  // May be do more fast clean
  std::memset(nInstances.data(), 0, nInstances.size() * sizeof(nInstances[0]));

  for (std::size_t i = 0; i < instanceMatrices.size(); ++i)
  {
    const auto meshIdx = instanceMeshes[i];
    const auto& instanceMatrix = instanceMatrices[i];
    for (std::size_t j = 0; j < meshes[meshIdx].relemCount; j++)
    {
      const auto relemIdx = meshes[meshIdx].firstRelem + j;
      if (!IsVisble(bbs[relemIdx], glob_tm * instanceMatrix))
      {
        continue;
      }
      nInstances[relemIdx]++;
      *matrices++ = instanceMatrix;
    }
  }
}
