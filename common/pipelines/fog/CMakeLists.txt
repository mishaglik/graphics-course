add_library(fog_pipeline Fog.cpp)

target_include_directories(fog_pipeline PUBLIC ../..)

# Allows C++ code to inlcude C++-GLSL compat
target_include_directories(fog_pipeline INTERFACE shaders)
# Allow GLSL code to include helper files and compat
target_shader_include_directories(fog_pipeline INTERFACE shaders)

target_link_libraries(fog_pipeline PUBLIC etna targets gui PRIVATE scene)


target_add_shaders(fog_pipeline
  shaders/fog.vert
  shaders/fog.frag
  shaders/fog_resolver.frag
  shaders/fog_resolver.vert
)