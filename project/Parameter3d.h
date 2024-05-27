#pragma once

#include <glm/glm.hpp>

namespace Para3d {
    // 求解器参数
    const float dt = 2e-6;
    const int substep = 4;

    // physical paras for water
    const float supportRadius = 0.025;
    const float particalRadius = 0.005;
    const float particalDiameter = particalRadius * 2.0;
    const float gravity = 9.8f;
    const float density0 = 1000.0f;
    const float stiffness = 5.0f;
    const float exponent = 7.0f;
    const float viscosity = 8e-6f;
    const float gMass = 0.5;


    //physical paras for smoke
    const float s_supportRadius = 0.025;
    const float s_particalRadius = 0.005;
    const float s_particalDiameter = particalRadius * 2.0;
    const float s_gravity = -9.8f; 
    const float s_density0 = 1.0f; 
    const float s_stiffness = 0.07f; 
    const float s_exponent = 1.0f; 
    const float s_viscosity = 1e-4f; 
    const float s_gMass = 0.0006f; 


    // 光学参数
    const float IOR = 1.3;
    const float IOR_BIAS = 0.02;
    const glm::vec3 F0 = { 0.15, 0.15, 0.15 };
    const glm::vec3 FLUID_COLOR = { 0.1, 0.5, 1.0 };
    const glm::vec3 SHADOW_COLOR = 0.5f * FLUID_COLOR;
    const float CAUSTIC_FACTOR = 0.004;
    const float THICKNESS_FACTOR = 0.5;

    // 几何参数
    const float zFar = 100.0f;
    const float zNear = 0.1f;
}