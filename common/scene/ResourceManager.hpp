#pragma once

#include <filesystem>

#include "SingleResourceManager.hpp"
#include "Material.hpp"
#include "Texture.hpp"
#include "etna/Buffer.hpp"
#include "etna/DescriptorSet.hpp"
#include "etna/Sampler.hpp"
#include "shaders/material.hpp"

namespace scene {

class ResourceManager {
public:
    void init(); 

    const auto& operator[](Material   ::Id id) const { return m_materials[id]; }
    const auto& operator[](Texture    ::Id id) const { return m_textures [id]; }
  
    const auto& get(Material   ::Id id) { return m_materials[id]; }
    const auto& get(Texture    ::Id id) { return m_textures [id]; }

    std::size_t texturesSize() const { return m_textures.size(); }

    template< class... Args >
    Material::Id emplaceMaterial( Args&&... args ) {
        m_materials.emplace(std::forward<Args>(args)...);
        return static_cast<Material::Id>(m_materials.size()-1);
    }

    template< class... Args >
    Texture::Id emplaceTexture( Args&&... args ) {
        m_textures.emplace(std::forward<Args>(args)...);
        auto id = static_cast<Texture::Id>(m_textures.size()-1);
        m_bindings       .emplace_back(etna::Binding{1, m_textures[id].image.genBinding(m_sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {0, vk::RemainingMipLevels, 0, 1}), uint32_t(id)});
        m_bindingsArrayed.emplace_back(etna::Binding{1, m_textures[id].image.genBinding(m_sampler.get(), vk::ImageLayout::eShaderReadOnlyOptimal, {.type=vk::ImageViewType::e2DArray}), uint32_t(id)});
        
        return id;
    }

    Texture::Id primitiveTexture(uint8_t rgba); 
    
    Texture::Id createSingleColorTexture(glm::vec4 color); 

    Texture::Id loadFromFile(std::filesystem::path path);
    Texture::Id loadArrayedFromFile(std::filesystem::path path);

    vk::DescriptorSet getSet() { return m_set.getVkSet(); }
    vk::DescriptorSet getSetArrayed() { return m_setArrayed.getVkSet(); }

    void finalize();

    Material::Id validateMaterial(uint32_t id) { return id < (uint32_t)m_materials.size() ? Material::Id{id} : Material::Id::Invalid; }

private:
    GpuMaterial* gpuMaterial() {return reinterpret_cast<GpuMaterial*>(m_materialsBuffer.data()); }
    void copyLastMaterial();
private:
    SingleResourceManager<Material> m_materials;
    SingleResourceManager<Texture > m_textures;
    
    std::array<Texture::Id, 16> m_colorTextures;
    
    etna::Buffer m_materialsBuffer;
    etna::Sampler m_sampler;
    std::vector<etna::Binding> m_bindings;
    std::vector<etna::Binding> m_bindingsArrayed;

    etna::PersistentDescriptorSet m_set;
    etna::PersistentDescriptorSet m_setArrayed;
};

}