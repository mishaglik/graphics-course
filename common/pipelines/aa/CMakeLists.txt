add_library(aa_pipeline AA.cpp)

target_include_directories(aa_pipeline PUBLIC ../..)

# Allows C++ code to inlcude C++-GLSL compat
target_include_directories(aa_pipeline INTERFACE shaders)
# Allow GLSL code to include helper files and compat
target_shader_include_directories(aa_pipeline INTERFACE shaders)

target_link_libraries(aa_pipeline PUBLIC etna targets gui PRIVATE scene)


target_add_shaders(aa_pipeline
  shaders/aa.vert
  shaders/aa.frag
)