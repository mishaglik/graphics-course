#include "TerrainTransparent.hpp"

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
TerrainTransparentPipeline::allocate()
{
  waterGenerator.allocate();
}

void 
TerrainTransparentPipeline::loadShaders() 
{
    etna::create_program(
        "terrain_transparent",
        { TERRAIN_PIPELINE_SHADERS_ROOT "terrain.vert.spv",
          TERRAIN_PIPELINE_SHADERS_ROOT "terrain.tesc.spv",
          TERRAIN_PIPELINE_SHADERS_ROOT "terrain_transparent.tese.spv",
          TERRAIN_PIPELINE_SHADERS_ROOT "terrain_transparent.frag.spv"}
    );
    waterGenerator.loadShaders();
}

void 
TerrainTransparentPipeline::setup() 
{
    waterGenerator.setup();
    auto& pipelineManager = etna::get_context().getPipelineManager();

    pipeline = pipelineManager.createGraphicsPipeline(
    "terrain_transparent",
    etna::GraphicsPipeline::CreateInfo{
      .inputAssemblyConfig = {.topology = vk::PrimitiveTopology::ePatchList},
      .tessellationConfig = {
        .patchControlPoints = 4,
      },
      
      .blendingConfig = {
        .attachments = {
          vk::PipelineColorBlendAttachmentState{
            .blendEnable = vk::True,
            .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eZero,
            .dstAlphaBlendFactor = vk::BlendFactor::eOne,
            .alphaBlendOp = vk::BlendOp::eAdd,
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
    "terrain_transparent",
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
            .blendEnable = vk::True,
            .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eZero,
            .dstAlphaBlendFactor = vk::BlendFactor::eOne,
            .alphaBlendOp = vk::BlendOp::eAdd,
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
      .filter = vk::Filter::eLinear,
      .addressMode = vk::SamplerAddressMode::eRepeat,
      .name = "tilingSampler",
    });
    defaultSampler = etna::Sampler({
      .name = "defaultSampler",
    });
}

void 
TerrainTransparentPipeline::drawGui()
{
  if(ImGui::TreeNode("Generator")) {
    waterGenerator.drawGui();
    ImGui::TreePop();
  }
  ImGui::Checkbox("Update enabled", &updateEnabled);
  if(ImGui::Button("Update")) {
    ready = false;
  }
  ImGui::SliderFloat("Wave height", &pushConstants.maxHeight, 0, 15);
  ImGui::Checkbox("Wireframe sea", &wireframe);
}

void 
TerrainTransparentPipeline::debugInput(const Keyboard& kb)
{
  waterGenerator.debugInput(kb);
}

void
TerrainTransparentPipeline::render(vk::CommandBuffer cmd_buf, targets::GBuffer&, const RenderContext& ctx)
{
  auto& skybox = ctx.sceneMgr->resources()[ctx.sceneMgr->skybox()].image;

  ETNA_PROFILE_GPU(cmd_buf, renderTerrain);
  pushConstants.mat  = ctx.worldViewProj;
  pushConstants.camPos  = ctx.camPos;
  pushConstants.seaLevel = ctx.sceneMgr->terrain().seaLevel();
  pushConstants.time = static_cast<float>(ctx.frameTime);
  
  auto& currentPipeline = wireframe ? pipelineDebug : pipeline;
  
  cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, currentPipeline.getVkPipeline());
  
  auto terrainShader = etna::get_shader_program("terrain_transparent");
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
        drawSubChunk(cmd_buf, ctx.sceneMgr->terrain().water(), level.chunk, {dx, dy}, skybox, mask);
      }
    } 
  }

}

void 
TerrainTransparentPipeline::drawSubChunk(vk::CommandBuffer cmd_buf, targets::WaterChunk& water, targets::TerrainChunk& glob_chunk, glm::uvec2 index, const etna::Image& skybox, uint8_t chunk_mask)
{
  auto terrainShader = etna::get_shader_program("terrain_transparent");

  auto set = etna::create_descriptor_set(
    terrainShader.getDescriptorLayoutId(0),
    cmd_buf,
    {
      etna::Binding{0, water                 .genBinding(tilingSampler .get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{1, glob_chunk.getImage(1).genBinding(tilingSampler .get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{2, glob_chunk.getImage(2).genBinding(tilingSampler .get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      etna::Binding{3, skybox                .genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=6, .type=vk::ImageViewType::eCube})}
    }
  );

  cmd_buf.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline.getVkPipelineLayout(), //NOTE - Both pipelines share same layout. 
    0,
    {set.getVkSet(), set1.getVkSet()},
    {}
  );
  
  const size_t nChunks = std::max(static_cast<uint64_t>(4ul), static_cast<uint64_t>(2ul)); //FIXME: heightMapResolution / MAX_TESCELLATION 
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

void 
TerrainTransparentPipeline::prepare (vk::CommandBuffer cmd_buf, const RenderContext& ctx) {
  if(!updateEnabled && ready)
    return;
  etna::set_state(
    cmd_buf, 
    ctx.sceneMgr->terrain().water().get(), 
    vk::PipelineStageFlagBits2::eComputeShader, 
    vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite | vk::AccessFlagBits2::eShaderWrite, 
    vk::ImageLayout::eGeneral, 
    vk::ImageAspectFlagBits::eColor,
    ForceSetState::eTrue
  );
  etna::flush_barriers(cmd_buf);

  waterGenerator.render(cmd_buf, ctx.sceneMgr->terrain().water(), static_cast<float>(ctx.frameTime));
  etna::set_state(
    cmd_buf, 
    ctx.sceneMgr->terrain().water().get(), 
    vk::PipelineStageFlagBits2::eAllCommands, 
    vk::AccessFlagBits2::eShaderSampledRead | vk::AccessFlagBits2::eShaderRead, 
    vk::ImageLayout::eShaderReadOnlyOptimal, 
    vk::ImageAspectFlagBits::eColor,
    ForceSetState::eTrue
  );
  etna::flush_barriers(cmd_buf);
  ready = true;
}

} /* namespace pipes */
