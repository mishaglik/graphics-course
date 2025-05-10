add_library(particles_pipeline Particles.cpp)

target_include_directories(particles_pipeline PUBLIC ../..)

# Allows C++ code to inlcude C++-GLSL compat
target_include_directories(particles_pipeline INTERFACE shaders)
# Allow GLSL code to include helper files and compat
target_shader_include_directories(particles_pipeline INTERFACE shaders)
target_shader_include_directories(particles_pipeline PUBLIC ${PROJECT_SOURCE_DIR}/common/shaders)

target_link_libraries(particles_pipeline PUBLIC etna targets gui PRIVATE scene render_utils)


target_add_shaders(particles_pipeline
  shaders/particles.vert
  shaders/particles.frag
  shaders/particles.comp
)