add_library(box_pipeline Box.cpp)

target_include_directories(box_pipeline PUBLIC ../..)

# Allows C++ code to inlcude C++-GLSL compat
target_include_directories(box_pipeline INTERFACE shaders)
# Allow GLSL code to include helper files and compat
target_shader_include_directories(box_pipeline INTERFACE shaders)
target_shader_include_directories(box_pipeline PUBLIC ${PROJECT_SOURCE_DIR}/common/shaders)
target_shader_include_directories(box_pipeline PUBLIC ${PROJECT_SOURCE_DIR}/render_utils/shaders)

target_link_libraries(box_pipeline PUBLIC etna targets gui PRIVATE scene render_utils)


target_add_shaders(box_pipeline
  shaders/box.vert
  shaders/box.frag
)