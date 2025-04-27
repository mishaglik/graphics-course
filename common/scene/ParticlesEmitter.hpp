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
        WorldBoard,
        ScreenBoard,
        Box,
        N_EMITTERS,
    };

    struct SpawnerParams {
        float rate        = 1.f;
        float maxSpeed    = 10.f;
        glm::vec3 direction = glm::vec3{0.f, 1.f, 0.f};
        float directionFactor   = 1.f;
        float speedRandomFactor = 1.f;
        float spawnRadius       = 0.f;
        float lifetime          = 5.f;
    };

    ParticlesEmitter(ParticleType type, glm::vec4 pos, Material::Id material, SpawnerParams params) : m_info{pos, glm::vec2{1, 1}, uint32_t(type), uint32_t(material), {1, 1, 1, 1}, {1.f, 1.f, 1.f, 1.f}, {}}, m_params(params) {}
    ParticlesEmitter(EmitterInfo info, SpawnerParams params) : m_info{info}, m_params{params} { m_bezier[0] = m_info.fadeBezier[0]; m_bezier[1] = m_info.fadeBezier[1]; m_bezier[2] = m_info.fadeBezier[2]; m_bezier[3] = m_info.fadeBezier[3];}
    explicit ParticlesEmitter(EmitterInfo info) : scene::ParticlesEmitter{info, {}} {}

    ParticlesEmitter& setSize(glm::vec2 size) { m_info.size = size; return *this;}

    SpawnerParams& spawnerParams() { return m_params; }

    void update(glm::vec4 z_view, float time);

    void drawGui();

    bool operator<(const ParticlesEmitter& oth) const { return m_camZ < oth.m_camZ; }
    
    struct ParticleCpuInfo {
        glm::vec4 position;
        glm::vec4 velocity;
        float birthtime;
    };

    std::size_t size() const { return m_particles.size(); }
    const ParticleCpuInfo* data() const { return m_particles.data(); }
    auto begin() { return m_particles.begin(); }
    auto end  () { return m_particles.end  (); }

    const EmitterInfo& info() const { return m_info; }

    uint32_t verticesPerParticle() const;

private:
    void emitParticle();
private:
    EmitterInfo m_info;
    SpawnerParams m_params;
    float m_prevTime = 0.f;
    float m_lastSpawnTime = 0.f;
    float m_camZ = 0.f; 
    float m_bezier[5] = {0.f, 0.f, 1.f, 1.f};
    std::vector<ParticleCpuInfo> m_particles;
};
const char* to_string(ParticlesEmitter::ParticleType type);
}

#endif /* SCENE_PARTICLESEMITTER_HPP */
