#include "TerrainManager.hpp"

#include <imgui.h>

namespace scene {

void 
TerrainManager::loadTextures()
{
    m_textures[0] = m_resources.loadFromFile(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "grass.jpg" );
    m_textures[1] = m_resources.loadFromFile(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "sand.jpg"  );
    m_textures[2] = m_resources.loadFromFile(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "snow.jpg"  );
    m_textures[3] = m_resources.loadFromFile(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "rock.jpg"  );
    m_textures[4] = m_resources.loadFromFile(GRAPHICS_COURSE_RESOURCES_ROOT "/textures/terrain/" "ground.jpg");
    m_textures[5] = Texture::Id::Undefined;

    m_sampler = etna::Sampler({
        .filter = vk::Filter::eNearest,
        .addressMode = vk::SamplerAddressMode::eRepeat,
        .name = "tilingSampler",
      });
}

void 
TerrainManager::drawGui()
{
    if(ImGui::SliderInt("Terrain scale", &m_terrainScale, 1, 32)) {
        terrainValid = true;
    }
    
    if(ImGui::SliderFloat("Frequency", &startFrequency, 0, 1)) {
        terrainValid = false;
    }
    
    if(ImGui::Button("Regenerate")) {
        terrainValid = false;
    }
    
    ImGui::SliderFloat("Max height", &m_maxHeight, 1, 100);
    ImGui::SliderFloat("Sea level", &m_seaLevel, 0, m_maxHeight);
    ImGui::SliderInt("Active layers", &m_activeLayers, 1, N_CLIP_LEVELS);

}

void 
TerrainManager::allocate() {
    for(std::size_t i = 0; i < N_CLIP_LEVELS; ++i) {
      auto& level = m_levels[i];
      level.allocate({32<<i, 32<<i}, heightMapResolution);
    }
    m_water.allocate({256, 256});
}

etna::DescriptorSet 
TerrainManager::textureSet(vk::CommandBuffer cmd_buf, etna::DescriptorLayoutId dl_id)
{
    return etna::create_descriptor_set(
        dl_id,
        cmd_buf,
        {
            etna::Binding{0, m_resources[m_textures[0]].image.genBinding(m_sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
            etna::Binding{1, m_resources[m_textures[1]].image.genBinding(m_sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
            etna::Binding{2, m_resources[m_textures[2]].image.genBinding(m_sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
            etna::Binding{3, m_resources[m_textures[3]].image.genBinding(m_sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
            etna::Binding{4, m_resources[m_textures[4]].image.genBinding(m_sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)},
            etna::Binding{5, m_resources[m_textures[5]].image.genBinding(m_sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal)}
        }
    );
}

}