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

    tilingSampler = etna::Sampler({
      .filter = vk::Filter::eNearest,
      .addressMode = vk::SamplerAddressMode::eRepeat,
      .name = "tilingSampler",
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
  
  auto terrainShader = etna::get_shader_program("terrain_shader");
  set1 = etna::create_descriptor_set(
    terrainShader.getDescriptorLayoutId(1),
    cmd_buf,
    {
      etna::Binding{0, (*ctx.sceneMgr)[m_textures[0]].image.genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{1, (*ctx.sceneMgr)[m_textures[1]].image.genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{2, (*ctx.sceneMgr)[m_textures[2]].image.genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{3, (*ctx.sceneMgr)[m_textures[3]].image.genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{4, (*ctx.sceneMgr)[m_textures[4]].image.genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}
    }
  );
  for(int i = 0; i < activeLayers; ++i) {
    auto& level = levels[i];
    for(unsigned x = 0; x < 4; ++x) {
      for(unsigned y = 0; y < 4; ++y) {
        int dx = ((x - level.getIPos().x) % 4 + 6) % 4;
        int dy = ((y - level.getIPos().y) % 4 + 6) % 4;
        uint8_t mask = 0;
        if (i != 0) {
          glm::ivec2 idx = (level.getChunk().iPos + glm::ivec2{dx, dy}) * 2;
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
        // drawChunk(cmd_buf, chunk, mask);
        drawSubChunk(cmd_buf, level.getChunk(), {dx, dy}, mask);
      }
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
  auto terrainShader = etna::get_shader_program("terrain_shader");

  auto set = etna::create_descriptor_set(
    terrainShader.getDescriptorLayoutId(0),
    cmd_buf,
    {
      etna::Binding{0, cur_chunk.getImage(0).genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{1, cur_chunk.getImage(1).genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{2, cur_chunk.getImage(2).genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}
    }
  );

  

  cmd_buf.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline.getVkPipelineLayout(), //NOTE - Both pipelines share same layout. 
    0,
    {set.getVkSet(), set1.getVkSet()},
    {}
  );

  const size_t nChunks = std::max(static_cast<uint64_t>(2ul), heightMapResolution / MAX_TESCELLATION);
  const size_t nHalfChunks = nChunks / 2;

  pushConstants.extent = cur_chunk.getExtentPos() / float(nChunks);
  pushConstants.degree = 256;
  pushConstants.nHalfChunks = static_cast<glm::uint>(nHalfChunks);
  pushConstants.base = cur_chunk.getStartPos();

  for(size_t i = 0; i < 2; ++i) {
    for(size_t j = 0; j < 2; ++j) {
      pushConstants.subChunk = static_cast<glm::uint>(2*i+j);
      if((chunk_mask & 1) != 0) {
        cmd_buf.pushConstants(
          pipeline.getVkPipelineLayout(), 
          vk::ShaderStageFlagBits::eVertex |
          vk::ShaderStageFlagBits::eTessellationEvaluation |
          vk::ShaderStageFlagBits::eTessellationControl |
          vk::ShaderStageFlagBits::eFragment,
          0, 
          sizeof(pushConstants), &pushConstants
        );
        
        cmd_buf.draw(4, static_cast<uint32_t>(nHalfChunks * nHalfChunks), 0, 0);
      }
      chunk_mask >>= 1;
    }
  }
}

void 
TerrainPipeline::drawSubChunk(vk::CommandBuffer cmd_buf, targets::TerrainChunk& glob_chunk, glm::uvec2 index, uint8_t chunk_mask)
{
  auto terrainShader = etna::get_shader_program("terrain_shader");

  auto set = etna::create_descriptor_set(
    terrainShader.getDescriptorLayoutId(0),
    cmd_buf,
    {
      etna::Binding{0, glob_chunk.getImage(0).genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{1, glob_chunk.getImage(1).genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{2, glob_chunk.getImage(2).genBinding(tilingSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}
    }
  );

  cmd_buf.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline.getVkPipelineLayout(), //NOTE - Both pipelines share same layout. 
    0,
    {set.getVkSet(), set1.getVkSet()},
    {}
  );

  const size_t nChunks = std::max(static_cast<uint64_t>(2ul), heightMapResolution / MAX_TESCELLATION);
  const size_t nHalfChunks = nChunks / 2;

  pushConstants.extent = glob_chunk.getExtentPos() / float(nChunks) / 4.f;
  pushConstants.degree = 256;
  pushConstants.nHalfChunks = static_cast<glm::uint>(nHalfChunks);
  pushConstants.base = glob_chunk.getStartPos() + glm::vec2(index) * glob_chunk.getExtentPos() / 4.f;
  pushConstants.corner = 0x0;
  if(index.x == 0) pushConstants.corner |= 0x01;
  if(index.x == 3) pushConstants.corner |= 0x02;
  if(index.y == 0) pushConstants.corner |= 0x04;
  if(index.y == 3) pushConstants.corner |= 0x08;

  for(size_t i = 0; i < 2; ++i) {
    for(size_t j = 0; j < 2; ++j) {
      pushConstants.subChunk = static_cast<glm::uint>(2*i+j);
      if((chunk_mask & 1) != 0) {
        cmd_buf.pushConstants(
          pipeline.getVkPipelineLayout(), 
          vk::ShaderStageFlagBits::eVertex |
          vk::ShaderStageFlagBits::eTessellationEvaluation |
          vk::ShaderStageFlagBits::eTessellationControl |
          vk::ShaderStageFlagBits::eFragment,
          0, 
          sizeof(pushConstants), &pushConstants
        );
        
        cmd_buf.draw(4, static_cast<uint32_t>(nHalfChunks * nHalfChunks), 0, 0);
      }
      chunk_mask >>= 1;
    }
  }
}

void TerrainPipeline::loadTextures(SceneManager& scene_mgr)
{
  m_textures[0] = scene_mgr.loadTexture(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "grass.jpg");
  m_textures[1] = scene_mgr.loadTexture(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "sand.jpg");
  m_textures[2] = scene_mgr.loadTexture(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "snow.jpg");
  m_textures[3] = scene_mgr.loadTexture(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "rock.jpg");
  m_textures[4] = scene_mgr.loadTexture(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "ground.jpg");
}


} /* namespace pipes */
