#pragma once

#include <glm/glm.hpp>

namespace Para3d {
    // 求解器参数
    const float dt = 2e-4;
    const int substep = 8;

    // 物理参数
    const float supportRadius = 0.030;
    const float particalRadius = 0.005;
    const float particalDiameter = particalRadius * 2.0;
    const float gravity = 9.8f;
    const float density0 = 1000.0f;
    const float stiffness = 5.0f;
    const float exponent = 7.0f;
    const float viscosity = 1e-6f;

    // 光学参数
    const float IOR = 1.3;
    const float IOR_BIAS = 0.02;
    const glm::vec3 F0 = { 0.15, 0.15, 0.15 };
    const glm::vec3 FLUID_COLOR = { 0.1, 0.5, 1.0 };
    const glm::vec3 SHADOW_COLOR = 0.5f * FLUID_COLOR;
    const float CAUSTIC_FACTOR = 0.004;
    const float THICKNESS_FACTOR = 0.5;

    // 几何参数
    const float zFar = 100.0;
    const float zNear = 0.1;
}