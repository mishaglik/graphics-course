#pragma once

#include <glm/glm.hpp>
#include "scene/SceneManager.hpp"
#include "shaders/projview.hpp"

namespace pipes {

struct RenderContext {
    glm::mat4x4 worldViewProj;
    glm::mat4x4 worldView;
    glm::mat4x4 worldIView;
    glm::mat4x4 worldProj;
    glm::mat4x4 curCamIpv;
    glm::mat4x4 worldViewChunkInv;
    glm::mat4x4 camViewChunkInv;
    glm::vec3 camPos;
    double frameTime = 0.;
    float dt = 0.;
    SceneManager* sceneMgr;
    glm::uvec2 resolution;
    etna::Buffer worldViewMatrices;
    uint32_t worldId;
    uint32_t camWorldId;

    WorldViewProjMatrices* getMatrices() { return reinterpret_cast<WorldViewProjMatrices*>(worldViewMatrices.data());}
};
}