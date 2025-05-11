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
    m_gpuData.unmap();
}


void 
ParticlesEmitter::drawGui()
{
    ImGui::SeparatorText("Emitter settings");
    ImGui::SetItemTooltip("Affects already emitted particles");
    ImGui::LabelText("Material", "%u", (unsigned)m_info.material); //FIXME: Make ImGui show material
    ImGui::LabelText("Type: ", "%s", to_string(ParticleType{m_info.type}));
    ImGui::InputFloat3("Position", &m_info.position.x, "%.3f")  && invalidate();
    ImGui::InputFloat2("Size", &m_info.size.x, "%.3f")          && invalidate();
    ImGui::SliderFloat("Size fade", &m_info.fadeSize, 0.f, 1.f) && invalidate();
    ImGui::ColorEdit4("Fade color", &m_info.fadeColor.x)        && invalidate();
    ImGui::Bezier("Fading", m_bezier ) != 0                     && invalidate();
    
    ImGui::SeparatorText("Generator settings");
    ImGui::SetItemTooltip("Affects only new particles");

    ImGui::InputFloat("Spawn rate", &m_info.spawnRate)          && invalidate();
    ImGui::InputFloat("Max speed", &m_info.maxSpeed)            && invalidate();
    if(ImGui::SliderFloat3("Direction", &m_info.direction.x, 0.f, 1.f)) {
        if(m_info.direction.length() > 0)
            m_info.direction = glm::normalize(m_info.direction);
        invalidate();
    }
    ImGui::SliderFloat("Direction factor", &m_info.directionFactor, 0.f, 1.f)      && invalidate();
    ImGui::SliderFloat("Speed random factor", &m_info.speedRandomFactor, 0.f, 1.f) && invalidate();
    ImGui::InputFloat("Spawn radius", &m_info.spawnRadius)                         && invalidate();
    ImGui::InputFloat("Lifetime", &m_info.lifetime)                                && invalidate();
    if(ImGui::Button("Delete")) {
        m_deleted = true;
    }
}

void 
ParticlesEmitter::update(EmitterInfo* emitter_info, EmitterSpawnInfo* emitter_spawn_info)
{
    if(m_actual) return;
    m_actual = true;
    //NOTE - Theese static casts mostly for emum -> uint conversion. It's programmer responsibility make sure here everything is correct;
    //LINK common/scene/ParticlesEmitter.hpp:26
    //LINK common/shaders/particleInfo.hpp:18
    //LINK common/shaders/particleInfo.hpp:33
    emitter_info->position       = static_cast<decltype(emitter_info->position      )>(m_info.position);
    emitter_info->size           = static_cast<decltype(emitter_info->size          )>(m_info.size);
    emitter_info->type           = static_cast<decltype(emitter_info->type          )>(m_info.type);
    emitter_info->material       = static_cast<decltype(emitter_info->material      )>(m_info.material);
    emitter_info->fadeColor      = static_cast<decltype(emitter_info->fadeColor     )>(m_info.fadeColor);
    emitter_info->fadeSize_pad.x = static_cast<decltype(emitter_info->fadeSize_pad.x)>(m_info.fadeSize);

    emitter_info->fadeBezier.x   = m_bezier[0];
    emitter_info->fadeBezier.y   = m_bezier[1];
    emitter_info->fadeBezier.z   = m_bezier[2];
    emitter_info->fadeBezier.w   = m_bezier[3];

    emitter_spawn_info->direction         = glm::vec4(m_info.direction, m_info.directionFactor);

    emitter_spawn_info->rate              = static_cast<decltype(emitter_spawn_info->rate              )>(m_info.spawnRate        );
    emitter_spawn_info->maxSpeed          = static_cast<decltype(emitter_spawn_info->maxSpeed          )>(m_info.maxSpeed         );
    emitter_spawn_info->speedRandomFactor = static_cast<decltype(emitter_spawn_info->speedRandomFactor )>(m_info.speedRandomFactor);
    emitter_spawn_info->spawnRadius       = static_cast<decltype(emitter_spawn_info->spawnRadius       )>(m_info.spawnRadius      );
    emitter_spawn_info->lifetime          = static_cast<decltype(emitter_spawn_info->lifetime          )>(m_info.lifetime         );

}
}

