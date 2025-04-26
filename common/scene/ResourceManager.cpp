#include "ResourceManager.hpp"
#include "etna/BlockingTransferHelper.hpp"
#include "etna/Etna.hpp"
#include "etna/GlobalContext.hpp"
#include "etna/OneShotCmdMgr.hpp"
#include "etna/RenderTargetStates.hpp"

#include "stb_image.h"


namespace scene {

void ResourceManager::init() {
    m_sampler = etna::Sampler({
      .name = "Default",
    });  

    
    Texture::Id undefinedTex = loadFromFile(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/undefined.png");
    if(undefinedTex != Texture::Id::Undefined) {
        spdlog::log(spdlog::level::critical, "Undefined texture has bad Id");
        std::terminate();
    }

    auto cmdMgr = etna::get_context().createOneShotCmdMgr();
    auto cmdBuf = cmdMgr->start();
    ETNA_CHECK_VK_RESULT(cmdBuf.begin(vk::CommandBufferBeginInfo{}));
    for(size_t i = 0; i < 16; i++) {
        etna::Image tex = etna::get_context().createImage({
            .extent = {1, 1, 1},
            .name = "stub" + std::to_string(i),
            .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
            .type = vk::ImageType::e2D,
        });
        
        etna::RenderTargetState renderTargets(
            cmdBuf,
            {{0, 0}, {1, 1}},
            {{.image=tex.get(), .view=tex.getView({}), .clearColorValue=
                {
                    (i & 0x1) != 0 ? 1.f : 0.f, 
                    (i & 0x2) != 0 ? 1.f : 0.f, 
                    (i & 0x4) != 0 ? 1.f : 0.f, 
                    (i & 0x8) != 0 ? 1.f : 0.f
                }
            }},
            {}
        );
        etna::set_state(
            cmdBuf, 
            tex.get(), 
            vk::PipelineStageFlagBits2::eAllCommands, 
            vk::AccessFlagBits2::eShaderSampledRead, 
            vk::ImageLayout::eShaderReadOnlyOptimal, 
            vk::ImageAspectFlagBits::eColor
        );
        m_colorTextures[i] = emplaceTexture(std::move(tex));
    }    
    ETNA_CHECK_VK_RESULT(cmdBuf.end());
    cmdMgr->submitAndWait(cmdBuf);
    m_materialsBuffer = etna::get_context().createBuffer({
      .size = uint32_t(N_MAX_MATERIALS * sizeof(GpuMaterial)),
      .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
      .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
      .name = "materials",
    });
    m_materialsBuffer.map();


    auto undefinedMaterial = emplaceMaterial(Material{
        .baseColorTexture = Texture::Id::Undefined,
        .metallicRoughnessTexture = primitiveTexture(0x0),
        .emissiveFactorTexture    = primitiveTexture(0x0),
    });

    if (undefinedMaterial != Material::Id::Undefined) {
        spdlog::log(spdlog::level::critical, "Undefined material has bad Id");
        std::terminate();
    }
}

//TODO: Remove CPU material storage at all. 
void 
ResourceManager::copyLastMaterial() {

}

Texture::Id 
ResourceManager::primitiveTexture(uint8_t rgba)
{
    rgba &= 0xF;
    return m_colorTextures[rgba];
}

Texture::Id 
ResourceManager::createSingleColorTexture(glm::vec4 color)
{    
  auto cmdMgr = etna::get_context().createOneShotCmdMgr();
  auto cmdBuf = cmdMgr->start();
  Texture::Id id = Texture::Id::Invalid;
  ETNA_CHECK_VK_RESULT(cmdBuf.begin(vk::CommandBufferBeginInfo{}));
  {
    etna::Image tex = etna::get_context().createImage({
      .extent = {1, 1, 1},
      .name = "stub",
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
      .type = vk::ImageType::e2D,
    });

    etna::RenderTargetState renderTargets(
        cmdBuf,
        {{0, 0}, {1, 1}},
        {{.image=tex.get(), .view=tex.getView({}), .clearColorValue={color.r, color.g, color.b, color.a}}},
        {}
    );
    etna::set_state(
        cmdBuf, 
        tex.get(), 
        vk::PipelineStageFlagBits2::eAllCommands, 
        vk::AccessFlagBits2::eShaderSampledRead, 
        vk::ImageLayout::eShaderReadOnlyOptimal, 
        vk::ImageAspectFlagBits::eColor
    );
    id = emplaceTexture(std::move(tex));
  }
  ETNA_CHECK_VK_RESULT(cmdBuf.end());
  cmdMgr->submitAndWait(cmdBuf);
  return id;
}    

Texture::Id 
ResourceManager::loadFromFile(std::filesystem::path filepath)
{
  #if 0
    if(m_textures.size() > 0) {
      return Texture::Id::Undefined;
    }
  #endif
    auto& ctx = etna::get_context();
    int width, height, nChans;
    auto uri = filepath.filename().generic_string<char>();
    auto* imageBytes =
      stbi_load(filepath.generic_string<char>().c_str(), &width, &height, &nChans, STBI_rgb_alpha);
    
    if (imageBytes == nullptr)
    {
      spdlog::log(spdlog::level::err, "Image \"{}\" load is unsuccessful", uri);
      return Texture::Id::Invalid;
    }
    size_t size = static_cast<std::size_t>(width * height * 4);

    auto buf = ctx.createBuffer({
      .size = static_cast<vk::DeviceSize>(size),
      .bufferUsage = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
      .name = "tmp load buf",
    });

    etna::BlockingTransferHelper transferHelper({
      .stagingSize = size,
    });

    auto cmdMgr = ctx.createOneShotCmdMgr();
    transferHelper.uploadBuffer(
      *cmdMgr, buf, 0, std::span<const std::byte>((std::byte*)imageBytes, size));

    auto img = ctx.createImage({
      .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
      .name = uri,
      .format = vk::Format::eR8G8B8A8Unorm,
      .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
        vk::ImageUsageFlagBits::eTransferDst,
    });

    auto cmdBuf = cmdMgr->start();
    ETNA_CHECK_VK_RESULT(cmdBuf.begin(vk::CommandBufferBeginInfo{}));
    {
      etna::set_state(
        cmdBuf,
        img.get(),
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageAspectFlagBits::eColor);
      etna::flush_barriers(cmdBuf);


      vk::BufferImageCopy bic[1]{};
      bic[0].setImageExtent({static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1});
      bic[0].setImageOffset({});
      bic[0].setImageSubresource({
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .layerCount = 1,
      });


      cmdBuf.copyBufferToImage(buf.get(), img.get(), vk::ImageLayout::eTransferDstOptimal, bic);

      etna::set_state(
        cmdBuf,
        img.get(),
        vk::PipelineStageFlagBits2::eFragmentShader,
        vk::AccessFlagBits2::eShaderRead,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor);

      etna::flush_barriers(cmdBuf);
    }
    ETNA_CHECK_VK_RESULT(cmdBuf.end());
    cmdMgr->submitAndWait(std::move(cmdBuf));
    spdlog::info("New texture: {} {{", m_textures.size());
    spdlog::info("    .name={}", uri);
    spdlog::info("}}");

    return emplaceTexture(std::move(img));
}

void 
ResourceManager::finalize() {
  auto staticMesh = etna::get_shader_program("staticmesh_shader");
  if(m_materials.size() > N_MAX_MATERIALS) {
    spdlog::error("Resource manager: m_materials.size() > N_MAX_MATERIALS");
    return;
  }
  
  for(size_t i = 0; i < std::min(std::size_t(N_MAX_MATERIALS), m_materials.size()); ++i) {
    GpuMaterial* gmat = gpuMaterial() + i;
    Material& cmat = m_materials.get(static_cast<Material::Id>(i));
    
    gmat->baseColorTexture         = static_cast<glm::uint>(cmat.baseColorTexture        );
    gmat->normalTexture            = static_cast<glm::uint>(cmat.normalTexture           );
    gmat->metallicRoughnessTexture = static_cast<glm::uint>(cmat.metallicRoughnessTexture);
    gmat->emissiveFactorTexture    = static_cast<glm::uint>(cmat.emissiveFactorTexture   );
    
    gmat->baseColor  = cmat.baseColor;
    gmat->emr_factor = cmat.EMR_Factor;
  }

  m_bindings.emplace_back(etna::Binding{0, m_materialsBuffer.genBinding()});
  m_set = etna::create_persistent_descriptor_set(
    staticMesh.getDescriptorLayoutId(1),
    m_bindings,
    true
  );

}

}