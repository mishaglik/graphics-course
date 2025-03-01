#include "Terrain.hpp"

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
TerrainPipeline::allocate()
{
    terrainGenerator.allocate();
    chunk.allocate({heightMapResolution, heightMapResolution});
    tmp  .allocate({heightMapResolution, heightMapResolution});
    // auto& ctx = etna::get_context();
}

void 
TerrainPipeline::loadShaders() 
{
    terrainGenerator.loadShaders();
    etna::create_program(
        "terrain_shader",
        { TERRAIN_PIPELINE_SHADERS_ROOT "terrain.vert.spv",
          TERRAIN_PIPELINE_SHADERS_ROOT "terrain.tesc.spv",
          TERRAIN_PIPELINE_SHADERS_ROOT "terrain.tese.spv",
          TERRAIN_PIPELINE_SHADERS_ROOT "terrain.frag.spv"}
    );
}

void 
TerrainPipeline::setup() 
{
    terrainGenerator.setup();
    auto& pipelineManager = etna::get_context().getPipelineManager();

    pipeline = pipelineManager.createGraphicsPipeline(
    "terrain_shader",
    etna::GraphicsPipeline::CreateInfo{
      .inputAssemblyConfig = {.topology = vk::PrimitiveTopology::ePatchList},
      .tessellationConfig = {
        .patchControlPoints = 4,
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

    pipelineDebug = pipelineManager.createGraphicsPipeline(
    "terrain_shader",
    etna::GraphicsPipeline::CreateInfo{
      .inputAssemblyConfig = {.topology = vk::PrimitiveTopology::ePatchList},
      .tessellationConfig = { 
        .patchControlPoints = 4,
      },
      .rasterizationConfig = {
        .polygonMode = vk::PolygonMode::eLine,
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
TerrainPipeline::drawGui()
{
    if(ImGui::TreeNode("Generator settings")) {
        if(ImGui::SliderInt("Terrain scale", &terrainScale, 1, 12)) {
          terrainValid = true;
        }
        
        if(ImGui::SliderFloat("Frequency", &startFrequency, 0, 10)) {
          terrainValid = false;
        }
        
        terrainGenerator.drawGui();
        if(ImGui::Button("Regenerate")) {
          terrainValid = false;
        }
        ImGui::TreePop();
    }

    ImGui::SliderFloat("Max height", &pushConstants.maxHeight, 1, 100);
    ImGui::SliderFloat("Sea level", &pushConstants.seaLevel, -pushConstants.maxHeight, pushConstants.maxHeight);
    ImGui::Checkbox("Wireframe [F3]", &wireframe);
}

void 
TerrainPipeline::debugInput(const Keyboard& kb)
{
    terrainGenerator.debugInput(kb);
    if (kb[KeyboardKey::kF3] == ButtonState::Falling) {
        wireframe = !wireframe;
        spdlog::info("terrain wireframe is {}", wireframe ? "on" : "off");
    }
}

targets::GBuffer& 
TerrainPipeline::render(vk::CommandBuffer cmd_buf, targets::GBuffer& target, const RenderContext& ctx)
{
  ETNA_PROFILE_GPU(cmd_buf, renderTerrain);
  pushConstants.mat  = ctx.worldViewProj;
    
  auto& currentPipeline = wireframe ? pipelineDebug : pipeline;
  
  cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, currentPipeline.getVkPipeline());
  
  drawChunk(cmd_buf, chunk);

  return target;
}

void 
TerrainPipeline::regenerateTerrainIfNeeded(vk::CommandBuffer cmd_buf)
{
  if(terrainValid)
    return;

  ETNA_PROFILE_GPU(cmd_buf, terrainGenerator);
  chunk.setPosition({-1, -1}, {2, 2});
  generate_chunk(cmd_buf, terrainGenerator, chunk, tmp, startFrequency, terrainScale);
  chunk.setState(cmd_buf,
    vk::PipelineStageFlagBits2::eTessellationEvaluationShader, 
    vk::AccessFlagBits2::eShaderSampledRead, 
    vk::ImageLayout::eShaderReadOnlyOptimal, 
    vk::ImageAspectFlagBits::eColor
  );
  terrainValid = true;
}

void 
TerrainPipeline::drawChunk(vk::CommandBuffer cmd_buf, targets::TerrainChunk& cur_chunk)
{
  auto& hmap = cur_chunk.getImage(0); 
  
  auto terrainShader = etna::get_shader_program("terrain_shader");

  auto set = etna::create_descriptor_set(
    terrainShader.getDescriptorLayoutId(0),
    cmd_buf,
    {etna::Binding{0, hmap.genBinding(terrainGenerator.getSampler().get(), vk::ImageLayout::eShaderReadOnlyOptimal)}}
  );

  cmd_buf.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline.getVkPipelineLayout(), //NOTE - Both pipelines share same layout. 
    0,
    {set.getVkSet()},
    {}
  );

  const size_t nChunks = heightMapResolution / MAX_TESCELLATION;

  pushConstants.extent = cur_chunk.getExtentPos() / float(nChunks);
  pushConstants.degree = 256;
  for (size_t x = 0; x < nChunks; ++x) {
    for (size_t y = 0; y < nChunks; ++y) {
      pushConstants.base = cur_chunk.getStartPos() + glm::vec2{float(x), float(y)} * pushConstants.extent;

        cmd_buf.pushConstants(
            pipeline.getVkPipelineLayout(), 
              vk::ShaderStageFlagBits::eVertex |
              vk::ShaderStageFlagBits::eTessellationEvaluation |
              vk::ShaderStageFlagBits::eTessellationControl,
              0, 
            sizeof(pushConstants), &pushConstants
        );

        cmd_buf.draw(4, 1, 0, 0);
    }
  }
}

} /* namespace pipes */
