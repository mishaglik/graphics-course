#include "StaticMesh.hpp"

#include <etna/BlockingTransferHelper.hpp>
#include <etna/Etna.hpp>
#include <etna/Profiling.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>

#include "scene/SceneManager.hpp"

#include <imgui.h>
#include <glm/glm.hpp>

#include "stb_image.h"

namespace pipes {

void 
StaticMeshPipeline::allocate()
{
    auto& ctx = etna::get_context();
    instanceMatricesBuf = ctx.createBuffer({
      .size = N_MAX_INSTANCES * sizeof(glm::mat4x4),
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
      .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
      .name = "instanceMatrices",
    });
    instanceMatricesBuf.map();

   

    defaultSampler = etna::Sampler({
      .filter = vk::Filter::eLinear,
      .name = "staticMeshSampler",
    });
}

void 
StaticMeshPipeline::loadShaders() 
{
    etna::create_program(
        "staticmesh_shader",
        { STATICMESH_PIPELINE_SHADERS_ROOT "staticmesh.vert.spv",
          STATICMESH_PIPELINE_SHADERS_ROOT "staticmesh.frag.spv"}
    );
    etna::create_program(
      "staticmesh_shadow",
      { STATICMESH_PIPELINE_SHADERS_ROOT "staticmesh.vert.spv" }
    );
}

void 
StaticMeshPipeline::setup() 
{
  auto& pipelineManager = etna::get_context().getPipelineManager();

  etna::VertexShaderInputDescription sceneVertexInputDesc{
    .bindings = {etna::VertexShaderInputDescription::Binding{
      .byteStreamDescription = SceneManager::getVertexFormatDescription(),
    }},
  };

  pipeline = pipelineManager.createGraphicsPipeline(
    "staticmesh_shader",
    etna::GraphicsPipeline::CreateInfo{
      .vertexShaderInput = sceneVertexInputDesc,
      .rasterizationConfig =
        vk::PipelineRasterizationStateCreateInfo{
          .polygonMode = vk::PolygonMode::eFill,
          .cullMode = vk::CullModeFlagBits::eBack,
          .frontFace = vk::FrontFace::eCounterClockwise,
          .lineWidth = 1.f,
        },
      .blendingConfig = {
        .attachments = {
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
          },
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
          },
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
          },
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::False,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
          },
        },
        .logicOpEnable = false,
        .logicOp = {},
        .blendConstants = {},
      },
      .fragmentShaderOutput =
        {
          .colorAttachmentFormats = RenderTarget::COLOR_ATTACHMENT_FORMATS,
          .depthAttachmentFormat = RenderTarget::DEPTH_ATTACHMENT_FORMAT,
        },
    });

}

void 
StaticMeshPipeline::drawGui()
{
  ImGui::Checkbox("culling", &enableCulling);
  ImGui::Checkbox("use normal maps", &normalMap);
  ImGui::InputFloat3("starting pos", &pushConst2M.pos.x);
}

void 
StaticMeshPipeline::debugInput(const Keyboard& kb)
{
  if (kb[KeyboardKey::kN] == ButtonState::Falling) {
    normalMap = !normalMap;
    spdlog::info("normal maps are {}", normalMap ? "on" : "off");
  }
}

void
StaticMeshPipeline::render(vk::CommandBuffer cmd_buf,  const RenderContext& ctx)
{
    
  cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());
  if (!ctx.sceneMgr->getVertexBuffer())
    return;

  prepareFrame(ctx);

  auto staticMesh = etna::get_shader_program("staticmesh_shader");

  cmd_buf.bindVertexBuffers(0, {ctx.sceneMgr->getVertexBuffer()}, {0});
  cmd_buf.bindIndexBuffer(ctx.sceneMgr->getIndexBuffer(), 0, vk::IndexType::eUint32);


  pushConst2M.wId = ctx.worldId;
  auto set0 = etna::create_descriptor_set(
  staticMesh.getDescriptorLayoutId(0),
  cmd_buf,
  {
    etna::Binding{0, instanceMatricesBuf.genBinding()},
  });
  auto set1 = ctx.sceneMgr->resources().getSet();
  auto set2 = etna::create_descriptor_set(
    staticMesh.getDescriptorLayoutId(2),
    cmd_buf,
    {{etna::Binding{0, ctx.worldViewMatrices.genBinding()}}}
  );
  auto relems = ctx.sceneMgr->getRenderElements();
  std::size_t firstInstance = 0;


  auto* commands = reinterpret_cast<DrawCmd* >(drawCommandsBuf.data());
  for (std::size_t j = 0; j < relems.size(); ++j)
  {
    //Skip drawing water as it is rendered by terrain
    if (j == 8) {
      firstInstance += nInstances[j];
      continue;
    }  
    if (nInstances[j] != 0)
    {
      const auto& relem = relems[j];
      
      commands[j].cmd.setIndexCount(relem.indexCount);
      commands[j].cmd.setInstanceCount(nInstances[j]);
      commands[j].cmd.setFirstIndex(relem.indexOffset);
      commands[j].cmd.setVertexOffset(relem.vertexOffset);
      commands[j].cmd.setFirstInstance(firstInstance);
      commands[j].material = glm::uint(relem.materialId);
    }
    firstInstance += nInstances[j];
  }

  auto set3 = etna::create_descriptor_set(
    staticMesh.getDescriptorLayoutId(3),
    cmd_buf,
    {{etna::Binding{0, drawCommandsBuf.genBinding()}}}
  );

  cmd_buf.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline.getVkPipelineLayout(),
    0,
    {set0.getVkSet(), set1, set2.getVkSet(), set3.getVkSet()},
    {}
  );

  cmd_buf.pushConstants<PushConstants>(
    pipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, {pushConst2M});
  cmd_buf.drawIndexedIndirect(drawCommandsBuf.get(), 0, relems.size(), static_cast<uint32_t>(sizeof(DrawCmd)));
}

static bool 
IsNotVisble(const glm::mat2x3 /*bounds*/, const glm::mat4x4& /*transform*/)
{
  return false;
  #if 0
  const glm::vec3 origin = transform * glm::vec4(bounds[0], 1.f);
  glm::mat3 oldExtent = glm::mat3(
    bounds[1].x, 0, 0,
    0, bounds[1].y, 0,
    0, 0, bounds[1].z
  );
  const glm::vec3 extent = glm::abs(glm::mat3(transform) * oldExtent) * glm::vec3(1.f, 1.f, 1.f);
  
  if(origin.z + extent.z < 0) return true;

  glm::vec3 lc = glm::vec3(origin) + extent;
  glm::vec3 rc = glm::vec3(origin) - extent;
  return 
        lc.x < -lc.z || 
        lc.y < -lc.z || 
        rc.x >  lc.z || 
        rc.y >  lc.z;
  #endif
}

void 
StaticMeshPipeline::reserve(std::size_t n) {
  spdlog::info("n = {}", n);
  nInstances.assign(n, 0); 
  auto& ctx = etna::get_context();

  drawCommandsBuf = ctx.createBuffer({
    .size = n * sizeof(DrawCmd),
    .bufferUsage = vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    .name = "drawCommandsBuf",
  });
  drawCommandsBuf.map();
}

void 
StaticMeshPipeline::prepareFrame(const RenderContext& ctx)
{
  auto instanceMeshes = ctx.sceneMgr->getInstanceMeshes();
  auto instanceMatrices = ctx.sceneMgr->getInstanceMatrices();
  auto meshes = ctx.sceneMgr->getMeshes();
  auto bbs = ctx.sceneMgr->getBounds();

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
      if (enableCulling && (ctx.worldId == 0) && IsNotVisble(bbs[relemIdx], ctx.worldViewProj * instanceMatrix))
      {
        continue;
      }
      nInstances[relemIdx]++;
      *matrices++ = instanceMatrix;
    }
  }
}

} /* namespace pipes */
