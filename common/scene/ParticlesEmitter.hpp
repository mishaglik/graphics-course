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

    //NOTE: IMPORTANT: When changing this struct change next structs accordingly!
    //LINK common/shaders/particleInfo.hpp:18
    //LINK common/shaders/particleInfo.hpp:33
    //LINK common/scene/ParticlesEmitter.cpp:72
    struct EmitterCreateInfo {
        ParticleType type;
        Material::Id material;
        glm::vec4  position;
        glm::vec2  size;
        glm::vec4  fadeColor = glm::vec4{1.f, 1.f, 1.f, 1.f};
        float fadeSize = 1.f;
        float spawnRate = 1.f;
        float lifetime  = 5.f;
        float maxSpeed  = 10.f;
        glm::vec3 direction = glm::vec3{0.f, 1.f, 0.f};
        float directionFactor   = 0.f;
        float speedRandomFactor = 1.f;
        float spawnRadius       = 0.f;
        float fadeBezier[4]     = {};
    };

    explicit ParticlesEmitter(EmitterCreateInfo info) : m_info(info) { std::memcpy(m_bezier, m_info.fadeBezier, 4 * sizeof(float)); }

    void allocate();

    void update(EmitterInfo* emitter_info, EmitterSpawnInfo* emitter_spawn_info);

    void drawGui();

    bool deleted() const { return m_deleted; }

    etna::BufferBinding gpuBuf() const { return m_gpuData.genBinding(); }
private:
    void emitParticle();
    bool invalidate() { m_actual = false; return false; }
private:
    EmitterCreateInfo m_info;
    bool m_actual = false;
    bool m_deleted = false;
    float m_bezier[5] = {0.f, 0.f, 1.f, 1.f};
    etna::Buffer m_gpuData;
};

const char* to_string(ParticlesEmitter::ParticleType type);
}

#endif /* SCENE_PARTICLESEMITTER_HPP */
