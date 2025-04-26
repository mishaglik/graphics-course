#ifndef SCENE_PARTICLESMANAGER_HPP
#define SCENE_PARTICLESMANAGER_HPP

#include "ParticlesEmitter.hpp"

namespace scene {

class ParticlesManager {
public:
    void allocate();
        
    void loadTextures() {}

    void drawGui();

    void addEmitter(ParticlesEmitter emitter) {
        m_emitters.emplace_back(std::move(emitter));
    }

    void update(float time);

    etna::BufferBinding genBinding() { return m_particles.genBinding(); }

    std::size_t size() const { return m_emitters.size(); }

    auto begin() { return m_emitters.begin(); }
    auto end()   { return m_emitters.end();   }
private:
    void reserve(std::size_t n);

private:
    std::vector<ParticlesEmitter> m_emitters;
    etna::Buffer m_particles;
    std::size_t m_capacity = 0;
};

}

#endif /* SCENE_PARTICLESMANAGER_HPP */
