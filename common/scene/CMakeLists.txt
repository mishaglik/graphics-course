
add_library(scene SceneManager.cpp ResourceManager.cpp TerrainManager.cpp Cliplevel.cpp ParticlesEmitter.cpp ParticlesManager.cpp)

target_include_directories(scene PUBLIC ..)

target_link_libraries(scene PUBLIC glm::glm tinygltf etna gui targets render_utils)
