add_library(scene_pipeline Scene.cpp)

target_include_directories(scene_pipeline PUBLIC ../..)

# Allows C++ code to inlcude C++-GLSL compat
target_include_directories(scene_pipeline INTERFACE shaders)
# Allow GLSL code to include helper files and compat
# target_shader_include_directories(scene_pipeline INTERFACE shaders)
# target_shader_include_directories(scene_pipeline PUBLIC ${PROJECT_SOURCE_DIR}/common/shaders)

target_link_libraries(scene_pipeline PUBLIC etna targets gui staticmesh_pipeline terrain_pipeline PRIVATE scene render_utils)


# target_add_shaders(scene_pipeline
#   # shaders/scene.vert
#   # shaders/scene.frag
# )