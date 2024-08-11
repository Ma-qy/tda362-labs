#pragma once

#ifndef W_CUBE_SPLINE_H
#define W_CUBE_SPLINE_H


#include <glm/glm.hpp>
#include <vector>

namespace Glb {

    class WCubicSpline3d {
    private:
        float mH;
        float mH2;
        float mH3;
        float mSigma;
        uint32_t mBufferSize;
        std::vector<glm::vec2> mValueAndGradFactorBuffer;

    public:
        WCubicSpline3d() = delete;
        explicit WCubicSpline3d(float h);
        ~WCubicSpline3d();

        float_t* GetData();
        uint32_t GetBufferSize();

    private:
        float CalculateValue(float distance);
        float CalculateGradFactor(float distance);
        float numericalIntegration(float a, float b, int n);
        float integrand(float q);

    };

}

#endif // !W_CUBE_SPLINE_H