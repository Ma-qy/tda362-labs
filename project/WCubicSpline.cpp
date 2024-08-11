#define M_PI 3.1415926

#include "WCubicSpline.h"


#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace Glb {

    WCubicSpline3d::WCubicSpline3d(float h) {
        mH = h;
        mH2 = h * h;
        mH3 = mH2 * h;
        //mSigma = 8.0 / (glm::pi<float>() * mH3);

        int numIntervals = 10000; // Number of intervals for the numerical integration
        float integralValue = numericalIntegration(0.0f, 1.0f, numIntervals);
        float volumeFactor = 4.0f * M_PI * mH3;
        mSigma = 1.0f / (volumeFactor * integralValue);

        mBufferSize = 128;
        //store precomputed values and gradients of the cubic spline function
        mValueAndGradFactorBuffer = std::vector<glm::vec2>(mBufferSize);

        for (uint32_t i = 0; i < mBufferSize; i++) {
            float distance = ((float)i + 0.5f) * mH / mBufferSize;  // [0, h]
            mValueAndGradFactorBuffer[i].r = CalculateValue(distance);
            mValueAndGradFactorBuffer[i].g = CalculateGradFactor(distance);
        }
    }
    // Define the integrand for the cubic spline kernel in 3D
    float WCubicSpline3d::integrand(float q) {
        if (0 <= q && q < 0.5f) {
            return (6.0f * (q * q * q - q * q) + 1.0f) * q * q;
        }
        else if (0.5f <= q && q < 1.0f) {
            return 2.0f * std::pow(1.0f - q, 3) * q * q;
        }
        else {
            return 0.0f;
        }
    }

    float WCubicSpline3d::numericalIntegration(float a, float b, int n) {
        float h = (b - a) / n;

        float ina = integrand(a);
        float inb = integrand(b);
        float integral = 0.5f * (ina + inb);
        for (int i = 1; i < n; ++i) {
            float x = a + i * h;
            integral += integrand(x);
        }
        integral *= h;
        return integral;
    }

    
    WCubicSpline3d::~WCubicSpline3d() {

    }

    float_t* WCubicSpline3d::GetData() {
        return (float_t*)mValueAndGradFactorBuffer.data();
    }


    uint32_t WCubicSpline3d::GetBufferSize() {
        return mBufferSize;
    }

    //W(r,h)
    float WCubicSpline3d::CalculateValue(float distance) {
        float r = std::abs(distance);
        float q = r / mH;
        float q2 = q * q;
        float q3 = q * q2;
        float res = 0.0f;
        if (q < 0.5f) {
            res = 6.0f * (q3 - q2) + 1.0f;
            res *= mSigma;
            return res;
        }
        else if (q >= 0.5f && q < 1.0f) {
            res = 1.0f - q;
            res = std::pow(res, 3) * 2.0f;
            res *= mSigma;
            return res;
        }
        return res;
    }

    float WCubicSpline3d::CalculateGradFactor(float distance) {
        float res = 0.0f;
        if (distance < 1e-5) {
            return res;
        }

        float q = distance / mH;

        if (q < 0.5f) {
            res = 6.0f * (3.0f * q * q - 2.0f * q) * mSigma / (mH * distance);
            return res;
        }
        else if (q >= 0.5 && q < 1.0f) {
            res = -6.0f * std::powf(1.0f - q, 2) * mSigma / (mH * distance);
            return res;
        }
        return res;
    }

}