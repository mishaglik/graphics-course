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

    void addEmitter(ParticlesEmitter::EmitterCreateInfo emitter) {
        m_emitters.emplace_back(std::move(emitter));
        m_emitters.back().allocate();
    }

    void update();

    const ParticlesEmitter& operator[](std::size_t i) { return m_emitters[i]; }

    std::size_t size() const { return m_emitters.size(); }

    auto begin() { return m_emitters.begin(); }
    auto end()   { return m_emitters.end();   }

    const etna::Buffer& emitterGPUData() const { return m_gpuData; }

private:
    void reserve(std::size_t n);

private:
    etna::Buffer m_gpuData;
    ResourceManager& m_resources;
    std::vector<ParticlesEmitter> m_emitters;
    ParticlesEmitter::EmitterCreateInfo m_newEmitterInfo;
};

}

#endif /* SCENE_PARTICLESMANAGER_HPP */
