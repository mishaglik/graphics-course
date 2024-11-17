#pragma once

#include "etna/Buffer.hpp"
#include "etna/OneShotCmdMgr.hpp"
#include <etna/GraphicsPipeline.hpp>
#include <etna/Image.hpp>
#include <etna/Sampler.hpp>
#include <glm/glm.hpp>

class Renderer
{
public: 
    Renderer() {}
    ~Renderer() {}

    void initPipelines(glm::uvec2 resolution, vk::Format swapchain_format);

    void loadResource(etna::OneShotCmdMgr& cmd_mgr, const char* name);

    void render(
    vk::CommandBuffer cmd_buf, vk::Image target_image, vk::ImageView target_image_view, uint32_t constants_size, void* constants);

private:
    std::vector<etna::Image> textures;
    etna::Sampler defaultSampler;
    
    etna::Buffer constants;
    etna::GraphicsPipeline basePipeline;

    etna::Image skybox;
    etna::Sampler sbSampler;
    etna::GraphicsPipeline skyboxPipeline;

    void initView();

    glm::uvec2 resolution;
};