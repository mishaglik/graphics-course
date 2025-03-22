#pragma once

#include <filesystem>

#include "SingleResourceManager.hpp"
#include "Material.hpp"
#include "Texture.hpp"

namespace scene {

class ResourceManger {
public:
    void init(); 

    const auto& operator[](Material   ::Id id) const { return m_materials[id]; }
    const auto& operator[](Texture    ::Id id) const { return m_textures [id]; }
  
    const auto& get(Material   ::Id id) { return m_materials[id]; }
    const auto& get(Texture    ::Id id) { return m_textures [id]; }


    template< class... Args >
    Material::Id emplaceMaterial( Args&&... args ) {
        m_materials.emplace(std::forward<Args>(args)...);
        return static_cast<Material::Id>(m_materials.size()-1);
    }

    template< class... Args >
    Texture::Id emplaceTexture( Args&&... args ) {
        m_textures.emplace(std::forward<Args>(args)...);
        return static_cast<Texture::Id>(m_textures.size()-1);
    }

    Texture::Id primitiveTexture(uint8_t rgba); 

    Texture::Id loadFromFile(std::filesystem::path path);

private:
    SingleResourceManager<Material> m_materials;
    SingleResourceManager<Texture > m_textures;

    std::array<Texture::Id, 16> m_colorTextures;
};

}