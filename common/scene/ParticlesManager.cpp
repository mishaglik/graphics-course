#include "ParticlesManager.hpp"
#include "etna/GlobalContext.hpp"
#include "imgui.h"
#include <algorithm>

namespace scene {

void
ParticlesManager::update() {
    auto* einfo  = reinterpret_cast<EmitterInfo* >(m_gpuData.data());
    auto* espawn = reinterpret_cast<EmitterSpawnInfo* >(einfo + N_MAX_EMITTERS);
    
    for(size_t i = 0; i < m_emitters.size(); i++) {
        if(m_emitters[i].deleted()) [[unlikely]] {
            std::swap(m_emitters[i], m_emitters.back());
            m_emitters.pop_back();    
            i--;
            continue;
        }
        m_emitters[i].update(einfo+i, espawn+i);
    }
}

void 
ParticlesManager::allocate()
{
    m_gpuData = etna::get_context().createBuffer({
        .size = (sizeof(EmitterInfo) + sizeof(EmitterSpawnInfo)) * N_MAX_EMITTERS,
        .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
        .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        .name = "emitter_info_gpu_data",
    });
    m_gpuData.map();
}

void
ParticlesManager::drawGui()
{
    ImGui::Begin("Particle emitters");
    for(auto& emitter: m_emitters) {
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if(ImGui::TreeNode(&emitter, "Emitter")) {
            emitter.drawGui();
            ImGui::TreePop();
        }
    }
    ImGui::SeparatorText("Create new emitter");
    ImGui::InputFloat3("Position", &m_newEmitterInfo.position.x);
    if(ImGui::BeginCombo("Type", to_string(ParticlesEmitter::ParticleType{m_newEmitterInfo.type}))) {
        for(uint32_t typ = 0; typ < (uint32_t)ParticlesEmitter::ParticleType::N_EMITTERS; typ++) {
            if(ImGui::Selectable(to_string(ParticlesEmitter::ParticleType{typ}), typ == (uint32_t)m_newEmitterInfo.type)) {
                m_newEmitterInfo.type = ParticlesEmitter::ParticleType{typ};
            }
        }
        ImGui::EndCombo();
    }
    ImGui::InputFloat2("Size", &m_newEmitterInfo.size.x);
    //Material 
    {
        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        ImGui::BeginDisabled((uint32_t)m_newEmitterInfo.material-1 != (glm::uint)m_resources.validateMaterial((uint32_t)m_newEmitterInfo.material-1));
        if (ImGui::ArrowButton("##left", ImGuiDir_Left))   { (*(uint32_t*)&m_newEmitterInfo.material)--; }
        ImGui::EndDisabled();
        ImGui::SameLine(0.0f, spacing);
        ImGui::BeginDisabled((uint32_t)m_newEmitterInfo.material+1 != (glm::uint)m_resources.validateMaterial((uint32_t)m_newEmitterInfo.material+1));
        if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { (*(uint32_t*)&m_newEmitterInfo.material)++; }
        ImGui::EndDisabled();
        ImGui::PopItemFlag();
        ImGui::SameLine();
        ImGui::Text("%d", (uint32_t)m_newEmitterInfo.material);
        m_newEmitterInfo.material = m_resources.validateMaterial((uint32_t)m_newEmitterInfo.material);
        //TODO: Material preview
    }
    
    bool canCreate = Material::Id{m_newEmitterInfo.material} != Material::Id::Invalid                      && 
                    (uint32_t)m_newEmitterInfo.type > 0                                                    && 
                    (uint32_t)m_newEmitterInfo.type < (uint32_t)ParticlesEmitter::ParticleType::N_EMITTERS && 
                    m_emitters.size() < N_MAX_EMITTERS                                                     && 
                    (m_newEmitterInfo.lifetime > 0)                                                        &&
                    m_newEmitterInfo.direction.length() > 0;
                    
    ImGui::BeginDisabled(!canCreate);
    if(ImGui::Button("Create")) {
        m_emitters.emplace_back(m_newEmitterInfo);
        m_emitters.back().allocate();
    }
    ImGui::EndDisabled();
    
    ImGui::End();
}



}