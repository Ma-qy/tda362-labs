#pragma once

#include <glm/glm.hpp>
#include "Parameter3d.h"
#include <vector>
#include "WCubicSpline.h"

namespace Fluid3d {
    struct ParticleInfo3d
    {
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 velocity;
        alignas(16) glm::vec3 accleration;
        alignas(4) float_t density;
        alignas(4) float_t pressure;
        alignas(4) float_t pressDivDensity;
        alignas(4) uint32_t blockId;
    };

    struct NeighborInfo
    {
        alignas(16) glm::vec3 radius;
        alignas(4) float_t distance;
        alignas(4) int32_t neighborId;
    };


    ///////////////////////////
    // Core Particle class
    ///////////////////////////
    class ParticleSystem 
    {
    public:
        // particle
        float mSupportRadius = Para3d::supportRadius;  // ֧�Ű뾶
        float mSupportRadiusSquare = mSupportRadius * mSupportRadius; // ֧�Ű뾶��ƽ��
        float mParticalRadius = Para3d::particalRadius;   // ���Ӱ뾶
        float mParticalDiameter = Para3d::particalDiameter;  // ����ֱ��
        
        float mVolume = std::pow(mParticalRadius, 3);   // ���
        float mMass = Para3d::density0 * mVolume;  // ����
        float mViscosity = Para3d::viscosity;      // ճ��ϵ��
        float mExponent = Para3d::exponent;        // ѹ��ָ��
        float mStiffness = Para3d::stiffness;        // �ն�
        std::vector<ParticleInfo3d> mParticalInfos;
        int mMaxNeighbors = 512;


        //container(boundaries)
        glm::vec3 mLowerBound = glm::vec3(FLT_MAX);
        glm::vec3 mUpperBound = glm::vec3(-FLT_MAX);
        glm::vec3 mContainerCenter = glm::vec3(0.0f);
        glm::uvec3 mBlockNum = glm::uvec3(0);    // XYZ���м���block
        glm::vec3 mBlockSize = glm::vec3(0.0f);
        std::vector<glm::uvec2> mBlockExtens;
        std::vector<int32_t> mBlockIdOffs;


        //cubic spline function
        Glb::WCubicSpline3d mW = Glb::WCubicSpline3d(mSupportRadius);


    public:
        ParticleSystem();
        ~ParticleSystem();

        void SetContainerSize(glm::vec3 corner, glm::vec3 size);
        int32_t AddFluidBlock(glm::vec3 corner, glm::vec3 size, glm::vec3 v0, float particalSpace);
        uint32_t GetBlockIdByPosition(glm::vec3 position);
        void UpdateData();
    };
    
}
