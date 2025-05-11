#include "ParticlesEmitter.hpp"
#include "etna/GlobalContext.hpp"
#include "imgui.h"
#include "gui/Bezier.hpp"

#include <algorithm>



namespace scene {

const char* to_string(ParticlesEmitter::ParticleType type) {
    switch (type) {
    
    case ParticlesEmitter::ParticleType::Undefined:     return "Undefined";
    case ParticlesEmitter::ParticleType::Board:         return "Board";
    case ParticlesEmitter::ParticleType::ScreenBoard:   return "ScreenBoard";
    case ParticlesEmitter::ParticleType::WorldBoard:    return "WorldBoard";
    case ParticlesEmitter::ParticleType::Box:           return "Box";

    case ParticlesEmitter::ParticleType::N_EMITTERS:    
    case ParticlesEmitter::ParticleType::Invalid:
    default:
        return "Invalid";
    }
}

void
ParticlesEmitter::allocate() 
{
    m_gpuData = etna::get_context().createBuffer({
        .size = 2 * sizeof(glm::vec4) * N_MAX_PARTICLES_PER_EMITTER,
        .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
        .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        .name = "emitter_gpu_data",
    });
    m_gpuData.map();
    auto* ppos = reinterpret_cast<glm::vec4* >(m_gpuData.data());
    for(std::size_t i = 0; i < N_MAX_PARTICLES_PER_EMITTER; i++) {
        ppos[i] = glm::vec4(0, 0, 0, 2);
    }
}


void 
ParticlesEmitter::drawGui()
{
    ImGui::SeparatorText("Emitter settings");
    ImGui::SetItemTooltip("Affects already emitted particles");
    ImGui::InputFloat3("Position", &m_info.position.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat2("Size", &m_info.size.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::LabelText("Material", "%u", m_info.material);
    ImGui::LabelText("Type: ", "%s", to_string(ParticleType{m_info.type}));
    ImGui::SliderFloat("Size fade", &m_info.fadeSize_pad.x, 0.f, 1.f);
    ImGui::ColorEdit4("Fade color", &m_info.fadeColor.x);
    ImGui::Bezier("Fading", m_bezier);
    
    ImGui::SeparatorText("Generator settings");
    ImGui::SetItemTooltip("Affects only new particles");

    ImGui::InputFloat("Spawn rate", &m_params.rate);
    ImGui::InputFloat("Max speed", &m_params.maxSpeed);
    if(ImGui::SliderFloat3("Direction", &m_params.direction.x, 0.f, 1.f)) {
        m_params.direction = glm::normalize(m_params.direction);
    }
    ImGui::SliderFloat("Direction factor", &m_params.directionFactor, 0.f, 1.f);
    ImGui::SliderFloat("Speed random factor", &m_params.speedRandomFactor, 0.f, 1.f);
    ImGui::InputFloat("Spawn radius", &m_params.spawnRadius);
    ImGui::InputFloat("Lifetime", &m_params.lifetime);
}

void 
ParticlesEmitter::update(glm::vec4 z_view, float time)
{
    m_camZ = glm::dot(m_info.position, z_view);
    #if 0
    float dt = time - m_prevTime;
    for(std::size_t i = 0; i < m_particles.size(); i++) {
        if(time > m_particles[i].birthtime + m_params.lifetime ) {
            std::swap(m_particles[i], m_particles.back());
            m_particles.pop_back();
            i--;
            continue;
        }

        m_particles[i].position += m_particles[i].velocity * dt;
        m_particles[i].position.w = (time - m_particles[i].birthtime) / m_params.lifetime;
    }
    if(time - m_lastSpawnTime > m_params.rate && m_particles.size() < N_MAX_PARTICLES_PER_EMITTER) {
        m_lastSpawnTime = time;
        emitParticle();
    }
    m_info.count = uint32_t(size());
    m_prevTime = time;
    m_info.fadeBezier[0] = m_bezier[0];
    m_info.fadeBezier[1] = m_bezier[1];
    m_info.fadeBezier[2] = m_bezier[2];
    m_info.fadeBezier[3] = m_bezier[3];

    auto* ppos = reinterpret_cast<glm::vec4* >(m_gpuData.data());
    auto* pvel = ppos + N_MAX_PARTICLES_PER_EMITTER;
    for(std::size_t i = 0; i < m_particles.size(); i++) {
        ppos[i] = m_particles[i].position;
        pvel[i] = m_particles[i].velocity;
    }
    #endif
    m_prevTime = time;
    m_info.fadeBezier[0] = m_bezier[0];
    m_info.fadeBezier[1] = m_bezier[1];
    m_info.fadeBezier[2] = m_bezier[2];
    m_info.fadeBezier[3] = m_bezier[3];
    if(time - m_lastSpawnTime > m_params.rate) {
        emitParticle();
        m_lastSpawnTime = time;
    }
}

uint32_t
ParticlesEmitter::verticesPerParticle() const {
    switch (ParticleType{m_info.type}) {

    case ParticleType::Board:       return 4;
    case ParticleType::WorldBoard:  return 4;
    case ParticleType::ScreenBoard: return 4;
    case ParticleType::Box:         return 14;
    case ParticleType::Undefined:
    case ParticleType::Invalid:
    case ParticleType::N_EMITTERS:
    default:
        spdlog::error("Bad m_info.type: {}", to_string(ParticleType{m_info.type}));
        std::terminate();
    }
}

static float 
randf() {
    return float(rand()) / float(RAND_MAX);
}

static float 
randsf() {
    return 2.f * randf() - 1.f;
}

// static glm::vec4 
// rand4f() {
//     return glm::vec4(randf(), randf(), randf(), randf());
// }

static glm::vec3
rand3f() {
    return glm::vec3(randsf(), randsf(), randsf());
}

static glm::vec4 
rand3fz() {
    return glm::vec4(randsf(), randsf(), randsf(), 0);
}

void 
ParticlesEmitter::emitParticle() {
    glm::vec4 position = glm::vec4{glm::vec3(m_info.position), 0} + m_params.spawnRadius * rand3fz();
    glm::vec3 velocity = m_params.maxSpeed * glm::mix(1.f, randf(), m_params.speedRandomFactor) * glm::mix(glm::normalize(rand3f()), m_params.direction, m_params.directionFactor); 
    auto* ppos = reinterpret_cast<glm::vec4* >(m_gpuData.data());
    auto* pvel = ppos + N_MAX_PARTICLES_PER_EMITTER;
    ppos[N_MAX_PARTICLES_PER_EMITTER-1] = position;
    pvel[N_MAX_PARTICLES_PER_EMITTER-1] = glm::vec4(velocity, 1.f / m_params.lifetime);
}

}

