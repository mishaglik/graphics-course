

add_executable(inflight_frames
  main.cpp
  App.cpp
  Renderer.cpp
)



target_link_libraries(inflight_frames
  PRIVATE glfw etna glm::glm wsi gui scene render_utils Tracy::TracyClient)

target_add_shaders(inflight_frames
  shaders/toy.vert
  shaders/toy.frag
  shaders/skybox.frag
)

set(RESOURCE_DIR "${PROJECT_SOURCE_DIR}/resources/")
add_compile_definitions(inflight_frames
  PRIVATE RESOURCE_DIR="${RESOURCE_DIR}"
)
