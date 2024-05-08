#include "WCubicSpline.h"
#include <glm/ext.hpp>
#include <iostream>

namespace Glb {

    WCubicSpline3d::WCubicSpline3d(float h) {
        mH = h;
        mH2 = h * h;
        mH3 = mH2 * h;
        mSigma = 8.0 / (glm::pi<float>() * mH3);

        mBufferSize = 128;
        //store precomputed values and gradients of the cubic spline function
        mValueAndGradFactorBuffer = std::vector<glm::vec2>(mBufferSize);

        for (uint32_t i = 0; i < mBufferSize; i++) {
            float distance = ((float)i + 0.5f) * mH / mBufferSize;  // [0, h]
            mValueAndGradFactorBuffer[i].r = CalculateValue(distance);
            mValueAndGradFactorBuffer[i].g = CalculateGradFactor(distance);
        }
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