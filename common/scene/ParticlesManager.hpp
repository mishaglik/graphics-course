#ifndef SCENE_PARTICLESMANAGER_HPP
#define SCENE_PARTICLESMANAGER_HPP

#include "ParticlesEmitter.hpp"
#include "scene/ResourceManager.hpp"

namespace scene {

class ParticlesManager {
public:
    explicit ParticlesManager(ResourceManager& resources) : m_resources(resources) {}
    void allocate();
        
    void loadTextures() {}

    void drawGui();

    void addEmitter(ParticlesEmitter emitter) {
        m_emitters.emplace_back(std::move(emitter));
    }

    void update(glm::vec4 z_view, float time);

    

    std::size_t size() const { return m_emitters.size(); }

    auto begin() { return m_emitters.begin(); }
    auto end()   { return m_emitters.end();   }
private:
    void reserve(std::size_t n);

private:
    ResourceManager& m_resources;
    std::vector<ParticlesEmitter> m_emitters;
    EmitterInfo m_newEmitterInfo;
};

}

#endif /* SCENE_PARTICLESMANAGER_HPP */
