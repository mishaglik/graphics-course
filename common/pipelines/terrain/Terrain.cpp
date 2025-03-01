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
    for(std::size_t i = 0; i < N_CLIP_LEVELS; ++i) {
      auto& level = levels[i];
      level.allocate({32<<i, 32<<i}, heightMapResolution);
    }
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
        if(ImGui::SliderInt("Terrain scale", &terrainScale, 1, 32)) {
          terrainValid = true;
        }
        
        if(ImGui::SliderFloat("Frequency", &startFrequency, 0, 1)) {
          terrainValid = false;
        }
        
        terrainGenerator.drawGui();
        if(ImGui::Button("Regenerate")) {
          terrainValid = false;
        }
        ImGui::TreePop();
    }

    ImGui::SliderFloat("Max height", &pushConstants.maxHeight, 1, 100);
    ImGui::SliderFloat("Sea level", &pushConstants.seaLevel, 0, pushConstants.maxHeight);
    ImGui::SliderInt("Active layers", &activeLayers, 1, N_CLIP_LEVELS);
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
  pushConstants.camPos  = ctx.camPos;
    
  auto& currentPipeline = wireframe ? pipelineDebug : pipeline;
  
  cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, currentPipeline.getVkPipeline());
  
  for(int i = 0; i < activeLayers; ++i) {
    auto& level = levels[i];
    for(auto& chunk: level.getChunks())
    {
      uint8_t mask = 0;
      if (i != 0) {
        glm::ivec2 idx = chunk.iPos * 2;
        glm::ivec2 bIdx = levels[i-1].getIPos();
        if(idx.x - bIdx.x >  0) mask |= 0b1100;
        if(idx.x - bIdx.x >  1) mask |= 0b1111;
        if(idx.x - bIdx.x < -2) mask |= 0b0011;
        if(idx.x - bIdx.x < -3) mask |= 0b1111;

        if(idx.y - bIdx.y >  0) mask |= 0b1010;
        if(idx.y - bIdx.y >  1) mask |= 0b1111;
        if(idx.y - bIdx.y < -2) mask |= 0b0101;
        if(idx.y - bIdx.y < -3) mask |= 0b1111;
      } else {
        mask = 0xF;
      }
      drawChunk(cmd_buf, chunk, mask);
    }
  }

  return target;
}

void 
TerrainPipeline::regenerateTerrainIfNeeded(vk::CommandBuffer cmd_buf, glm::vec2 pos)
{
  ETNA_PROFILE_GPU(cmd_buf, terrainGenerator);
  for(std::size_t i = 0; i < N_CLIP_LEVELS; ++i) {
    auto& level = levels[i];
    level.update(cmd_buf, terrainGenerator, pos, tmp, startFrequency, terrainScale-std::min((int)i, terrainScale-1), !terrainValid);
  }

  terrainValid = true;
}

void 
TerrainPipeline::drawChunk(vk::CommandBuffer cmd_buf, targets::TerrainChunk& cur_chunk, uint8_t chunk_mask)
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

  const size_t nChunks = std::max(2ul, heightMapResolution / MAX_TESCELLATION);
  const size_t nHalfChunks = nChunks / 2;

  pushConstants.extent = cur_chunk.getExtentPos() / float(nChunks);
  pushConstants.degree = 256;
  pushConstants.nHalfChunks = nHalfChunks;
  pushConstants.base = cur_chunk.getStartPos();
  for(size_t i = 0; i < 2; ++i) {
    for(size_t j = 0; j < 2; ++j) {
      pushConstants.subChunk = 2*i+j;
      if((chunk_mask & 1) != 0) {
        cmd_buf.pushConstants(
          pipeline.getVkPipelineLayout(), 
          vk::ShaderStageFlagBits::eVertex |
          vk::ShaderStageFlagBits::eTessellationEvaluation |
          vk::ShaderStageFlagBits::eTessellationControl,
          0, 
          sizeof(pushConstants), &pushConstants
        );
        
        cmd_buf.draw(4, nHalfChunks * nHalfChunks, 0, 0);
      }
      chunk_mask >>= 1;
    }
  }
}

} /* namespace pipes */
