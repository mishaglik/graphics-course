add_library(water_pipeline Water.cpp)

target_include_directories(water_pipeline PUBLIC ../..)

# Allows C++ code to inlcude C++-GLSL compat
target_include_directories(water_pipeline INTERFACE shaders)
# Allow GLSL code to include helper files and compat
target_shader_include_directories(water_pipeline INTERFACE shaders)

target_link_libraries(water_pipeline PUBLIC etna targets gui PRIVATE scene render_utils)


target_add_shaders(water_pipeline
  shaders/water.comp
)