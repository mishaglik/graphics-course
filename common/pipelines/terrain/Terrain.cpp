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
    terrainGenerator.drawGui();

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
  pushConstants.seaLevel = ctx.sceneMgr->terrain().seaLevel();
  pushConstants.maxHeight = ctx.sceneMgr->terrain().maxHeight();
    
  auto& currentPipeline = wireframe ? pipelineDebug : pipeline;
  
  cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, currentPipeline.getVkPipeline());
  
  auto terrainShader = etna::get_shader_program("terrain_shader");
  set1 = ctx.sceneMgr->terrain().textureSet(cmd_buf, terrainShader.getDescriptorLayoutId(1));
  
  auto levels = ctx.sceneMgr->terrain().levels();
  for(size_t i = 0; i < levels.size(); ++i) {
    auto& level = levels[i];
    for(unsigned x = 0; x < 4; ++x) {
      for(unsigned y = 0; y < 4; ++y) {
        int dx = ((x - level.pos.x) % 4 + 6) % 4;
        int dy = ((y - level.pos.y) % 4 + 6) % 4;
        uint8_t mask = 0;
        if (i != 0) {
          glm::ivec2 idx = (level.chunk.iPos + glm::ivec2{dx, dy}) * 2;
          glm::ivec2 bIdx = levels[i-1].pos;
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
        drawSubChunk(cmd_buf, level.chunk, {dx, dy}, mask);
      }
    } 
  }

  return target;
}

void 
TerrainPipeline::regenerateTerrainIfNeeded(vk::CommandBuffer cmd_buf, glm::vec2 pos, scene::TerrainManager& terrain)
{
  ETNA_PROFILE_GPU(cmd_buf, terrainGenerator);
  auto levels = terrain.levels();
  for(std::size_t i = 0; i < levels.size(); ++i) {
    auto& level = levels[i];
    glm::ivec2 newPos = static_cast<glm::ivec2>(glm::trunc(pos / level.step));
    if(newPos == level.pos && terrain.isUpToDate()) 
        continue;
    level.pos = newPos;
    level.chunk.iPos = level.pos + glm::ivec2{-2, -2};
    level.chunk.setPosition(static_cast<glm::vec2>(level.pos + glm::ivec2{-2, -2}) * level.step);
    level.chunk.setState(cmd_buf,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
        vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite, 
        vk::ImageLayout::eColorAttachmentOptimal, 
        vk::ImageAspectFlagBits::eColor
    );
    etna::flush_barriers(cmd_buf);
    {
        etna::RenderTargetState renderTarget{
            cmd_buf,
            {{0, 0}, {level.chunk.getResolution().x, level.chunk.getResolution().y}},
            level.chunk.getColorAttachments(),
            {},
            BarrierBehavoir::eSuppressBarriers
        };
        if(!terrain.isUpToDate()) {
            std::array<vk::ClearAttachment, targets::TerrainChunk::N_COLOR_ATTACHMENTS> clearAtts{
                vk::ClearAttachment{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .colorAttachment = 0,
                },
                vk::ClearAttachment{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .colorAttachment = 1,
                },
                vk::ClearAttachment{
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .colorAttachment = 2,
                }
            };
            std::array<vk::ClearRect, targets::TerrainChunk::N_COLOR_ATTACHMENTS> clearRects{
                vk::ClearRect{
                    .rect = {{0, 0}, {level.chunk.getResolution().x, level.chunk.getResolution().y}},
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                vk::ClearRect{
                    .rect = {{0, 0}, {level.chunk.getResolution().x, level.chunk.getResolution().y}},
                    .baseArrayLayer = 0,
                    .layerCount = 1
                },
                vk::ClearRect{
                    .rect = {{0, 0}, {level.chunk.getResolution().x, level.chunk.getResolution().y}},
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };
            cmd_buf.clearAttachments(clearAtts, clearRects);
        }
        for(int i = -2; i < 2; i++) {
            for(int j = -2; j < 2; j++) {
                //NOTE - Some arithmetics to implement reuse of detailed
                int ix = ((level.pos.x + i) % 4 + 4) % 4;
                int iy = ((level.pos.y + j) % 4 + 4) % 4;
                auto& chunk = level.ipos[4 * ix + iy];
                if(chunk == (level.pos + glm::ivec2{i, j}) && terrain.isUpToDate()) 
                    continue;
                chunk = (level.pos + glm::ivec2{i, j});
                glm::vec2 chunkPos = static_cast<glm::vec2>(level.pos + glm::ivec2{i, j}) * level.step;
                terrainGenerator.reset(chunkPos, level.chunk.getExtentPos() / 4.f, terrain.frequency());
                glm::vec2 texStart  = glm::vec2(ix / 4.f, iy / 4.f);
                glm::vec2 texExtent{.25f, .25f};
               
                terrainGenerator.setSubregion(texStart, texExtent);
                terrainGenerator.render(cmd_buf, level.chunk, static_cast<uint32_t>(terrain.terrainScale()-std::min((int)i, terrain.terrainScale()-1)));
            }
        }
    }
    level.chunk.setState(cmd_buf,
        vk::PipelineStageFlagBits2::eTessellationEvaluationShader | vk::PipelineStageFlagBits2::eFragmentShader, 
        vk::AccessFlagBits2::eShaderSampledRead, 
        vk::ImageLayout::eShaderReadOnlyOptimal, 
        vk::ImageAspectFlagBits::eColor
    );
  }

  terrain.setValid(true);
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

  const size_t nChunks = std::max(static_cast<uint64_t>(2ul), static_cast<uint64_t>(2ul)); //FIXME: heightMapResolution / MAX_TESCELLATION
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
  
  const size_t nChunks = std::max(static_cast<uint64_t>(2ul), static_cast<uint64_t>(2ul)); //FIXME: heightMapResolution / MAX_TESCELLATION
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

void TerrainPipeline::loadTextures(SceneManager&)
{

}


} /* namespace pipes */
