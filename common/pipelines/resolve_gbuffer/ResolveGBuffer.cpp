#include "ResolveGBuffer.hpp"

#include <etna/BlockingTransferHelper.hpp>
#include <etna/Etna.hpp>
#include <etna/Profiling.hpp>
#include <etna/GlobalContext.hpp>
#include <etna/PipelineManager.hpp>
#include <etna/RenderTargetStates.hpp>

#include <imgui.h>

#include "stb_image.h"
#include <math.h>
#ifndef M_PIf
#define M_PIf 3.14159265358979323846f
#endif
namespace pipes {


float rand_float() {
  return float(rand()) / float(RAND_MAX);
}

void 
ResolveGBufferPipeline::allocate(glm::uvec2 resolution)
{
    auto& ctx = etna::get_context();
    samplingPoints = ctx.createBuffer({
      .size = 400 * sizeof(glm::vec4),
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
      .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
      .name = "Sampling points",
    });
    samplingPoints.map();
    auto* points = reinterpret_cast<glm::vec4*>(samplingPoints.data());
    
    for(size_t i = 0; i < 400; i++) {
      float r = rand_float();
      float a = rand_float();
      points[i] = glm::vec4((r - 0.5) * cos(2 * M_PIf * a), (r - 0.5) * sin(2 * M_PIf * a), 0, 0); 
    }
    diffuse.allocate({resolution.x / 2, resolution.y / 2});
}

void 
ResolveGBufferPipeline::loadShaders() 
{
  etna::create_program(
    "deferred_shader",
    {RESOLVEGBUFFER_PIPELINE_SHADERS_ROOT "deferred.vert.spv",
     RESOLVEGBUFFER_PIPELINE_SHADERS_ROOT "deferred.frag.spv"}
  );
  etna::create_program(
    "diffuse_shader",
    {RESOLVEGBUFFER_PIPELINE_SHADERS_ROOT "deferred.vert.spv",
     RESOLVEGBUFFER_PIPELINE_SHADERS_ROOT "diffuse.frag.spv"}
  );
  etna::create_program(
    "sphere_deferred_shader",
    {RESOLVEGBUFFER_PIPELINE_SHADERS_ROOT "sphere.vert.spv",
     RESOLVEGBUFFER_PIPELINE_SHADERS_ROOT "sphere_deferred.frag.spv"}
  );

  etna::create_program(
    "sphere_shader",
    {RESOLVEGBUFFER_PIPELINE_SHADERS_ROOT "sphere.vert.spv",
     RESOLVEGBUFFER_PIPELINE_SHADERS_ROOT "sphere.frag.spv"}
  );

  defaultSampler = etna::Sampler({
    .filter = vk::Filter::eLinear,
    .name = "resolveGbufferSampler",
  });
}

void 
ResolveGBufferPipeline::setup() 
{
  auto& pipelineManager = etna::get_context().getPipelineManager();

  deferredLightPipeline = pipelineManager.createGraphicsPipeline(
  "deferred_shader",
  etna::GraphicsPipeline::CreateInfo{
    .depthConfig = {
      .depthTestEnable = vk::True,
      .depthWriteEnable = vk::True,
      .depthCompareOp = vk::CompareOp::eLess,
      .maxDepthBounds = 1.f,
    },
    .fragmentShaderOutput = {
      .colorAttachmentFormats = RenderTarget::COLOR_ATTACHMENT_FORMATS,
      .depthAttachmentFormat = RenderTarget::DEPTH_ATTACHMENT_FORMAT,
    },
  });

  diffuseLightPipeline = pipelineManager.createGraphicsPipeline(
    "diffuse_shader",
    etna::GraphicsPipeline::CreateInfo{
      .fragmentShaderOutput = {
        .colorAttachmentFormats = decltype(diffuse)::COLOR_ATTACHMENT_FORMATS,
        .depthAttachmentFormat  = decltype(diffuse)::DEPTH_ATTACHMENT_FORMAT,
      },
    });

  spherePipeline = pipelineManager.createGraphicsPipeline(
  "sphere_shader",
  etna::GraphicsPipeline::CreateInfo{
    .inputAssemblyConfig = { 
      .topology = vk::PrimitiveTopology::eTriangleStrip,
    },
    .rasterizationConfig = {
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .lineWidth = 1.f,
    },
    
    .blendingConfig = {
      .attachments = {
        vk::PipelineColorBlendAttachmentState{
          .blendEnable = vk::True,
          .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
          .dstColorBlendFactor = vk::BlendFactor::eOne,
          .colorBlendOp = vk::BlendOp::eAdd,
          .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        },
      },
      .logicOpEnable = false,
      .logicOp = {},
      .blendConstants = {},
    },
    .depthConfig = {
      .depthTestEnable = vk::True,
      .depthWriteEnable = vk::False,
      .depthCompareOp = vk::CompareOp::eLessOrEqual,
      .maxDepthBounds = 1.f,
    },
    .fragmentShaderOutput = {
      .colorAttachmentFormats = RenderTarget::COLOR_ATTACHMENT_FORMATS,
      .depthAttachmentFormat = RenderTarget::DEPTH_ATTACHMENT_FORMAT,
    },
  });

  sphereDeferredPipeline = pipelineManager.createGraphicsPipeline(
  "sphere_deferred_shader",
  etna::GraphicsPipeline::CreateInfo{
    .inputAssemblyConfig = { 
      .topology = vk::PrimitiveTopology::eTriangleStrip,
    },
    .rasterizationConfig = {
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eFront,
      .lineWidth = 1.f,
    },
    
    .blendingConfig = {
      .attachments = {
        vk::PipelineColorBlendAttachmentState{
          .blendEnable = vk::True,
          .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
          .dstColorBlendFactor = vk::BlendFactor::eOne,
          .colorBlendOp = vk::BlendOp::eAdd,
          .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        },
      },
      .logicOpEnable = false,
      .logicOp = {},
      .blendConstants = {},
    },
    .depthConfig = {
      .depthTestEnable = vk::True,
      .depthWriteEnable = vk::False,
      .depthCompareOp = vk::CompareOp::eGreaterOrEqual,
      .maxDepthBounds = 1.f,
    },
    .fragmentShaderOutput = {
      .colorAttachmentFormats = RenderTarget::COLOR_ATTACHMENT_FORMATS,
      .depthAttachmentFormat = RenderTarget::DEPTH_ATTACHMENT_FORMAT,
    },
  });
}

void 
ResolveGBufferPipeline::drawGui()
{
  ImGui::Checkbox("Use pbr", &usePbr);
  if(ImGui::Checkbox("Use gi", &globalIllumination)) {
    pushConstants.gi = globalIllumination ? 1 : 0;
  }
  ImGui::Checkbox("Enable secondary lighting", &secondaryLight);
  ImGui::Checkbox("Enable secondary lighting sources", &secondaryLightSources);

  ImGui::Checkbox("Normal as color[WIP]", &normalAsAlbedo);
}

void 
ResolveGBufferPipeline::debugInput(const Keyboard& /*kb*/)
{

}

void
ResolveGBufferPipeline::prepare(vk::CommandBuffer cmd_buf, targets::GBuffer& source, targets::GBuffer& shadow, uint32_t shadow_cascades, const RenderContext& ctx)
{
  etna::set_state(
    cmd_buf, 
    diffuse.get(), 
    vk::PipelineStageFlagBits2::eColorAttachmentOutput, 
    vk::AccessFlagBits2::eColorAttachmentWrite, 
    vk::ImageLayout::eColorAttachmentOptimal, 
    vk::ImageAspectFlagBits::eColor
  );
  etna::flush_barriers(cmd_buf);
  auto& skybox = ctx.sceneMgr->resources()[ctx.sceneMgr->skybox()].image;

  ETNA_PROFILE_GPU(cmd_buf, pipelines_diffuse_render);
  {
    etna::RenderTargetState renderTarget({
      cmd_buf,
      {{0, 0}, {diffuse.getResolution().x, diffuse.getResolution().y}},
      diffuse.getColorAttachments(),
      diffuse.getDepthAttachment(),
      {}
    });
    auto diffuseLightShader = etna::get_shader_program("diffuse_shader");
    auto& pipeline = diffuseLightPipeline;

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());

    auto set = etna::create_descriptor_set(
      diffuseLightShader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, source.getImage(0).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{1, source.getImage(1).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{2, source.getImage(2).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{3, source.getImage(3).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{4, source.getImage(4).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        
        etna::Binding{5, shadow.getImage(0).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},
        etna::Binding{6, shadow.getImage(1).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},
        etna::Binding{7, shadow.getImage(2).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},
        etna::Binding{8, shadow.getImage(3).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},
        etna::Binding{9, shadow.getImage(4).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},

        etna::Binding{10, skybox.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=6, .type=vk::ImageViewType::eCube})},
        etna::Binding{11, ctx.worldViewMatrices.genBinding()},
        etna::Binding{12, samplingPoints.genBinding()}
      }
    );
    
    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipeline.getVkPipelineLayout(),
      0,
      {set.getVkSet()},
      {}
    );

    pushConstants.pos = ctx.sceneMgr->getLights()[LightSource::Id::Sun].position;
    pushConstants.color = ctx.sceneMgr->getLights()[LightSource::Id::Sun].colorRange;
    pushConstants.pbr = usePbr ? 1 : 0;

    cmd_buf.pushConstants(
      pipeline.getVkPipelineLayout(), 
      vk::ShaderStageFlagBits::eFragment,
      0,
      uint32_t(sizeof(PushConstants)),
      &pushConstants
    );

    cmd_buf.draw(3, 1, 0, 0);
  }

  etna::set_state(
    cmd_buf, 
    diffuse.get(), 
    vk::PipelineStageFlagBits2::eFragmentShader, 
    vk::AccessFlagBits2::eShaderSampledRead, 
    vk::ImageLayout::eShaderReadOnlyOptimal, 
    vk::ImageAspectFlagBits::eColor
  );
}

void
ResolveGBufferPipeline::render(vk::CommandBuffer cmd_buf, targets::GBuffer& source, targets::GBuffer& shadow, uint32_t shadow_cascades, const RenderContext& ctx)
{
  auto& skybox = ctx.sceneMgr->resources()[ctx.sceneMgr->skybox()].image;

  ETNA_PROFILE_GPU(cmd_buf, pipelines_resolvegbuffer_render);
  {
    auto deferredLightShader = etna::get_shader_program("deferred_shader");
    auto& pipeline = deferredLightPipeline;

    cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());

    auto set = etna::create_descriptor_set(
      deferredLightShader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, source.getImage(0).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{1, source.getImage(1).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{2, source.getImage(2).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{3, source.getImage(3).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{4, source.getImage(4).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        
        etna::Binding{5, shadow.getImage(0).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},
        etna::Binding{6, shadow.getImage(1).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},
        etna::Binding{7, shadow.getImage(2).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},
        etna::Binding{8, shadow.getImage(3).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},
        etna::Binding{9, shadow.getImage(4).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=shadow_cascades, .type=vk::ImageViewType::e2DArray})},

        etna::Binding{10, skybox.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.layerCount=6, .type=vk::ImageViewType::eCube})},
        etna::Binding{11, ctx.worldViewMatrices.genBinding()},
        etna::Binding{12, diffuse.genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}

      }
    );
    
    cmd_buf.bindDescriptorSets(
      vk::PipelineBindPoint::eGraphics,
      pipeline.getVkPipelineLayout(),
      0,
      {set.getVkSet()},
      {}
    );

    pushConstants.pos = ctx.sceneMgr->getLights()[LightSource::Id::Sun].position;
    pushConstants.color = ctx.sceneMgr->getLights()[LightSource::Id::Sun].colorRange;
    pushConstants.pbr = usePbr ? 1 : 0;

    cmd_buf.pushConstants(
      pipeline.getVkPipelineLayout(), 
      vk::ShaderStageFlagBits::eFragment,
      0,
      uint32_t(sizeof(PushConstants)),
      &pushConstants
    );

    cmd_buf.draw(3, 1, 0, 0);
  }
  if(secondaryLight)
    renderSphereDeferred(cmd_buf, source, ctx);

  if(secondaryLightSources)
    renderSphere(cmd_buf, ctx);
}


void ResolveGBufferPipeline::renderSphereDeferred(vk::CommandBuffer cmd_buf, targets::GBuffer& source, const RenderContext& ctx)
{
  ETNA_PROFILE_GPU(cmd_buf, renderSphereDeferred);
  auto sphereShader = etna::get_shader_program("sphere_deferred_shader");
  auto& pipeline = sphereDeferredPipeline;

  cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());

  auto set = etna::create_descriptor_set(
      sphereShader.getDescriptorLayoutId(0),
      cmd_buf,
      {
        etna::Binding{0, source.getImage(0).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{1, source.getImage(1).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{2, source.getImage(2).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{3, source.getImage(3).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
        etna::Binding{4, source.getImage(4).genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
      }
    );
    
  cmd_buf.bindDescriptorSets(
    vk::PipelineBindPoint::eGraphics,
    pipeline.getVkPipelineLayout(),
    0,
    {set.getVkSet()},
    {}
  );
  auto& lights = ctx.sceneMgr->getLights();
  for(const auto& light : lights) {
    const float dist = glm::length(glm::vec3(ctx.worldViewProj * glm::vec4(light.position.x, light.position.y, light.position.z, 1)));
    const float fovCorrection = glm::length(glm::vec3(ctx.worldViewProj[0]));
    uint32_t n = static_cast<uint32_t>((5000.f * fovCorrection * light.colorRange.w / dist));
    if (n < 4) {
      continue;
    }
    n = std::min(n, 128u);
    pushConstantsSphere.pos   = light.position;
    pushConstantsSphere.color = light.colorRange;
    pushConstantsSphere.degree = M_PIf / n;
    pushConstantsSphere.pbr = usePbr ? 1 : 0;

    pushConstants.pos += light.floatingAmplitude * glm::sin(light.floatingSpeed * static_cast<float>(ctx.frameTime));
    pushConstants.pos.w = light.position.w;
    cmd_buf.pushConstants(
      pipeline.getVkPipelineLayout(), 
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      uint32_t(sizeof(PushConstantsSphere)),
      &pushConstantsSphere
    );

    cmd_buf.draw(2 * n + 2, n, 0, 0);
  }
}

void ResolveGBufferPipeline::renderSphere(vk::CommandBuffer cmd_buf, const RenderContext& ctx)
{
  auto& pipeline = spherePipeline;

  cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.getVkPipeline());

  auto& lights = ctx.sceneMgr->getLights();
  for(const auto& light : lights) {
    const float dist = glm::length(glm::vec3(ctx.worldViewProj * glm::vec4(light.position.x, light.position.y, light.position.z, 1)));
    const float fovCorrection = glm::length(glm::vec3(ctx.worldViewProj[0]));
    uint32_t n = static_cast<uint32_t>((900.f * fovCorrection * light.visibleRadius / dist));
    if (n == 0) continue;
    n = std::min(n, 128u);
    n = std::max(n,   5u);
    pushConstantsSphere.pos   = light.position;
    pushConstantsSphere.color = light.colorRange;
    pushConstantsSphere.degree = M_PIf / n;

    pushConstantsSphere.pos.w = light.visibleRadius;
    pushConstantsSphere.pos += light.floatingAmplitude * glm::sin(light.floatingSpeed * static_cast<float>(ctx.frameTime));
    cmd_buf.pushConstants(
      pipeline.getVkPipelineLayout(), 
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      0,
      uint32_t(sizeof(PushConstantsSphere)),
      &pushConstantsSphere
    );

    cmd_buf.draw(2 * n + 2, n, 0, 0);
  }
}

} /* namespace pipes */
