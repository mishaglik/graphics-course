#include "ParticlesManager.hpp"
#include "etna/GlobalContext.hpp"
#include "imgui.h"
#include <algorithm>

namespace scene {

void
ParticlesManager::update(glm::vec4 z_view, float time) {
    for(auto& emitter : m_emitters) {
        emitter.update(z_view, time);
    }
}

void 
ParticlesManager::allocate()
{

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
        //TODO: Add delete button
    }
    ImGui::SeparatorText("Create new emitter");
    ImGui::InputFloat3("Position", &m_newEmitterInfo.position.x);
    if(ImGui::BeginCombo("Type", to_string(ParticlesEmitter::ParticleType{m_newEmitterInfo.type}))) {
        for(uint32_t typ = 0; typ < (uint32_t)ParticlesEmitter::ParticleType::N_EMITTERS; typ++) {
            if(ImGui::Selectable(to_string(ParticlesEmitter::ParticleType{typ}), typ == m_newEmitterInfo.type)) {
                m_newEmitterInfo.type = typ;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::InputFloat2("Size", &m_newEmitterInfo.size.x);
    //Material 
    {
        float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        ImGui::BeginDisabled(m_newEmitterInfo.material-1 != (glm::uint)m_resources.validateMaterial(m_newEmitterInfo.material-1));
        if (ImGui::ArrowButton("##left", ImGuiDir_Left))   { m_newEmitterInfo.material--; }
        ImGui::EndDisabled();
        ImGui::SameLine(0.0f, spacing);
        ImGui::BeginDisabled(m_newEmitterInfo.material+1 != (glm::uint)m_resources.validateMaterial(m_newEmitterInfo.material+1));
        if (ImGui::ArrowButton("##right", ImGuiDir_Right)) { m_newEmitterInfo.material++; }
        ImGui::EndDisabled();
        ImGui::PopItemFlag();
        ImGui::SameLine();
        ImGui::Text("%d", m_newEmitterInfo.material);
        m_newEmitterInfo.material = (glm::uint)m_resources.validateMaterial(m_newEmitterInfo.material);
        //TODO: Material preview
    }
    
    bool canCreate = Material::Id{m_newEmitterInfo.material} != Material::Id::Invalid && m_newEmitterInfo.type > 0 && m_newEmitterInfo.type < (uint32_t)ParticlesEmitter::ParticleType::N_EMITTERS && m_emitters.size() < N_MAX_EMITTERS;
    ImGui::BeginDisabled(!canCreate);
    if(ImGui::Button("Create")) {
        m_emitters.emplace_back(m_newEmitterInfo);
    }
    ImGui::EndDisabled();
    
    ImGui::End();
}



}