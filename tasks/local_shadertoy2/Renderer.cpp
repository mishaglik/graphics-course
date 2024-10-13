#include "Renderer.hpp"
#include "etna/BlockingTransferHelper.hpp"
#include "etna/Etna.hpp"
#include "etna/GlobalContext.hpp"
#include "etna/PipelineManager.hpp"
#include "etna/RenderTargetStates.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const vk::Format CUBEMAP_FORMAT = vk::Format::eR8G8B8A8Unorm;

void
Renderer::initPipelines(glm::uvec2 res, vk::Format swapchain_format)
{
    resolution = res;
    auto& ctxt = etna::get_context();

    constants = ctxt.createBuffer(etna::Buffer::CreateInfo{
        .size = sizeof(int),
        .bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer,
        .memoryUsage = VMA_MEMORY_USAGE_CPU_ONLY,
        .name = "constants",
    });

    constants.map();

    skybox = ctxt.createImage({
        .extent = {2048, 2048, 1},
        .name = "skybox",
        .format = CUBEMAP_FORMAT,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
        .layers = 6,
        .flags = vk::ImageCreateFlagBits::eCubeCompatible,
    });

    sbSampler = etna::Sampler({
        .filter = vk::Filter::eLinear,
        .name = "sbSampler",
    });

    defaultSampler = etna::Sampler({
        .filter = vk::Filter::eNearest,
        .addressMode = vk::SamplerAddressMode::eRepeat,
        .name = "defaultSampler",
    });

    basePipeline = ctxt.getPipelineManager().createGraphicsPipeline(
        "toy_shader", 
        {
            .fragmentShaderOutput = {
                .colorAttachmentFormats = {swapchain_format}
            },
        }
    );

    skyboxPipeline = ctxt.getPipelineManager().createGraphicsPipeline(
        "skybox_shader",
        {
            .fragmentShaderOutput = {
                .colorAttachmentFormats = {CUBEMAP_FORMAT}
            }
        }  
    );
}


void 
Renderer::render( vk::CommandBuffer cmd_buf, 
             vk::Image target_image, 
             vk::ImageView target_image_view,
             size_t push_constants_size,
             void* push_constants
             )
{
    // skybox image render
    {
        etna::set_state(cmd_buf, 
            skybox.get(), 
            vk::PipelineStageFlagBits2::eFragmentShader, 
            {}, 
            vk::ImageLayout::eGeneral, 
            vk::ImageAspectFlagBits::eColor,
            6
        );
        etna::flush_barriers(cmd_buf);


        auto sbShader = etna::get_shader_program("skybox_shader");

        auto set = etna::create_descriptor_set(
            sbShader.getDescriptorLayoutId(0),
            cmd_buf,
            {
                etna::Binding{0, textures[0].genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
                etna::Binding{1, textures[1].genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
                etna::Binding{2, textures[2].genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
            });
        
        for (uint32_t i = 0; i < 6; ++ i) {
            etna::RenderTargetState renderTargets(
                cmd_buf,
                {{0, 0}, {2048, 2048}
                },
                {{.image = skybox.get(), .view = skybox.getView({
                    .layerCount = 1, 
                    .baseArrayLayer = i,
                    .type = vk::ImageViewType::e2D, 
                    })}},
                {},
                1
                );
            
            cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, skyboxPipeline.getVkPipeline());
            cmd_buf.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                skyboxPipeline.getVkPipelineLayout(),
                0,
                {set.getVkSet()},
                {});

            //TODO: Push layer id to shader
            cmd_buf.pushConstants(
                skyboxPipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex, 0, push_constants_size, push_constants);
            cmd_buf.draw(3, 1, 0, 0);       
        }
    }

    

    // main image render
    {
        auto toyShader = etna::get_shader_program("toy_shader");

        auto set = etna::create_descriptor_set(
            toyShader.getDescriptorLayoutId(0),
            cmd_buf,
            {
                etna::Binding
                {   
                    0, 
                    etna::ImageBinding
                    {
                        skybox,
                        vk::DescriptorImageInfo
                        {
                            sbSampler.get(), 
                            skybox.getView({
                                .layerCount = 6, 
                                .type = vk::ImageViewType::eCube, 
                            }),
                            vk::ImageLayout::eGeneral
                        }
                    }
                },
                //FIXME: Unhardcode
                etna::Binding{1, textures[3].genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
                etna::Binding{2, textures[4].genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
                etna::Binding{3, textures[2].genBinding(defaultSampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
            });


        etna::RenderTargetState renderTargets(
            cmd_buf,
            {{0, 0}, {resolution.x, resolution.y}
            },
            {{.image = target_image, .view = target_image_view}},
            {}
            );


        cmd_buf.bindPipeline(vk::PipelineBindPoint::eGraphics, basePipeline.getVkPipeline());
        cmd_buf.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            basePipeline.getVkPipelineLayout(),
            0,
            {set.getVkSet()},
            {});

        cmd_buf.pushConstants(
            basePipeline.getVkPipelineLayout(), vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex, 0, push_constants_size, push_constants);
        
        cmd_buf.draw(3, 1, 0, 0);        
    }
}

void
Renderer::loadResource(etna::OneShotCmdMgr& cmd_mgr, const char* name)
{
    int width, height, nChans;
    auto* imageBytes = stbi_load(name, &width, &height, &nChans, STBI_rgb_alpha);
    if (imageBytes == nullptr)
    {
      spdlog::log(spdlog::level::err, "Image load is unsucessufll");
      return;
    }

    size_t size = width * height * 4;

    auto& ctx = etna::get_context();
    auto buf = ctx.createBuffer({
        .size = static_cast<vk::DeviceSize>(size),
        .bufferUsage = vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst,
        .name = "load buf",
    });

    etna::BlockingTransferHelper transferHelper({
        .stagingSize = size,
    });

    transferHelper.uploadBuffer(
    cmd_mgr, buf, 0, std::span<const std::byte>((std::byte*)imageBytes, size));

    auto img = ctx.createImage({
        .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
        .name = name,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
    });

    auto cmdBuf = cmd_mgr.start();
    ETNA_CHECK_VK_RESULT(cmdBuf.begin(vk::CommandBufferBeginInfo{}));
    {
        etna::set_state(
            cmdBuf,
            img.get(),
            // We are going to use the texture at the transfer stage...
            vk::PipelineStageFlagBits2::eTransfer,
            // ...to transfer-read stuff from it...
            vk::AccessFlagBits2::eTransferRead,
            // ...and want it to have the appropriate layout.
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageAspectFlagBits::eColor
        );
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
            // ...to transfer-read stuff from it...
            vk::AccessFlagBits2::eShaderRead,
            // ...and want it to have the appropriate layout.
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::ImageAspectFlagBits::eColor
        );

        etna::flush_barriers(cmdBuf);
    
    }
    ETNA_CHECK_VK_RESULT(cmdBuf.end());
    cmd_mgr.submitAndWait(std::move(cmdBuf));

    textures.emplace_back(std::move(img));
}
