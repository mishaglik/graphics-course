#pragma once
#include "etna/GlobalContext.hpp"
#include "etna/Image.hpp"
#include "etna/RenderTargetStates.hpp"
#include <glm/glm.hpp>
namespace targets {

template<vk::Format FORMAT, vk::ImageUsageFlagBits... flags>
class Buffer {
public:
    static const std::vector<vk::Format> COLOR_ATTACHMENT_FORMATS;// = {vk::Format::eB10G11R11UfloatPack32};
    static const vk::Format              DEPTH_ATTACHMENT_FORMAT  = vk::Format::eUndefined;
    static const constexpr int N_COLOR_ATTACHMENTS = 1;

    const std::vector<etna::RenderTargetState::AttachmentParams>& getColorAttachments() { return colorAttachmentParams; }
    
    etna::RenderTargetState::AttachmentParams getDepthAttachment() { return {}; }
    
    glm::uvec2 getResolution() { return resolution; }

    void allocate(glm::uvec2 extent)
    {
        resolution = extent;
        auto& ctx = etna::get_context();
    
        color_buffer = ctx.createImage(etna::Image::CreateInfo{
            .extent = vk::Extent3D{resolution.x, resolution.y, 1},
        #if defined (__GNUG__)
            .name = __PRETTY_FUNCTION__,
        #else
            .name = "targets::Buffer",
        #endif
            .format = COLOR_ATTACHMENT_FORMATS[0],
            .imageUsage = (flags | ... | vk::ImageUsageFlagBits::eColorAttachment),
        });

        colorAttachmentParams = {
            {
                .image = color_buffer.get(),
                .view = color_buffer.getView({}),
            }
        };
    }

    vk::Image get() { return color_buffer.get(); }

    etna::ImageBinding genBinding(vk::Sampler sampler, vk::ImageLayout layout) const {return color_buffer.genBinding(sampler, layout);}

private:
    etna::Image color_buffer;
    glm::uvec2 resolution{};

    std::vector<etna::RenderTargetState::AttachmentParams> colorAttachmentParams;
};

template<vk::Format FORMAT, vk::ImageUsageFlagBits... flags>
const std::vector<vk::Format> Buffer<FORMAT, flags...>::COLOR_ATTACHMENT_FORMATS = {FORMAT};

using WaterChunk = Buffer<vk::Format::eR16G16Sfloat, vk::ImageUsageFlagBits::eSampled, vk::ImageUsageFlagBits::eStorage>;
using FogBuffer = Buffer<vk::Format::eB10G11R11UfloatPack32, vk::ImageUsageFlagBits::eSampled>;

template<vk::Format FORMAT, vk::ImageUsageFlagBits... flags>
class BufferDepth {
public:
    static const std::vector<vk::Format> COLOR_ATTACHMENT_FORMATS;
    static const vk::Format              DEPTH_ATTACHMENT_FORMAT  = FORMAT;
    static const constexpr int N_COLOR_ATTACHMENTS = 0;

    const std::vector<etna::RenderTargetState::AttachmentParams> getColorAttachments() { return {}; }
    
    etna::RenderTargetState::AttachmentParams getDepthAttachment() { return depthAttachmentParams; }
    
    glm::uvec2 getResolution() { return resolution; }

    void allocate(glm::uvec2 extent)
    {
        resolution = extent;
        auto& ctx = etna::get_context();
    
        depth_buffer = ctx.createImage(etna::Image::CreateInfo{
            .extent = vk::Extent3D{resolution.x, resolution.y, 1},
        #if defined(__PRETTY_FUNCTION__)
            .name = __PRETTY_FUNCTION__,
        #else
            .name = "targets::BufferDepth",
        #endif
            .format = DEPTH_ATTACHMENT_FORMAT,
            .imageUsage = (flags | ... | vk::ImageUsageFlagBits::eDepthStencilAttachment),
        });

        depthAttachmentParams = {
            .image = depth_buffer.get(),
            .view  = depth_buffer.getView({}),
            .imageAspect = vk::ImageAspectFlagBits::eDepth,
        };
    }

    vk::Image get() { return depth_buffer.get(); }

    etna::ImageBinding genBinding(vk::Sampler sampler, vk::ImageLayout layout) {return depth_buffer.genBinding(sampler, layout);}

private:
    etna::Image depth_buffer;
    glm::uvec2 resolution{};

    etna::RenderTargetState::AttachmentParams depthAttachmentParams;
};

template<vk::Format FORMAT, vk::ImageUsageFlagBits... flags>
const std::vector<vk::Format> BufferDepth<FORMAT, flags...>::COLOR_ATTACHMENT_FORMATS = {};

using SimpleShadowMap = BufferDepth<vk::Format::eD16Unorm, vk::ImageUsageFlagBits::eSampled, vk::ImageUsageFlagBits::eTransferDst>;

}