cmake_minimum_required ( VERSION 3.0.2 )

project ( project )



# Find *all* shaders.
file(GLOB_RECURSE SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.geom"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
)
# Separate filter for shaders.
source_group("Shaders" FILES ${SHADERS})

# Build and link executable.
add_executable ( ${PROJECT_NAME}
    FluidShadowMap.h
    FluidShadowMap.cpp
    ComputeShader.cpp
    ComputeShader.h
    DepthFilter.cpp
    DepthFilter.h
    Global.h
    main3d.cpp
    Material.cpp
    Material.h
    Parameter3d.h
    ParticleSystem.cpp
    ParticleSystem.h
    RenderCamera.cpp
    RenderCamera.h
    RenderWidget.cpp
    RenderWidget.h
    SkyBox.cpp
    SkyBox.h
    WCubicSpline.cpp
    WCubicSpline.h
    ${SHADERS}
    )

# target_link_libraries ( ${PROJECT_NAME} labhelper )
config_build_output()
