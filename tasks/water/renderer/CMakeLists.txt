
add_executable(water_renderer
  main.cpp
  App.cpp
  Renderer.cpp
  WorldRenderer.cpp
)

target_link_libraries(water_renderer
  PRIVATE glfw etna glm::glm wsi gui scene render_utils targets pipelines)

# target_add_shaders(imgui_renderer

# )
