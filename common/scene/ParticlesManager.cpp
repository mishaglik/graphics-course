#include "ParticlesManager.hpp"
#include "etna/GlobalContext.hpp"
#include <algorithm>

namespace scene {

void
ParticlesManager::update(float time) {
    std::size_t n = 0;
    for(auto& emitter : m_emitters) {
        emitter.update(time);
        n += emitter.size();
    }
    //As emitters tends to stand still, use bubble sort. 
    {
        bool changed = true;
        while(changed) {
            changed = false;
            for(size_t i = 0; i + 1 < m_emitters.size(); i++) {
                if(m_emitters[i+1] < m_emitters[i]) {
                    std::swap(m_emitters[i], m_emitters[i+1]);
                    changed = true;
                }
            }
        }
    }
    if(n > m_capacity) reserve(n);
    ParticleInfo* pinfo = reinterpret_cast<ParticleInfo*>(m_particles.data());
    std::size_t i = 0;
    for(auto& emitter : m_emitters) {
        for(auto& particle: emitter) {
            pinfo[i++].position = particle.position;
        }
    }
}

void 
ParticlesManager::allocate()
{
    m_capacity = 64;
    m_particles = etna::get_context().createBuffer({
        .size =  uint32_t(m_capacity * sizeof(ParticleInfo)),
        .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
        .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        .name = "particles",
    });
    m_particles.map();
}


void 
ParticlesManager::reserve(std::size_t n) {
    while(m_capacity < n) m_capacity <<= 1;

    m_particles = etna::get_context().createBuffer({
        .size =  uint32_t(m_capacity * sizeof(ParticleInfo)),
        .bufferUsage = vk::BufferUsageFlagBits::eStorageBuffer,
        .memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        .name = "particles",
    });
    m_particles.map();
}

}