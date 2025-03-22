#pragma once

#include "ResourceManager.hpp"
#include "Cliplevel.hpp"


namespace scene {

class TerrainManager {
public:
    explicit TerrainManager(ResourceManager& resource_manager) : m_resources(resource_manager) {}

    float seaLevel () const {return m_seaLevel; }
    float maxHeight() const {return m_maxHeight;}

    float frequency () const {return startFrequency; }
    int terrainScale () const {return m_terrainScale; }

    bool isUpToDate() const { return terrainValid; } 
    
    std::span<terrain::Cliplevel> levels() { return std::span<terrain::Cliplevel>{m_levels.data(), static_cast<std::size_t>(m_activeLayers)}; }
    
    void allocate();
    
    void loadTextures();
    
    void drawGui();

    etna::DescriptorSet textureSet(vk::CommandBuffer cmd_buf, etna::DescriptorLayoutId dl_id);

    void setValid(bool valid) { terrainValid = valid; }

private:
    ResourceManager& m_resources;

    etna::Sampler m_sampler;
    std::array<Texture::Id, 6> m_textures;
    // std::array<Material::Id, 6> m_materials; //TODO: 

    float m_seaLevel = 14.f;
    float m_maxHeight = 64.f;
        
    static const std::size_t N_CLIP_LEVELS = 5;
    std::array<terrain::Cliplevel, N_CLIP_LEVELS> m_levels;

    int m_terrainScale = 7;
    int m_activeLayers = 3;

    float startFrequency = 0.003f;

    
    bool terrainValid = false;

    uint64_t heightMapResolution = 256;
    static const uint64_t MAX_TESCELLATION = 64; //TODO: Use vulkan info
};
}