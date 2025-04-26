#ifndef SCENE_PARTICLESEMITTER_HPP
#define SCENE_PARTICLESEMITTER_HPP

#include <glm/glm.hpp>

#include "etna/Buffer.hpp"

#include "Material.hpp"
#include "shaders/particleInfo.hpp"


namespace scene {

class ParticlesEmitter {
public:
    enum class ParticleType : uint32_t{
        Invalid   = ~uint32_t{0},
        Undefined = uint32_t{0},
        Board,
        Sphere,
        N_EMITTERS,
    };

    struct SpawnerParams {
        float spawnRate      = 1.f;
        float spawnVelocity  = 10.f;
        float spawnRadius    = 0.f; 
        float spawnLifetime  = 5.f;
    };

    ParticlesEmitter(ParticleType type, glm::vec4 pos, Material::Id material = Material::Id::Undefined, SpawnerParams params = SpawnerParams{10.f, 1.f, 0.f, 1e6f}) : m_info{pos, uint32_t(type), uint32_t(material)}, m_params(params) {}

    
    void update(float time);

    void drawGui();
    
    bool operator<(const ParticlesEmitter& oth) const { return m_info.position.z < oth.m_info.position.z; }
    
    struct ParticleCpuInfo {
        glm::vec4 position;
        glm::vec4 velocity;
        float lifetime;
    };

    std::size_t size() const { return m_particles.size(); }
    const ParticleCpuInfo* data() const { return m_particles.data(); }
    auto begin() { return m_particles.begin(); }
    auto end  () { return m_particles.end  (); }

    const EmitterInfo& info() const { return m_info; }
private:
    EmitterInfo m_info;
    SpawnerParams m_params;
    float m_prevTime = 0;
    float m_lastSpawnTime = 0;


    std::vector<ParticleCpuInfo> m_particles;
};

}

#endif /* SCENE_PARTICLESEMITTER_HPP */
