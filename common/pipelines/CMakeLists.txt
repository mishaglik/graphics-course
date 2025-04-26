
# Add just to please language servers :)
add_subdirectory(boilerplate)

add_subdirectory(skybox)
add_subdirectory(static_mesh)
add_subdirectory(resolve_gbuffer)
add_subdirectory(perlin)
add_subdirectory(terrain)
add_subdirectory(tonemap)
add_subdirectory(aa)
add_subdirectory(water)
add_subdirectory(fog)
add_subdirectory(box)
add_subdirectory(scene)
add_subdirectory(particles)

add_library(pipelines INTERFACE)

target_link_libraries(pipelines INTERFACE
    skybox_pipeline
    terrain_pipeline
    staticmesh_pipeline
    resolvegbuffer_pipeline
    tonemap_pipeline
    aa_pipeline
    box_pipeline
    scene_pipeline
    water_pipeline
    fog_pipeline
    particles_pipeline
)

add_custom_target(pipeline_shaders DEPENDS 
    skybox_pipeline_shaders
    terrain_pipeline_shaders
    staticmesh_pipeline_shaders
    resolvegbuffer_pipeline_shaders
    tonemap_pipeline_shaders
    perlin_pipeline_shaders
    aa_pipeline_shaders
    water_pipeline_shaders
    fog_pipeline_shaders
    box_pipeline_shaders
    particles_pipeline_shaders
    # scene_pipeline_shaders
)