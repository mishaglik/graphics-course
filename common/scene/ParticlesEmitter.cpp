#include "ParticlesEmitter.hpp"

#include <algorithm>



namespace scene {

void 
ParticlesEmitter::update(float time)
{
    float dt = time - m_prevTime;
    for(std::size_t i = 0; i < m_particles.size(); i++) {
        if(m_particles[i].lifetime < time) {
            std::swap(m_particles[i], m_particles.back());
            m_particles.pop_back();
            i--;
            continue;
        }

        m_particles[i].position += m_particles[i].velocity * dt;
    }
    // spdlog::info("time{}, last:{}, rate: {}", time, m_lastSpawnTime, m_params.spawnRate);
    if(time - m_lastSpawnTime > m_params.spawnRate && m_particles.size() < 1024) {
        m_lastSpawnTime = time;
        m_particles.emplace_back(m_info.position, glm::vec4(0, m_params.spawnVelocity, 0, 0), time + m_params.spawnLifetime);
    }
    std::stable_sort(m_particles.begin(), m_particles.end(), [](const ParticleCpuInfo& lhs, const ParticleCpuInfo& rhs) -> bool {
        //TODO: Proj matrix
        return lhs.position.z < rhs.position.z;
    });
    m_prevTime = time;
}

}
