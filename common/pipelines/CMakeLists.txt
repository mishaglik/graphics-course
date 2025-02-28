
# Add just to please language servers :)
add_subdirectory(boilerplate)

add_subdirectory(skybox)
add_subdirectory(static_mesh)
add_subdirectory(resolve_gbuffer)
add_subdirectory(perlin)
add_subdirectory(terrain)
add_subdirectory(tonemap)

add_library(pipelines INTERFACE)

target_link_libraries(pipelines INTERFACE
    skybox_pipeline
    terrain_pipeline
    staticmesh_pipeline
    resolvegbuffer_pipeline
    tonemap_pipeline
)

add_custom_target(pipeline_shaders DEPENDS 
    skybox_pipeline_shaders
    terrain_pipeline_shaders
    staticmesh_pipeline_shaders
    resolvegbuffer_pipeline_shaders
    tonemap_pipeline_shaders
    perlin_pipeline_shaders
)