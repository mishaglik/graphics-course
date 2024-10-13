
add_executable(local_shadertoy2
  main.cpp
  App.cpp
  Renderer.cpp
)

target_link_libraries(local_shadertoy2
  PRIVATE glfw etna glm::glm wsi gui scene render_utils)

target_add_shaders(local_shadertoy2
  shaders/toy.vert
  shaders/toy.frag
  shaders/skybox.frag
)

set(RESOURCE_DIR "${PROJECT_SOURCE_DIR}/resources/")
add_compile_definitions(local_shadertoy2
  PRIVATE RESOURCE_DIR="${RESOURCE_DIR}"
)
